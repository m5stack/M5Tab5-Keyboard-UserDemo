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
/// 返回共享�?SYS I2C 总线句柄（i2c_bus 包装层，延迟初始化的单例）�?
i2c_bus_handle_t m5tab5_get_sys_i2c_bus();

/// Return the underlying IDF i2c_master_bus_handle_t for esp_lcd_new_panel_io_i2c.
/// 返回底层 IDF �?i2c_master_bus_handle_t，供 esp_lcd_new_panel_io_i2c 使用�?
i2c_master_bus_handle_t m5tab5_get_sys_i2c_master_bus_handle();

}  // namespace m5::tab5