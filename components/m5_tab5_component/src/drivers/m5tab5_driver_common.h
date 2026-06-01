/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "esp_err.h"
#include "esp_log.h"

#include "i2c_bus.h"
#include "driver/i2c_master.h"

#include "m5tab5_types.h"

namespace m5::tab5 {

inline const char* m5tab5_driver_log_tag()
{
    return "m5tab5.driver";
}

/// Return the shared SYS I2C bus handle (i2c_bus wrapper, lazily initialized singleton).
/// è¿åå±äº«ç?SYS I2C æ»çº¿å¥æï¼i2c_bus
/// åè£å±ï¼å»¶è¿åå§åçåä¾ï¼ã?
i2c_bus_handle_t m5tab5_get_sys_i2c_bus();

/// Return the underlying IDF i2c_master_bus_handle_t for esp_lcd_new_panel_io_i2c.
/// è¿ååºå± IDF ç?i2c_master_bus_handle_tï¼ä¾ esp_lcd_new_panel_io_i2c ä½¿ç¨ã?
i2c_master_bus_handle_t m5tab5_get_sys_i2c_master_bus_handle();

}  // namespace m5::tab5