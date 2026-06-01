/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 *
 * Public RTC types for m5_tab5_component (RX8130 built-in RTC).
 * This header has NO dependency on M5Unified or Arduino.
 */
#pragma once

#include <cstdint>
#include <time.h>
#include "esp_err.h"

namespace m5::tab5 {

/// Time of day. Negative value (-1) means "don't care / not set".
/// 一天中的时间。负值（-1）表示“不关心 / 未设置”�?
struct m5tab5_rtc_time_t {
    int8_t hours;    ///< 0-23 / 0 �?23 小时
    int8_t minutes;  ///< 0-59 / 0 �?59 分钟
    int8_t seconds;  ///< 0-59 / 0 �?59 �?

    m5tab5_rtc_time_t(int8_t h = -1, int8_t m = -1, int8_t s = -1) : hours(h), minutes(m), seconds(s)
    {
    }

    explicit m5tab5_rtc_time_t(const struct tm& t) : hours(t.tm_hour), minutes(t.tm_min), seconds(t.tm_sec)
    {
    }
};

/// Calendar date. Negative value (-1) means "don't care / not set".
/// 日历日期。负值（-1）表示“不关心 / 未设置”�?
struct m5tab5_rtc_date_t {
    int16_t year;    ///< 2000-2099 (RX8130 stores century-less, adjusted to 2000-2099) / 2000 �?2099（RX8130
                     ///< 不存世纪位，这里统一折算�?2000 �?2099�?
    int8_t month;    ///< 1-12 / 1 �?12 �?
    int8_t day;      ///< 1-31 / 1 �?31 �?
    int8_t weekday;  ///< 0=Sun, 1=Mon, ... 6=Sat / 0=周日�?=周一�?..�?=周六

    m5tab5_rtc_date_t(int16_t y = 2000, int8_t mo = 1, int8_t d = -1, int8_t wd = -1)
        : year(y), month(mo), day(d), weekday(wd)
    {
    }

    explicit m5tab5_rtc_date_t(const struct tm& t)
        : year(static_cast<int16_t>(t.tm_year + 1900)),
          month(static_cast<int8_t>(t.tm_mon + 1)),
          day(static_cast<int8_t>(t.tm_mday)),
          weekday(static_cast<int8_t>(t.tm_wday))
    {
    }
};

/// Combined date and time.
/// 组合后的日期和时间�?
struct m5tab5_rtc_datetime_t {
    m5tab5_rtc_date_t date;
    m5tab5_rtc_time_t time;

    m5tab5_rtc_datetime_t() = default;
    m5tab5_rtc_datetime_t(const m5tab5_rtc_date_t& d, const m5tab5_rtc_time_t& t) : date(d), time(t)
    {
    }

    explicit m5tab5_rtc_datetime_t(const struct tm& t) : date(t), time(t)
    {
    }

    /// Convert to a POSIX tm struct (no timezone adjustment).
    /// 转换�?POSIX tm 结构体（不做时区修正）�?
    struct tm to_tm() const
    {
        struct tm t = {};
        t.tm_year   = date.year - 1900;
        t.tm_mon    = date.month - 1;
        t.tm_mday   = date.day;
        t.tm_wday   = date.weekday;
        t.tm_hour   = time.hours;
        t.tm_min    = time.minutes;
        t.tm_sec    = time.seconds;
        return t;
    }
};

// ── Global Singleton RTC Interface / 全局单例 RTC 接口
// ───────────────────── Operates on the built-in RX8130 singleton using the
// SYS I2C bus. 使用 SYS I2C 总线操作内置�?RX8130 单例实例�? All functions are thread-safe (protected by
// the i2c_bus semaphore). 所有函数都具备线程安全性（�?i2c_bus 信号量保护）�? Must be called after
// m5tab5_component::begin() has succeeded. 必须�?m5tab5_component::begin() 成功后调用�?

/// Lazily initialize the RX8130 singleton. Idempotent.
/// 延迟初始�?RX8130 单例，支持重复调用�?
/// @return ESP_OK if the chip is present and initialized,
///         ESP_ERR_NOT_FOUND if no RX8130 is detected.
/// @return 如果芯片存在并初始化成功则返�?ESP_OK�?
///         如果未检测到 RX8130 则返�?ESP_ERR_NOT_FOUND�?
esp_err_t m5tab5_rtc_init();

/// Read the current date and time from the RX8130.
/// �?RX8130 读取当前日期和时间�?
/// @return ESP_ERR_INVALID_STATE if m5tab5_rtc_init() has not succeeded yet.
/// @return 如果 m5tab5_rtc_init() 尚未成功，则返回 ESP_ERR_INVALID_STATE�?
esp_err_t m5tab5_rtc_get_datetime(m5tab5_rtc_datetime_t* out);

/// Write date and time to the RX8130.
/// 将日期和时间写入 RX8130�?
/// @return ESP_ERR_INVALID_STATE if m5tab5_rtc_init() has not succeeded yet.
/// @return 如果 m5tab5_rtc_init() 尚未成功，则返回 ESP_ERR_INVALID_STATE�?
esp_err_t m5tab5_rtc_set_datetime(const m5tab5_rtc_datetime_t* dt);

/// Return true if the RX8130 backup-voltage low flag (VBLF) is set.
/// 如果 RX8130 的备份低电压标志（VBLF）被置位，则返回 true�?
/// When true, the stored time may be invalid and should be set again.
/// �?true 时表示已存时间可能无效，建议重新设置�?
bool m5tab5_rtc_volt_low();

}  // namespace m5::tab5
