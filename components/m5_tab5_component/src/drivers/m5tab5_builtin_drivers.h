/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "m5tab5_types.h"

namespace m5::tab5 {

const m5tab5_display_driver_descriptor_t* m5tab5_get_builtin_display_st7121_driver();

const m5tab5_display_driver_descriptor_t* m5tab5_get_builtin_display_st7123_driver();

const m5tab5_display_driver_descriptor_t* m5tab5_get_builtin_display_ili9881_driver();

const m5tab5_device_driver_descriptor_t* m5tab5_get_builtin_touch_st7121_driver();

const m5tab5_device_driver_descriptor_t* m5tab5_get_builtin_touch_st7123_driver();

const m5tab5_device_driver_descriptor_t* m5tab5_get_builtin_touch_gt911_driver();

const m5tab5_device_driver_descriptor_t* m5tab5_get_builtin_ioexpander_pi4ioe5v6408_driver();

}  // namespace m5::tab5