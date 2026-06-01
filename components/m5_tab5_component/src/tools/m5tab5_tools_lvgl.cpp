/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#include "m5tab5_tools.h"

#include "esp_check.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"
#include "esp_lvgl_port.h"

#include "drivers/m5tab5_driver_common.h"

namespace m5::tab5 {
namespace {

struct DisplayHandles {
    void* mipi_dsi_bus;
    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_handle_t panel;
};

struct TouchHandles {
    esp_lcd_panel_io_handle_t tp_io;
    esp_lcd_touch_handle_t tp;
};

static constexpr uint32_t LCD_W = 720;
static constexpr uint32_t LCD_H = 1280;

}  // namespace

esp_err_t m5tab5_lvgl_init(const m5tab5_runtime_t& runtime, lv_display_t** lv_display, lv_indev_t** lv_touch_indev)
{
    const char* TAG = m5tab5_driver_log_tag();

    ESP_RETURN_ON_FALSE(runtime.display_handle != nullptr, ESP_ERR_INVALID_STATE, TAG,
                        "lvgl_init: display not initialized");
    ESP_RETURN_ON_FALSE(lv_display != nullptr, ESP_ERR_INVALID_ARG, TAG, "lvgl_init: lv_display output is null");
    ESP_RETURN_ON_FALSE(lv_touch_indev != nullptr, ESP_ERR_INVALID_ARG, TAG,
                        "lvgl_init: lv_touch_indev output is null");

    *lv_display     = nullptr;
    *lv_touch_indev = nullptr;

    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    port_cfg.task_stack      = 16 * 1024;
    port_cfg.task_affinity   = 1;
    ESP_RETURN_ON_ERROR(lvgl_port_init(&port_cfg), TAG, "lvgl_port_init failed");

    auto* display_handles = static_cast<DisplayHandles*>(runtime.display_handle);

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle      = nullptr,
        .panel_handle   = display_handles->panel,
        .control_handle = nullptr,
        .buffer_size    = LCD_W * LCD_H,
        .double_buffer  = true,
        .trans_size     = 0,
        .hres           = LCD_W,
        .vres           = LCD_H,
        .monochrome     = false,
        .rotation       = {.swap_xy = false, .mirror_x = false, .mirror_y = false},
        .rounder_cb     = nullptr,
        .color_format   = LV_COLOR_FORMAT_RGB565,
        .flags =
            {.buff_dma = 0, .buff_spiram = 0, .sw_rotate = 0, .swap_bytes = 0, .full_refresh = 0, .direct_mode = 1},
    };
    const lvgl_port_display_dsi_cfg_t dsi_cfg = {
        .flags = {.avoid_tearing = 1},
    };

    *lv_display = lvgl_port_add_disp_dsi(&disp_cfg, &dsi_cfg);
    if (*lv_display == nullptr) {
        ESP_LOGE(TAG, "lvgl_port_add_disp_dsi failed");
        return ESP_FAIL;
    }

    if (runtime.touch_handle != nullptr) {
        auto* touch_handles = static_cast<TouchHandles*>(runtime.touch_handle);

        const lvgl_port_touch_cfg_t touch_cfg = {
            .disp   = *lv_display,
            .handle = touch_handles->tp,
            .scale  = {.x = 1.0f, .y = 1.0f},
        };
        *lv_touch_indev = lvgl_port_add_touch(&touch_cfg);
        if (*lv_touch_indev == nullptr) {
            ESP_LOGW(TAG, "lvgl_port_add_touch failed 鈥?touch will be unavailable");
        }
    }

    ESP_LOGI(TAG, "LVGL initialized: %ux%u, touch=%s", LCD_W, LCD_H, *lv_touch_indev ? "yes" : "no");
    return ESP_OK;
}

}  // namespace m5::tab5