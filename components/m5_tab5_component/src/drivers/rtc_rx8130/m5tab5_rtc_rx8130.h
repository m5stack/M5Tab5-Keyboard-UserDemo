/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 *
 * RX8130 RTC driver for m5_tab5_component.
 * m5_tab5_component �?RX8130 RTC 驱动�?
 * Uses espressif/i2c_bus (the same SYS I2C bus as other built-in
 * peripherals). 使用 espressif/i2c_bus，并与其他板载外设共用同一�?SYS I2C 总线�?
 *
 * RX8130CE/RX8130CO I2C address: 0x32
 * RX8130CE/RX8130CO �?I2C 地址�?x32
 * Reference: M5Unified RX8130_Class (MIT), Epson RX8130 datasheet.
 * 参考资料：M5Unified RX8130_Class（MIT）以�?Epson RX8130 数据手册�?
 */
#pragma once

#include <cstdint>
#include "esp_err.h"
#include "i2c_bus.h"
#include "m5tab5_rtc.h"

namespace m5::tab5 {

// ── RX8130 Register Map / RX8130
// 寄存器映�?─────────────────────────────────
constexpr uint8_t RX8130_REG_SEC     = 0x10;  ///< Seconds (BCD) / 秒（BCD�?
constexpr uint8_t RX8130_REG_MIN     = 0x11;  ///< Minutes (BCD) / 分（BCD�?
constexpr uint8_t RX8130_REG_HOUR    = 0x12;  ///< Hours (BCD, 24h) / 时（BCD�?4 小时制）
constexpr uint8_t RX8130_REG_WEEKDAY = 0x13;  ///< Weekday (one-hot, bit0=Sun) / 星期（独热编码，bit0=周日�?
constexpr uint8_t RX8130_REG_DAY     = 0x14;  ///< Day of month (BCD) / 日期（BCD�?
constexpr uint8_t RX8130_REG_MONTH   = 0x15;  ///< Month (BCD) / 月份（BCD�?
constexpr uint8_t RX8130_REG_YEAR    = 0x16;  ///< Year mod 100 (BCD) / 年份�?100 取模（BCD�?
constexpr uint8_t RX8130_REG_MIN_ALARM  = 0x17;  ///< Alarm minute / 闹钟分钟
constexpr uint8_t RX8130_REG_HOUR_ALARM = 0x18;  ///< Alarm hour / 闹钟小时
constexpr uint8_t RX8130_REG_WDAY_ALARM = 0x19;  ///< Alarm weekday or day / 闹钟星期或日�?
constexpr uint8_t RX8130_REG_TIMER0     = 0x1A;  ///< Timer count LSB / 定时器计数低字节
constexpr uint8_t RX8130_REG_TIMER1     = 0x1B;  ///< Timer count MSB / 定时器计数高字节
constexpr uint8_t RX8130_REG_EXTENSION =
    0x1C;  ///< Extension register (TSEL, WADA, TE ...) / 扩展寄存器（TSEL、WADA、TE 等）
constexpr uint8_t RX8130_REG_FLAG = 0x1D;  ///< Flag register (VBLF, AF, TF ...) / 标志寄存器（VBLF、AF、TF 等）
constexpr uint8_t RX8130_REG_CONTROL0 = 0x1E;  ///< Control register 0 (AIE, TIE ...) / 控制寄存�?0（AIE、TIE 等）
constexpr uint8_t RX8130_REG_CONTROL1 =
    0x1F;  ///< Control register 1 (INIEN, CHSEL ...) / 控制寄存�?1（INIEN、CHSEL 等）

// ── Bit Masks /
// 位掩�?───────────────────────────────────────────────────────
constexpr uint8_t RX8130_FLAG_VBLF = 0x80;  ///< Voltage-low flag (backup voltage) / 低电压标志（备份电源�?
constexpr uint8_t RX8130_FLAG_AF   = 0x08;  ///< Alarm flag / 闹钟标志
constexpr uint8_t RX8130_FLAG_TF   = 0x10;  ///< Timer flag / 定时器标�?
constexpr uint8_t RX8130_CTRL0_AIE = 0x08;  ///< Alarm interrupt enable / 闹钟中断使能
constexpr uint8_t RX8130_CTRL0_TIE = 0x10;  ///< Timer interrupt enable / 定时器中断使�?
constexpr uint8_t RX8130_EXT_TE    = 0x10;  ///< Timer enable / 定时器使�?
constexpr uint8_t RX8130_EXT_WADA = 0x08;  ///< Week (0) or day (1) alarm selector / 星期�?）或日期�?）闹钟选择
constexpr uint8_t RX8130_EXT_TSEL_MASK = 0x07;  ///< Timer source select bits / 定时器时钟源选择�?
constexpr uint8_t RX8130_CTRL1_INIEN   = 0x10;  ///< Interrupt input enable / 中断输入使能
constexpr uint8_t RX8130_CTRL1_CHSEL   = 0x20;  ///< Interrupt charge select / 中断充电选择

// ── Default I2C Address / 默认 I2C 地址
// ─────────────────────────────────────
constexpr uint8_t RX8130_I2C_ADDR = 0x32;

// ── Driver Handle / 驱动句柄
// ────────────────────────────────────────────────
struct m5tab5_rtc_rx8130_t {
    i2c_bus_device_handle_t dev = nullptr;  ///< i2c_bus device handle / i2c_bus 设备句柄
    bool initialised            = false;
};

// ── Lifecycle / 生命周期
// ─────────────────────────────────────────────────────

/**
 * @brief  Create and initialize an RX8130 device on the given bus.
 *         在指定总线上创建并初始化一�?RX8130 设备�?
 *
 * Allocates an i2c_bus_device_handle internally. The handle must be released
 * with m5tab5_rtc_rx8130_delete() when no longer needed.
 * 内部会分配一�?i2c_bus_device_handle，不再需要时必须通过
 * m5tab5_rtc_rx8130_delete() 释放�?
 *
 * @param  bus     SYS I2C bus handle (from m5tab5_get_sys_i2c_bus()).
 * @param  out_rtc Pointer to caller-allocated m5tab5_rtc_rx8130_t that will
 *                 be filled on success.
 * @param  bus     SYS I2C 总线句柄（来�?m5tab5_get_sys_i2c_bus()）�?
 * @param  out_rtc
 * 由调用方分配�?m5tab5_rtc_rx8130_t，对成功结果进行回填�?
 * @return ESP_OK on success, otherwise an
 * error code.
 * @return 成功返回 ESP_OK，否则返回对应错误码�?
 */
esp_err_t m5tab5_rtc_rx8130_init(i2c_bus_handle_t bus, m5tab5_rtc_rx8130_t* out_rtc);

/**
 * @brief  Release resources acquired by m5tab5_rtc_rx8130_init().
 *         释放 m5tab5_rtc_rx8130_init() 获取的资源�?
 */
void m5tab5_rtc_rx8130_delete(m5tab5_rtc_rx8130_t* rtc);

// ── Time Read and Write / 时间读写
// ──────────────────────────────────────────

/**
 * @brief  Read the current date and time from the RX8130.
 *         �?RX8130 读取当前日期和时间�?
 * @param  rtc  Initialized RTC handle.
 * @param  out  Output datetime (must not be NULL).
 * @param  rtc  已初始化�?RTC 句柄�?
 * @param  out  输出日期时间对象（不能为空）�?
 * @return
 * ESP_OK on success.
 * @return 成功返回 ESP_OK�?
 */
esp_err_t m5tab5_rtc_rx8130_get_datetime(const m5tab5_rtc_rx8130_t* rtc, m5tab5_rtc_datetime_t* out);

/**
 * @brief  Write the date and time to the RX8130.
 *         将日期和时间写入 RX8130�?
 *
 * Pass a datetime where undesired fields are set to -1 to skip writing that
 * part (date only, time only, or both).
 * 若不希望写入某些字段，可将其设为
 * -1，从而跳过对应部分（仅日期、仅时间或两者）�?
 *
 * @param  rtc  Initialized RTC handle.
 * @param  dt   Datetime to set (date and/or time).
 * @param  rtc  已初始化�?RTC 句柄�?
 * @param  dt
 * 要设置的日期时间（可包含日期、时间或两者）�?
 * @return ESP_OK on success.
 * @return 成功返回 ESP_OK�?
 */
esp_err_t m5tab5_rtc_rx8130_set_datetime(m5tab5_rtc_rx8130_t* rtc, const m5tab5_rtc_datetime_t* dt);

// ── Status /
// 状态接�?───────────────────────────────────────────────────────

/**
 * @brief  Return true if the backup-voltage low flag (VBLF) is set.
 *         如果备份低电压标志（VBLF）被置位，则返回 true�?
 *
 * When true, the RTC may have lost its time data and should be reset.
 * �?true 时说�?RTC 可能已经丢失时间数据，建议重新设置时间�?
 */
bool m5tab5_rtc_rx8130_volt_low(const m5tab5_rtc_rx8130_t* rtc);

/**
 * @brief  Return true if any IRQ flag (AF or TF) is set.
 *         如果任意 IRQ 标志（AF �?TF）被置位，则返回 true�?
 */
bool m5tab5_rtc_rx8130_irq_status(const m5tab5_rtc_rx8130_t* rtc);

/**
 * @brief  Clear all pending IRQ flags (AF and TF).
 *         清除所有待处理 IRQ 标志（AF �?TF）�?
 */
esp_err_t m5tab5_rtc_rx8130_clear_irq(m5tab5_rtc_rx8130_t* rtc);

/**
 * @brief  Disable all IRQ sources (AIE and TIE cleared).
 *         禁用全部 IRQ 源（清除 AIE �?TIE）�?
 */
esp_err_t m5tab5_rtc_rx8130_disable_irq(m5tab5_rtc_rx8130_t* rtc);

// ── Alarm / 闹钟功能
// ─────────────────────────────────────────────────────────

/**
 * @brief  Set a one-shot alarm IRQ.
 *         配置一次性闹�?IRQ�?
 *
 * @param  rtc   Initialized RTC handle.
 * @param  date  Alarm date (use NULL or set day/weekday to -1 to ignore).
 * @param  time  Alarm time (use NULL or set hours/minutes to -1 to ignore).
 * @param  rtc   已初始化�?RTC 句柄�?
 * @param  date  闹钟日期（传 NULL 或将 day/weekday 设为 -1
 * 表示忽略）�?
 * @param  time  闹钟时间（传 NULL 或将 hours/minutes 设为 -1 表示忽略）�?
 *
 * @return 1 if alarm was enabled, 0 if nothing was set, negative on error.
 * @return 启用闹钟返回 1；若未设置任何内容返�?0；出错返回负值�?
 */
int m5tab5_rtc_rx8130_set_alarm_irq(m5tab5_rtc_rx8130_t* rtc, const m5tab5_rtc_date_t* date,
                                    const m5tab5_rtc_time_t* time);

// ── Periodic Timer /
// 周期定时�?─────────────────────────────────────────────

/**
 * @brief  Configure the periodic countdown timer.
 *         配置周期性倒计时定时器�?
 *
 * @param  rtc        Initialized RTC handle.
 * @param  timer_msec Timer period in milliseconds. 0 = disable.
 * @param  rtc        已初始化�?RTC 句柄�?
 * @param  timer_msec 定时器周期，单位毫秒；传 0
 * 表示禁用�?
 * @return Actual period set in milliseconds (rounded due to hardware resolution), or 0 if disabled.
 * @return 实际设置成功的毫秒周期值（会受硬件分辨率影响而取整）；若禁用则返�?0�?
 */
uint32_t m5tab5_rtc_rx8130_set_timer_irq(m5tab5_rtc_rx8130_t* rtc, uint32_t timer_msec);

}  // namespace m5::tab5
