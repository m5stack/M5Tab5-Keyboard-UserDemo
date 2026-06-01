/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 *
 * RX8130 RTC driver implementation.
 * RX8130 RTC 驱动实现�?
 * Logic ported from M5Unified RX8130_Class (MIT, M5Stack).
 * 逻辑移植�?M5Unified RX8130_Class（MIT，M5Stack）�?
 * Adapted to use espressif/i2c_bus instead of M5Unified
 * I2C_Class. 已调整为使用 espressif/i2c_bus，而不�?M5Unified I2C_Class�?
 */

#include "drivers/rtc_rx8130/m5tab5_rtc_rx8130.h"
#include "drivers/m5tab5_driver_common.h"

#include "esp_log.h"
#include <cstdlib>
#include <cstring>

static const char* TAG = "m5tab5.rtc.rx8130";

namespace m5::tab5 {

// ── BCD Helpers / BCD 辅助函数
// ──────────────────────────────────────────────

static inline uint8_t bcd_to_byte(uint8_t bcd)
{
    return (uint8_t)(((bcd >> 4) * 10u) + (bcd & 0x0Fu));
}

static inline uint8_t byte_to_bcd(uint8_t val)
{
    uint8_t hi = val / 10u;
    return (uint8_t)((hi << 4) | (val - hi * 10u));
}

// ── Register Helpers /
// 寄存器辅助函�?───────────────────────────────────────

static esp_err_t rx8130_read(const m5tab5_rtc_rx8130_t* rtc, uint8_t reg, uint8_t* buf, size_t len)
{
    return i2c_bus_read_bytes(rtc->dev, reg, len, buf);
}

static esp_err_t rx8130_read8(const m5tab5_rtc_rx8130_t* rtc, uint8_t reg, uint8_t* val)
{
    return i2c_bus_read_byte(rtc->dev, reg, val);
}

static esp_err_t rx8130_write(m5tab5_rtc_rx8130_t* rtc, uint8_t reg, const uint8_t* buf, size_t len)
{
    return i2c_bus_write_bytes(rtc->dev, reg, len, const_cast<uint8_t*>(buf));
}

static esp_err_t rx8130_write8(m5tab5_rtc_rx8130_t* rtc, uint8_t reg, uint8_t val)
{
    return i2c_bus_write_byte(rtc->dev, reg, val);
}

static esp_err_t rx8130_bit_on(m5tab5_rtc_rx8130_t* rtc, uint8_t reg, uint8_t mask)
{
    uint8_t v     = 0;
    esp_err_t err = rx8130_read8(rtc, reg, &v);
    if (err != ESP_OK) return err;
    return rx8130_write8(rtc, reg, v | mask);
}

static esp_err_t rx8130_bit_off(m5tab5_rtc_rx8130_t* rtc, uint8_t reg, uint8_t mask)
{
    uint8_t v     = 0;
    esp_err_t err = rx8130_read8(rtc, reg, &v);
    if (err != ESP_OK) return err;
    return rx8130_write8(rtc, reg, v & (uint8_t)~mask);
}

// ── Lifecycle / 生命周期
// ─────────────────────────────────────────────────────

esp_err_t m5tab5_rtc_rx8130_init(i2c_bus_handle_t bus, m5tab5_rtc_rx8130_t* out_rtc)
{
    if (!bus || !out_rtc) return ESP_ERR_INVALID_ARG;

    out_rtc->dev         = nullptr;
    out_rtc->initialised = false;

    i2c_bus_device_handle_t dev = i2c_bus_device_create(bus, RX8130_I2C_ADDR, 400000);
    if (!dev) {
        ESP_LOGE(TAG, "i2c_bus_device_create failed (addr=0x%02X)", RX8130_I2C_ADDR);
        return ESP_ERR_NO_MEM;
    }

    // Probe: try reading the FLAG register; any I2C ACK is enough. / 探测阶段尝试读取 FLAG
    // 寄存器，只要�?I2C ACK 即可判断设备存在�?
    uint8_t flag_val = 0;
    esp_err_t err    = i2c_bus_read_byte(dev, RX8130_REG_FLAG, &flag_val);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "no ACK from RX8130 at 0x%02X (%s)", RX8130_I2C_ADDR, esp_err_to_name(err));
        i2c_bus_device_delete(&dev);
        return ESP_ERR_NOT_FOUND;
    }

    out_rtc->dev = dev;

    // Initialize: enable interrupt input and charge select (INIEN | CHSEL). /
    // 初始化：使能中断输入与充电选择位（INIEN | CHSEL）�?
    err = rx8130_bit_on(out_rtc, RX8130_REG_CONTROL1, RX8130_CTRL1_INIEN | RX8130_CTRL1_CHSEL);
    // Clear control register 0 (disable all IRQ sources). / 清空控制寄存�?0（禁用全�?IRQ 源）�?
    err |= rx8130_write8(out_rtc, RX8130_REG_CONTROL0, 0x00);
    // Clear the flag register to acknowledge any stale flags. /
    // 清空标志寄存器，以确认并清除遗留标志位�?
    err |= rx8130_write8(out_rtc, RX8130_REG_FLAG, 0x00);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "register initialisation failed (%s)", esp_err_to_name(err));
        i2c_bus_device_delete(&dev);
        out_rtc->dev = nullptr;
        return err;
    }

    out_rtc->initialised = true;
    ESP_LOGI(TAG, "RX8130 initialised (FLAG=0x%02X)", flag_val);
    if (flag_val & RX8130_FLAG_VBLF) {
        ESP_LOGW(TAG, "VBLF set �?backup voltage was low; please set the RTC time");
    }
    return ESP_OK;
}

void m5tab5_rtc_rx8130_delete(m5tab5_rtc_rx8130_t* rtc)
{
    if (!rtc) return;
    if (rtc->dev) {
        i2c_bus_device_delete(&rtc->dev);
    }
    rtc->initialised = false;
}

// ── Time Read and Write / 时间读写
// ──────────────────────────────────────────

esp_err_t m5tab5_rtc_rx8130_get_datetime(const m5tab5_rtc_rx8130_t* rtc, m5tab5_rtc_datetime_t* out)
{
    if (!rtc || !rtc->initialised || !out) return ESP_ERR_INVALID_ARG;

    // Read all seven time/date registers in one burst: 0x10 to 0x16. / 一次性突发读�?7
    // 个时间日期寄存器�?x10 �?0x16�?
    uint8_t buf[7] = {};
    esp_err_t err  = rx8130_read(rtc, RX8130_REG_SEC, buf, sizeof(buf));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "get_datetime read failed (%s)", esp_err_to_name(err));
        return err;
    }

    out->time.seconds = (int8_t)bcd_to_byte(buf[0] & 0x7Fu);  // SEC bits 6-0 / 秒寄存器�?6 �?0 �?
    out->time.minutes = (int8_t)bcd_to_byte(buf[1] & 0x7Fu);  // MIN bits 6-0 / 分寄存器�?6 �?0 �?
    out->time.hours = (int8_t)bcd_to_byte(buf[2] & 0x3Fu);  // HOUR bits 5-0 (24h) / 时寄存器�?5 �?0 位（24 小时制）
    out->date.weekday = (int8_t)(__builtin_ctz(buf[3]));  // WDAY one-hot to LSB position / 星期独热编码转换为最低位索引
    out->date.day   = (int8_t)bcd_to_byte(buf[4] & 0x3Fu);    // DAY bits 5-0 / 日期寄存器第 5 �?0 �?
    out->date.month = (int8_t)bcd_to_byte(buf[5] & 0x1Fu);    // MON bits 4-0 / 月寄存器�?4 �?0 �?
    out->date.year  = (int16_t)(bcd_to_byte(buf[6]) + 2000);  // YEAR offset from 2000 / 年份�?2000 为偏移基�?

    return ESP_OK;
}

esp_err_t m5tab5_rtc_rx8130_set_datetime(m5tab5_rtc_rx8130_t* rtc, const m5tab5_rtc_datetime_t* dt)
{
    if (!rtc || !rtc->initialised || !dt) return ESP_ERR_INVALID_ARG;

    bool write_time = (dt->time.seconds >= 0 || dt->time.minutes >= 0 || dt->time.hours >= 0);
    bool write_date = (dt->date.day >= 0 || dt->date.month >= 0 || dt->date.year >= 0);

    if (!write_time && !write_date) return ESP_ERR_INVALID_ARG;

    uint8_t buf[7]    = {};
    uint8_t reg_start = write_time ? RX8130_REG_SEC : RX8130_REG_WEEKDAY;
    int idx           = 0;

    if (write_time) {
        buf[idx++] = byte_to_bcd((uint8_t)(dt->time.seconds >= 0 ? dt->time.seconds : 0));
        buf[idx++] = byte_to_bcd((uint8_t)(dt->time.minutes >= 0 ? dt->time.minutes : 0));
        buf[idx++] = byte_to_bcd((uint8_t)(dt->time.hours >= 0 ? dt->time.hours : 0));
    }
    if (write_date) {
        // WEEKDAY register uses one-hot encoding. / WEEKDAY 寄存器使用独热编码�?
        buf[idx++] = (uint8_t)(1u << (dt->date.weekday >= 0 ? (dt->date.weekday & 0x07) : 0));
        buf[idx++] = byte_to_bcd((uint8_t)(dt->date.day >= 0 ? dt->date.day : 1));
        buf[idx++] = byte_to_bcd((uint8_t)(dt->date.month >= 0 ? dt->date.month : 1));
        buf[idx++] = byte_to_bcd((uint8_t)(dt->date.year >= 0 ? (dt->date.year % 100) : 0));
    }

    esp_err_t err = rx8130_write(rtc, reg_start, buf, (size_t)idx);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "set_datetime write failed (%s)", esp_err_to_name(err));
    }
    return err;
}

// ── Status /
// 状态接�?───────────────────────────────────────────────────────

bool m5tab5_rtc_rx8130_volt_low(const m5tab5_rtc_rx8130_t* rtc)
{
    if (!rtc || !rtc->initialised) return false;
    uint8_t flag = 0;
    if (rx8130_read8(rtc, RX8130_REG_FLAG, &flag) != ESP_OK) return false;
    return (flag & RX8130_FLAG_VBLF) != 0;
}

bool m5tab5_rtc_rx8130_irq_status(const m5tab5_rtc_rx8130_t* rtc)
{
    if (!rtc || !rtc->initialised) return false;
    uint8_t flag = 0;
    if (rx8130_read8(rtc, RX8130_REG_FLAG, &flag) != ESP_OK) return false;
    return (flag & (RX8130_FLAG_AF | RX8130_FLAG_TF)) != 0;
}

esp_err_t m5tab5_rtc_rx8130_clear_irq(m5tab5_rtc_rx8130_t* rtc)
{
    if (!rtc || !rtc->initialised) return ESP_ERR_INVALID_ARG;
    // Clear the AF and TF bits. / 清除 AF �?TF 标志位�?
    return rx8130_bit_off(rtc, RX8130_REG_FLAG, RX8130_FLAG_AF | RX8130_FLAG_TF);
}

esp_err_t m5tab5_rtc_rx8130_disable_irq(m5tab5_rtc_rx8130_t* rtc)
{
    if (!rtc || !rtc->initialised) return ESP_ERR_INVALID_ARG;
    return rx8130_bit_off(rtc, RX8130_REG_CONTROL0, RX8130_CTRL0_AIE | RX8130_CTRL0_TIE);
}

// ── Alarm / 闹钟功能
// ─────────────────────────────────────────────────────────

int m5tab5_rtc_rx8130_set_alarm_irq(m5tab5_rtc_rx8130_t* rtc, const m5tab5_rtc_date_t* date,
                                    const m5tab5_rtc_time_t* time)
{
    if (!rtc || !rtc->initialised) return -1;

    // 0x80 in alarm registers means "ignore / disabled". / 闹钟寄存器中�?0x80 表示“忽�?/ 禁用”�?
    uint8_t buf[3]  = {0x80, 0x80, 0x80};
    bool irq_enable = false;

    if (time) {
        if (time->minutes >= 0) {
            irq_enable = true;
            buf[0]     = byte_to_bcd((uint8_t)time->minutes) & 0x7Fu;
        }
        if (time->hours >= 0) {
            irq_enable = true;
            buf[1]     = byte_to_bcd((uint8_t)time->hours) & 0x3Fu;
        }
    }

    if (date) {
        int flg_wada = -1;  // 0=week alarm, 1=day alarm / 0 表示按星期闹钟，1 表示按日期闹�?
        if (date->day >= 0) {
            flg_wada = 1;
            buf[2]   = byte_to_bcd((uint8_t)date->day) & 0x3Fu;
        } else if (date->weekday >= 0) {
            flg_wada = 0;
            buf[2]   = (uint8_t)(1u << (date->weekday & 0x07u));
        }
        if (flg_wada >= 0) {
            irq_enable = true;
            esp_err_t err;
            if (flg_wada) {
                err = rx8130_bit_on(rtc, RX8130_REG_EXTENSION, RX8130_EXT_WADA);
            } else {
                err = rx8130_bit_off(rtc, RX8130_REG_EXTENSION, RX8130_EXT_WADA);
            }
            if (err != ESP_OK) return -1;
        }
    }

    // Write MIN_ALARM, HOUR_ALARM, and WDAY/DAY_ALARM. / 写入 MIN_ALARM、HOUR_ALARM �?WDAY/DAY_ALARM�?
    esp_err_t err = rx8130_write(rtc, RX8130_REG_MIN_ALARM, buf, 3);
    if (err != ESP_OK) return -1;

    if (irq_enable) {
        err = rx8130_bit_on(rtc, RX8130_REG_CONTROL0, RX8130_CTRL0_AIE);
    } else {
        err = rx8130_bit_off(rtc, RX8130_REG_CONTROL0, RX8130_CTRL0_AIE);
    }
    return (err == ESP_OK) ? (int)irq_enable : -1;
}

// ── Periodic Timer /
// 周期定时�?─────────────────────────────────────────────

uint32_t m5tab5_rtc_rx8130_set_timer_irq(m5tab5_rtc_rx8130_t* rtc, uint32_t msec)
{
    if (!rtc || !rtc->initialised) return 0;

    // Select the best timer source (TSEL bits) based on the requested period. /
    // 根据请求周期选择最合适的定时器时钟源（TSEL 位）�?
    uint32_t div = 1;  // Hz divider: counter ticks per second / 频率分频：每秒计数器跳变次数
    uint32_t mul = 1;  // multiplier: seconds per count / 乘数：每次计数对应的秒数
    uint8_t tsel = 0;

    if (msec < (65536u * 1000u / 4096u)) {  // about 16 s -> 4096 Hz / �?16 秒，对应 4096 Hz
        tsel = 0x00;
        div  = 4096;
    } else if (msec < (65536u * 1000u / 64u)) {  // about 1024 s -> 64 Hz / �?1024 秒，对应 64 Hz
        tsel = 0x01;
        div  = 64;
    } else if (msec < (65536u * 1000u)) {  // about 65535 s -> 1 Hz / �?65535 秒，对应 1 Hz
        tsel = 0x02;
        div  = 1;
    } else if (msec < (65536u * 60u * 1000u)) {  // about 45 days -> per-minute / �?45 天，对应每分钟计�?
        tsel = 0x03;
        mul  = 60;
    } else {  // about 1.25 years -> per-hour / �?1.25 年，对应每小时计�?
        tsel = 0x04;
        mul  = 3600;
    }

    uint32_t result    = 0;
    uint8_t regdata[3] = {};
    if (rx8130_read(rtc, RX8130_REG_TIMER0, regdata, 3) != ESP_OK) return 0;

    uint32_t mul_ms = mul * 1000u;
    uint32_t cycle  = (msec * div + (mul_ms >> 1)) / mul_ms;
    if (cycle > 65535u) cycle = 65535u;
    result = cycle * mul_ms / div;

    regdata[0] = (uint8_t)(cycle & 0xFFu);
    regdata[1] = (uint8_t)((cycle >> 8) & 0xFFu);

    if (cycle > 0) {
        // Set TSEL bits and TE (timer enable) while preserving other EXTENSION bits. / 设置 TSEL 位与
        // TE（定时器使能），同时保留其他 EXTENSION 位�?
        regdata[2] = (uint8_t)((regdata[2] & ~(RX8130_EXT_TE | RX8130_EXT_TSEL_MASK)) | RX8130_EXT_TE | tsel);
        rx8130_bit_on(rtc, RX8130_REG_CONTROL0, RX8130_CTRL0_TIE);
    } else {
        regdata[2] &= (uint8_t)~RX8130_EXT_TE;
        rx8130_bit_off(rtc, RX8130_REG_CONTROL0, RX8130_CTRL0_TIE);
    }
    rx8130_write(rtc, RX8130_REG_TIMER0, regdata, 3);
    return result;
}

// ── Global singleton
// ─────────────────────────────────────────────────────────

static m5tab5_rtc_rx8130_t s_global_rtc;

esp_err_t m5tab5_rtc_init()
{
    if (s_global_rtc.initialised) return ESP_OK;
    i2c_bus_handle_t bus = m5tab5_get_sys_i2c_bus();
    if (!bus) {
        ESP_LOGE(TAG, "m5tab5_rtc_init: SYS I2C bus not ready (call begin() first)");
        return ESP_ERR_INVALID_STATE;
    }
    return m5tab5_rtc_rx8130_init(bus, &s_global_rtc);
}

esp_err_t m5tab5_rtc_get_datetime(m5tab5_rtc_datetime_t* out)
{
    if (!s_global_rtc.initialised) return ESP_ERR_INVALID_STATE;
    return m5tab5_rtc_rx8130_get_datetime(&s_global_rtc, out);
}

esp_err_t m5tab5_rtc_set_datetime(const m5tab5_rtc_datetime_t* dt)
{
    if (!s_global_rtc.initialised) return ESP_ERR_INVALID_STATE;
    return m5tab5_rtc_rx8130_set_datetime(&s_global_rtc, dt);
}

bool m5tab5_rtc_volt_low()
{
    if (!s_global_rtc.initialised) return false;
    return m5tab5_rtc_rx8130_volt_low(&s_global_rtc);
}

}  // namespace m5::tab5
