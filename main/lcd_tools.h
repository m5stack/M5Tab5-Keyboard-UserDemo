/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LCD_TOOLS_H
#define LCD_TOOLS_H

#include <esp_err.h>

namespace m5 {
namespace tab5 {
class m5tab5_component;
}
}  // namespace m5

// Initialize LVGL using the native LCD/touch handles provided by m5_tab5_component.
esp_err_t app_lcd_lvgl_init(m5::tab5::m5tab5_component& board);

#endif  // LCD_TOOLS_H
