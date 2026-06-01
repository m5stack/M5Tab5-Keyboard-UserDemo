/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * @file lvgl_port.h
 * @brief LVGL IDF ç§»æ¤å±?- ä¸»æ¥å?
 *
 * æä¾ LVGL ä¸?ESP-IDF çéæï¼åæ¬ï¼?
 * - LVGL ä»»å¡ç®¡ç
 * - æ¾ç¤ºé©±å¨ (é»è®¤/DSI/RGB)
 * - è§¦æ¸è¾å¥
 *
 * ä½¿ç¨ç¤ºä¾:
 * @code
 * // 1. åå§å?LVGL ç«¯å£
 * lvgl_port_cfg_t cfg = lvgl_PORT_INIT_CONFIG();
 * lvgl_port_init(&cfg);
 *
 * // 2. æ·»å  DSI æ¾ç¤º (æ¯æ PPA æè½¬)
 * lvgl_disp_cfg_t disp_cfg = { ... };
 * lvgl_disp_dsi_cfg_t dsi_cfg = { .flags.avoid_tearing = 1, .flags.use_ppa = 1 };
 * lv_display_t *disp = lvgl_port_add_disp_dsi(&disp_cfg, &dsi_cfg);
 *
 * // 3. æ·»å è§¦æ¸
 * lvgl_touch_cfg_t touch_cfg = { .disp = disp, .handle = tp_handle };
 * lv_indev_t *touch = lvgl_port_add_touch(&touch_cfg);
 *
 * // 4. ä½¿ç¨ LVGL
 * lvgl_port_lock(0);
 * // ... LVGL æä½ ...
 * lvgl_port_unlock();
 * @endcode
 */

#pragma once

#include "esp_err.h"
#include "esp_heap_caps.h"
#include "lvgl.h"
#include "lvgl_port_disp.h"
#include "lvgl_port_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LVGL ç«¯å£ä»»å¡äºä»¶ç±»å
 */
typedef enum {
    lvgl_PORT_EVENT_DISPLAY = 0x01,
    lvgl_PORT_EVENT_TOUCH   = 0x02,
    lvgl_PORT_EVENT_USER    = 0x80,
} lvgl_port_event_type_t;

/**
 * @brief LVGL ç«¯å£ä»»å¡äºä»¶
 */
typedef struct {
    lvgl_port_event_type_t type;
    void *param;
} lvgl_port_event_t;

/**
 * @brief åå§åéç½®ç»æ?
 */
typedef struct {
    int task_priority;     /*!< LVGL ä»»å¡ä¼åçº?*/
    int task_stack;        /*!< LVGL ä»»å¡æ å¤§å°?*/
    int task_affinity;     /*!< LVGL ä»»å¡æ ¸å¿äº²åæ?(-1 è¡¨ç¤ºæ äº²åæ? */
    int task_max_sleep_ms; /*!< LVGL ä»»å¡æå¤§ä¼ç æ¶é?*/
    unsigned task_stack_caps; /*!< LVGL ä»»å¡æ åå­è½å?(è§?esp_heap_caps.h) */
    int timer_period_ms;      /*!< LVGL å®æ¶å¨å¨æ?(ms) */
} lvgl_port_cfg_t;

/**
 * @brief LVGL ç«¯å£é»è®¤éç½®
 */
#define lvgl_PORT_INIT_CONFIG()                                        \
    {                                                                  \
        .task_priority     = 4,                                        \
        .task_stack        = 16384,                                    \
        .task_affinity     = -1,                                       \
        .task_max_sleep_ms = 500,                                      \
        .task_stack_caps   = MALLOC_CAP_INTERNAL | MALLOC_CAP_DEFAULT, \
        .timer_period_ms   = 5,                                        \
    }

/**
 * @brief åå§å?LVGL ç«¯å£
 *
 * @note æ­¤å½æ°åå§å LVGL å¹¶åå»ºå®æ¶å¨åä»»å?
 *
 * @param cfg éç½®åæ°
 * @return
 *      - ESP_OK æå
 *      - ESP_ERR_INVALID_ARG åæ°æ æ
 *      - ESP_ERR_INVALID_STATE esp_timer åºæªåå§å?
 *      - ESP_ERR_NO_MEM åå­åéå¤±è´¥
 */
esp_err_t lvgl_port_init(const lvgl_port_cfg_t *cfg);

/**
 * @brief ååå§å LVGL ç«¯å£
 *
 * @return
 *      - ESP_OK æå
 *      - ESP_ERR_TIMEOUT åæ­¢ LVGL ä»»å¡è¶æ¶
 */
esp_err_t lvgl_port_deinit(void);

/**
 * @brief è·å LVGL äºæ¥é?
 *
 * @param timeout_ms è¶æ¶æ¶é´ (ms)ï¼? è¡¨ç¤ºæ°¸ä¹ç­å¾
 * @return
 *      - true æåè·åé?
 *      - false è·åéå¤±è´?
 */
bool lvgl_port_lock(uint32_t timeout_ms);

/**
 * @brief éæ¾ LVGL äºæ¥é?
 */
void lvgl_port_unlock(void);

/**
 * @brief åæ­¢ LVGL å®æ¶å?
 *
 * @return
 *      - ESP_OK æå
 *      - ESP_ERR_INVALID_STATE å®æ¶å¨æªè¿è¡
 */
esp_err_t lvgl_port_stop(void);

/**
 * @brief æ¢å¤ LVGL å®æ¶å?
 *
 * @return
 *      - ESP_OK æå
 *      - ESP_ERR_INVALID_STATE å®æ¶å¨æªè¿è¡
 */
esp_err_t lvgl_port_resume(void);

/**
 * @brief å¤é LVGL ä»»å¡
 *
 * @param event äºä»¶ç±»å
 * @param param åæ° (ä¿ç)
 * @return
 *      - ESP_OK æå
 *      - ESP_ERR_NOT_SUPPORTED æªå®ç?
 *      - ESP_ERR_INVALID_STATE éåæªåå§å
 */
esp_err_t lvgl_port_task_wake(lvgl_port_event_type_t event, void *param);

#ifdef __cplusplus
}
#endif
