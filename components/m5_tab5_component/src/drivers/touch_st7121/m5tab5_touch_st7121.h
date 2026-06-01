/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "m5tab5_types.h"
#include "esp_lcd_touch.h"

namespace m5::tab5 {

const m5tab5_device_driver_descriptor_t* m5tab5_get_touch_st7121_driver();

/// Handles created by the ST7121 touch driver and stored in runtime->touch_handle.
/// ST7121 触摸驱动创建的句柄，保存�?runtime->touch_handle
/// 中�?
struct m5tab5_touch_st7121_handles_t {
    esp_lcd_panel_io_handle_t tp_io = nullptr;
    esp_lcd_touch_handle_t tp       = nullptr;
};

}  // namespace m5::tab5
