/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * SPDX-License-Identifier: Apache-2.0
 * ST7123 MIPI-DSI touch panel driver (TDDI) for M5Stack Tab5
 * 适用�?M5Stack Tab5 �?ST7123 MIPI-DSI 触摸面板驱动（TDDI�?
 * Standalone esp_lcd_touch driver adapted from the esp-iot-solution example.
 * 独立�?esp_lcd_touch 驱动，改编自 esp-iot-solution 示例�?
 */
#pragma once

#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a new ST7123 touch driver.
 *        创建一个新�?ST7123 触摸驱动实例�?
 *
 * @note  The I2C communication must be initialized before this function is used.
 * @note  调用该函数前必须先完�?I2C 通信初始化�?
 * @note  ST7123 is a TDDI panel, so the LCD must be initialized before touch can work.
 * @note  ST7123 属于 TDDI
 * 面板，因此必须先初始�?LCD，触摸功能才能正常工作�?
 *
 * @param[in]  io     LCD panel IO handle, created by esp_lcd_new_panel_io_i2c()
 * @param[in]  config Touch panel configuration
 * @param[out] tp     Returned touch panel handle
 * @param[in]  io     �?esp_lcd_new_panel_io_i2c() 创建�?LCD 面板 IO 句柄
 * @param[in]  config 触摸面板配置
 * @param[out] tp     返回的触摸面板句�?
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG if parameter is invalid
 *      - ESP_ERR_NO_MEM if out of memory
 *      - ESP_OK 成功
 *      - ESP_ERR_INVALID_ARG 参数无效
 *      - ESP_ERR_NO_MEM 内存不足
 */
esp_err_t esp_lcd_touch_new_i2c_st7123(const esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *config,
                                       esp_lcd_touch_handle_t *tp);

/**
 * @brief I2C address of the ST7123 touch controller.
 *        ST7123 触摸控制器的 I2C 地址�?
 */
#define ESP_LCD_TOUCH_IO_I2C_ST7123_ADDRESS (0x55)

/**
 * @brief Touch IO configuration macro for ST7123.
 *        ST7123 的触�?IO 配置宏�?
 */
#define ESP_LCD_TOUCH_IO_I2C_ST7123_CONFIG()                                                                      \
    {                                                                                                             \
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_ST7123_ADDRESS, .control_phase_bytes = 1, .lcd_cmd_bits = 16, .flags = { \
            .disable_control_phase = 1,                                                                           \
        }                                                                                                         \
    }

#ifdef __cplusplus
}
#endif
