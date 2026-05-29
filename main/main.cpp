#include <stdio.h>
#include <inttypes.h>
#include <esp_log.h>
#include <esp_err.h>

#include "lcd_tools.h"
#include "ui_app.h"
#include "app_build_info.h"
#include "m5_tab5_component.h"
#include "m5_tab5_keyboard.h" // Hardware Keyboard Driver
#include "lvgl_port.h"      // Added for lvgl_port_lock/unlock
static const char* TAG = "main";

static m5::tab5::m5tab5_component s_tab5_board;

// Hardware Keyboard Instance
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

    // 测试TAB5_KB_REG_I2C_ADDR寄存器功能
    // run_keyboard_i2c_address_test();

    // Initialize LCD, Backlight, Touch and LVGL
    ret = app_lcd_lvgl_init(s_tab5_board);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LCD/LVGL: %s", esp_err_to_name(ret));
        return;
    }

    // 初始化硬件键盘，切换到普通模式，串口打印按下硬件键盘的log
    ESP_LOGI(TAG, "Initializing M5Tab5 Keyboard...");
    
    // Initialize Keyboard with separate I2C bus (I2C_NUM_1) and Hardware Interrupt
    // 使用独立 I2C 总线 (I2C_NUM_1) 和硬件中断初始化键盘
    // Pins: SDA=0, SCL=1, INT=50
    // 注意: 即使键盘未连接也继续启动，热插拔监控任务会自动处理重连
    m5_tab5_kb_err_t kb_err = device_kb.begin(
        I2C_NUM_1,
        M5_TAB5_KB_DEFAULT_ADDR,
        M5_TAB5_KB_DEFAULT_SDA,
        M5_TAB5_KB_DEFAULT_SCL,
        M5_TAB5_KB_I2C_FREQ_400K,
        M5_TAB5_KB_DEFAULT_INT,
        M5_TAB5_KB_INT_MODE_HARDWARE
    );
    
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
        ESP_LOGW(TAG, "Keyboard not detected at boot (err=%d), hotplug monitor will handle reconnection", kb_err);
    }

    // 创建 UI 界面（无论键盘是否连接都继续）
    lvgl_port_lock(0);
    ret = lvgl_ui_create(&device_kb, M5TAB5_MENU_SCOPE_ALL);
    lvgl_port_unlock();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create UI: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "UI created successfully");

    // 主循环
    ESP_LOGI(TAG, "Entering main loop...");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/*

    //创建一个线程，模拟输入字符串
    xTaskCreate([](void *arg){
        vTaskDelay(pdMS_TO_TICKS(2000));  // 等待2秒

        const char* text = "Hello,M5stack!";
        ESP_LOGI(TAG, "Starting to simulate string: %s", text);

        // 获取键盘实例
        M5Tab5Keyboard* kb = getM5Tab5KeyboardInstance();
        if (!kb) {
            ESP_LOGE(TAG, "Keyboard instance not found!");
            vTaskDelete(NULL);
            return;
        }

        // 逐个字符输入，每次输入后释放锁让LVGL刷新
        for (size_t i = 0; text[i] != '\0'; i++) {
            lvgl_port_lock(0);

            char single_char[2] = {text[i], '\0'};
            kb->simulateString(single_char, 0);

            lvgl_port_unlock();

            // 字符间延迟，让LVGL有时间刷新UI
            // 大写字母和特殊字符需要更长的延迟，因为组合键内部已经有延迟
            if ((text[i] >= 'A' && text[i] <= 'Z') || text[i] == ',') {
                vTaskDelay(pdMS_TO_TICKS(50));  // 组合键后短延迟
            } else {
                vTaskDelay(pdMS_TO_TICKS(150)); // 普通字符正常延迟
            }
        }

        ESP_LOGI(TAG, "String simulation complete");

        // 任务完成后删除自己
        vTaskDelete(NULL);
    }, "sim_str", 4096, NULL, 5, NULL);

*/

/*
    // Initialize LCD, Backlight, Touch and LVGL
    esp_err_t ret = app_lcd_lvgl_init(&io_expander_low_0x43, &io_expander_high_0x44);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LCD/LVGL: %s", esp_err_to_name(ret));
        return;
    }

    // 初始化硬件键盘，切换到普通模式，串口打印按下硬件键盘的log
    ESP_LOGI(TAG, "Initializing M5Tab5 Keyboard...");
    
    // Initialize Keyboard with separate I2C bus (I2C_NUM_1) and Hardware Interrupt
    // 使用独立 I2C 总线 (I2C_NUM_1) 和硬件中断初始化键盘
    // Pins: SDA=0, SCL=1, INT=50
    m5_tab5_kb_err_t kb_err = device_kb.begin(
        I2C_NUM_1, 
        M5_TAB5_KB_DEFAULT_ADDR, 
        M5_TAB5_KB_DEFAULT_SDA, 
        M5_TAB5_KB_DEFAULT_SCL, 
        M5_TAB5_KB_I2C_FREQ_400K, 
        M5_TAB5_KB_DEFAULT_INT, 
        M5_TAB5_KB_INT_MODE_HARDWARE
    );
    
    uint8_t kb_version = 0x00;
    if (kb_err == M5_TAB5_KB_OK) {
        ESP_LOGI(TAG, "Keyboard initialized successfully. Default Normal Mode Enabled.");
        if (device_kb.getVersion(&kb_version) != M5_TAB5_KB_OK) {
            ESP_LOGW(TAG, "Failed to read keyboard firmware version");
            kb_version = 0x00;
        } else {
             ESP_LOGI(TAG, "Keyboard FW: 0x%02X", kb_version);
        }

        // 设置键盘模式
        // device_kb.setMode(M5_TAB5_KB_MODE_STRING);
    } else {
        ESP_LOGE(TAG, "Failed to initialize Keyboard: %d", kb_err);
    }

    // 创建 UI 界面
    lvgl_port_lock(0);
    ret = lvgl_ui_create(&device_kb, M5TAB5_MENU_SCOPE_ALL);
    lvgl_port_unlock();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create UI: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "UI created successfully");
*/

