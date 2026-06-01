/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ESP LCD touch: GT911
 *        ESP LCD è§¦æ¸é©±å¨ï¼GT911
 */

#pragma once

#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a new GT911 touch driver.
 *        åå»ºä¸ä¸ªæ°ç?GT911 è§¦æ¸é©±å¨å®ä¾ã?
 *
 * @note The I2C communication must be initialized before this function is used.
 * @note è°ç¨è¯¥å½æ°åå¿é¡»åå®æ?I2C éä¿¡åå§åã?
 *
 * @param io LCD/Touch panel IO handle
 * @param config Touch configuration
 * @param out_touch Touch instance handle
 * @param io LCD/Touch é¢æ¿ IO å¥æ
 * @param config è§¦æ¸éç½®
 * @param out_touch è§¦æ¸å®ä¾å¥æè¾åºåæ°
 * @return
 *      - ESP_OK                    on success
 *      - ESP_ERR_NO_MEM            if there is no memory for allocating main structure
 *      - ESP_OK                    æå
 *      - ESP_ERR_NO_MEM            ä¸»ç»æä½åéå¤±è´¥ï¼åå­ä¸è¶?
 */
esp_err_t esp_lcd_touch_new_i2c_gt911(const esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *config,
                                      esp_lcd_touch_handle_t *out_touch);

/**
 * @brief I2C address of the GT911 controller.
 *        GT911 æ§å¶å¨ç I2C å°åã?
 *
 * @note When power-on detects a low level on the interrupt GPIO, the address is 0x5D.
 * @note ä¸çµæ¶å¦ææ£æµå°ä¸­æ­ GPIO ä¸ºä½çµå¹³ï¼åå°åä¸?0x5Dã?
 * @note When the interrupt GPIO is
 * high, the address is 0x14.
 * @note å¦æä¸­æ­ GPIO ä¸ºé«çµå¹³ï¼åå°åä¸?0x14ã?
 *
 */
#define ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS        (0x5D)
#define ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP (0x14)

/**
 * @brief GT911 configuration type.
 *        GT911 éç½®ç±»åã?
 *
 */
typedef struct {
    uint8_t dev_addr; /*!< I2C device address / I2C è®¾å¤å°å */
} esp_lcd_touch_io_gt911_config_t;

/**
 * @brief Touch IO configuration structure.
 *        è§¦æ¸ IO éç½®ç»æã?
 *
 */
#define ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG()                                                           \
    {                                                                                                 \
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS, .control_phase_bytes = 1, .dc_bit_offset = 0, \
        .lcd_cmd_bits = 16, .flags = {                                                                \
            .disable_control_phase = 1,                                                               \
        }                                                                                             \
    }

#ifdef __cplusplus
}
#endif
