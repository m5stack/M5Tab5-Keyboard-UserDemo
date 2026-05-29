/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ESP LCD touch: GT911
 *        ESP LCD 触摸驱动：GT911
 */

#pragma once

#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a new GT911 touch driver.
 *        创建一个新�?GT911 触摸驱动实例�?
 *
 * @note The I2C communication must be initialized before this function is used.
 * @note 调用该函数前必须先完�?I2C 通信初始化�?
 *
 * @param io LCD/Touch panel IO handle
 * @param config Touch configuration
 * @param out_touch Touch instance handle
 * @param io LCD/Touch 面板 IO 句柄
 * @param config 触摸配置
 * @param out_touch 触摸实例句柄输出参数
 * @return
 *      - ESP_OK                    on success
 *      - ESP_ERR_NO_MEM            if there is no memory for allocating main structure
 *      - ESP_OK                    成功
 *      - ESP_ERR_NO_MEM            主结构体分配失败，内存不�?
 */
esp_err_t esp_lcd_touch_new_i2c_gt911(const esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *config, esp_lcd_touch_handle_t *out_touch);

/**
 * @brief I2C address of the GT911 controller.
 *        GT911 控制器的 I2C 地址�?
 *
 * @note When power-on detects a low level on the interrupt GPIO, the address is 0x5D.
 * @note 上电时如果检测到中断 GPIO 为低电平，则地址�?0x5D�?
 * @note When the interrupt GPIO is high, the address is 0x14.
 * @note 如果中断 GPIO 为高电平，则地址�?0x14�?
 *
 */
#define ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS          (0x5D)
#define ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP   (0x14)

/**
 * @brief GT911 configuration type.
 *        GT911 配置类型�?
 *
 */
typedef struct {
    uint8_t dev_addr;  /*!< I2C device address / I2C 设备地址 */
} esp_lcd_touch_io_gt911_config_t;

/**
 * @brief Touch IO configuration structure.
 *        触摸 IO 配置结构�?
 *
 */
#define ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG()           \
    {                                       \
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS, \
        .control_phase_bytes = 1,           \
        .dc_bit_offset = 0,                 \
        .lcd_cmd_bits = 16,                 \
        .flags =                            \
        {                                   \
            .disable_control_phase = 1,     \
        }                                   \
    }

#ifdef __cplusplus
}
#endif
