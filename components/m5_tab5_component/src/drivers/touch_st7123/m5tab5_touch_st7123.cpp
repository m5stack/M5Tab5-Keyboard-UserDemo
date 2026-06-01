/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#include "drivers/touch_st7123/m5tab5_touch_st7123.h"
#include "drivers/touch_st7123/esp_lcd_touch_st7123.h"

#include "drivers/m5tab5_driver_common.h"
#include "m5tab5_pinmap.h"

#include "esp_check.h"
#include "esp_lcd_panel_io.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

namespace m5::tab5 {
namespace {

static m5tab5_touch_st7123_handles_t s_handles;

esp_err_t m5tab5_touch_st7123_probe(m5tab5_runtime_t* runtime)
{
    (void)runtime;
    ESP_LOGI(m5tab5_driver_log_tag(), "touch_st7123 probe (TDDI â?requires LCD init first)");
    return ESP_OK;
}

esp_err_t m5tab5_touch_st7123_init(m5tab5_runtime_t* runtime)
{
    const char* TAG = m5tab5_driver_log_tag();

    // 1. Get the shared SYS I2C master bus handle / è·åå±äº«ç?SYS I2C
    // ä¸»æºæ»çº¿å¥æ
    i2c_master_bus_handle_t i2c_bus = m5tab5_get_sys_i2c_master_bus_handle();
    if (!i2c_bus) {
        ESP_LOGE(TAG, "SYS I2C master bus not available");
        return ESP_ERR_INVALID_STATE;
    }

    // 2. Create I2C panel IO for ST7123 touch / ä¸?ST7123 è§¦æ¸æ§å¶å¨åå»?I2C é¢æ¿
    // IO
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_ST7123_CONFIG();
    tp_io_config.scl_speed_hz                  = 400000;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(i2c_bus, &tp_io_config, &s_handles.tp_io), TAG,
                        "new panel IO I2C failed");

    // 3. Create the ST7123 touch handle / åå»º ST7123 è§¦æ¸å¥æ
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max        = 720,
        .y_max        = 1280,
        .rst_gpio_num = M5TAB5_PIN_LCD_TOUCH_RST,  // NC, controlled via IO expander / NCï¼ç± IO
                                                   // æ©å±å¨æ§å?
        .int_gpio_num = M5TAB5_PIN_LCD_TOUCH_INT,  // GPIO23 / GPIO23 ä¸­æ­è?
        .levels =
            {
                .reset     = 0,
                .interrupt = 0,
            },
        .flags =
            {
                .swap_xy  = 0,
                .mirror_x = 0,
                .mirror_y = 0,
            },
    };
    ESP_RETURN_ON_ERROR(esp_lcd_touch_new_i2c_st7123(s_handles.tp_io, &tp_cfg, &s_handles.tp), TAG,
                        "new ST7123 touch failed");

    ESP_LOGI(TAG, "ST7123 touch panel initialized");

    runtime->touch_handle = &s_handles;
    return ESP_OK;
}

const m5tab5_device_driver_descriptor_t k_m5tab5_touch_st7123_driver = {
    .id    = "m5tab5.touch.st7123",
    .probe = m5tab5_touch_st7123_probe,
    .init  = m5tab5_touch_st7123_init,
};

}  // namespace

const m5tab5_device_driver_descriptor_t* m5tab5_get_touch_st7123_driver()
{
    return &k_m5tab5_touch_st7123_driver;
}

}  // namespace m5::tab5

#pragma GCC diagnostic pop