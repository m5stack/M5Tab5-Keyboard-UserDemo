/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 *
 * INA226 current and power monitor driver for m5_tab5_component.
 * m5_tab5_component ç?INA226 çµæµä¸åççæµé©±å¨ã?
 * Uses espressif/i2c_bus (the same SYS I2C bus as other
 * built-in peripherals). ä½¿ç¨ espressif/i2c_busï¼å¹¶ä¸å¶ä»æ¿è½½å¤è®¾å±ç¨åä¸æ?SYS I2C æ»çº¿ã?
 * Tab5
 * hardware: Tab5 ç¡¬ä»¶åæ°ï¼?
 *   I2C address : 0x41 I2C å°å    : 0x41 Shunt       : 5 mÎ© (0.005 Î©)
 *   åæµçµé»    : 5 mÎ©ï¼?.005 Î©ï¼?
 *   Max current : 2.0 A
 *   æå¤§çµæµ?   : 2.0 A
 *
 * Reference: Texas Instruments INA226 datasheet (SBOS547).
 * åèèµæï¼Texas Instruments INA226 æ°æ®æåï¼SBOS547ï¼ã?
 */
#pragma once

#include <cstdint>
#include "esp_err.h"
#include "i2c_bus.h"

namespace m5::tab5 {

// ââ INA226 Default I2C Address / INA226 é»è®¤ I2C å°å
// ââââââââââââââââââââââ
constexpr uint8_t INA226_I2C_ADDR_TAB5 = 0x41;

// ââ INA226 Register Addresses / INA226 å¯å­å¨å°å
// ââââââââââââââââââââââââââ
constexpr uint8_t INA226_REG_CONFIG  = 0x00;
constexpr uint8_t INA226_REG_SHUNT_V = 0x01;  ///< signed, 2.5 ÂµV/LSB / æç¬¦å·ï¼2.5 ÂµV æ¯?LSB
constexpr uint8_t INA226_REG_BUS_V   = 0x02;  ///< unsigned, 1.25 mV/LSB / æ ç¬¦å·ï¼1.25 mV æ¯?LSB
constexpr uint8_t INA226_REG_POWER =
    0x03;  ///< unsigned, 25 Ã current_lsb W/LSB / æ ç¬¦å·ï¼25 Ã current_lsb W æ¯?LSB
constexpr uint8_t INA226_REG_CURRENT     = 0x04;  ///< signed, current_lsb A/LSB / æç¬¦å·ï¼current_lsb A æ¯?LSB
constexpr uint8_t INA226_REG_CALIBRATION = 0x05;
constexpr uint8_t INA226_REG_MFR_ID      = 0xFE;  ///< expected 0x5449 / ææå¼ä¸º 0x5449
constexpr uint8_t INA226_REG_DIE_ID      = 0xFF;  ///< expected 0x2260 / ææå¼ä¸º 0x2260

// ââ CONFIG Field Values / CONFIG
// å­æ®µå?âââââââââââââââââââââââââââââââââââââ
constexpr uint16_t INA226_CFG_RST = 1u << 15;

/// AVG field (bits [11:9]).
/// AVG å­æ®µï¼ä½ [11:9]ï¼ã?
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
/// è½¬æ¢æ¶é´å­æ®µï¼ä½ [8:6] ä¸?VBUSCTï¼ä½ [5:3] ä¸?VSHCTï¼ã?
enum class Ina226ConvTime : uint16_t {
    US_140   = 0,
    US_204   = 1,
    US_332   = 2,
    US_588   = 3,
    MS_1_1   = 4,  ///< 1.1 ms (default on reset) / 1.1 msï¼å¤ä½é»è®¤å¼ï¼
    MS_2_116 = 5,
    MS_4_156 = 6,
    MS_8_244 = 7,
};

/// Operating mode (bits [2:0]).
/// å·¥ä½æ¨¡å¼ï¼ä½ [2:0]ï¼ã?
enum class Ina226Mode : uint16_t {
    POWER_DOWN     = 0,
    SHUNT_TRIG     = 1,
    BUS_TRIG       = 2,
    SHUNT_BUS_TRIG = 3,
    ADC_OFF        = 4,
    SHUNT_CONT     = 5,
    BUS_CONT       = 6,
    SHUNT_BUS_CONT = 7,  ///< default on reset / å¤ä½é»è®¤æ¨¡å¼
};

// ââ Configuration Passed to m5tab5_ina226_init() / ä¼ ç» m5tab5_ina226_init() çéç½?ââ
struct m5tab5_ina226_config_t {
    i2c_bus_handle_t bus    = nullptr;
    uint8_t i2c_addr        = INA226_I2C_ADDR_TAB5;
    uint32_t freq_hz        = 400000;
    float shunt_ohms        = 0.005f;  ///< Tab5 shunt resistor 5 mÎ© / Tab5 ä½¿ç¨ç?5 mÎ© åæµçµé»
    float max_current_a     = 2.0f;    ///< determines current_lsb / ç¨äºè®¡ç® current_lsb
    Ina226Avg averaging     = Ina226Avg::AVG_16;
    Ina226ConvTime bus_ct   = Ina226ConvTime::MS_1_1;
    Ina226ConvTime shunt_ct = Ina226ConvTime::MS_1_1;
    Ina226Mode mode         = Ina226Mode::SHUNT_BUS_CONT;
};

// ââ Opaque Driver Handle / ä¸éæé©±å¨å¥æ
// âââââââââââââââââââââââââââââââââââ
struct m5tab5_ina226_t {
    i2c_bus_device_handle_t dev = nullptr;
    float current_lsb =
        0.0f;  ///< A per LSB, set during init / æ¯ä¸ª LSB å¯¹åºçå®å¹æ°ï¼å¨åå§åé¶æ®µè®¾ç½?
};

// ââ Lifecycle / çå½å¨æ
// ââââââââââââââââââââââââââââââââââââââââââââââââââââââ

/**
 * @brief  Initialize the INA226 on the given I2C bus.
 *         å¨æå®ç I2C æ»çº¿ä¸åå§å INA226ã?
 *
 * Verifies the manufacturer and die ID, resets the chip, programs the CONFIG
 * and CALIBRATION registers, and stores current_lsb in @p out.
 * ä¼æ ¡éªåå?ID ä¸è¯ç?IDãå¤ä½è¯çãåå?CONFIG ä¸?CALIBRATION å¯å­å¨ï¼
 * å¹¶å° current_lsb ä¿å­å?@p out ä¸­ã?
 *
 * @param  cfg  Configuration (bus handle, shunt, max current, etc.).
 * @param  out  Caller-allocated handle; filled on success.
 * @param  cfg  éç½®åæ°ï¼æ»çº¿å¥æãåæµçµé»ãæå¤§çµæµç­ï¼ã?
 * @param  out
 * ç±è°ç¨æ¹åéçå¥æå¯¹è±¡ï¼æåæ¶ä¼è¢«å¡«åã?
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if no
 * INA226 found, or another error code on I2C failure.
 * @return æåè¿å ESP_OKï¼è¥æªæ¾å?INA226 è¿å ESP_ERR_NOT_FOUNDï¼?
 *         è?I2C
 * éä¿¡å¤±è´¥åè¿åå¶ä»éè¯¯ç ã?
 */
esp_err_t m5tab5_ina226_init(const m5tab5_ina226_config_t* cfg, m5tab5_ina226_t* out);

/**
 * @brief  Release the I2C device resources acquired by m5tab5_ina226_init().
 *         éæ¾ m5tab5_ina226_init() è·åç?I2C è®¾å¤èµæºã?
 */
void m5tab5_ina226_deinit(m5tab5_ina226_t* dev);

// ââ Measurement / æµéæ¥å£
// âââââââââââââââââââââââââââââââââââââââââââââââââââ

/**
 * @brief  Read bus voltage (V).
 *         è¯»åæ»çº¿çµåï¼Vï¼ã?
 * @note   1.25 mV/LSB, unsigned 16-bit register.
 * @note   è¯¥å¯å­å¨ä¸ºæ ç¬¦å· 16 ä½ï¼åè¾¨çä¸º 1.25 mV/LSBã?
 */
esp_err_t m5tab5_ina226_read_bus_voltage(const m5tab5_ina226_t* dev, float* voltage_v);

/**
 * @brief  Read shunt voltage (V).
 *         è¯»ååæµçµé»çµåï¼Vï¼ã?
 * @note   2.5 ÂµV/LSB, signed 16-bit register.
 * @note   è¯¥å¯å­å¨ä¸ºæç¬¦å· 16 ä½ï¼åè¾¨çä¸º 2.5 ÂµV/LSBã?
 */
esp_err_t m5tab5_ina226_read_shunt_voltage(const m5tab5_ina226_t* dev, float* shunt_v);

/**
 * @brief  Read current (A).
 *         è¯»åçµæµï¼Aï¼ã?
 * @note   Uses the current_lsb stored in @p dev. CALIBRATION must have been
 *         programmed, which is done automatically by m5tab5_ina226_init().
 * @note   ä½¿ç¨ä¿å­å?@p dev ä¸­ç current_lsbãCALIBRATION å¯å­å¨å¿é¡»å·²ç»åå¥ï¼
 *         èè¿ä¸æ­¥ä¼ç?m5tab5_ina226_init() èªå¨å®æã?
 */
esp_err_t m5tab5_ina226_read_current(const m5tab5_ina226_t* dev, float* current_a);

/**
 * @brief  Read power (W).
 *         è¯»ååçï¼Wï¼ã?
 * @note   LSB = 25 Ã current_lsb.
 * @note   æ¯ä¸ª LSB å¯¹åº 25 Ã current_lsbã?
 */
esp_err_t m5tab5_ina226_read_power(const m5tab5_ina226_t* dev, float* power_w);

}  // namespace m5::tab5
