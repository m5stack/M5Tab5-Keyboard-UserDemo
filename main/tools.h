/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef TOOLS_H
#define TOOLS_H

#include <esp_err.h>

#ifdef __cplusplus
namespace m5::tab5 {
class m5tab5_component;
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 运行键盘 I2C 地址寄存器测试
 *
 * 测试 M5_TAB5_KB_REG_I2C_ADDR 寄存器功能，
 * 最终将寄存器配置回默认地址 0x6D
 */
void run_keyboard_i2c_address_test(void);

#ifdef __cplusplus
}

esp_err_t start_battery_charge_monitor(m5::tab5::m5tab5_component* board);
#endif

#endif  // TOOLS_H
