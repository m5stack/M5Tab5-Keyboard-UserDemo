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

const m5tab5_display_driver_descriptor_t* m5tab5_get_lcd_st7121_driver();

/// Handles created by the ST7121 display driver and stored in runtime->display_handle.
/// ST7121 æ¾ç¤ºé©±å¨åå»ºçå¥æï¼ä¿å­å?runtime->display_handle ä¸­ã?
struct m5tab5_lcd_st7121_handles_t {
    esp_lcd_dsi_bus_handle_t   mipi_dsi_bus = nullptr;
    esp_lcd_panel_io_handle_t  io           = nullptr;
    esp_lcd_panel_handle_t     panel        = nullptr;
};

}  // namespace m5::tab5
