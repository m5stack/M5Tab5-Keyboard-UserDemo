/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#include "variants/m5tab5_variant_registry.h"

namespace m5::tab5 {

const m5tab5_variant_descriptor_t* m5tab5_get_variant_descriptor(M5TAB5_VariantId variant_id)
{
    switch (variant_id) {
        case M5TAB5_VARIANT_REFERENCE:
        case M5TAB5_VARIANT_TAB5_LCD_ILI9881_TOUCH_GT911:
            return m5tab5_get_reference_variant_descriptor();
        case M5TAB5_VARIANT_TAB5_LCD_ST7123_TOUCH_ST7123:
            return m5tab5_get_lcd_st7123_touch_st7123_variant_descriptor();
        case M5TAB5_VARIANT_TAB5_LCD_ST7121_TOUCH_ST7121:
            return m5tab5_get_lcd_st7121_touch_st7121_variant_descriptor();
        case M5TAB5_VARIANT_AUTO:
        default:
            return m5tab5_detect_variant_descriptor();
    }
}

const m5tab5_variant_descriptor_t* m5tab5_get_default_variant_descriptor()
{
    return m5tab5_get_reference_variant_descriptor();
}

const m5tab5_variant_descriptor_t* m5tab5_detect_variant_descriptor()
{
    return m5tab5_autodetect_variant_descriptor();
}

}  // namespace m5::tab5