/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "m5tab5_types.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_mipi_dsi.h"

namespace m5::tab5 {

const m5tab5_display_driver_descriptor_t* m5tab5_get_lcd_st7123_driver();

/// Handles created by the ST7123 display driver and stored in runtime->display_handle.
/// ST7123 显示驱动创建的句柄，保存�?runtime->display_handle
/// 中�?
struct m5tab5_lcd_st7123_handles_t {
    esp_lcd_dsi_bus_handle_t mipi_dsi_bus = nullptr;
    esp_lcd_panel_io_handle_t io          = nullptr;
    esp_lcd_panel_handle_t panel          = nullptr;
};

}  // namespace m5::tab5