/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <esp_log.h>
#include <esp_err.h>

#include "lcd_tools.h"
#include "tools.h"
#include "ui_app.h"
#include "app_build_info.h"
#include "m5_tab5_component.h"
#include "m5_tab5_keyboard.h"
#include "lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "main";

static m5::tab5::m5tab5_component s_tab5_board;
m5::M5Tab5Keyboard device_kb;

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "SW version: %s", FIRMWARE_VERSION);
    ESP_LOGI(TAG, "Build time: %s", FIRMWARE_BUILD_TIME);

    ESP_LOGI(TAG, "Initializing Tab5 hardware via m5_tab5_component");

    m5::tab5::m5tab5_component_config_t board_cfg = {};

    esp_err_t ret = s_tab5_board.begin(board_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize Tab5 hardware: %s", esp_err_to_name(ret));
        return;
    }

    const m5::tab5::m5tab5_variant_descriptor_t* variant = s_tab5_board.variant();
    if (variant != nullptr) {
        ESP_LOGI(TAG, "Detected Tab5 variant: %s", variant->id);
    }

    ret = s_tab5_board.wlan_power(true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to power on C6: %s", esp_err_to_name(ret));
        return;
    }

    // Initialize LCD, Backlight, Touch and LVGL
    ret = app_lcd_lvgl_init(s_tab5_board);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LCD/LVGL: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "Initializing M5Tab5 Keyboard...");

    // Initialize keyboard using standard I2C_NUM_1 and hardware interrupt pin
    m5_tab5_kb_err_t kb_err =
        device_kb.begin(I2C_NUM_1, M5_TAB5_KB_DEFAULT_ADDR, M5_TAB5_KB_DEFAULT_SDA, M5_TAB5_KB_DEFAULT_SCL,
                        M5_TAB5_KB_I2C_FREQ_400K, M5_TAB5_KB_DEFAULT_INT, M5_TAB5_KB_INT_MODE_HARDWARE);

    uint8_t kb_version = 0x00;
    if (kb_err == M5_TAB5_KB_OK) {
        ESP_LOGI(TAG, "Keyboard initialized successfully. Default Normal Mode Enabled.");
        if (device_kb.getVersion(&kb_version) != M5_TAB5_KB_OK) {
            ESP_LOGW(TAG, "Failed to read keyboard firmware version");
            kb_version = 0x00;
        } else {
            ESP_LOGI(TAG, "Keyboard FW: 0x%02X", kb_version);
        }
    } else {
        ESP_LOGW(TAG, "Keyboard not detected at boot, hotplug monitor will handle reconnection");
    }

    // Create user interface
    lvgl_port_lock(0);
    ret = lvgl_ui_create(&device_kb, M5TAB5_MENU_SCOPE_ALL);
    lvgl_port_unlock();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create UI: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "UI created successfully");

    ret = start_battery_charge_monitor(&s_tab5_board);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start battery charge monitor: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "Entering main loop...");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
