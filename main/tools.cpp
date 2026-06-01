/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

﻿#include "tools.h"
#include <stdio.h>
#include <esp_log.h>
#include <esp_err.h>
#include "m5_tab5_keyboard.h"
#include <i2c_bus.h>

static const char* TOOLS_TAG = "tools";

// I2C 测试配置
static constexpr i2c_port_t KB_TEST_I2C_PORT = I2C_NUM_1;
static constexpr gpio_num_t KB_TEST_I2C_SDA = static_cast<gpio_num_t>(M5_TAB5_KB_DEFAULT_SDA);
static constexpr gpio_num_t KB_TEST_I2C_SCL = static_cast<gpio_num_t>(M5_TAB5_KB_DEFAULT_SCL);
static constexpr uint32_t KB_TEST_I2C_FREQ = M5_TAB5_KB_I2C_FREQ_400K;
static constexpr int16_t KB_I2C_TEST_START_ADDR = -1;  // -1: 0x08开始全量测试 >=0x08: 设备当前可能在该地址，先恢复再从该地址继续测试

// 硬件键盘实例
static m5::M5Tab5Keyboard device_kb;

static i2c_bus_handle_t create_kb_test_i2c_bus()
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = KB_TEST_I2C_SDA,
        .scl_io_num = KB_TEST_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {
            .clk_speed = KB_TEST_I2C_FREQ,
        },
        .clk_flags = 0,
    };

    i2c_bus_handle_t bus = i2c_bus_create(KB_TEST_I2C_PORT, &conf);
    if (bus == nullptr) {
        ESP_LOGE(TOOLS_TAG, "Failed to create keyboard test I2C bus");
    }
    return bus;
}

static uint8_t scan_i2c_devices(i2c_bus_handle_t bus, bool occupied[0x80])
{
    uint8_t addresses[120] = {0};
    uint8_t found = i2c_bus_scan(bus, addresses, static_cast<uint8_t>(sizeof(addresses)));

    for (uint8_t i = 0; i < 0x80; ++i) {
        occupied[i] = false;
    }

    ESP_LOGI(TOOLS_TAG, "I2C scan found %u device(s)", found);
    for (uint8_t i = 0; i < found && i < sizeof(addresses); ++i) {
        uint8_t addr = addresses[i];
        if (addr < 0x80) {
            occupied[addr] = true;
        }
        ESP_LOGI(TOOLS_TAG, "  - I2C device @ 0x%02X", addr);
    }

    return found;
}

static bool init_keyboard_with_addr(i2c_bus_handle_t bus, uint8_t addr)
{
    m5_tab5_kb_err_t err = device_kb.begin(
        bus,
        addr,
        KB_TEST_I2C_FREQ,
        M5_TAB5_KB_INT_MODE_DISABLED
    );

    if (err != M5_TAB5_KB_OK) {
        ESP_LOGE(TOOLS_TAG, "Keyboard begin failed @ 0x%02X, err=%d", addr, err);
        return false;
    }

    uint8_t version = 0;
    err = device_kb.getVersion(&version);
    if (err != M5_TAB5_KB_OK) {
        ESP_LOGE(TOOLS_TAG, "Keyboard getVersion failed @ 0x%02X, err=%d", addr, err);
        device_kb.end();
        return false;
    }

    ESP_LOGI(TOOLS_TAG, "Keyboard initialized @ 0x%02X, FW=0x%02X", addr, version);
    return true;
}

static bool restore_addr_via_api(i2c_bus_handle_t bus, uint8_t current_addr, uint8_t target_addr)
{
    // 通过驱动 API 改地址: begin(current) -> setI2CAddress(target) -> end()
    if (!init_keyboard_with_addr(bus, current_addr)) {
        ESP_LOGE(TOOLS_TAG, "API restore: init at 0x%02X failed", current_addr);
        return false;
    }

    m5_tab5_kb_err_t err = device_kb.setI2CAddress(target_addr);
    if (err != M5_TAB5_KB_OK) {
        ESP_LOGE(TOOLS_TAG, "API restore: setI2CAddress(0x%02X) failed from 0x%02X, err=%d",
                 target_addr, current_addr, err);
        device_kb.end();
        return false;
    }

    vTaskDelay(pdMS_TO_TICKS(500));
    device_kb.end();

    ESP_LOGW(TOOLS_TAG, "API restore: 0x%02X -> 0x%02X success", current_addr, target_addr);
    return true;
}

static void run_kb_i2c_addr_register_test(i2c_bus_handle_t bus, int16_t start_addr)
{
    const uint8_t kMinAddr = 0x08;
    const uint8_t kMaxAddr = 0x77;
    const uint8_t kDefaultAddr = M5_TAB5_KB_DEFAULT_ADDR;

    // ========================================
    // Phase 0: 确定起始地址并确保设备在默认地址
    // ========================================
    bool occupied[0x80] = {false};
    scan_i2c_devices(bus, occupied);

    uint8_t actual_start = kMinAddr;

    if (start_addr < 0) {
        // -1: 全量测试，设备应在默认地址
        ESP_LOGI(TOOLS_TAG, "[INIT] Fresh start, device expected at default 0x%02X", kDefaultAddr);
        if (!init_keyboard_with_addr(bus, kDefaultAddr)) {
            ESP_LOGE(TOOLS_TAG, "[INIT] Cannot find device at default addr 0x%02X, abort", kDefaultAddr);
            return;
        }
        actual_start = kMinAddr;
    } else {
        // start_addr >= 0: 设备可能在 start_addr 或默认地址
        uint8_t resume_addr = static_cast<uint8_t>(start_addr);
        if (resume_addr < kMinAddr) resume_addr = kMinAddr;
        if (resume_addr > kMaxAddr) resume_addr = kMaxAddr;
        actual_start = resume_addr;

        ESP_LOGI(TOOLS_TAG, "[INIT] Resume mode, device may be at 0x%02X or default 0x%02X",
                 resume_addr, kDefaultAddr);

        if (resume_addr != kDefaultAddr && occupied[resume_addr]
            && init_keyboard_with_addr(bus, resume_addr)) {
            // 设备在 resume_addr，需要先恢复到默认地址
            ESP_LOGW(TOOLS_TAG, "[INIT] Device found at 0x%02X, restoring to default 0x%02X",
                     resume_addr, kDefaultAddr);

            m5_tab5_kb_err_t err = device_kb.setI2CAddress(kDefaultAddr);
            if (err != M5_TAB5_KB_OK) {
                ESP_LOGE(TOOLS_TAG, "[INIT] setI2CAddress(0x%02X) failed from 0x%02X, err=%d, abort",
                         kDefaultAddr, resume_addr, err);
                device_kb.end();
                return;
            }

            vTaskDelay(pdMS_TO_TICKS(500));
            device_kb.end();

            // 验证恢复成功
            scan_i2c_devices(bus, occupied);
            if (!occupied[kDefaultAddr]) {
                ESP_LOGE(TOOLS_TAG, "[INIT] Default addr 0x%02X not found after restore", kDefaultAddr);
                // 回读诊断: 设备可能仍在 resume_addr，看寄存器实际值
                if (init_keyboard_with_addr(bus, resume_addr)) {
                    uint8_t readback_addr = 0;
                    if (device_kb.getI2CAddress(&readback_addr) == M5_TAB5_KB_OK) {
                        ESP_LOGE(TOOLS_TAG, "[INIT] Diagnostic: device at 0x%02X, reg I2C_ADDR=0x%02X",
                                 resume_addr, readback_addr);
                    }
                    device_kb.end();
                }
                return;
            }
            if (!init_keyboard_with_addr(bus, kDefaultAddr)) {
                ESP_LOGE(TOOLS_TAG, "[INIT] Cannot init at default addr after restore, abort");
                return;
            }
            ESP_LOGI(TOOLS_TAG, "[INIT] Device restored to default 0x%02X successfully", kDefaultAddr);
        } else if (occupied[kDefaultAddr] && init_keyboard_with_addr(bus, kDefaultAddr)) {
            // 设备在默认地址
            ESP_LOGI(TOOLS_TAG, "[INIT] Device found at default addr 0x%02X", kDefaultAddr);
        } else {
            ESP_LOGE(TOOLS_TAG, "[INIT] Cannot find device at 0x%02X or default 0x%02X, abort",
                     resume_addr, kDefaultAddr);
            return;
        }
    }

    // ========================================
    // Phase 1: 测试循环 (设备已在默认地址并已初始化)
    // ========================================
    uint16_t tested_count = 0;
    uint16_t passed_count = 0;
    uint16_t failed_count = 0;
    uint16_t skipped_count = 0;

    for (uint8_t target_addr = actual_start; target_addr <= kDefaultAddr; ++target_addr) {
        // 每轮扫描总线
        ESP_LOGI(TOOLS_TAG, "[PRE-SCAN] before testing target 0x%02X", target_addr);
        scan_i2c_devices(bus, occupied);

        if (target_addr == kDefaultAddr) {
            ESP_LOGI(TOOLS_TAG, "Skip 0x%02X (default address)", target_addr);
            ++skipped_count;
            continue;
        }

        if (occupied[target_addr]) {
            ESP_LOGW(TOOLS_TAG, "Skip 0x%02X (address already occupied)", target_addr);
            ++skipped_count;
            continue;
        }

        ++tested_count;

        // Step 1: 确保设备在默认地址并已初始化
        device_kb.end();
        if (!init_keyboard_with_addr(bus, kDefaultAddr)) {
            ESP_LOGE(TOOLS_TAG, "Cannot init keyboard on default addr 0x%02X before test", kDefaultAddr);
            ++failed_count;
            break;
        }

        // Step 2: 改地址 default -> target
        ESP_LOGI(TOOLS_TAG, "[TEST] I2C_ADDR: 0x%02X -> 0x%02X", kDefaultAddr, target_addr);
        m5_tab5_kb_err_t err = device_kb.setI2CAddress(target_addr);
        if (err != M5_TAB5_KB_OK) {
            ESP_LOGE(TOOLS_TAG, "setI2CAddress(0x%02X) failed, err=%d", target_addr, err);
            ++failed_count;
            device_kb.end();
            // 设备可能仍在默认地址，尝试重新初始化继续下一轮
            if (init_keyboard_with_addr(bus, kDefaultAddr)) {
                continue;
            }
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
        device_kb.end();

        // Step 3: 扫描验证 target 存在
        bool occupied_after[0x80] = {false};
        ESP_LOGI(TOOLS_TAG, "[POST-WRITE-SCAN] after setting 0x%02X", target_addr);
        scan_i2c_devices(bus, occupied_after);

        if (!occupied_after[target_addr]) {
            ESP_LOGE(TOOLS_TAG, "Target 0x%02X not found after write", target_addr);
            ++failed_count;

            if (occupied_after[kDefaultAddr]) {
                // 改址未生效，设备仍在默认地址，回读寄存器诊断
                ESP_LOGE(TOOLS_TAG, "[FAIL] Default 0x%02X still exists, write not effective for 0x%02X",
                         kDefaultAddr, target_addr);
                if (init_keyboard_with_addr(bus, kDefaultAddr)) {
                    uint8_t readback_addr = 0;
                    if (device_kb.getI2CAddress(&readback_addr) == M5_TAB5_KB_OK) {
                        ESP_LOGE(TOOLS_TAG, "Diagnostic: device at 0x%02X, reg I2C_ADDR=0x%02X",
                                 kDefaultAddr, readback_addr);
                    }
                    continue;
                }
            }
            break;
        }

        // Step 4: 在 target 地址初始化并验证
        if (!init_keyboard_with_addr(bus, target_addr)) {
            ESP_LOGE(TOOLS_TAG, "Init at target 0x%02X failed, try API restore to default", target_addr);
            ++failed_count;

            // 尝试通过 API 恢复: begin(target) + setI2CAddress(default)
            vTaskDelay(pdMS_TO_TICKS(100));
            if (restore_addr_via_api(bus, target_addr, kDefaultAddr)) {
                bool occupied_recovery[0x80] = {false};
                scan_i2c_devices(bus, occupied_recovery);
                if (occupied_recovery[kDefaultAddr] && init_keyboard_with_addr(bus, kDefaultAddr)) {
                    ESP_LOGW(TOOLS_TAG, "Recovered to default after init failure at 0x%02X", target_addr);
                    continue;
                }
            }
            ESP_LOGE(TOOLS_TAG, "Cannot recover from init failure at 0x%02X, abort", target_addr);
            break;
        }

        // Step 5: 恢复地址 target -> default
        ESP_LOGI(TOOLS_TAG, "[RESTORE] I2C_ADDR: 0x%02X -> 0x%02X", target_addr, kDefaultAddr);
        err = device_kb.setI2CAddress(kDefaultAddr);
        if (err != M5_TAB5_KB_OK) {
            ESP_LOGE(TOOLS_TAG, "Restore setI2CAddress(0x%02X) failed from 0x%02X, err=%d",
                     kDefaultAddr, target_addr, err);
            ++failed_count;
            device_kb.end();

            // 尝试通过 API 恢复
            vTaskDelay(pdMS_TO_TICKS(100));
            if (restore_addr_via_api(bus, target_addr, kDefaultAddr)) {
                bool occupied_recovery[0x80] = {false};
                scan_i2c_devices(bus, occupied_recovery);
                if (occupied_recovery[kDefaultAddr] && init_keyboard_with_addr(bus, kDefaultAddr)) {
                    ESP_LOGW(TOOLS_TAG, "Recovered to default via API after restore failure");
                    continue;
                }
            }
            ESP_LOGE(TOOLS_TAG, "Cannot recover after restore failure, abort");
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
        device_kb.end();

        // Step 6: 扫描验证默认地址存在
        bool occupied_restore[0x80] = {false};
        ESP_LOGI(TOOLS_TAG, "[POST-RESTORE-SCAN] after restoring default 0x%02X", kDefaultAddr);
        scan_i2c_devices(bus, occupied_restore);

        if (!occupied_restore[kDefaultAddr]) {
            ESP_LOGE(TOOLS_TAG, "Default addr 0x%02X not found after restore", kDefaultAddr);
            ++failed_count;

            // 设备可能仍在 target，回读寄存器诊断后尝试恢复
            if (occupied_restore[target_addr]) {
                ESP_LOGW(TOOLS_TAG, "Device still at 0x%02X, reading register for diagnostic", target_addr);
                if (init_keyboard_with_addr(bus, target_addr)) {
                    uint8_t readback_addr = 0;
                    if (device_kb.getI2CAddress(&readback_addr) == M5_TAB5_KB_OK) {
                        ESP_LOGE(TOOLS_TAG, "Diagnostic: device at 0x%02X, reg I2C_ADDR=0x%02X",
                                 target_addr, readback_addr);
                    }
                    // 尝试通过已初始化的设备恢复
                    if (device_kb.setI2CAddress(kDefaultAddr) == M5_TAB5_KB_OK) {
                        vTaskDelay(pdMS_TO_TICKS(500));
                        device_kb.end();
                        scan_i2c_devices(bus, occupied_restore);
                        if (occupied_restore[kDefaultAddr]
                            && init_keyboard_with_addr(bus, kDefaultAddr)) {
                            ESP_LOGW(TOOLS_TAG, "Recovered to default via diagnostic restore");
                            continue;
                        }
                    }
                    device_kb.end();
                }
            }
            ESP_LOGE(TOOLS_TAG, "Cannot recover, abort");
            break;
        }

        // Step 7: 重新初始化在默认地址，为下一轮做准备
        if (!init_keyboard_with_addr(bus, kDefaultAddr)) {
            ESP_LOGE(TOOLS_TAG, "Re-init at default addr failed after restore, abort");
            ++failed_count;
            break;
        }

        ESP_LOGI(TOOLS_TAG, "[PASS] target 0x%02X tested and restored to 0x%02X",
                 target_addr, kDefaultAddr);
        ++passed_count;
    }

    device_kb.end();

    if (failed_count > 0) {
        ESP_LOGE(TOOLS_TAG,
                 "[SUMMARY] tested=%u, pass=%u, fail=%u, skip=%u",
                 tested_count,
                 passed_count,
                 failed_count,
                 skipped_count);
    } else {
        ESP_LOGI(TOOLS_TAG,
                 "[SUMMARY] tested=%u, pass=%u, fail=%u, skip=%u",
                 tested_count,
                 passed_count,
                 failed_count,
                 skipped_count);
    }
}

void run_keyboard_i2c_address_test(void)
{
    ESP_LOGI(TOOLS_TAG, "Starting keyboard I2C address register test...");
    
    i2c_bus_handle_t kb_test_bus = create_kb_test_i2c_bus();
    if (kb_test_bus != nullptr) {
        run_kb_i2c_addr_register_test(kb_test_bus, KB_I2C_TEST_START_ADDR);
        i2c_bus_delete(&kb_test_bus);
    } else {
        ESP_LOGE(TOOLS_TAG, "Failed to create I2C bus for keyboard test");
    }
    
    ESP_LOGI(TOOLS_TAG, "Keyboard I2C address register test completed");
}
