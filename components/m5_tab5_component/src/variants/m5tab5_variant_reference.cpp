/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#include "variants/m5tab5_variant_registry.h"

#include "drivers/m5tab5_builtin_drivers.h"

namespace m5::tab5 {
namespace {

const m5tab5_variant_descriptor_t k_m5tab5_reference_variant = {
    .variant_id  = M5TAB5_VARIANT_TAB5_LCD_ILI9881_TOUCH_GT911,
    .id          = "m5tab5.variant.reference",
    .description = "Reference Tab5 assembly based on ILI9881 + GT911 + PI4IOE5V6408",
    .display     = m5tab5_get_builtin_display_ili9881_driver(),
    .touch       = m5tab5_get_builtin_touch_gt911_driver(),
    .ioexpander  = m5tab5_get_builtin_ioexpander_pi4ioe5v6408_driver(),
    .probe       = nullptr,
};

const m5tab5_variant_descriptor_t k_m5tab5_st7123_variant = {
    .variant_id  = M5TAB5_VARIANT_TAB5_LCD_ST7123_TOUCH_ST7123,
    .id          = "m5tab5.variant.st7123",
    .description = "Alternative Tab5 assembly based on ST7123 + ST7123 + PI4IOE5V6408",
    .display     = m5tab5_get_builtin_display_st7123_driver(),
    .touch       = m5tab5_get_builtin_touch_st7123_driver(),
    .ioexpander  = m5tab5_get_builtin_ioexpander_pi4ioe5v6408_driver(),
    .probe       = nullptr,
};

const m5tab5_variant_descriptor_t k_m5tab5_st7121_variant = {
    .variant_id  = M5TAB5_VARIANT_TAB5_LCD_ST7121_TOUCH_ST7121,
    .id          = "m5tab5.variant.st7121",
    .description = "Alternative Tab5 assembly based on ST7121 + ST7121 + PI4IOE5V6408",
    .display     = m5tab5_get_builtin_display_st7121_driver(),
    .touch       = m5tab5_get_builtin_touch_st7121_driver(),
    .ioexpander  = m5tab5_get_builtin_ioexpander_pi4ioe5v6408_driver(),
    .probe       = nullptr,
};

}  // namespace

const m5tab5_variant_descriptor_t* m5tab5_get_reference_variant_descriptor()
{
    return &k_m5tab5_reference_variant;
}

const m5tab5_variant_descriptor_t* m5tab5_get_lcd_st7123_touch_st7123_variant_descriptor()
{
    return &k_m5tab5_st7123_variant;
}

const m5tab5_variant_descriptor_t* m5tab5_get_lcd_st7121_touch_st7121_variant_descriptor()
{
    return &k_m5tab5_st7121_variant;
}

}  // namespace m5::tab5