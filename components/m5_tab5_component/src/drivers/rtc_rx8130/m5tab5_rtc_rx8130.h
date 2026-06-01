/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 *
 * RX8130 RTC driver for m5_tab5_component.
 * m5_tab5_component ç?RX8130 RTC é©±å¨ã?
 * Uses espressif/i2c_bus (the same SYS I2C bus as other built-in
 * peripherals). ä½¿ç¨ espressif/i2c_busï¼å¹¶ä¸å¶ä»æ¿è½½å¤è®¾å±ç¨åä¸æ?SYS I2C æ»çº¿ã?
 *
 * RX8130CE/RX8130CO I2C address: 0x32
 * RX8130CE/RX8130CO ç?I2C å°åï¼?x32
 * Reference: M5Unified RX8130_Class (MIT), Epson RX8130 datasheet.
 * åèèµæï¼M5Unified RX8130_Classï¼MITï¼ä»¥å?Epson RX8130 æ°æ®æåã?
 */
#pragma once

#include <cstdint>
#include "esp_err.h"
#include "i2c_bus.h"
#include "m5tab5_rtc.h"

namespace m5::tab5 {

// ââ RX8130 Register Map / RX8130
// å¯å­å¨æ å°?âââââââââââââââââââââââââââââââââ
constexpr uint8_t RX8130_REG_SEC     = 0x10;  ///< Seconds (BCD) / ç§ï¼BCDï¼?
constexpr uint8_t RX8130_REG_MIN     = 0x11;  ///< Minutes (BCD) / åï¼BCDï¼?
constexpr uint8_t RX8130_REG_HOUR    = 0x12;  ///< Hours (BCD, 24h) / æ¶ï¼BCDï¼?4 å°æ¶å¶ï¼
constexpr uint8_t RX8130_REG_WEEKDAY = 0x13;  ///< Weekday (one-hot, bit0=Sun) / ææï¼ç¬ç­ç¼ç ï¼bit0=å¨æ¥ï¼?
constexpr uint8_t RX8130_REG_DAY     = 0x14;  ///< Day of month (BCD) / æ¥æï¼BCDï¼?
constexpr uint8_t RX8130_REG_MONTH   = 0x15;  ///< Month (BCD) / æä»½ï¼BCDï¼?
constexpr uint8_t RX8130_REG_YEAR    = 0x16;  ///< Year mod 100 (BCD) / å¹´ä»½å¯?100 åæ¨¡ï¼BCDï¼?
constexpr uint8_t RX8130_REG_MIN_ALARM  = 0x17;  ///< Alarm minute / é¹éåé
constexpr uint8_t RX8130_REG_HOUR_ALARM = 0x18;  ///< Alarm hour / é¹éå°æ¶
constexpr uint8_t RX8130_REG_WDAY_ALARM = 0x19;  ///< Alarm weekday or day / é¹éææææ¥æ?
constexpr uint8_t RX8130_REG_TIMER0     = 0x1A;  ///< Timer count LSB / å®æ¶å¨è®¡æ°ä½å­è
constexpr uint8_t RX8130_REG_TIMER1     = 0x1B;  ///< Timer count MSB / å®æ¶å¨è®¡æ°é«å­è
constexpr uint8_t RX8130_REG_EXTENSION =
    0x1C;  ///< Extension register (TSEL, WADA, TE ...) / æ©å±å¯å­å¨ï¼TSELãWADAãTE ç­ï¼
constexpr uint8_t RX8130_REG_FLAG =
    0x1D;  ///< Flag register (VBLF, AF, TF ...) / æ å¿å¯å­å¨ï¼VBLFãAFãTF ç­ï¼
constexpr uint8_t RX8130_REG_CONTROL0 =
    0x1E;  ///< Control register 0 (AIE, TIE ...) / æ§å¶å¯å­å?0ï¼AIEãTIE ç­ï¼
constexpr uint8_t RX8130_REG_CONTROL1 =
    0x1F;  ///< Control register 1 (INIEN, CHSEL ...) / æ§å¶å¯å­å?1ï¼INIENãCHSEL ç­ï¼

// ââ Bit Masks /
// ä½æ©ç ?âââââââââââââââââââââââââââââââââââââââââââââââââââââââ
constexpr uint8_t RX8130_FLAG_VBLF = 0x80;  ///< Voltage-low flag (backup voltage) / ä½çµåæ å¿ï¼å¤ä»½çµæºï¼?
constexpr uint8_t RX8130_FLAG_AF   = 0x08;  ///< Alarm flag / é¹éæ å¿
constexpr uint8_t RX8130_FLAG_TF   = 0x10;  ///< Timer flag / å®æ¶å¨æ å¿?
constexpr uint8_t RX8130_CTRL0_AIE = 0x08;  ///< Alarm interrupt enable / é¹éä¸­æ­ä½¿è½
constexpr uint8_t RX8130_CTRL0_TIE = 0x10;  ///< Timer interrupt enable / å®æ¶å¨ä¸­æ­ä½¿è?
constexpr uint8_t RX8130_EXT_TE    = 0x10;  ///< Timer enable / å®æ¶å¨ä½¿è?
constexpr uint8_t RX8130_EXT_WADA =
    0x08;  ///< Week (0) or day (1) alarm selector / ææï¼?ï¼ææ¥æï¼?ï¼é¹ééæ©
constexpr uint8_t RX8130_EXT_TSEL_MASK = 0x07;  ///< Timer source select bits / å®æ¶å¨æ¶éæºéæ©ä½?
constexpr uint8_t RX8130_CTRL1_INIEN   = 0x10;  ///< Interrupt input enable / ä¸­æ­è¾å¥ä½¿è½
constexpr uint8_t RX8130_CTRL1_CHSEL   = 0x20;  ///< Interrupt charge select / ä¸­æ­åçµéæ©

// ââ Default I2C Address / é»è®¤ I2C å°å
// âââââââââââââââââââââââââââââââââââââ
constexpr uint8_t RX8130_I2C_ADDR = 0x32;

// ââ Driver Handle / é©±å¨å¥æ
// ââââââââââââââââââââââââââââââââââââââââââââââââ
struct m5tab5_rtc_rx8130_t {
    i2c_bus_device_handle_t dev = nullptr;  ///< i2c_bus device handle / i2c_bus è®¾å¤å¥æ
    bool initialised            = false;
};

// ââ Lifecycle / çå½å¨æ
// âââââââââââââââââââââââââââââââââââââââââââââââââââââ

/**
 * @brief  Create and initialize an RX8130 device on the given bus.
 *         å¨æå®æ»çº¿ä¸åå»ºå¹¶åå§åä¸ä¸?RX8130 è®¾å¤ã?
 *
 * Allocates an i2c_bus_device_handle internally. The handle must be released
 * with m5tab5_rtc_rx8130_delete() when no longer needed.
 * åé¨ä¼åéä¸ä¸?i2c_bus_device_handleï¼ä¸åéè¦æ¶å¿é¡»éè¿
 * m5tab5_rtc_rx8130_delete() éæ¾ã?
 *
 * @param  bus     SYS I2C bus handle (from m5tab5_get_sys_i2c_bus()).
 * @param  out_rtc Pointer to caller-allocated m5tab5_rtc_rx8130_t that will
 *                 be filled on success.
 * @param  bus     SYS I2C æ»çº¿å¥æï¼æ¥è?m5tab5_get_sys_i2c_bus()ï¼ã?
 * @param  out_rtc
 * ç±è°ç¨æ¹åéç?m5tab5_rtc_rx8130_tï¼å¯¹æåç»æè¿è¡åå¡«ã?
 * @return ESP_OK on success, otherwise an
 * error code.
 * @return æåè¿å ESP_OKï¼å¦åè¿åå¯¹åºéè¯¯ç ã?
 */
esp_err_t m5tab5_rtc_rx8130_init(i2c_bus_handle_t bus, m5tab5_rtc_rx8130_t* out_rtc);

/**
 * @brief  Release resources acquired by m5tab5_rtc_rx8130_init().
 *         éæ¾ m5tab5_rtc_rx8130_init() è·åçèµæºã?
 */
void m5tab5_rtc_rx8130_delete(m5tab5_rtc_rx8130_t* rtc);

// ââ Time Read and Write / æ¶é´è¯»å
// ââââââââââââââââââââââââââââââââââââââââââ

/**
 * @brief  Read the current date and time from the RX8130.
 *         ä»?RX8130 è¯»åå½åæ¥æåæ¶é´ã?
 * @param  rtc  Initialized RTC handle.
 * @param  out  Output datetime (must not be NULL).
 * @param  rtc  å·²åå§åç?RTC å¥æã?
 * @param  out  è¾åºæ¥ææ¶é´å¯¹è±¡ï¼ä¸è½ä¸ºç©ºï¼ã?
 * @return
 * ESP_OK on success.
 * @return æåè¿å ESP_OKã?
 */
esp_err_t m5tab5_rtc_rx8130_get_datetime(const m5tab5_rtc_rx8130_t* rtc, m5tab5_rtc_datetime_t* out);

/**
 * @brief  Write the date and time to the RX8130.
 *         å°æ¥æåæ¶é´åå¥ RX8130ã?
 *
 * Pass a datetime where undesired fields are set to -1 to skip writing that
 * part (date only, time only, or both).
 * è¥ä¸å¸æåå¥æäºå­æ®µï¼å¯å°å¶è®¾ä¸º
 * -1ï¼ä»èè·³è¿å¯¹åºé¨åï¼ä»æ¥æãä»æ¶é´æä¸¤èï¼ã?
 *
 * @param  rtc  Initialized RTC handle.
 * @param  dt   Datetime to set (date and/or time).
 * @param  rtc  å·²åå§åç?RTC å¥æã?
 * @param  dt
 * è¦è®¾ç½®çæ¥ææ¶é´ï¼å¯åå«æ¥æãæ¶é´æä¸¤èï¼ã?
 * @return ESP_OK on success.
 * @return æåè¿å ESP_OKã?
 */
esp_err_t m5tab5_rtc_rx8130_set_datetime(m5tab5_rtc_rx8130_t* rtc, const m5tab5_rtc_datetime_t* dt);

// ââ Status /
// ç¶ææ¥å?âââââââââââââââââââââââââââââââââââââââââââââââââââââââ

/**
 * @brief  Return true if the backup-voltage low flag (VBLF) is set.
 *         å¦æå¤ä»½ä½çµåæ å¿ï¼VBLFï¼è¢«ç½®ä½ï¼åè¿å trueã?
 *
 * When true, the RTC may have lost its time data and should be reset.
 * ä¸?true æ¶è¯´æ?RTC å¯è½å·²ç»ä¸¢å¤±æ¶é´æ°æ®ï¼å»ºè®®éæ°è®¾ç½®æ¶é´ã?
 */
bool m5tab5_rtc_rx8130_volt_low(const m5tab5_rtc_rx8130_t* rtc);

/**
 * @brief  Return true if any IRQ flag (AF or TF) is set.
 *         å¦æä»»æ IRQ æ å¿ï¼AF æ?TFï¼è¢«ç½®ä½ï¼åè¿å trueã?
 */
bool m5tab5_rtc_rx8130_irq_status(const m5tab5_rtc_rx8130_t* rtc);

/**
 * @brief  Clear all pending IRQ flags (AF and TF).
 *         æ¸é¤ææå¾å¤ç IRQ æ å¿ï¼AF å?TFï¼ã?
 */
esp_err_t m5tab5_rtc_rx8130_clear_irq(m5tab5_rtc_rx8130_t* rtc);

/**
 * @brief  Disable all IRQ sources (AIE and TIE cleared).
 *         ç¦ç¨å¨é¨ IRQ æºï¼æ¸é¤ AIE ä¸?TIEï¼ã?
 */
esp_err_t m5tab5_rtc_rx8130_disable_irq(m5tab5_rtc_rx8130_t* rtc);

// ââ Alarm / é¹éåè½
// âââââââââââââââââââââââââââââââââââââââââââââââââââââââââ

/**
 * @brief  Set a one-shot alarm IRQ.
 *         éç½®ä¸æ¬¡æ§é¹é?IRQã?
 *
 * @param  rtc   Initialized RTC handle.
 * @param  date  Alarm date (use NULL or set day/weekday to -1 to ignore).
 * @param  time  Alarm time (use NULL or set hours/minutes to -1 to ignore).
 * @param  rtc   å·²åå§åç?RTC å¥æã?
 * @param  date  é¹éæ¥æï¼ä¼  NULL æå° day/weekday è®¾ä¸º -1
 * è¡¨ç¤ºå¿½ç¥ï¼ã?
 * @param  time  é¹éæ¶é´ï¼ä¼  NULL æå° hours/minutes è®¾ä¸º -1 è¡¨ç¤ºå¿½ç¥ï¼ã?
 *
 * @return 1 if alarm was enabled, 0 if nothing was set, negative on error.
 * @return å¯ç¨é¹éè¿å 1ï¼è¥æªè®¾ç½®ä»»ä½åå®¹è¿å?0ï¼åºéè¿åè´å¼ã?
 */
int m5tab5_rtc_rx8130_set_alarm_irq(m5tab5_rtc_rx8130_t* rtc, const m5tab5_rtc_date_t* date,
                                    const m5tab5_rtc_time_t* time);

// ââ Periodic Timer /
// å¨æå®æ¶å?âââââââââââââââââââââââââââââââââââââââââââââ

/**
 * @brief  Configure the periodic countdown timer.
 *         éç½®å¨ææ§åè®¡æ¶å®æ¶å¨ã?
 *
 * @param  rtc        Initialized RTC handle.
 * @param  timer_msec Timer period in milliseconds. 0 = disable.
 * @param  rtc        å·²åå§åç?RTC å¥æã?
 * @param  timer_msec å®æ¶å¨å¨æï¼åä½æ¯«ç§ï¼ä¼  0
 * è¡¨ç¤ºç¦ç¨ã?
 * @return Actual period set in milliseconds (rounded due to hardware resolution), or 0 if disabled.
 * @return å®éè®¾ç½®æåçæ¯«ç§å¨æå¼ï¼ä¼åç¡¬ä»¶åè¾¨çå½±åèåæ´ï¼ï¼è¥ç¦ç¨åè¿å?0ã?
 */
uint32_t m5tab5_rtc_rx8130_set_timer_irq(m5tab5_rtc_rx8130_t* rtc, uint32_t timer_msec);

}  // namespace m5::tab5
