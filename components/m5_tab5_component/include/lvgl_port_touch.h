/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * @file lvgl_port_touch.h
 * @brief LVGL è§¦æ¸è¾å¥æ¥å£
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
 * @brief è§¦æ¸éç½®ç»æ
 */
typedef struct {
    lv_display_t *disp;               /*!< LVGL æ¾ç¤ºå¥æ */
    esp_lcd_touch_handle_t handle;    /*!< LCD è§¦æ¸å¥æ */
    struct {
        float x;                      /*!< X è½´ç¼©æ¾æ¯ä¾?*/
        float y;                      /*!< Y è½´ç¼©æ¾æ¯ä¾?*/
    } scale;
} lvgl_touch_cfg_t;

/**
 * @brief æ·»å  LCD è§¦æ¸ä½ä¸ºè¾å¥è®¾å¤
 *
 * @param touch_cfg è§¦æ¸éç½®ç»æ
 * @return lv_indev_t* æåè¿å LVGL è¾å¥è®¾å¤å¥æï¼å¤±è´¥è¿å?NULL
 */
lv_indev_t *lvgl_port_add_touch(const lvgl_touch_cfg_t *touch_cfg);

/**
 * @brief ç§»é¤ LCD è§¦æ¸è¾å¥è®¾å¤
 *
 * @param touch è¾å¥è®¾å¤å¥æ
 * @return ESP_OK æå
 */
esp_err_t lvgl_port_remove_touch(lv_indev_t *touch);

/**
 * @brief è®¾ç½®è§¦æ¸æè½¬è§åº¦
 *
 * å½ä½¿ç?PPA ç¡¬ä»¶æè½¬æ¶ï¼LVGL ç?display rotation ä¸ä¼è¢«è®¾ç½®ï¼
 * éè¦åç¬è®¾ç½®è§¦æ¸çæè½¬è§åº¦æ¥å¹éæ¾ç¤ºæè½¬ã?
 *
 * @param touch è¾å¥è®¾å¤å¥æ
 * @param rotation æè½¬è§åº¦ (LV_DISPLAY_ROTATION_0/90/180/270)
 * @return ESP_OK æå
 */
esp_err_t lvgl_port_set_touch_rotation(lv_indev_t *touch, lv_display_rotation_t rotation);

/**
 * @brief è·åè§¦æ¸å½åæè½¬è§åº¦
 *
 * @param touch è¾å¥è®¾å¤å¥æ
 * @return lv_display_rotation_t å½åæè½¬è§åº¦
 */
lv_display_rotation_t lvgl_port_get_touch_rotation(lv_indev_t *touch);
#endif

#ifdef __cplusplus
}
#endif
