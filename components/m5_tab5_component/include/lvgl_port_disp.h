/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * @file lvgl_port_disp.h
 * @brief LVGL æ¾ç¤ºé©±å¨æ¥å£
 *
 * æä¾ä¸ç§æ¾ç¤ºæ¥å£ï¼?
 * 1. lvgl_port_add_disp()     - é»è®¤æ¥å£ (I2C/SPI/I8080)
 * 2. lvgl_port_add_disp_dsi() - MIPI-DSI æ¥å£
 * 3. lvgl_port_add_disp_rgb() - RGB å¹¶è¡æ¥å£
 *
 * æ ¸å¿ç¹æ§ï¼
 * - PPA ç¡¬ä»¶å éæè½?(ESP32-P4)
 * - åºåç»å¶ (partial update)
 * - åç¼å?+ vsync é²æè£?
 */

#pragma once

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief æè½¬éç½®
 */
typedef struct {
    bool swap_xy;   /*!< LCD å±å¹ X/Y äº¤æ¢ (å?esp_lcd é©±å¨ä¸? */
    bool mirror_x;  /*!< LCD å±å¹ X éå (å?esp_lcd é©±å¨ä¸? */
    bool mirror_y;  /*!< LCD å±å¹ Y éå (å?esp_lcd é©±å¨ä¸? */
} lvgl_rotation_cfg_t;

/**
 * @brief æ¾ç¤ºéç½®ç»æ (éç¨)
 */
typedef struct {
    esp_lcd_panel_io_handle_t io_handle;      /*!< LCD é¢æ¿ IO å¥æ */
    esp_lcd_panel_handle_t panel_handle;      /*!< LCD é¢æ¿å¥æ */
    esp_lcd_panel_handle_t control_handle;    /*!< LCD é¢æ¿æ§å¶å¥æ (å¯é? */

    uint32_t buffer_size;     /*!< å±å¹ç¼å²åºå¤§å°?(åç´ æ? */
    bool     double_buffer;   /*!< æ¯å¦åéåç¼å?*/
    uint32_t trans_size;      /*!< ä¼ è¾ç¼å²åºå¤§å°?(å¯é? ç¨äº SRAM ä¸­è½¬) */

    uint32_t hres;            /*!< LCD æ°´å¹³åè¾¨ç?*/
    uint32_t vres;            /*!< LCD åç´åè¾¨ç?*/

    bool monochrome;          /*!< æ¯å¦ä¸ºåè²æ¾ç¤?(1bit/pixel) */

    lvgl_rotation_cfg_t rotation;       /*!< å±å¹æè½¬éç½® (ä»ç¡¬ä»¶ç¶æ? */
    lv_color_format_t color_format;           /*!< é¢è²æ ¼å¼ */

    struct {
        unsigned int buff_dma: 1;       /*!< LVGL ç¼å²åºä½¿ç?DMA åå­ */
        unsigned int buff_spiram: 1;    /*!< LVGL ç¼å²åºä½¿ç?PSRAM */
        unsigned int sw_rotate: 1;      /*!< ä½¿ç¨è½¯ä»¶æè½¬ (è¾æ¢) æ?PPA (å¦æå¯ç¨) */
        unsigned int swap_bytes: 1;     /*!< RGB565 å­èäº¤æ¢ */
        unsigned int full_refresh: 1;   /*!< å§ç»å¨å±å·æ° */
        unsigned int direct_mode: 1;    /*!< ä½¿ç¨å¨å±ç¼å²åºï¼ç´æ¥ç»å¶å°ç»å¯¹åæ ?*/
    } flags;
} lvgl_disp_cfg_t;

/**
 * @brief RGB æ¾ç¤ºç¹å®éç½®
 */
typedef struct {
    struct {
        unsigned int bb_mode: 1;        /*!< ä½¿ç¨ bounce buffer æ¨¡å¼ */
        unsigned int avoid_tearing: 1;  /*!< ä½¿ç¨åé¨ RGB ç¼å²åºä½ä¸?LVGL ç»å¾ç¼å²åºä»¥é¿åæè£ */
    } flags;
} lvgl_disp_rgb_cfg_t;

/**
 * @brief MIPI-DSI æ¾ç¤ºç¹å®éç½®
 */
typedef struct {
    lv_display_rotation_t sw_rotation;  /*!< è½¯ä»¶æè½¬è§åº¦ (PPA å é? */
    struct {
        unsigned int avoid_tearing: 1;  /*!< ä½¿ç¨åé¨ DSI ç¼å²åºä½ä¸?LVGL ç»å¾ç¼å²åºä»¥é¿åæè£ */
        unsigned int use_ppa: 1;        /*!< ä½¿ç¨ PPA ç¡¬ä»¶å éæè½?*/
    } flags;
} lvgl_disp_dsi_cfg_t;

/**
 * @brief æ·»å  I2C/SPI/I8080 æ¾ç¤ºå?LVGL
 *
 * @param disp_cfg æ¾ç¤ºéç½®ç»æ
 * @return lv_display_t* æåè¿å LVGL æ¾ç¤ºå¥æï¼å¤±è´¥è¿å?NULL
 */
lv_display_t *lvgl_port_add_disp(const lvgl_disp_cfg_t *disp_cfg);

/**
 * @brief æ·»å  MIPI-DSI æ¾ç¤ºå?LVGL
 *
 * @note æ¯æ PPA ç¡¬ä»¶å éæè½?+ åºåç»å¶ + avoid_tearing
 *
 * @param disp_cfg æ¾ç¤ºéç½®ç»æ
 * @param dsi_cfg MIPI-DSI ç¹å®éç½®
 * @return lv_display_t* æåè¿å LVGL æ¾ç¤ºå¥æï¼å¤±è´¥è¿å?NULL
 */
lv_display_t *lvgl_port_add_disp_dsi(const lvgl_disp_cfg_t *disp_cfg, const lvgl_disp_dsi_cfg_t *dsi_cfg);

/**
 * @brief æ·»å  RGB å¹¶è¡æ¾ç¤ºå?LVGL
 *
 * @param disp_cfg æ¾ç¤ºéç½®ç»æ
 * @param rgb_cfg RGB ç¹å®éç½®
 * @return lv_display_t* æåè¿å LVGL æ¾ç¤ºå¥æï¼å¤±è´¥è¿å?NULL
 */
lv_display_t *lvgl_port_add_disp_rgb(const lvgl_disp_cfg_t *disp_cfg, const lvgl_disp_rgb_cfg_t *rgb_cfg);

/**
 * @brief ä»?LVGL ç§»é¤æ¾ç¤º
 *
 * @param disp æ¾ç¤ºå¥æ
 * @return ESP_OK æå
 */
esp_err_t lvgl_port_remove_disp(lv_display_t *disp);

/**
 * @brief éç¥ LVGL æ°æ®å·²å·æ°å° LCD
 *
 * @param disp LVGL æ¾ç¤ºå¥æ
 */
void lvgl_port_flush_ready(lv_display_t *disp);

#ifdef __cplusplus
}
#endif
