/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#include "drivers/m5tab5_builtin_drivers.h"
#include "drivers/lcd_st7121/m5tab5_lcd_st7121.h"
#include "drivers/lcd_st7123/m5tab5_lcd_st7123.h"
#include "drivers/lcd_ili9881/m5tab5_lcd_ili9881.h"
#include "drivers/touch_st7121/m5tab5_touch_st7121.h"
#include "drivers/touch_st7123/m5tab5_touch_st7123.h"
#include "drivers/touch_gt911/m5tab5_touch_gt911.h"
#include "drivers/extio_pi4ioe5v6408/m5tab5_extio_pi4ioe5v6408.h"

namespace m5::tab5 {

const m5tab5_display_driver_descriptor_t* m5tab5_get_builtin_display_st7121_driver()
{
    return m5tab5_get_lcd_st7121_driver();
}

const m5tab5_display_driver_descriptor_t* m5tab5_get_builtin_display_st7123_driver()
{
    return m5tab5_get_lcd_st7123_driver();
}

const m5tab5_display_driver_descriptor_t* m5tab5_get_builtin_display_ili9881_driver()
{
    return m5tab5_get_lcd_ili9881_driver();
}

const m5tab5_device_driver_descriptor_t* m5tab5_get_builtin_touch_st7121_driver()
{
    return m5tab5_get_touch_st7121_driver();
}

const m5tab5_device_driver_descriptor_t* m5tab5_get_builtin_touch_st7123_driver()
{
    return m5tab5_get_touch_st7123_driver();
}

const m5tab5_device_driver_descriptor_t* m5tab5_get_builtin_touch_gt911_driver()
{
    return m5tab5_get_touch_gt911_driver();
}

const m5tab5_device_driver_descriptor_t* m5tab5_get_builtin_ioexpander_pi4ioe5v6408_driver()
{
    return m5tab5_get_extio_pi4ioe5v6408_driver();
}

}  // namespace m5::tab5