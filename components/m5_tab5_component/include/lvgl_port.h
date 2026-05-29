/**
 * @file lvgl_port.h
 * @brief LVGL IDF 移植�?- 主接�?
 *
 * 提供 LVGL �?ESP-IDF 的集成，包括�?
 * - LVGL 任务管理
 * - 显示驱动 (默认/DSI/RGB)
 * - 触摸输入
 *
 * 使用示例:
 * @code
 * // 1. 初始�?LVGL 端口
 * lvgl_port_cfg_t cfg = lvgl_PORT_INIT_CONFIG();
 * lvgl_port_init(&cfg);
 *
 * // 2. 添加 DSI 显示 (支持 PPA 旋转)
 * lvgl_disp_cfg_t disp_cfg = { ... };
 * lvgl_disp_dsi_cfg_t dsi_cfg = { .flags.avoid_tearing = 1, .flags.use_ppa = 1 };
 * lv_display_t *disp = lvgl_port_add_disp_dsi(&disp_cfg, &dsi_cfg);
 *
 * // 3. 添加触摸
 * lvgl_touch_cfg_t touch_cfg = { .disp = disp, .handle = tp_handle };
 * lv_indev_t *touch = lvgl_port_add_touch(&touch_cfg);
 *
 * // 4. 使用 LVGL
 * lvgl_port_lock(0);
 * // ... LVGL 操作 ...
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
 * @brief LVGL 端口任务事件类型
 */
typedef enum {
    lvgl_PORT_EVENT_DISPLAY = 0x01,
    lvgl_PORT_EVENT_TOUCH   = 0x02,
    lvgl_PORT_EVENT_USER    = 0x80,
} lvgl_port_event_type_t;

/**
 * @brief LVGL 端口任务事件
 */
typedef struct {
    lvgl_port_event_type_t type;
    void *param;
} lvgl_port_event_t;

/**
 * @brief 初始化配置结�?
 */
typedef struct {
    int task_priority;          /*!< LVGL 任务优先�?*/
    int task_stack;             /*!< LVGL 任务栈大�?*/
    int task_affinity;          /*!< LVGL 任务核心亲和�?(-1 表示无亲和�? */
    int task_max_sleep_ms;      /*!< LVGL 任务最大休眠时�?*/
    unsigned task_stack_caps;   /*!< LVGL 任务栈内存能�?(�?esp_heap_caps.h) */
    int timer_period_ms;        /*!< LVGL 定时器周�?(ms) */
} lvgl_port_cfg_t;

/**
 * @brief LVGL 端口默认配置
 */
#define lvgl_PORT_INIT_CONFIG()              \
    {                                              \
        .task_priority = 4,                        \
        .task_stack = 16384,                       \
        .task_affinity = -1,                       \
        .task_max_sleep_ms = 500,                  \
        .task_stack_caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_DEFAULT, \
        .timer_period_ms = 5,                      \
    }

/**
 * @brief 初始�?LVGL 端口
 *
 * @note 此函数初始化 LVGL 并创建定时器和任�?
 *
 * @param cfg 配置参数
 * @return
 *      - ESP_OK 成功
 *      - ESP_ERR_INVALID_ARG 参数无效
 *      - ESP_ERR_INVALID_STATE esp_timer 库未初始�?
 *      - ESP_ERR_NO_MEM 内存分配失败
 */
esp_err_t lvgl_port_init(const lvgl_port_cfg_t *cfg);

/**
 * @brief 反初始化 LVGL 端口
 *
 * @return
 *      - ESP_OK 成功
 *      - ESP_ERR_TIMEOUT 停止 LVGL 任务超时
 */
esp_err_t lvgl_port_deinit(void);

/**
 * @brief 获取 LVGL 互斥�?
 *
 * @param timeout_ms 超时时间 (ms)�? 表示永久等待
 * @return
 *      - true 成功获取�?
 *      - false 获取锁失�?
 */
bool lvgl_port_lock(uint32_t timeout_ms);

/**
 * @brief 释放 LVGL 互斥�?
 */
void lvgl_port_unlock(void);

/**
 * @brief 停止 LVGL 定时�?
 *
 * @return
 *      - ESP_OK 成功
 *      - ESP_ERR_INVALID_STATE 定时器未运行
 */
esp_err_t lvgl_port_stop(void);

/**
 * @brief 恢复 LVGL 定时�?
 *
 * @return
 *      - ESP_OK 成功
 *      - ESP_ERR_INVALID_STATE 定时器未运行
 */
esp_err_t lvgl_port_resume(void);

/**
 * @brief 唤醒 LVGL 任务
 *
 * @param event 事件类型
 * @param param 参数 (保留)
 * @return
 *      - ESP_OK 成功
 *      - ESP_ERR_NOT_SUPPORTED 未实�?
 *      - ESP_ERR_INVALID_STATE 队列未初始化
 */
esp_err_t lvgl_port_task_wake(lvgl_port_event_type_t event, void *param);

#ifdef __cplusplus
}
#endif
