/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 *
 * INA226 current and power monitor driver for m5_tab5_component.
 * m5_tab5_component �?INA226 电流与功率监测驱动�?
 * Uses espressif/i2c_bus (the same SYS I2C bus as other
 * built-in peripherals). 使用 espressif/i2c_bus，并与其他板载外设共用同一�?SYS I2C 总线�?
 * Tab5
 * hardware: Tab5 硬件参数�?
 *   I2C address : 0x41 I2C 地址    : 0x41 Shunt       : 5 mΩ (0.005 Ω)
 *   分流电阻    : 5 mΩ�?.005 Ω�?
 *   Max current : 2.0 A
 *   最大电�?   : 2.0 A
 *
 * Reference: Texas Instruments INA226 datasheet (SBOS547).
 * 参考资料：Texas Instruments INA226 数据手册（SBOS547）�?
 */
#pragma once

#include <cstdint>
#include "esp_err.h"
#include "i2c_bus.h"

namespace m5::tab5 {

// ── INA226 Default I2C Address / INA226 默认 I2C 地址
// ──────────────────────
constexpr uint8_t INA226_I2C_ADDR_TAB5 = 0x41;

// ── INA226 Register Addresses / INA226 寄存器地址
// ──────────────────────────
constexpr uint8_t INA226_REG_CONFIG  = 0x00;
constexpr uint8_t INA226_REG_SHUNT_V = 0x01;  ///< signed, 2.5 µV/LSB / 有符号，2.5 µV �?LSB
constexpr uint8_t INA226_REG_BUS_V   = 0x02;  ///< unsigned, 1.25 mV/LSB / 无符号，1.25 mV �?LSB
constexpr uint8_t INA226_REG_POWER = 0x03;  ///< unsigned, 25 × current_lsb W/LSB / 无符号，25 × current_lsb W �?LSB
constexpr uint8_t INA226_REG_CURRENT     = 0x04;  ///< signed, current_lsb A/LSB / 有符号，current_lsb A �?LSB
constexpr uint8_t INA226_REG_CALIBRATION = 0x05;
constexpr uint8_t INA226_REG_MFR_ID      = 0xFE;  ///< expected 0x5449 / 期望值为 0x5449
constexpr uint8_t INA226_REG_DIE_ID      = 0xFF;  ///< expected 0x2260 / 期望值为 0x2260

// ── CONFIG Field Values / CONFIG
// 字段�?─────────────────────────────────────
constexpr uint16_t INA226_CFG_RST = 1u << 15;

/// AVG field (bits [11:9]).
/// AVG 字段（位 [11:9]）�?
enum class Ina226Avg : uint16_t {
    AVG_1    = 0,
    AVG_4    = 1,
    AVG_16   = 2,
    AVG_64   = 3,
    AVG_128  = 4,
    AVG_256  = 5,
    AVG_512  = 6,
    AVG_1024 = 7,
};

/// Conversion time field (bits [8:6] VBUSCT, [5:3] VSHCT).
/// 转换时间字段（位 [8:6] �?VBUSCT，位 [5:3] �?VSHCT）�?
enum class Ina226ConvTime : uint16_t {
    US_140   = 0,
    US_204   = 1,
    US_332   = 2,
    US_588   = 3,
    MS_1_1   = 4,  ///< 1.1 ms (default on reset) / 1.1 ms（复位默认值）
    MS_2_116 = 5,
    MS_4_156 = 6,
    MS_8_244 = 7,
};

/// Operating mode (bits [2:0]).
/// 工作模式（位 [2:0]）�?
enum class Ina226Mode : uint16_t {
    POWER_DOWN     = 0,
    SHUNT_TRIG     = 1,
    BUS_TRIG       = 2,
    SHUNT_BUS_TRIG = 3,
    ADC_OFF        = 4,
    SHUNT_CONT     = 5,
    BUS_CONT       = 6,
    SHUNT_BUS_CONT = 7,  ///< default on reset / 复位默认模式
};

// ── Configuration Passed to m5tab5_ina226_init() / 传给 m5tab5_ina226_init() 的配�?──
struct m5tab5_ina226_config_t {
    i2c_bus_handle_t bus    = nullptr;
    uint8_t i2c_addr        = INA226_I2C_ADDR_TAB5;
    uint32_t freq_hz        = 400000;
    float shunt_ohms        = 0.005f;  ///< Tab5 shunt resistor 5 mΩ / Tab5 使用�?5 mΩ 分流电阻
    float max_current_a     = 2.0f;    ///< determines current_lsb / 用于计算 current_lsb
    Ina226Avg averaging     = Ina226Avg::AVG_16;
    Ina226ConvTime bus_ct   = Ina226ConvTime::MS_1_1;
    Ina226ConvTime shunt_ct = Ina226ConvTime::MS_1_1;
    Ina226Mode mode         = Ina226Mode::SHUNT_BUS_CONT;
};

// ── Opaque Driver Handle / 不透明驱动句柄
// ───────────────────────────────────
struct m5tab5_ina226_t {
    i2c_bus_device_handle_t dev = nullptr;
    float current_lsb = 0.0f;  ///< A per LSB, set during init / 每个 LSB 对应的安培数，在初始化阶段设�?
};

// ── Lifecycle / 生命周期
// ──────────────────────────────────────────────────────

/**
 * @brief  Initialize the INA226 on the given I2C bus.
 *         在指定的 I2C 总线上初始化 INA226�?
 *
 * Verifies the manufacturer and die ID, resets the chip, programs the CONFIG
 * and CALIBRATION registers, and stores current_lsb in @p out.
 * 会校验厂�?ID 与芯�?ID、复位芯片、写�?CONFIG �?CALIBRATION 寄存器，
 * 并将 current_lsb 保存�?@p out 中�?
 *
 * @param  cfg  Configuration (bus handle, shunt, max current, etc.).
 * @param  out  Caller-allocated handle; filled on success.
 * @param  cfg  配置参数（总线句柄、分流电阻、最大电流等）�?
 * @param  out
 * 由调用方分配的句柄对象，成功时会被填充�?
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if no
 * INA226 found, or another error code on I2C failure.
 * @return 成功返回 ESP_OK；若未找�?INA226 返回 ESP_ERR_NOT_FOUND�?
 *         �?I2C
 * 通信失败则返回其他错误码�?
 */
esp_err_t m5tab5_ina226_init(const m5tab5_ina226_config_t* cfg, m5tab5_ina226_t* out);

/**
 * @brief  Release the I2C device resources acquired by m5tab5_ina226_init().
 *         释放 m5tab5_ina226_init() 获取�?I2C 设备资源�?
 */
void m5tab5_ina226_deinit(m5tab5_ina226_t* dev);

// ── Measurement / 测量接口
// ───────────────────────────────────────────────────

/**
 * @brief  Read bus voltage (V).
 *         读取总线电压（V）�?
 * @note   1.25 mV/LSB, unsigned 16-bit register.
 * @note   该寄存器为无符号 16 位，分辨率为 1.25 mV/LSB�?
 */
esp_err_t m5tab5_ina226_read_bus_voltage(const m5tab5_ina226_t* dev, float* voltage_v);

/**
 * @brief  Read shunt voltage (V).
 *         读取分流电阻电压（V）�?
 * @note   2.5 µV/LSB, signed 16-bit register.
 * @note   该寄存器为有符号 16 位，分辨率为 2.5 µV/LSB�?
 */
esp_err_t m5tab5_ina226_read_shunt_voltage(const m5tab5_ina226_t* dev, float* shunt_v);

/**
 * @brief  Read current (A).
 *         读取电流（A）�?
 * @note   Uses the current_lsb stored in @p dev. CALIBRATION must have been
 *         programmed, which is done automatically by m5tab5_ina226_init().
 * @note   使用保存�?@p dev 中的 current_lsb。CALIBRATION 寄存器必须已经写入，
 *         而这一步会�?m5tab5_ina226_init() 自动完成�?
 */
esp_err_t m5tab5_ina226_read_current(const m5tab5_ina226_t* dev, float* current_a);

/**
 * @brief  Read power (W).
 *         读取功率（W）�?
 * @note   LSB = 25 × current_lsb.
 * @note   每个 LSB 对应 25 × current_lsb�?
 */
esp_err_t m5tab5_ina226_read_power(const m5tab5_ina226_t* dev, float* power_w);

}  // namespace m5::tab5
