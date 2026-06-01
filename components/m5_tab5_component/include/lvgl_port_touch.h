/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * @file lvgl_port_touch.h
 * @brief LVGL 触摸输入接口
 */

#pragma once

#include "esp_err.h"
#include "lvgl.h"

#if __has_include("esp_lcd_touch.h")
#include "esp_lcd_touch.h"
#define lvgl_PORT_TOUCH_COMPONENT 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef lvgl_PORT_TOUCH_COMPONENT
/**
 * @brief 触摸配置结构
 */
typedef struct {
    lv_display_t *disp;            /*!< LVGL 显示句柄 */
    esp_lcd_touch_handle_t handle; /*!< LCD 触摸句柄 */
    struct {
        float x; /*!< X 轴缩放比�?*/
        float y; /*!< Y 轴缩放比�?*/
    } scale;
} lvgl_touch_cfg_t;

/**
 * @brief 添加 LCD 触摸作为输入设备
 *
 * @param touch_cfg 触摸配置结构
 * @return lv_indev_t* 成功返回 LVGL
 * 输入设备句柄，失败返�?NULL
 */
lv_indev_t *lvgl_port_add_touch(const lvgl_touch_cfg_t *touch_cfg);

/**
 * @brief 移除 LCD 触摸输入设备
 *
 * @param touch 输入设备句柄
 * @return ESP_OK 成功
 */
esp_err_t lvgl_port_remove_touch(lv_indev_t *touch);

/**
 * @brief 设置触摸旋转角度
 *
 * 当使�?PPA 硬件旋转时，LVGL �?display rotation
 * 不会被设置，
 * 需要单独设置触摸的旋转角度来匹配显示旋转�?
 *
 * @param touch 输入设备句柄
 * @param rotation 旋转角度 (LV_DISPLAY_ROTATION_0/90/180/270)
 * @return ESP_OK 成功
 */
esp_err_t lvgl_port_set_touch_rotation(lv_indev_t *touch, lv_display_rotation_t rotation);

/**
 * @brief 获取触摸当前旋转角度
 *
 * @param touch 输入设备句柄
 * @return lv_display_rotation_t 当前旋转角度
 */
lv_display_rotation_t lvgl_port_get_touch_rotation(lv_indev_t *touch);
#endif

#ifdef __cplusplus
}
#endif
