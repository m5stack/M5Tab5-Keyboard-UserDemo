/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 *
 * Self-contained driver for the PI4IOE5V6408 I/O expander.
 * PI4IOE5V6408 IO 扩展器的自包含驱动头文件�?
 * This header has no dependency on m5tab5_types.h or other
 * drivers. 该头文件不依�?m5tab5_types.h 或其他驱动�?
 * It can be used standalone or integrated via the
 * descriptor factory. 既可以单独使用，也可以通过描述符工厂集成到组件层�?
 */
#pragma once

#include <cstdint>

#include "esp_err.h"
#include "driver/gpio.h"
#include "i2c_bus.h"

namespace m5::tab5 {

// ── PI4IOE5V6408 Register Addresses / PI4IOE5V6408 寄存器地址 ──────────────
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

// ── Pin Enum / 引脚枚举
// ─────────────────────────────────────────────────────
// Self-contained and independent of m5tab5_pinmap.h.
// 该定义自包含且独立于 m5tab5_pinmap.h�?
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

// ── Pin Mode / 引脚模式
// ─────────────────────────────────────────────────────
enum M5TAB5_ExtIo_PI4IOE5V6408_PinMode : uint8_t {
    M5TAB5_EXTIO_PI4IOE5V6408_PIN_INPUT = 0,
    M5TAB5_EXTIO_PI4IOE5V6408_PIN_OUTPUT,
    M5TAB5_EXTIO_PI4IOE5V6408_PIN_INPUT_PULLUP,
    M5TAB5_EXTIO_PI4IOE5V6408_PIN_INPUT_PULLDOWN,
};

// ── Configuration / 配置结构
// ────────────────────────────────────────────────
// The caller provides an already-created I2C bus handle.
// 调用方需要传入已经创建好�?I2C 总线句柄�?
struct m5tab5_extio_pi4ioe5v6408_config_t {
    i2c_bus_handle_t i2c_bus_handle = nullptr;
    uint8_t i2c_addr                = 0x43;
    uint32_t scl_speed_hz           = 400000;
    gpio_num_t int_pin = GPIO_NUM_NC;  // GPIO_NUM_NC means no interrupt / GPIO_NUM_NC 表示不使用中�?
};

// ── Device Context /
// 设备上下�?─────────────────────────────────────────────
// Stack-allocatable, with no dynamic memory.
// 可在栈上分配，不使用动态内存�?
struct m5tab5_extio_pi4ioe5v6408_t {
    i2c_bus_device_handle_t i2c_dev = nullptr;
    gpio_num_t int_pin              = GPIO_NUM_NC;
    uint8_t i2c_addr                = 0;
};

// ── Driver API / 驱动接口
// ───────────────────────────────────────────────────

/// Initialize: add the device to the I2C bus, perform a soft reset, and verify the device ID.
/// 初始化：将设备加�?I2C 总线，执行软复位，并校验设备 ID�?
esp_err_t m5tab5_extio_pi4ioe5v6408_init(const m5tab5_extio_pi4ioe5v6408_config_t* config,
                                         m5tab5_extio_pi4ioe5v6408_t* dev);

/// Remove the device from the I2C bus and clear the context.
/// �?I2C 总线上移除设备，并清空上下文�?
void m5tab5_extio_pi4ioe5v6408_deinit(m5tab5_extio_pi4ioe5v6408_t* dev);

/// Configure a pin as input, output, or input with pull-up or pull-down.
/// 将引脚配置为输入、输出，或带上拉/下拉的输入模式�?
esp_err_t m5tab5_extio_pi4ioe5v6408_set_pin_mode(m5tab5_extio_pi4ioe5v6408_t* dev, M5TAB5_ExtIo_PI4IOE5V6408_Pin pin,
                                                 M5TAB5_ExtIo_PI4IOE5V6408_PinMode mode);

/// Write a HIGH or LOW level to a single output pin.
/// 向单个输出引脚写入高电平或低电平�?
esp_err_t m5tab5_extio_pi4ioe5v6408_write_pin(m5tab5_extio_pi4ioe5v6408_t* dev, M5TAB5_ExtIo_PI4IOE5V6408_Pin pin,
                                              bool level);

/// Read the state of a single input pin.
/// 读取单个输入引脚的状态�?
esp_err_t m5tab5_extio_pi4ioe5v6408_read_pin(m5tab5_extio_pi4ioe5v6408_t* dev, M5TAB5_ExtIo_PI4IOE5V6408_Pin pin,
                                             bool* level);

/// Read all eight input pin states as a bitmask (bit 0 = pin 0).
/// 以位掩码形式读取全部 8 个输入引脚状态（bit 0 对应 pin 0）�?
esp_err_t m5tab5_extio_pi4ioe5v6408_read_all_pins(m5tab5_extio_pi4ioe5v6408_t* dev, uint8_t* out_state);

/// Write all eight output pin states as a bitmask.
/// 以位掩码形式写入全部 8 个输出引脚状态�?
esp_err_t m5tab5_extio_pi4ioe5v6408_write_all_pins(m5tab5_extio_pi4ioe5v6408_t* dev, uint8_t state);

/// Perform a software reset on the chip (all registers return to default values).
/// 对芯片执行软件复位（所有寄存器恢复默认值）�?
esp_err_t m5tab5_extio_pi4ioe5v6408_soft_reset(m5tab5_extio_pi4ioe5v6408_t* dev);

/// Read the raw interrupt status register (it clears on read).
/// 读取原始中断状态寄存器（读取后会清除）�?
esp_err_t m5tab5_extio_pi4ioe5v6408_read_interrupt_status(m5tab5_extio_pi4ioe5v6408_t* dev, uint8_t* status);

/// Configure the interrupt mask for a pin: enable (unmask) or disable (mask) its interrupt.
/// 设置单个引脚的中断屏蔽：使能（取消屏蔽）或禁用（屏蔽）中断�?
esp_err_t m5tab5_extio_pi4ioe5v6408_set_interrupt_mask(m5tab5_extio_pi4ioe5v6408_t* dev,
                                                       M5TAB5_ExtIo_PI4IOE5V6408_Pin pin, bool enable);

// ── Component Integration /
// 组件层集�?──────────────────────────────────────
// Forward declared here, so m5tab5_types.h is not required.
// 此处仅做前向声明，因此无需包含 m5tab5_types.h�?
struct m5tab5_device_driver_descriptor_t;
const m5tab5_device_driver_descriptor_t* m5tab5_get_extio_pi4ioe5v6408_driver();

}  // namespace m5::tab5