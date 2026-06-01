/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdint>

namespace m5::tab5 {

enum M5TAB5_VariantId : std::uint32_t {
    M5TAB5_VARIANT_AUTO                         = 0,
    M5TAB5_VARIANT_REFERENCE                    = 1,
    M5TAB5_VARIANT_TAB5_LCD_ILI9881_TOUCH_GT911 = 2,
    M5TAB5_VARIANT_TAB5_LCD_ST7123_TOUCH_ST7123 = 3,
    M5TAB5_VARIANT_TAB5_LCD_ST7121_TOUCH_ST7121 = 4,
};

}  // namespace m5::tab5