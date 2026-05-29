/**
 * @file lvgl_port.cpp
 * @brief LVGL IDF 移植层实�?- 主模�?
 */

#include "lvgl_port.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

static const char *TAG = "lvgl_port";

// LVGL 端口上下�?
typedef struct {
    TaskHandle_t task_handle;
    esp_timer_handle_t tick_timer;
    SemaphoreHandle_t lvgl_mutex;
    int task_max_sleep_ms;
    bool initialized;
    bool running;
} lvgl_port_ctx_t;

static lvgl_port_ctx_t s_port_ctx{};

// LVGL 定时器回�?
static void lvgl_tick_cb(void *arg)
{
    lv_tick_inc(*(int *)arg);
}

// LVGL 任务
static void lvgl_task(void *arg)
{
    ESP_LOGI(TAG, "LVGL task started");

    // 等待一小段时间让系统稳�?
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "LVGL task entering main loop");

    while (s_port_ctx.running) {
        // 使用短超时而不是无限等�?
        if (xSemaphoreTakeRecursive(s_port_ctx.lvgl_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            uint32_t sleep_ms = lv_timer_handler();
            xSemaphoreGiveRecursive(s_port_ctx.lvgl_mutex);

            if ((int)sleep_ms > s_port_ctx.task_max_sleep_ms) {
                sleep_ms = s_port_ctx.task_max_sleep_ms;
            }
            if (sleep_ms < 1) {
                sleep_ms = 1;
            }
            vTaskDelay(pdMS_TO_TICKS(sleep_ms));
        } else {
            ESP_LOGW(TAG, "LVGL task: failed to acquire lock");
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    // 任务退出前清理
    s_port_ctx.task_handle = NULL;
    vTaskDelete(NULL);
}

esp_err_t lvgl_port_init(const lvgl_port_cfg_t *cfg)
{
    if (s_port_ctx.initialized) {
        ESP_LOGW(TAG, "LVGL port already initialized");
        return ESP_OK;
    }

    if (cfg == NULL) {
        ESP_LOGE(TAG, "Invalid configuration");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Initializing LVGL port...");

    // 初始�?LVGL
    lv_init();

    // 创建递归互斥�?
    s_port_ctx.lvgl_mutex = xSemaphoreCreateRecursiveMutex();
    if (s_port_ctx.lvgl_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create LVGL mutex");
        return ESP_ERR_NO_MEM;
    }

    s_port_ctx.task_max_sleep_ms = cfg->task_max_sleep_ms;

    // 创建 LVGL tick 定时�?
    static int timer_period_ms;
    timer_period_ms = cfg->timer_period_ms;

    const esp_timer_create_args_t timer_args = {
        .callback = lvgl_tick_cb,
        .arg = &timer_period_ms,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "lvgl_tick",
        .skip_unhandled_events = false,
    };

    esp_err_t ret = esp_timer_create(&timer_args, &s_port_ctx.tick_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create LVGL tick timer: %s", esp_err_to_name(ret));
        vSemaphoreDelete(s_port_ctx.lvgl_mutex);
        return ret;
    }

    ret = esp_timer_start_periodic(s_port_ctx.tick_timer, cfg->timer_period_ms * 1000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start LVGL tick timer: %s", esp_err_to_name(ret));
        esp_timer_delete(s_port_ctx.tick_timer);
        vSemaphoreDelete(s_port_ctx.lvgl_mutex);
        return ret;
    }

    // 创建 LVGL 任务
    s_port_ctx.running = true;

    unsigned stack_caps = cfg->task_stack_caps;
    if (stack_caps == 0) {
        stack_caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_DEFAULT;
    }

    StackType_t *stack = (StackType_t *)heap_caps_malloc(cfg->task_stack, stack_caps);
    if (stack == NULL) {
        ESP_LOGE(TAG, "Failed to allocate task stack");
        esp_timer_stop(s_port_ctx.tick_timer);
        esp_timer_delete(s_port_ctx.tick_timer);
        vSemaphoreDelete(s_port_ctx.lvgl_mutex);
        return ESP_ERR_NO_MEM;
    }

    static StaticTask_t task_buffer;
    s_port_ctx.task_handle = xTaskCreateStaticPinnedToCore(
        lvgl_task,
        "lvgl_task",
        cfg->task_stack / sizeof(StackType_t),
        NULL,
        cfg->task_priority,
        stack,
        &task_buffer,
        cfg->task_affinity
    );

    if (s_port_ctx.task_handle == NULL) {
        ESP_LOGE(TAG, "Failed to create LVGL task");
        heap_caps_free(stack);
        esp_timer_stop(s_port_ctx.tick_timer);
        esp_timer_delete(s_port_ctx.tick_timer);
        vSemaphoreDelete(s_port_ctx.lvgl_mutex);
        return ESP_ERR_NO_MEM;
    }

    s_port_ctx.initialized = true;
    ESP_LOGI(TAG, "LVGL port initialized successfully");

    return ESP_OK;
}

esp_err_t lvgl_port_deinit(void)
{
    if (!s_port_ctx.initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Deinitializing LVGL port...");

    // 停止任务
    s_port_ctx.running = false;

    // 等待任务退�?
    int timeout = 50;
    while (s_port_ctx.task_handle != NULL && timeout > 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
        timeout--;
    }

    if (s_port_ctx.task_handle != NULL) {
        ESP_LOGW(TAG, "LVGL task did not exit in time, forcing delete");
        vTaskDelete(s_port_ctx.task_handle);
        s_port_ctx.task_handle = NULL;
    }

    // 停止定时�?
    if (s_port_ctx.tick_timer) {
        esp_timer_stop(s_port_ctx.tick_timer);
        esp_timer_delete(s_port_ctx.tick_timer);
        s_port_ctx.tick_timer = NULL;
    }

    // 删除互斥�?
    if (s_port_ctx.lvgl_mutex) {
        vSemaphoreDelete(s_port_ctx.lvgl_mutex);
        s_port_ctx.lvgl_mutex = NULL;
    }

    s_port_ctx.initialized = false;
    ESP_LOGI(TAG, "LVGL port deinitialized");

    return ESP_OK;
}

bool lvgl_port_lock(uint32_t timeout_ms)
{
    if (s_port_ctx.lvgl_mutex == NULL) {
        return false;
    }

    TickType_t ticks = (timeout_ms == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(s_port_ctx.lvgl_mutex, ticks) == pdTRUE;
}

void lvgl_port_unlock(void)
{
    if (s_port_ctx.lvgl_mutex) {
        xSemaphoreGiveRecursive(s_port_ctx.lvgl_mutex);
    }
}

esp_err_t lvgl_port_stop(void)
{
    if (s_port_ctx.tick_timer == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    return esp_timer_stop(s_port_ctx.tick_timer);
}

esp_err_t lvgl_port_resume(void)
{
    if (s_port_ctx.tick_timer == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    // 获取原来的周期并重新启动
    return esp_timer_start_periodic(s_port_ctx.tick_timer, 5 * 1000);  // 默认 5ms
}

esp_err_t lvgl_port_task_wake(lvgl_port_event_type_t event, void *param)
{
    // 目前简单实现，后续可以添加事件队列
    (void)event;
    (void)param;
    return ESP_OK;
}
