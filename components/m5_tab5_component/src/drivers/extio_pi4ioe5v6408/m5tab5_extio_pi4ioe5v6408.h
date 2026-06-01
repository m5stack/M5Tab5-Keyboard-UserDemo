/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 *
 * Self-contained driver for the PI4IOE5V6408 I/O expander.
 * PI4IOE5V6408 IO æ©å±å¨çèªåå«é©±å¨å¤´æä»¶ã?
 * This header has no dependency on m5tab5_types.h or other
 * drivers. è¯¥å¤´æä»¶ä¸ä¾èµ?m5tab5_types.h æå¶ä»é©±å¨ã?
 * It can be used standalone or integrated via the
 * descriptor factory. æ¢å¯ä»¥åç¬ä½¿ç¨ï¼ä¹å¯ä»¥éè¿æè¿°ç¬¦å·¥åéæå°ç»ä»¶å±ã?
 */
#pragma once

#include <cstdint>

#include "esp_err.h"
#include "driver/gpio.h"
#include "i2c_bus.h"

namespace m5::tab5 {

// ââ PI4IOE5V6408 Register Addresses / PI4IOE5V6408 å¯å­å¨å°å ââââââââââââââ
constexpr uint8_t M5TAB5_PI4IO_REG_DEVICE_ID   = 0x01;
constexpr uint8_t M5TAB5_PI4IO_REG_IO_DIR      = 0x03;
constexpr uint8_t M5TAB5_PI4IO_REG_OUT_STATE   = 0x05;
constexpr uint8_t M5TAB5_PI4IO_REG_OUT_HI_Z    = 0x07;
constexpr uint8_t M5TAB5_PI4IO_REG_IN_DEFAULT  = 0x09;
constexpr uint8_t M5TAB5_PI4IO_REG_PULL_ENABLE = 0x0B;
constexpr uint8_t M5TAB5_PI4IO_REG_PULL_SELECT = 0x0D;
constexpr uint8_t M5TAB5_PI4IO_REG_IN_STATUS   = 0x0F;
constexpr uint8_t M5TAB5_PI4IO_REG_INT_MASK    = 0x11;
constexpr uint8_t M5TAB5_PI4IO_REG_INT_STATUS  = 0x13;

// ââ Pin Enum / å¼èæä¸¾
// âââââââââââââââââââââââââââââââââââââââââââââââââââââ
// Self-contained and independent of m5tab5_pinmap.h.
// è¯¥å®ä¹èªåå«ä¸ç¬ç«äº m5tab5_pinmap.hã?
enum M5TAB5_ExtIo_PI4IOE5V6408_Pin : uint8_t {
    M5TAB5_EXTIO_PI4IOE5V6408_PIN_0   = 0,
    M5TAB5_EXTIO_PI4IOE5V6408_PIN_1   = 1,
    M5TAB5_EXTIO_PI4IOE5V6408_PIN_2   = 2,
    M5TAB5_EXTIO_PI4IOE5V6408_PIN_3   = 3,
    M5TAB5_EXTIO_PI4IOE5V6408_PIN_4   = 4,
    M5TAB5_EXTIO_PI4IOE5V6408_PIN_5   = 5,
    M5TAB5_EXTIO_PI4IOE5V6408_PIN_6   = 6,
    M5TAB5_EXTIO_PI4IOE5V6408_PIN_7   = 7,
    M5TAB5_EXTIO_PI4IOE5V6408_PIN_MAX = 8,
};

// ââ Pin Mode / å¼èæ¨¡å¼
// âââââââââââââââââââââââââââââââââââââââââââââââââââââ
enum M5TAB5_ExtIo_PI4IOE5V6408_PinMode : uint8_t {
    M5TAB5_EXTIO_PI4IOE5V6408_PIN_INPUT = 0,
    M5TAB5_EXTIO_PI4IOE5V6408_PIN_OUTPUT,
    M5TAB5_EXTIO_PI4IOE5V6408_PIN_INPUT_PULLUP,
    M5TAB5_EXTIO_PI4IOE5V6408_PIN_INPUT_PULLDOWN,
};

// ââ Configuration / éç½®ç»æ
// ââââââââââââââââââââââââââââââââââââââââââââââââ
// The caller provides an already-created I2C bus handle.
// è°ç¨æ¹éè¦ä¼ å¥å·²ç»åå»ºå¥½ç?I2C æ»çº¿å¥æã?
struct m5tab5_extio_pi4ioe5v6408_config_t {
    i2c_bus_handle_t i2c_bus_handle = nullptr;
    uint8_t i2c_addr                = 0x43;
    uint32_t scl_speed_hz           = 400000;
    gpio_num_t int_pin = GPIO_NUM_NC;  // GPIO_NUM_NC means no interrupt / GPIO_NUM_NC è¡¨ç¤ºä¸ä½¿ç¨ä¸­æ?
};

// ââ Device Context /
// è®¾å¤ä¸ä¸æ?âââââââââââââââââââââââââââââââââââââââââââââ
// Stack-allocatable, with no dynamic memory.
// å¯å¨æ ä¸åéï¼ä¸ä½¿ç¨å¨æåå­ã?
struct m5tab5_extio_pi4ioe5v6408_t {
    i2c_bus_device_handle_t i2c_dev = nullptr;
    gpio_num_t int_pin              = GPIO_NUM_NC;
    uint8_t i2c_addr                = 0;
};

// ââ Driver API / é©±å¨æ¥å£
// âââââââââââââââââââââââââââââââââââââââââââââââââââ

/// Initialize: add the device to the I2C bus, perform a soft reset, and verify the device ID.
/// åå§åï¼å°è®¾å¤å å?I2C æ»çº¿ï¼æ§è¡è½¯å¤ä½ï¼å¹¶æ ¡éªè®¾å¤ IDã?
esp_err_t m5tab5_extio_pi4ioe5v6408_init(const m5tab5_extio_pi4ioe5v6408_config_t* config,
                                         m5tab5_extio_pi4ioe5v6408_t* dev);

/// Remove the device from the I2C bus and clear the context.
/// ä»?I2C æ»çº¿ä¸ç§»é¤è®¾å¤ï¼å¹¶æ¸ç©ºä¸ä¸æã?
void m5tab5_extio_pi4ioe5v6408_deinit(m5tab5_extio_pi4ioe5v6408_t* dev);

/// Configure a pin as input, output, or input with pull-up or pull-down.
/// å°å¼èéç½®ä¸ºè¾å¥ãè¾åºï¼æå¸¦ä¸æ/ä¸æçè¾å¥æ¨¡å¼ã?
esp_err_t m5tab5_extio_pi4ioe5v6408_set_pin_mode(m5tab5_extio_pi4ioe5v6408_t* dev, M5TAB5_ExtIo_PI4IOE5V6408_Pin pin,
                                                 M5TAB5_ExtIo_PI4IOE5V6408_PinMode mode);

/// Write a HIGH or LOW level to a single output pin.
/// ååä¸ªè¾åºå¼èåå¥é«çµå¹³æä½çµå¹³ã?
esp_err_t m5tab5_extio_pi4ioe5v6408_write_pin(m5tab5_extio_pi4ioe5v6408_t* dev, M5TAB5_ExtIo_PI4IOE5V6408_Pin pin,
                                              bool level);

/// Read the state of a single input pin.
/// è¯»ååä¸ªè¾å¥å¼èçç¶æã?
esp_err_t m5tab5_extio_pi4ioe5v6408_read_pin(m5tab5_extio_pi4ioe5v6408_t* dev, M5TAB5_ExtIo_PI4IOE5V6408_Pin pin,
                                             bool* level);

/// Read all eight input pin states as a bitmask (bit 0 = pin 0).
/// ä»¥ä½æ©ç å½¢å¼è¯»åå¨é¨ 8 ä¸ªè¾å¥å¼èç¶æï¼bit 0 å¯¹åº pin 0ï¼ã?
esp_err_t m5tab5_extio_pi4ioe5v6408_read_all_pins(m5tab5_extio_pi4ioe5v6408_t* dev, uint8_t* out_state);

/// Write all eight output pin states as a bitmask.
/// ä»¥ä½æ©ç å½¢å¼åå¥å¨é¨ 8 ä¸ªè¾åºå¼èç¶æã?
esp_err_t m5tab5_extio_pi4ioe5v6408_write_all_pins(m5tab5_extio_pi4ioe5v6408_t* dev, uint8_t state);

/// Perform a software reset on the chip (all registers return to default values).
/// å¯¹è¯çæ§è¡è½¯ä»¶å¤ä½ï¼ææå¯å­å¨æ¢å¤é»è®¤å¼ï¼ã?
esp_err_t m5tab5_extio_pi4ioe5v6408_soft_reset(m5tab5_extio_pi4ioe5v6408_t* dev);

/// Read the raw interrupt status register (it clears on read).
/// è¯»ååå§ä¸­æ­ç¶æå¯å­å¨ï¼è¯»ååä¼æ¸é¤ï¼ã?
esp_err_t m5tab5_extio_pi4ioe5v6408_read_interrupt_status(m5tab5_extio_pi4ioe5v6408_t* dev, uint8_t* status);

/// Configure the interrupt mask for a pin: enable (unmask) or disable (mask) its interrupt.
/// è®¾ç½®åä¸ªå¼èçä¸­æ­å±è½ï¼ä½¿è½ï¼åæ¶å±è½ï¼æç¦ç¨ï¼å±è½ï¼ä¸­æ­ã?
esp_err_t m5tab5_extio_pi4ioe5v6408_set_interrupt_mask(m5tab5_extio_pi4ioe5v6408_t* dev,
                                                       M5TAB5_ExtIo_PI4IOE5V6408_Pin pin, bool enable);

// ââ Component Integration /
// ç»ä»¶å±éæ?ââââââââââââââââââââââââââââââââââââââ
// Forward declared here, so m5tab5_types.h is not required.
// æ­¤å¤ä»åååå£°æï¼å æ­¤æ éåå« m5tab5_types.hã?
struct m5tab5_device_driver_descriptor_t;
const m5tab5_device_driver_descriptor_t* m5tab5_get_extio_pi4ioe5v6408_driver();

}  // namespace m5::tab5