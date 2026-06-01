/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * SPDX-License-Identifier: Apache-2.0
 * ST7123 MIPI-DSI touch panel driver (TDDI) for M5Stack Tab5
 * éç¨äº?M5Stack Tab5 ç?ST7123 MIPI-DSI è§¦æ¸é¢æ¿é©±å¨ï¼TDDIï¼?
 * Standalone esp_lcd_touch driver adapted from the esp-iot-solution example.
 * ç¬ç«ç?esp_lcd_touch é©±å¨ï¼æ¹ç¼èª esp-iot-solution ç¤ºä¾ã?
 */
#pragma once

#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a new ST7123 touch driver.
 *        åå»ºä¸ä¸ªæ°ç?ST7123 è§¦æ¸é©±å¨å®ä¾ã?
 *
 * @note  The I2C communication must be initialized before this function is used.
 * @note  è°ç¨è¯¥å½æ°åå¿é¡»åå®æ?I2C éä¿¡åå§åã?
 * @note  ST7123 is a TDDI panel, so the LCD must be initialized before touch can work.
 * @note  ST7123 å±äº TDDI
 * é¢æ¿ï¼å æ­¤å¿é¡»ååå§å?LCDï¼è§¦æ¸åè½æè½æ­£å¸¸å·¥ä½ã?
 *
 * @param[in]  io     LCD panel IO handle, created by esp_lcd_new_panel_io_i2c()
 * @param[in]  config Touch panel configuration
 * @param[out] tp     Returned touch panel handle
 * @param[in]  io     ç?esp_lcd_new_panel_io_i2c() åå»ºç?LCD é¢æ¿ IO å¥æ
 * @param[in]  config è§¦æ¸é¢æ¿éç½®
 * @param[out] tp     è¿åçè§¦æ¸é¢æ¿å¥æ?
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG if parameter is invalid
 *      - ESP_ERR_NO_MEM if out of memory
 *      - ESP_OK æå
 *      - ESP_ERR_INVALID_ARG åæ°æ æ
 *      - ESP_ERR_NO_MEM åå­ä¸è¶³
 */
esp_err_t esp_lcd_touch_new_i2c_st7123(const esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *config,
                                       esp_lcd_touch_handle_t *tp);

/**
 * @brief I2C address of the ST7123 touch controller.
 *        ST7123 è§¦æ¸æ§å¶å¨ç I2C å°åã?
 */
#define ESP_LCD_TOUCH_IO_I2C_ST7123_ADDRESS (0x55)

/**
 * @brief Touch IO configuration macro for ST7123.
 *        ST7123 çè§¦æ?IO éç½®å®ã?
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
