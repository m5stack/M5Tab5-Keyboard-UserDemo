/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "m5tab5_types.h"

namespace m5::tab5 {

const m5tab5_variant_descriptor_t* m5tab5_get_variant_descriptor(M5TAB5_VariantId variant_id);

const m5tab5_variant_descriptor_t* m5tab5_get_default_variant_descriptor();

const m5tab5_variant_descriptor_t* m5tab5_detect_variant_descriptor();

const m5tab5_variant_descriptor_t* m5tab5_get_reference_variant_descriptor();

const m5tab5_variant_descriptor_t* m5tab5_get_lcd_st7123_touch_st7123_variant_descriptor();

const m5tab5_variant_descriptor_t* m5tab5_get_lcd_st7121_touch_st7121_variant_descriptor();

const m5tab5_variant_descriptor_t* m5tab5_autodetect_variant_descriptor();

}  // namespace m5::tab5