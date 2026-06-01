/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * Shared SYS I2C master bus singleton for m5tab5 drivers.
 * m5tab5 驱动共用�?SYS I2C 主机总线单例�?
 * The SYS I2C bus (SDA=GPIO31, SCL=GPIO32, port 0) is used by:
 * SYS I2C 总线（SDA=GPIO31，SCL=GPIO32，端�?0）当前用于：
 *   - PI4IOE5V6408 IO expander (LCD reset + touch reset)
 *   - PI4IOE5V6408 IO 扩展器（LCD 复位 + 触摸复位�?
 *   - GT911 capacitive touch panel
 *   - GT911 电容触摸面板
 *   - ST7123 TDDI touch
 *   - ST7123 TDDI 触摸控制�?
 *
 * Uses espressif/i2c_bus for thread-safe bus management.
 * 使用 espressif/i2c_bus 提供线程安全的总线管理�?
 * Call m5tab5_get_sys_i2c_bus()               -> i2c_bus_handle_t
 * 调用 m5tab5_get_sys_i2c_bus()               -> i2c_bus_handle_t
 * Call m5tab5_get_sys_i2c_master_bus_handle() -> i2c_master_bus_handle_t
 * 调用 m5tab5_get_sys_i2c_master_bus_handle() -> i2c_master_bus_handle_t
 *   (needed by esp_lcd_new_panel_io_i2c)
 *   （供 esp_lcd_new_panel_io_i2c 使用�?
 */

#include "drivers/m5tab5_driver_common.h"
#include "m5tab5_pinmap.h"

#include "i2c_bus.h"
#include "esp_log.h"

static const char* TAG = "m5tab5.i2c";

namespace m5::tab5 {

static i2c_bus_handle_t s_sys_i2c_bus = nullptr;

i2c_bus_handle_t m5tab5_get_sys_i2c_bus()
{
    if (s_sys_i2c_bus) return s_sys_i2c_bus;

    i2c_config_t bus_cfg     = {};
    bus_cfg.mode             = I2C_MODE_MASTER;
    bus_cfg.sda_io_num       = M5TAB5_PIN_SYS_I2C_SDA;
    bus_cfg.scl_io_num       = M5TAB5_PIN_SYS_I2C_SCL;
    bus_cfg.sda_pullup_en    = true;
    bus_cfg.scl_pullup_en    = true;
    bus_cfg.master.clk_speed = 400000;

    s_sys_i2c_bus = i2c_bus_create((i2c_port_t)M5TAB5_I2C_PORT_SYS, &bus_cfg);
    if (!s_sys_i2c_bus) {
        ESP_LOGE(TAG, "SYS I2C bus init failed (port %d, SDA=%d SCL=%d)", (int)M5TAB5_I2C_PORT_SYS,
                 (int)M5TAB5_PIN_SYS_I2C_SDA, (int)M5TAB5_PIN_SYS_I2C_SCL);
        return nullptr;
    }
    ESP_LOGI(TAG, "SYS I2C bus created (port=%d SDA=%d SCL=%d)", (int)M5TAB5_I2C_PORT_SYS, (int)M5TAB5_PIN_SYS_I2C_SDA,
             (int)M5TAB5_PIN_SYS_I2C_SCL);
    return s_sys_i2c_bus;
}

i2c_master_bus_handle_t m5tab5_get_sys_i2c_master_bus_handle()
{
    i2c_bus_handle_t bus = m5tab5_get_sys_i2c_bus();
    if (!bus) return nullptr;
    return i2c_bus_get_internal_bus_handle(bus);
}

}  // namespace m5::tab5
