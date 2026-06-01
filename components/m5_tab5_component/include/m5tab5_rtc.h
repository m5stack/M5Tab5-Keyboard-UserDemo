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
/// ä¸å¤©ä¸­çæ¶é´ãè´å¼ï¼-1ï¼è¡¨ç¤ºâä¸å³å¿ / æªè®¾ç½®âã?
struct m5tab5_rtc_time_t {
    int8_t hours;    ///< 0-23 / 0 å?23 å°æ¶
    int8_t minutes;  ///< 0-59 / 0 å?59 åé
    int8_t seconds;  ///< 0-59 / 0 å?59 ç§?

    m5tab5_rtc_time_t(int8_t h = -1, int8_t m = -1, int8_t s = -1) : hours(h), minutes(m), seconds(s)
    {
    }

    explicit m5tab5_rtc_time_t(const struct tm& t) : hours(t.tm_hour), minutes(t.tm_min), seconds(t.tm_sec)
    {
    }
};

/// Calendar date. Negative value (-1) means "don't care / not set".
/// æ¥åæ¥æãè´å¼ï¼-1ï¼è¡¨ç¤ºâä¸å³å¿ / æªè®¾ç½®âã?
struct m5tab5_rtc_date_t {
    int16_t year;    ///< 2000-2099 (RX8130 stores century-less, adjusted to 2000-2099) / 2000 å?2099ï¼RX8130
                     ///< ä¸å­ä¸çºªä½ï¼è¿éç»ä¸æç®å?2000 å?2099ï¼?
    int8_t month;    ///< 1-12 / 1 å?12 æ?
    int8_t day;      ///< 1-31 / 1 å?31 æ?
    int8_t weekday;  ///< 0=Sun, 1=Mon, ... 6=Sat / 0=å¨æ¥ï¼?=å¨ä¸ï¼?..ï¼?=å¨å­

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
/// ç»ååçæ¥æåæ¶é´ã?
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
    /// è½¬æ¢ä¸?POSIX tm ç»æä½ï¼ä¸åæ¶åºä¿®æ­£ï¼ã?
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

// ââ Global Singleton RTC Interface / å¨å±åä¾ RTC æ¥å£
// âââââââââââââââââââââ Operates on the built-in RX8130 singleton using the
// SYS I2C bus. ä½¿ç¨ SYS I2C æ»çº¿æä½åç½®ç?RX8130 åä¾å®ä¾ã? All functions are thread-safe (protected by
// the i2c_bus semaphore). ææå½æ°é½å·å¤çº¿ç¨å®å¨æ§ï¼ç?i2c_bus ä¿¡å·éä¿æ¤ï¼ã? Must be called after
// m5tab5_component::begin() has succeeded. å¿é¡»å?m5tab5_component::begin() æååè°ç¨ã?

/// Lazily initialize the RX8130 singleton. Idempotent.
/// å»¶è¿åå§å?RX8130 åä¾ï¼æ¯æéå¤è°ç¨ã?
/// @return ESP_OK if the chip is present and initialized,
///         ESP_ERR_NOT_FOUND if no RX8130 is detected.
/// @return å¦æè¯çå­å¨å¹¶åå§åæååè¿å?ESP_OKï¼?
///         å¦ææªæ£æµå° RX8130 åè¿å?ESP_ERR_NOT_FOUNDã?
esp_err_t m5tab5_rtc_init();

/// Read the current date and time from the RX8130.
/// ä»?RX8130 è¯»åå½åæ¥æåæ¶é´ã?
/// @return ESP_ERR_INVALID_STATE if m5tab5_rtc_init() has not succeeded yet.
/// @return å¦æ m5tab5_rtc_init() å°æªæåï¼åè¿å ESP_ERR_INVALID_STATEã?
esp_err_t m5tab5_rtc_get_datetime(m5tab5_rtc_datetime_t* out);

/// Write date and time to the RX8130.
/// å°æ¥æåæ¶é´åå¥ RX8130ã?
/// @return ESP_ERR_INVALID_STATE if m5tab5_rtc_init() has not succeeded yet.
/// @return å¦æ m5tab5_rtc_init() å°æªæåï¼åè¿å ESP_ERR_INVALID_STATEã?
esp_err_t m5tab5_rtc_set_datetime(const m5tab5_rtc_datetime_t* dt);

/// Return true if the RX8130 backup-voltage low flag (VBLF) is set.
/// å¦æ RX8130 çå¤ä»½ä½çµåæ å¿ï¼VBLFï¼è¢«ç½®ä½ï¼åè¿å trueã?
/// When true, the stored time may be invalid and should be set again.
/// ä¸?true æ¶è¡¨ç¤ºå·²å­æ¶é´å¯è½æ æï¼å»ºè®®éæ°è®¾ç½®ã?
bool m5tab5_rtc_volt_low();

}  // namespace m5::tab5
