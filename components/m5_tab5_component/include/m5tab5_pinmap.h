/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "driver/gpio.h"

namespace m5::tab5 {

enum : int {
    M5TAB5_I2C_PORT_SYS   = 0,
    M5TAB5_I2C_PORT_EXT   = 1,
    M5TAB5_I2C_PORT_GROVE = 1,
};

enum : int {
    M5TAB5_EXTIO_ADDR_LOW  = 0x43,
    M5TAB5_EXTIO_ADDR_HIGH = 0x44,
};

enum M5TAB5_ExtIoPin : int {
    M5TAB5_EXTIO_PIN_0 = 0,
    M5TAB5_EXTIO_PIN_1 = 1,
    M5TAB5_EXTIO_PIN_2 = 2,
    M5TAB5_EXTIO_PIN_3 = 3,
    M5TAB5_EXTIO_PIN_4 = 4,
    M5TAB5_EXTIO_PIN_5 = 5,
    M5TAB5_EXTIO_PIN_6 = 6,
    M5TAB5_EXTIO_PIN_7 = 7,
};

enum M5TAB5_ExtIoAddrLowPin : int {
    M5TAB5_EXTIO_ADDR_LOW_RF_PTH_L_INT_H_EXT = M5TAB5_EXTIO_PIN_0,
    M5TAB5_EXTIO_ADDR_LOW_SPK_EN             = M5TAB5_EXTIO_PIN_1,
    M5TAB5_EXTIO_ADDR_LOW_EXT5V_EN           = M5TAB5_EXTIO_PIN_2,
    M5TAB5_EXTIO_ADDR_LOW_LCD_RST            = M5TAB5_EXTIO_PIN_4,
    M5TAB5_EXTIO_ADDR_LOW_TP_RST             = M5TAB5_EXTIO_PIN_5,
    M5TAB5_EXTIO_ADDR_LOW_CAM_RST            = M5TAB5_EXTIO_PIN_6,
    M5TAB5_EXTIO_ADDR_LOW_HP_DET             = M5TAB5_EXTIO_PIN_7,
};

enum M5TAB5_ExtIoAddrHighPin : int {
    M5TAB5_EXTIO_ADDR_HIGH_WLAN_PWR_EN  = M5TAB5_EXTIO_PIN_0,
    M5TAB5_EXTIO_ADDR_HIGH_USB5V_EN     = M5TAB5_EXTIO_PIN_3,
    M5TAB5_EXTIO_ADDR_HIGH_PWROFF_PLUSE = M5TAB5_EXTIO_PIN_4,
    M5TAB5_EXTIO_ADDR_HIGH_NCHG_QC_EN   = M5TAB5_EXTIO_PIN_5,
    M5TAB5_EXTIO_ADDR_HIGH_CHG_STAT     = M5TAB5_EXTIO_PIN_6,
    M5TAB5_EXTIO_ADDR_HIGH_CHG_EN       = M5TAB5_EXTIO_PIN_7,
};

constexpr gpio_num_t M5TAB5_PIN_SYS_I2C_SCL = GPIO_NUM_32;
constexpr gpio_num_t M5TAB5_PIN_SYS_I2C_SDA = GPIO_NUM_31;

constexpr gpio_num_t M5TAB5_PIN_EXT_I2C_SCL = GPIO_NUM_1;
constexpr gpio_num_t M5TAB5_PIN_EXT_I2C_SDA = GPIO_NUM_0;

constexpr gpio_num_t M5TAB5_PIN_GROVE_I2C_SCL = GPIO_NUM_54;
constexpr gpio_num_t M5TAB5_PIN_GROVE_I2C_SDA = GPIO_NUM_53;

constexpr gpio_num_t M5TAB5_PIN_KEYBOARD_INT = GPIO_NUM_50;

constexpr gpio_num_t M5TAB5_PIN_I2S_SCLK  = GPIO_NUM_27;
constexpr gpio_num_t M5TAB5_PIN_I2S_MCLK  = GPIO_NUM_30;
constexpr gpio_num_t M5TAB5_PIN_I2S_LCLK  = GPIO_NUM_29;
constexpr gpio_num_t M5TAB5_PIN_I2S_DOUT  = GPIO_NUM_26;
constexpr gpio_num_t M5TAB5_PIN_I2S_DSIN  = GPIO_NUM_28;
constexpr gpio_num_t M5TAB5_PIN_POWER_AMP = GPIO_NUM_NC;

constexpr gpio_num_t M5TAB5_PIN_LCD_BACKLIGHT = GPIO_NUM_22;
constexpr gpio_num_t M5TAB5_PIN_LCD_RST       = GPIO_NUM_NC;
constexpr gpio_num_t M5TAB5_PIN_LCD_TOUCH_RST = GPIO_NUM_NC;  // TP_RST controlled via IO expander (ADDR_LOW P5) /
                                                              // TP_RST �?IO 扩展器控制（ADDR_LOW P5�?
constexpr gpio_num_t M5TAB5_PIN_LCD_TOUCH_INT = GPIO_NUM_23;  // TP_INT direct GPIO / TP_INT 直接连接�?GPIO

constexpr gpio_num_t M5TAB5_PIN_SDMMC_DET = GPIO_NUM_NC;
constexpr gpio_num_t M5TAB5_PIN_SDMMC_CLK = GPIO_NUM_43;
constexpr gpio_num_t M5TAB5_PIN_SDMMC_CMD = GPIO_NUM_44;
constexpr gpio_num_t M5TAB5_PIN_SDMMC_D0  = GPIO_NUM_39;
constexpr gpio_num_t M5TAB5_PIN_SDMMC_D1  = GPIO_NUM_40;
constexpr gpio_num_t M5TAB5_PIN_SDMMC_D2  = GPIO_NUM_41;
constexpr gpio_num_t M5TAB5_PIN_SDMMC_D3  = GPIO_NUM_42;

// ── ESP32-C6 Module GPIO Assignments / ESP32-C6 模组 GPIO 分配
// ────────────── Physical connector pins 8-16;
// left = P4 signal, right = C6 GPIO.
// 物理连接器的 8 �?16 脚位，左侧为 P4 信号，右侧为 C6
// GPIO�?
//   pin 8  SDIO2_D3        �?C6 GPIO8
//   8 �? SDIO2_D3         �?C6 GPIO8
//   pin 9  VDDPST          �?power (no GPIO)
//   9 �? VDDPST           �?电源（无 GPIO�?
//   pin 10 SDIO2_D2        �?C6 GPIO9
//   10 �?SDIO2_D2         �?C6 GPIO9
//   pin 11 SDIO2_D1        �?C6 GPIO10
//   11 �?SDIO2_D1         �?C6 GPIO10
//   pin 12 SDIO2_D0        �?C6 GPIO11
//   12 �?SDIO2_D0         �?C6 GPIO11
//   pin 13 SDIO2_CLK       �?C6 GPIO12
//   13 �?SDIO2_CLK        �?C6 GPIO12
//   pin 14 SDIO2_CMD       �?C6 GPIO13
//   14 �?SDIO2_CMD        �?C6 GPIO13
//   pin 15 RF_C6_IO2       �?C6 GPIO14 (C6 internal RF path control, not driven from P4)
//   15 �?RF_C6_IO2        �?C6 GPIO14（C6 内部射频路径控制，不�?P4
//   驱动�? pin 16 SOC_EXTRF_RST   �?C6 GPIO15 (reset input on C6) 16 �?SOC_EXTRF_RST    �?C6
//   GPIO15（C6 复位输入�?
constexpr int M5TAB5_C6_GPIO_SDIO_D3  = 8;
constexpr int M5TAB5_C6_GPIO_SDIO_D2  = 9;
constexpr int M5TAB5_C6_GPIO_SDIO_D1  = 10;
constexpr int M5TAB5_C6_GPIO_SDIO_D0  = 11;
constexpr int M5TAB5_C6_GPIO_SDIO_CLK = 12;
constexpr int M5TAB5_C6_GPIO_SDIO_CMD = 13;
constexpr int M5TAB5_C6_GPIO_RF_CTL   = 14;  ///< C6 internal RF path (RF_C6_IO2)
constexpr int M5TAB5_C6_GPIO_RST      = 15;  ///< SOC_EXTRF_RST �?reset input on C6

// P4 GPIO48 is not physically connected to C6 RST; the C6 is power-cycled via the IO expander
// through WLAN_PWR_EN (ADDR_HIGH 0x44 P0).
// P4 GPIO48 实际并未连接�?C6 �?RST；C6 通过 IO 扩展器的 WLAN_PWR_EN（ADDR_HIGH 0x44
// P0）断电重启�? GPIO48 is assigned as a dummy value to satisfy esp-hosted's internal assert and
// is toggled only if SDIO card_init fails unexpectedly.
// GPIO48 仅作为占位值用于满�?esp-hosted 的内部断言，只有在 SDIO card_init
// 异常失败时才会被翻转�?
constexpr gpio_num_t M5TAB5_P4_GPIO_C6_RST_DUMMY = GPIO_NUM_48;

// ── RF Antenna Selection / 射频天线选择
// ──────────────────────────────────────
// Controlled via IO expander ADDR_LOW 0x43, P0.
// 通过 IO 扩展�?ADDR_LOW 0x43 �?P0 控制�?
// M5TAB5_EXTIO_ADDR_LOW_RF_PTH_L_INT_H_EXT: LOW = internal, HIGH = external.
// M5TAB5_EXTIO_ADDR_LOW_RF_PTH_L_INT_H_EXT：LOW 表示内置天线，HIGH
// 表示外置天线�?
enum m5tab5_rf_antenna_t : int {
    M5TAB5_RF_ANTENNA_INTERNAL = 0,  ///< Built-in PCB antenna (IO expander P0 = LOW) / 内置 PCB 天线（IO
                                     ///< 扩展�?P0 = LOW�?
    M5TAB5_RF_ANTENNA_EXTERNAL = 1,  ///< External U.FL / SMA antenna (IO expander P0 = HIGH) / 外置 U.FL / SMA
                                     ///< 天线（IO 扩展�?P0 = HIGH�?
};

// ── M-Bus 30-Pin Expansion Connector / M-Bus 30 针扩展连接器
// ──────────────── Physical pin layout
// (two rows, edge connector):
// 物理引脚布局为边缘连接器双排形式�?
//   odd  = 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29
//   奇数�?= 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29
//   even = 2, 4, 6, 8,10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30
//   偶数�?= 2, 4, 6, 8,10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30
//
// Pins 1 and 3-6 are power or ground rails, so they have no GPIO number (GPIO_NUM_NC).
// 1 脚与 3 �?6 脚为电源或地线，因此没有对应 GPIO
// 编号（GPIO_NUM_NC）�? Derivation: m5::M5Unified::_pin_table_mbus[], board_M5Tab5 row in
// M5Unified.cpp. 来源：m5::M5Unified::_pin_table_mbus[]，位�?M5Unified.cpp �?board_M5Tab5
// 行�?
//
// ┌─────┬────────────┬───────────────────────────────────────────────────────�?
// �?Pin �? ESP32-P4  �?Function                                              �?
// �?引脚 �? ESP32-P4  �?功能                                                  �?
// ├─────┼────────────┼───────────────────────────────────────────────────────�?
// �? 1  �? GND       �?Ground (power rail)                                   �?
// �? 1  �? GND       �?地线（电源轨�?                                       �?
// �? 2  �? GPIO16    �?General-purpose IO                                    �?
// �? 2  �? GPIO16    �?通用输入输出                                          �?
// �? 3  �? GND       �?Ground (power rail)                                   �?
// �? 3  �? GND       �?地线（电源轨�?                                       �?
// �? 4  �? GPIO17    �?General-purpose IO                                    �?
// �? 4  �? GPIO17    �?通用输入输出                                          �?
// �? 5  �? 3.3 V     �?3.3 V supply output (power rail)                      �?
// �? 5  �? 3.3 V     �?3.3 V 电源输出（电源轨�? �? �? 6  �? GND
// �?Ground (power rail)                                   �?
// �? 6  �? GND       �?地线（电源轨�?                                       �?
// �? 7  �? GPIO18    �?SPI MOSI                                              �?
// �? 7  �? GPIO18    �?SPI MOSI                                              �?
// �? 8  �? GPIO45    �?SPI CS                                                �?
// �? 8  �? GPIO45    �?SPI CS                                                �?
// �? 9  �? GPIO19    �?SPI MISO                                              �?
// �? 9  �? GPIO19    �?SPI MISO                                              �?
// �?10  �? GPIO52    �?SPI CLK                                               �?
// �?10  �? GPIO52    �?SPI CLK                                               �?
// �?11  �? GPIO5     �?SPI DC / general IO                                   �?
// �?11  �? GPIO5     �?SPI DC / 通用输入输出                                 �?
// �?12  �? NC        �?Not connected                                         �?
// �?12  �? NC        �?未连�?                                               �?
// �?13  �? GPIO38    �?UART0 RXD                                             �?
// �?13  �? GPIO38    �?UART0 RXD                                             �?
// �?14  �? GPIO37    �?UART0 TXD                                             �?
// �?14  �? GPIO37    �?UART0 TXD                                             �?
// �?15  �? GPIO7     �?UART1 RXD                                             �?
// �?15  �? GPIO7     �?UART1 RXD                                             �?
// �?16  �? GPIO6     �?UART1 TXD                                             �?
// �?16  �? GPIO6     �?UART1 TXD                                             �?
// �?17  �? GPIO31    �?SYS I2C SDA (shared with on-board peripherals)        �?
// �?17  �? GPIO31    �?SYS I2C SDA（与板载外设共用�? �? �?18  �?
// GPIO32    �?SYS I2C SCL (shared with on-board peripherals)        �? �?18  �? GPIO32    �?SYS I2C
// SCL（与板载外设共用�?                         �? �?19  �? GPIO3
// �?General-purpose IO                                    �?
// �?19  �? GPIO3     �?通用输入输出                                          �?
// �?20  �? GPIO4     �?General-purpose IO                                    �?
// �?20  �? GPIO4     �?通用输入输出                                          �?
// �?21  �? GPIO2     �?I2S MCLK (M-Bus audio path)                           �?
// �?21  �? GPIO2     �?I2S MCLK（M-Bus 音频通路�?                           �?
// �?22  �? GPIO48    �?I2S BCK / SCLK (M-Bus audio path)                     �?
// �?22  �? GPIO48    �?I2S BCK / SCLK（M-Bus 音频通路�?                     �?
// �?23  �? GPIO47    �?I2S DOUT / DATA (M-Bus audio path)                    �?
// �?23  �? GPIO47    �?I2S DOUT / DATA（M-Bus 音频通路�?                    �?
// �?24  �? GPIO35    �?I2S WS / LRCK (M-Bus audio path)                      �?
// �?24  �? GPIO35    �?I2S WS / LRCK（M-Bus 音频通路�?                      �?
// �?25  �? NC        �?Not connected                                         �?
// �?25  �? NC        �?未连�?                                               �?
// �?26  �? GPIO51    �?General-purpose IO / ADC                              �?
// �?26  �? GPIO51    �?通用输入输出 / ADC                                    �?
// �?27  �? NC        �?Not connected                                         �?
// �?27  �? NC        �?未连�?                                               �?
// �?28  �? NC        �?Not connected                                         �?
// �?28  �? NC        �?未连�?                                               �?
// �?29  �? NC        �?Not connected                                         �?
// �?29  �? NC        �?未连�?                                               �?
// �?30  �? NC        �?Not connected                                         �?
// �?30  �? NC        �?未连�?                                               �?
// └─────┴────────────┴───────────────────────────────────────────────────────�?

constexpr gpio_num_t M5TAB5_MBUS_PIN2  = GPIO_NUM_16;
constexpr gpio_num_t M5TAB5_MBUS_PIN4  = GPIO_NUM_17;
constexpr gpio_num_t M5TAB5_MBUS_PIN7  = GPIO_NUM_18;  ///< SPI MOSI / SPI MOSI
constexpr gpio_num_t M5TAB5_MBUS_PIN8  = GPIO_NUM_45;  ///< SPI CS / SPI CS
constexpr gpio_num_t M5TAB5_MBUS_PIN9  = GPIO_NUM_19;  ///< SPI MISO / SPI MISO
constexpr gpio_num_t M5TAB5_MBUS_PIN10 = GPIO_NUM_52;  ///< SPI CLK / SPI CLK
constexpr gpio_num_t M5TAB5_MBUS_PIN11 = GPIO_NUM_5;   ///< SPI DC or GPIO / SPI DC �?GPIO
constexpr gpio_num_t M5TAB5_MBUS_PIN13 = GPIO_NUM_38;  ///< UART0 RXD / UART0 接收
constexpr gpio_num_t M5TAB5_MBUS_PIN14 = GPIO_NUM_37;  ///< UART0 TXD / UART0 发�?
constexpr gpio_num_t M5TAB5_MBUS_PIN15 = GPIO_NUM_7;   ///< UART1 RXD / UART1 接收
constexpr gpio_num_t M5TAB5_MBUS_PIN16 = GPIO_NUM_6;   ///< UART1 TXD / UART1 发�?
constexpr gpio_num_t M5TAB5_MBUS_PIN17 =
    GPIO_NUM_31;  ///< SYS I2C SDA (= M5TAB5_PIN_SYS_I2C_SDA) / SYS I2C SDA（即 M5TAB5_PIN_SYS_I2C_SDA�?
constexpr gpio_num_t M5TAB5_MBUS_PIN18 =
    GPIO_NUM_32;  ///< SYS I2C SCL (= M5TAB5_PIN_SYS_I2C_SCL) / SYS I2C SCL（即 M5TAB5_PIN_SYS_I2C_SCL�?
constexpr gpio_num_t M5TAB5_MBUS_PIN19 = GPIO_NUM_3;
constexpr gpio_num_t M5TAB5_MBUS_PIN20 = GPIO_NUM_4;
constexpr gpio_num_t M5TAB5_MBUS_PIN21 = GPIO_NUM_2;   ///< I2S MCLK (M-Bus audio) / I2S MCLK（M-Bus 音频�?
constexpr gpio_num_t M5TAB5_MBUS_PIN22 = GPIO_NUM_48;  ///< I2S BCK (M-Bus audio) / I2S BCK（M-Bus 音频�?
constexpr gpio_num_t M5TAB5_MBUS_PIN23 = GPIO_NUM_47;  ///< I2S DOUT (M-Bus audio) / I2S DOUT（M-Bus 音频�?
constexpr gpio_num_t M5TAB5_MBUS_PIN24 = GPIO_NUM_35;  ///< I2S WS (M-Bus audio) / I2S WS（M-Bus 音频�?
constexpr gpio_num_t M5TAB5_MBUS_PIN26 = GPIO_NUM_51;

// Aliases matching M5Stack M-Bus functional names / �?M5Stack M-Bus
// 功能名称一致的别名
constexpr gpio_num_t M5TAB5_MBUS_SPI_MOSI = M5TAB5_MBUS_PIN7;
constexpr gpio_num_t M5TAB5_MBUS_SPI_CS   = M5TAB5_MBUS_PIN8;
constexpr gpio_num_t M5TAB5_MBUS_SPI_MISO = M5TAB5_MBUS_PIN9;
constexpr gpio_num_t M5TAB5_MBUS_SPI_CLK  = M5TAB5_MBUS_PIN10;
constexpr gpio_num_t M5TAB5_MBUS_SPI_DC   = M5TAB5_MBUS_PIN11;
constexpr gpio_num_t M5TAB5_MBUS_UART0_RX = M5TAB5_MBUS_PIN13;
constexpr gpio_num_t M5TAB5_MBUS_UART0_TX = M5TAB5_MBUS_PIN14;
constexpr gpio_num_t M5TAB5_MBUS_UART1_RX = M5TAB5_MBUS_PIN15;
constexpr gpio_num_t M5TAB5_MBUS_UART1_TX = M5TAB5_MBUS_PIN16;
constexpr gpio_num_t M5TAB5_MBUS_I2C_SDA  = M5TAB5_MBUS_PIN17;  ///< Shared with SYS I2C / �?SYS I2C 共用
constexpr gpio_num_t M5TAB5_MBUS_I2C_SCL  = M5TAB5_MBUS_PIN18;  ///< Shared with SYS I2C / �?SYS I2C 共用
constexpr gpio_num_t M5TAB5_MBUS_I2S_MCLK = M5TAB5_MBUS_PIN21;
constexpr gpio_num_t M5TAB5_MBUS_I2S_BCK  = M5TAB5_MBUS_PIN22;
constexpr gpio_num_t M5TAB5_MBUS_I2S_DOUT = M5TAB5_MBUS_PIN23;
constexpr gpio_num_t M5TAB5_MBUS_I2S_WS   = M5TAB5_MBUS_PIN24;

}  // namespace m5::tab5
