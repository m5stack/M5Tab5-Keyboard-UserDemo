/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "lvgl.h"
#include "m5tab5_types.h"

namespace m5::tab5 {

esp_err_t m5tab5_lvgl_init(const m5tab5_runtime_t& runtime, lv_display_t** lv_display, lv_indev_t** lv_touch_indev);

}  // namespace m5::tab5
