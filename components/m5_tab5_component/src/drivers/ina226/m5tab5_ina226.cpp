/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 *
 * INA226 current and power monitor driver implementation.
 * INA226 çµæµä¸åççæµé©±å¨å®ç°ã?
 * Derived from the INA226 datasheet (TI SBOS547) and M5Unified
 * INA226_Class (MIT). åºäº INA226 æ°æ®æåï¼TI SBOS547ï¼ä»¥å?M5Unified INA226_Classï¼MITï¼æ´çå®ç°ã?
 *
 * Uses espressif/i2c_bus for thread-safe I2C access. ä½¿ç¨ espressif/i2c_bus æä¾çº¿ç¨å®å¨ç?I2C è®¿é®ã?
 */

#include "drivers/ina226/m5tab5_ina226.h"
#include "drivers/m5tab5_driver_common.h"

#include "esp_log.h"
#include <cmath>
#include <cstring>

static const char* TAG = "m5tab5.ina226";

namespace m5::tab5 {

// ââ INA226 Identification Constants / INA226 è¯å«å¸¸é
// ââââââââââââââââââââââ
static constexpr uint16_t INA226_MANUFACTURER_ID = 0x5449u;  ///< "TI" / å¾·å·ä»ªå¨åå ID
static constexpr uint16_t INA226_DIE_ID          = 0x2260u;

// ââ Low-Level 16-Bit Register Helpers / åºå± 16 ä½å¯å­å¨è¾å©å½æ° ââââââââââââ

static esp_err_t ina226_read_reg16(i2c_bus_device_handle_t dev, uint8_t reg, uint16_t* out)
{
    uint8_t buf[2] = {};
    esp_err_t err  = i2c_bus_read_bytes(dev, reg, 2, buf);
    if (err != ESP_OK) return err;
    *out = (static_cast<uint16_t>(buf[0]) << 8) | buf[1];
    return ESP_OK;
}

static esp_err_t ina226_write_reg16(i2c_bus_device_handle_t dev, uint8_t reg, uint16_t val)
{
    uint8_t buf[2] = {
        static_cast<uint8_t>(val >> 8),
        static_cast<uint8_t>(val & 0xFF),
    };
    return i2c_bus_write_bytes(dev, reg, 2, buf);
}

// ââ CONFIG Register Builder / CONFIG å¯å­å¨æé å¨
// âââââââââââââââââââââââââââ

static uint16_t ina226_build_config(Ina226Avg avg, Ina226ConvTime bus_ct, Ina226ConvTime shunt_ct, Ina226Mode mode)
{
    // Bits [14:12] = 0b100 (fixed per datasheet, preserved after reset) / ä½?[14:12]
    // åºå®ä¸?0b100ï¼æ°æ®æåè§å®ï¼å¤ä½åä¿çï¼
    uint16_t cfg = 0x4000u;
    cfg |= (static_cast<uint16_t>(avg) & 0x7u) << 9;
    cfg |= (static_cast<uint16_t>(bus_ct) & 0x7u) << 6;
    cfg |= (static_cast<uint16_t>(shunt_ct) & 0x7u) << 3;
    cfg |= (static_cast<uint16_t>(mode) & 0x7u);
    return cfg;
}

// ââ Calibration Value /
// æ ¡åå¼è®¡ç®?ââââââââââââââââââââââââââââââââââââââââââ
// current_lsb = max_current_a / 32768  (A/LSB, the smallest full-scale step)
// current_lsb = max_current_a / 32768ï¼åä½?A/LSBï¼å¯¹åºæ»¡éç¨ä¸çæå°æ­¥è¿ï¼
// CAL = floor(0.00512 / (current_lsb * shunt_ohms))
// CAL = floor(0.00512 / (current_lsb * shunt_ohms))

static float ina226_compute_current_lsb(float max_current_a)
{
    return max_current_a / 32768.0f;
}

static uint16_t ina226_compute_cal(float current_lsb, float shunt_ohms)
{
    float cal_f  = 0.00512f / (current_lsb * shunt_ohms);
    uint16_t cal = static_cast<uint16_t>(cal_f);
    if (cal == 0) cal = 1;
    return cal;
}

// ââ Lifecycle / çå½å¨æ
// ââââââââââââââââââââââââââââââââââââââââââââââââââââââ

esp_err_t m5tab5_ina226_init(const m5tab5_ina226_config_t* cfg, m5tab5_ina226_t* out)
{
    if (!cfg || !out || !cfg->bus) return ESP_ERR_INVALID_ARG;

    out->dev         = nullptr;
    out->current_lsb = 0.0f;

    // â?Create the I2C device handle / åå»ºè®¾å¤ I2C å¥æ ââââââââââââââââ?
    i2c_bus_device_handle_t dev = i2c_bus_device_create(cfg->bus, cfg->i2c_addr, cfg->freq_hz);
    if (!dev) {
        ESP_LOGE(TAG, "i2c_bus_device_create failed (addr=0x%02X)", cfg->i2c_addr);
        return ESP_ERR_NO_MEM;
    }

    // â?Verify the manufacturer and die ID / æ ¡éªåå ID ä¸è¯ç?ID ââââââââ?
    uint16_t mfr = 0, die = 0;
    esp_err_t err = ina226_read_reg16(dev, INA226_REG_MFR_ID, &mfr);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "no ACK from INA226 at 0x%02X (%s)", cfg->i2c_addr, esp_err_to_name(err));
        i2c_bus_device_delete(&dev);
        return ESP_ERR_NOT_FOUND;
    }
    err = ina226_read_reg16(dev, INA226_REG_DIE_ID, &die);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "DIE_ID read failed (%s)", esp_err_to_name(err));
        i2c_bus_device_delete(&dev);
        return err;
    }
    if (mfr != INA226_MANUFACTURER_ID || die != INA226_DIE_ID) {
        ESP_LOGE(TAG, "INA226 ID mismatch: mfr=0x%04X die=0x%04X (expected 0x5449/0x2260)", mfr, die);
        i2c_bus_device_delete(&dev);
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "INA226 found at 0x%02X (mfr=0x%04X die=0x%04X)", cfg->i2c_addr, mfr, die);

    // â?Reset the device / å¤ä½è®¾å¤
    // âââââââââââââââââââââââââââââââââââââââââ
    err = ina226_write_reg16(dev, INA226_REG_CONFIG, INA226_CFG_RST);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "reset failed (%s)", esp_err_to_name(err));
        i2c_bus_device_delete(&dev);
        return err;
    }

    // â?Program CONFIG / åå¥ CONFIG
    // âââââââââââââââââââââââââââââââââââââââ
    uint16_t config_val = ina226_build_config(cfg->averaging, cfg->bus_ct, cfg->shunt_ct, cfg->mode);
    err                 = ina226_write_reg16(dev, INA226_REG_CONFIG, config_val);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "CONFIG write failed (%s)", esp_err_to_name(err));
        i2c_bus_device_delete(&dev);
        return err;
    }

    // â?Compute current_lsb and program CALIBRATION / è®¡ç® current_lsb å¹¶åå?CALIBRATION ââ
    float current_lsb = ina226_compute_current_lsb(cfg->max_current_a);
    uint16_t cal      = ina226_compute_cal(current_lsb, cfg->shunt_ohms);
    err               = ina226_write_reg16(dev, INA226_REG_CALIBRATION, cal);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "CALIBRATION write failed (%s)", esp_err_to_name(err));
        i2c_bus_device_delete(&dev);
        return err;
    }

    ESP_LOGI(TAG, "INA226 configured: CONFIG=0x%04X CAL=0x%04X current_lsb=%.2f mA", config_val, cal,
             current_lsb * 1000.0f);

    out->dev         = dev;
    out->current_lsb = current_lsb;
    return ESP_OK;
}

void m5tab5_ina226_deinit(m5tab5_ina226_t* dev)
{
    if (!dev) return;
    if (dev->dev) {
        i2c_bus_device_delete(&dev->dev);
    }
    dev->current_lsb = 0.0f;
}

// ââ Measurement / æµéæ¥å£
// âââââââââââââââââââââââââââââââââââââââââââââââââââ

esp_err_t m5tab5_ina226_read_bus_voltage(const m5tab5_ina226_t* dev, float* voltage_v)
{
    if (!dev || !dev->dev || !voltage_v) return ESP_ERR_INVALID_ARG;
    uint16_t raw  = 0;
    esp_err_t err = ina226_read_reg16(dev->dev, INA226_REG_BUS_V, &raw);
    if (err != ESP_OK) return err;
    *voltage_v = raw * 1.25e-3f;  // 1.25 mV/LSB / æ¯ä¸ª LSB å¯¹åº 1.25 mV
    return ESP_OK;
}

esp_err_t m5tab5_ina226_read_shunt_voltage(const m5tab5_ina226_t* dev, float* shunt_v)
{
    if (!dev || !dev->dev || !shunt_v) return ESP_ERR_INVALID_ARG;
    uint16_t raw  = 0;
    esp_err_t err = ina226_read_reg16(dev->dev, INA226_REG_SHUNT_V, &raw);
    if (err != ESP_OK) return err;
    *shunt_v =
        static_cast<int16_t>(raw) * 2.5e-6f;  // 2.5 ÂµV/LSB, signed / æç¬¦å·å¯å­å¨ï¼æ¯ä¸?LSB å¯¹åº 2.5 ÂµV
    return ESP_OK;
}

esp_err_t m5tab5_ina226_read_current(const m5tab5_ina226_t* dev, float* current_a)
{
    if (!dev || !dev->dev || !current_a) return ESP_ERR_INVALID_ARG;
    uint16_t raw  = 0;
    esp_err_t err = ina226_read_reg16(dev->dev, INA226_REG_CURRENT, &raw);
    if (err != ESP_OK) return err;
    *current_a = static_cast<int16_t>(raw) * dev->current_lsb;  // signed / æç¬¦å·å?
    return ESP_OK;
}

esp_err_t m5tab5_ina226_read_power(const m5tab5_ina226_t* dev, float* power_w)
{
    if (!dev || !dev->dev || !power_w) return ESP_ERR_INVALID_ARG;
    uint16_t raw  = 0;
    esp_err_t err = ina226_read_reg16(dev->dev, INA226_REG_POWER, &raw);
    if (err != ESP_OK) return err;
    *power_w = raw * (25.0f * dev->current_lsb);  // unsigned / æ ç¬¦å·å?
    return ESP_OK;
}

}  // namespace m5::tab5
