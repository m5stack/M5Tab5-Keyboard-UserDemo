/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "m5tab5_types.h"
#include "m5tab5_pinmap.h"
#include "m5tab5_rtc.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"
#include "lvgl.h"
namespace m5::tab5 {

class m5tab5_component {
public:
    esp_err_t begin(const m5tab5_component_config_t& config = {});

    /// Initialize the LVGL port and register the display and, if available, touch input.
    /// 初始?LVGL 端口，并注册显示以及可用的触摸输入?
    /// Must be called after begin() succeeds.
    /// 必须?begin() 成功后调用?
    esp_err_t lvgl_init();

    /// Control WLAN_PWR_EN via the IO expander (ADDR_HIGH 0x44 P0).
    /// 通过 IO 扩展器（ADDR_HIGH 0x44 P0）控?WLAN_PWR_EN?
    /// Must be called after begin() succeeds.
    /// 必须?begin() 成功后调用?
    esp_err_t wlan_power(bool enable);

    /// Switch the RF antenna path via the IO expander (ADDR_LOW 0x43 P0).
    /// 通过 IO 扩展器（ADDR_LOW 0x43 P0）切换射频天线路径?
    /// Use M5TAB5_RF_ANTENNA_INTERNAL (default) or M5TAB5_RF_ANTENNA_EXTERNAL.
    /// 可?M5TAB5_RF_ANTENNA_INTERNAL（默认）?M5TAB5_RF_ANTENNA_EXTERNAL?
    /// Must be called after begin() succeeds.
    /// 必须?begin() 成功后调用?
    esp_err_t set_rf_antenna(m5tab5_rf_antenna_t ant);

    /// Enable or disable the external 5 V supply (Grove / EXT5V) via the IO expander
    /// (ADDR_LOW 0x43 P2 = EXT5V_EN).
    /// 通过 IO 扩展器启用或禁用外部 5 V 电源（Grove / EXT5V），
    /// 对应 ADDR_LOW 0x43 P2 = EXT5V_EN?
    /// Note: begin() leaves EXT5V_EN HIGH by default; call ext5v_enable(true)
    /// explicitly before any peripheral that needs it, such as an NFC reader on
    /// the Grove 5 V rail.
    /// 注意：begin() 默认会让 EXT5V_EN 保持高电平；在使用任何依赖该电源的外设前?
    /// 例如连接?Grove 5 V 电源轨上?NFC 读卡器，请显式调?ext5v_enable(true)?
    /// Must be called after begin() succeeds.
    /// 必须?begin() 成功后调用?
    esp_err_t ext5v_enable(bool enable);

    /// Enable or disable the USB 5 V output (IO expander ADDR_HIGH 0x44 P3 = USB5V_EN).
    /// 启用或禁?USB 5 V 输出（IO 扩展?ADDR_HIGH 0x44 P3 = USB5V_EN）?
    /// Off by default. Must be called after begin() succeeds.
    /// 默认关闭，必须在 begin() 成功后调用?
    esp_err_t usb5v_enable(bool enable);

    // ── Charging Control / 充电控制 ───────────────────────────────────────────

    /// Select slow (standard) or fast (QC) charging current.
    /// 选择慢速（标准）或快速（QC）充电电流?
    /// Drives nCHG_QC_EN (IO expander ADDR_HIGH 0x44 P5, active-LOW):
    /// 通过 nCHG_QC_EN（IO 扩展?ADDR_HIGH 0x44 P5，低电平有效）进行控制：
    ///   fast=false ?nCHG_QC_EN=HIGH ?standard current (default after begin())
    ///   fast=false ?nCHG_QC_EN=HIGH ?标准充电电流（begin() 后的默认状态）
    ///   fast=true  ?nCHG_QC_EN=LOW  ?QC / high current
    ///   fast=true  ?nCHG_QC_EN=LOW  ?QC / 大电流充?
    /// Must be called after begin() succeeds.
    /// 必须?begin() 成功后调用?
    esp_err_t set_charge_fast(bool fast);

    /// Enable or disable charging via the IO expander (ADDR_HIGH 0x44 P7 = CHG_EN).
    /// 通过 IO 扩展器（ADDR_HIGH 0x44 P7 = CHG_EN）启用或禁用充电。
    /// Must be called after begin() succeeds.
    /// 必须在 begin() 成功后调用。
    esp_err_t charge_enable(bool enable);

    // ── RX8130 RTC / RX8130 实时时钟 ─────────────────────────────────────────

    /// Initialize the built-in RX8130 RTC on the SYS I2C bus.
    /// ?SYS I2C 总线上初始化内置 RX8130 RTC?
    /// Must be called after begin() succeeds (the SYS I2C bus is set up there).
    /// 必须?begin() 成功后调用（SYS I2C 总线会在其中完成初始化）?
    /// Returns ESP_ERR_NOT_FOUND if no RX8130 is detected.
    /// 若未检测到 RX8130，则返回 ESP_ERR_NOT_FOUND?
    esp_err_t rtc_init();

    /// Read the current date and time from the RX8130.
    /// ?RX8130 读取当前日期和时间?
    /// Returns ESP_ERR_INVALID_STATE if rtc_init() has not succeeded.
    /// 如果 rtc_init() 尚未成功，则返回 ESP_ERR_INVALID_STATE?
    esp_err_t rtc_get_datetime(m5tab5_rtc_datetime_t* out) const;

    /// Write date and time to the RX8130.
    /// 将日期和时间写入 RX8130?
    /// Set individual fields to -1 to leave them unchanged.
    /// 将单个字段设?-1 可保持该字段不变?
    /// Returns ESP_ERR_INVALID_STATE if rtc_init() has not succeeded.
    /// 如果 rtc_init() 尚未成功，则返回 ESP_ERR_INVALID_STATE?
    esp_err_t rtc_set_datetime(const m5tab5_rtc_datetime_t* dt);

    /// Return true if the RX8130 backup-voltage low flag (VBLF) is set,
    /// meaning the RTC data may be invalid.
    /// 如果 RX8130 的备份电压过低标志（VBLF）被置位，则返回 true?
    /// 这意味着 RTC 数据可能无效?
    bool rtc_volt_low() const;

    // ── Power Management / 电源管理 ───────────────────────────────────────────

    /// Power off the device by toggling PWROFF_PLUSE (IO expander ADDR_HIGH 0x44
    /// P4) 10 times with 50 ms intervals, then calling esp_deep_sleep_start().
    /// 通过每隔 50 ms 切换一?PWROFF_PLUSE（IO 扩展?ADDR_HIGH 0x44 P4），
    /// 共切?10 次后调用 esp_deep_sleep_start() 来关机?
    /// Falls back to deep sleep immediately if the IO expander is unavailable.
    /// 如果 IO 扩展器不可用，则会立即退化为直接进入深度睡眠?
    /// **This function does not return.**
    /// **该函数不会返回?*
    [[noreturn]] void power_off();

    /// Enter deep sleep. The system resumes from reset after @p wakeup_us
    /// microseconds. Pass 0 to sleep indefinitely, which requires an external
    /// wakeup source to be configured beforehand, such as ULP or LP-GPIO.
    /// 进入深度睡眠。系统会?@p wakeup_us 微秒后通过复位方式恢复运行?
    /// 传入 0 表示无限期休眠，此时需要事先配置外部唤醒源，例?ULP ?LP-GPIO?
    /// **This function does not return.**
    /// **该函数不会返回?*
    [[noreturn]] void deep_sleep(uint64_t wakeup_us = 0);

    /// Enter light sleep (clocks gated, RAM retained, peripherals paused).
    /// 进入浅睡眠（时钟门控、RAM 保留、外设暂停）?
    /// Returns after @p wakeup_us microseconds. Pass 0 to require an external
    /// wakeup source, which must be configured separately before calling.
    /// 会在 @p wakeup_us 微秒后返回。传?0 表示必须依赖外部唤醒源，且需在调用前单独配置?
    void light_sleep(uint64_t wakeup_us = 0);

    // ── INA226 Power Monitor / INA226 电压电流监测 ───────────────────────────

    struct ina226_reading_t {
        float bus_voltage_v;  ///< Battery or bus voltage [V] / 电池或总线电压 [V]
        float current_a;      ///< Charge (+) or discharge (-) current [A] / 充电?）或放电?）电?[A]
        float power_w;        ///< Power [W] / 功率 [W]
    };

    /// Initialize the built-in INA226 (I2C 0x41, SYS bus, 5 mΩ shunt, 2 A max).
    /// 初始化内?INA226（I2C 0x41，SYS 总线? mΩ 分流电阻，最?2 A）?
    /// Must be called after begin() succeeds.
    /// 必须?begin() 成功后调用?
    esp_err_t ina226_init();

    /// Read all INA226 measurements. Fields in @p out are valid only on ESP_OK.
    /// 读取 INA226 的全部测量值，仅在返回 ESP_OK ?@p out 中字段有效?
    esp_err_t ina226_read(ina226_reading_t* out);

    // ── Accessors / 访问?───────────────────────────────────────────────────

    const m5tab5_variant_descriptor_t* variant() const;

    const m5tab5_component_config_t& config() const;

    const m5tab5_runtime_t& runtime() const;

    /// Return the native LCD panel handle created by begin(), or nullptr.
    /// 返回 begin() 创建的原?LCD 面板句柄；若不存在则返回 nullptr?
    esp_lcd_panel_handle_t lcd_panel() const;

    /// Return the native touch handle created by begin(), or nullptr.
    /// 返回 begin() 创建的原生触摸句柄；若不存在则返?nullptr?
    esp_lcd_touch_handle_t touch_panel() const;

    /// Return the LVGL display handle registered by lvgl_init(), or nullptr.
    /// 返回?lvgl_init() 注册?LVGL 显示句柄；若不存在则返回 nullptr?
    lv_display_t* lv_display() const
    {
        return lv_display_;
    }

    /// Return the LVGL touch input device registered by lvgl_init(), or nullptr.
    /// 返回?lvgl_init() 注册?LVGL 触摸输入设备句柄；若不存在则返回 nullptr?
    lv_indev_t* lv_touch_indev() const
    {
        return lv_touch_indev_;
    }

private:
    m5tab5_component_config_t active_config_{};
    m5tab5_runtime_t runtime_{};
    const m5tab5_variant_descriptor_t* active_variant_ = nullptr;
    lv_display_t* lv_display_                          = nullptr;
    lv_indev_t* lv_touch_indev_                        = nullptr;
    void* rtc_handle_ = nullptr;     ///< Opaque m5tab5_rtc_rx8130_t*, allocated in rtc_init() /
                                     ///< 不透明?m5tab5_rtc_rx8130_t*，在 rtc_init() 中分?
    void* ina226_handle_ = nullptr;  ///< Opaque m5tab5_ina226_t*, allocated in ina226_init() /
                                     ///< 不透明?m5tab5_ina226_t*，在 ina226_init() 中分?
};

}  // namespace m5::tab5