/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "driver/gpio.h"

namespace m5::tab5 {

enum : int {
	M5TAB5_I2C_PORT_SYS = 0,
	M5TAB5_I2C_PORT_EXT = 1,
	M5TAB5_I2C_PORT_GROVE = 1,
};

enum : int {
	M5TAB5_EXTIO_ADDR_LOW = 0x43,
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
	M5TAB5_EXTIO_ADDR_LOW_SPK_EN = M5TAB5_EXTIO_PIN_1,
	M5TAB5_EXTIO_ADDR_LOW_EXT5V_EN = M5TAB5_EXTIO_PIN_2,
	M5TAB5_EXTIO_ADDR_LOW_LCD_RST = M5TAB5_EXTIO_PIN_4,
	M5TAB5_EXTIO_ADDR_LOW_TP_RST = M5TAB5_EXTIO_PIN_5,
	M5TAB5_EXTIO_ADDR_LOW_CAM_RST = M5TAB5_EXTIO_PIN_6,
	M5TAB5_EXTIO_ADDR_LOW_HP_DET = M5TAB5_EXTIO_PIN_7,
};

enum M5TAB5_ExtIoAddrHighPin : int {
	M5TAB5_EXTIO_ADDR_HIGH_WLAN_PWR_EN = M5TAB5_EXTIO_PIN_0,
	M5TAB5_EXTIO_ADDR_HIGH_USB5V_EN = M5TAB5_EXTIO_PIN_3,
	M5TAB5_EXTIO_ADDR_HIGH_PWROFF_PLUSE = M5TAB5_EXTIO_PIN_4,
	M5TAB5_EXTIO_ADDR_HIGH_NCHG_QC_EN = M5TAB5_EXTIO_PIN_5,
	M5TAB5_EXTIO_ADDR_HIGH_CHG_STAT = M5TAB5_EXTIO_PIN_6,
	M5TAB5_EXTIO_ADDR_HIGH_CHG_EN = M5TAB5_EXTIO_PIN_7,
};

constexpr gpio_num_t M5TAB5_PIN_SYS_I2C_SCL = GPIO_NUM_32;
constexpr gpio_num_t M5TAB5_PIN_SYS_I2C_SDA = GPIO_NUM_31;

constexpr gpio_num_t M5TAB5_PIN_EXT_I2C_SCL = GPIO_NUM_1;
constexpr gpio_num_t M5TAB5_PIN_EXT_I2C_SDA = GPIO_NUM_0;

constexpr gpio_num_t M5TAB5_PIN_GROVE_I2C_SCL = GPIO_NUM_54;
constexpr gpio_num_t M5TAB5_PIN_GROVE_I2C_SDA = GPIO_NUM_53;

constexpr gpio_num_t M5TAB5_PIN_KEYBOARD_INT = GPIO_NUM_50;

constexpr gpio_num_t M5TAB5_PIN_I2S_SCLK = GPIO_NUM_27;
constexpr gpio_num_t M5TAB5_PIN_I2S_MCLK = GPIO_NUM_30;
constexpr gpio_num_t M5TAB5_PIN_I2S_LCLK = GPIO_NUM_29;
constexpr gpio_num_t M5TAB5_PIN_I2S_DOUT = GPIO_NUM_26;
constexpr gpio_num_t M5TAB5_PIN_I2S_DSIN = GPIO_NUM_28;
constexpr gpio_num_t M5TAB5_PIN_POWER_AMP = GPIO_NUM_NC;

constexpr gpio_num_t M5TAB5_PIN_LCD_BACKLIGHT = GPIO_NUM_22;
constexpr gpio_num_t M5TAB5_PIN_LCD_RST = GPIO_NUM_NC;
constexpr gpio_num_t M5TAB5_PIN_LCD_TOUCH_RST = GPIO_NUM_NC;   // TP_RST controlled via IO expander (ADDR_LOW P5) / TP_RST ç?IO æ©å±å¨æ§å¶ï¼ADDR_LOW P5ï¼?
constexpr gpio_num_t M5TAB5_PIN_LCD_TOUCH_INT = GPIO_NUM_23;   // TP_INT direct GPIO / TP_INT ç´æ¥è¿æ¥å?GPIO

constexpr gpio_num_t M5TAB5_PIN_SDMMC_DET = GPIO_NUM_NC;
constexpr gpio_num_t M5TAB5_PIN_SDMMC_CLK = GPIO_NUM_43;
constexpr gpio_num_t M5TAB5_PIN_SDMMC_CMD = GPIO_NUM_44;
constexpr gpio_num_t M5TAB5_PIN_SDMMC_D0 = GPIO_NUM_39;
constexpr gpio_num_t M5TAB5_PIN_SDMMC_D1 = GPIO_NUM_40;
constexpr gpio_num_t M5TAB5_PIN_SDMMC_D2 = GPIO_NUM_41;
constexpr gpio_num_t M5TAB5_PIN_SDMMC_D3 = GPIO_NUM_42;

// ââ ESP32-C6 Module GPIO Assignments / ESP32-C6 æ¨¡ç» GPIO åé ââââââââââââââ
// Physical connector pins 8-16; left = P4 signal, right = C6 GPIO.
// ç©çè¿æ¥å¨ç 8 å?16 èä½ï¼å·¦ä¾§ä¸º P4 ä¿¡å·ï¼å³ä¾§ä¸º C6 GPIOã?
//   pin 8  SDIO2_D3        â?C6 GPIO8
//   8 è? SDIO2_D3         â?C6 GPIO8
//   pin 9  VDDPST          â?power (no GPIO)
//   9 è? VDDPST           â?çµæºï¼æ  GPIOï¼?
//   pin 10 SDIO2_D2        â?C6 GPIO9
//   10 è?SDIO2_D2         â?C6 GPIO9
//   pin 11 SDIO2_D1        â?C6 GPIO10
//   11 è?SDIO2_D1         â?C6 GPIO10
//   pin 12 SDIO2_D0        â?C6 GPIO11
//   12 è?SDIO2_D0         â?C6 GPIO11
//   pin 13 SDIO2_CLK       â?C6 GPIO12
//   13 è?SDIO2_CLK        â?C6 GPIO12
//   pin 14 SDIO2_CMD       â?C6 GPIO13
//   14 è?SDIO2_CMD        â?C6 GPIO13
//   pin 15 RF_C6_IO2       â?C6 GPIO14 (C6 internal RF path control, not driven from P4)
//   15 è?RF_C6_IO2        â?C6 GPIO14ï¼C6 åé¨å°é¢è·¯å¾æ§å¶ï¼ä¸ç?P4 é©±å¨ï¼?
//   pin 16 SOC_EXTRF_RST   â?C6 GPIO15 (reset input on C6)
//   16 è?SOC_EXTRF_RST    â?C6 GPIO15ï¼C6 å¤ä½è¾å¥ï¼?
constexpr int M5TAB5_C6_GPIO_SDIO_D3  = 8;
constexpr int M5TAB5_C6_GPIO_SDIO_D2  = 9;
constexpr int M5TAB5_C6_GPIO_SDIO_D1  = 10;
constexpr int M5TAB5_C6_GPIO_SDIO_D0  = 11;
constexpr int M5TAB5_C6_GPIO_SDIO_CLK = 12;
constexpr int M5TAB5_C6_GPIO_SDIO_CMD = 13;
constexpr int M5TAB5_C6_GPIO_RF_CTL   = 14;  ///< C6 internal RF path (RF_C6_IO2)
constexpr int M5TAB5_C6_GPIO_RST      = 15;  ///< SOC_EXTRF_RST â?reset input on C6

// P4 GPIO48 is not physically connected to C6 RST; the C6 is power-cycled via the IO expander
// through WLAN_PWR_EN (ADDR_HIGH 0x44 P0).
// P4 GPIO48 å®éå¹¶æªè¿æ¥å?C6 ç?RSTï¼C6 éè¿ IO æ©å±å¨ç WLAN_PWR_ENï¼ADDR_HIGH 0x44 P0ï¼æ­çµéå¯ã?
// GPIO48 is assigned as a dummy value to satisfy esp-hosted's internal assert and is toggled
// only if SDIO card_init fails unexpectedly.
// GPIO48 ä»ä½ä¸ºå ä½å¼ç¨äºæ»¡è¶?esp-hosted çåé¨æ­è¨ï¼åªæå¨ SDIO card_init å¼å¸¸å¤±è´¥æ¶æä¼è¢«ç¿»è½¬ã?
constexpr gpio_num_t M5TAB5_P4_GPIO_C6_RST_DUMMY = GPIO_NUM_48;

// ââ RF Antenna Selection / å°é¢å¤©çº¿éæ© ââââââââââââââââââââââââââââââââââââââ
// Controlled via IO expander ADDR_LOW 0x43, P0.
// éè¿ IO æ©å±å?ADDR_LOW 0x43 ç?P0 æ§å¶ã?
// M5TAB5_EXTIO_ADDR_LOW_RF_PTH_L_INT_H_EXT: LOW = internal, HIGH = external.
// M5TAB5_EXTIO_ADDR_LOW_RF_PTH_L_INT_H_EXTï¼LOW è¡¨ç¤ºåç½®å¤©çº¿ï¼HIGH è¡¨ç¤ºå¤ç½®å¤©çº¿ã?
enum m5tab5_rf_antenna_t : int {
	M5TAB5_RF_ANTENNA_INTERNAL = 0,  ///< Built-in PCB antenna (IO expander P0 = LOW) / åç½® PCB å¤©çº¿ï¼IO æ©å±å?P0 = LOWï¼?
	M5TAB5_RF_ANTENNA_EXTERNAL = 1,  ///< External U.FL / SMA antenna (IO expander P0 = HIGH) / å¤ç½® U.FL / SMA å¤©çº¿ï¼IO æ©å±å?P0 = HIGHï¼?
};

// ââ M-Bus 30-Pin Expansion Connector / M-Bus 30 éæ©å±è¿æ¥å¨ ââââââââââââââââ
// Physical pin layout (two rows, edge connector):
// ç©çå¼èå¸å±ä¸ºè¾¹ç¼è¿æ¥å¨åæå½¢å¼ï¼?
//   odd  = 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29
//   å¥æ°è?= 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29
//   even = 2, 4, 6, 8,10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30
//   å¶æ°è?= 2, 4, 6, 8,10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30
//
// Pins 1 and 3-6 are power or ground rails, so they have no GPIO number (GPIO_NUM_NC).
// 1 èä¸ 3 å?6 èä¸ºçµæºæå°çº¿ï¼å æ­¤æ²¡æå¯¹åº GPIO ç¼å·ï¼GPIO_NUM_NCï¼ã?
// Derivation: m5::M5Unified::_pin_table_mbus[], board_M5Tab5 row in M5Unified.cpp.
// æ¥æºï¼m5::M5Unified::_pin_table_mbus[]ï¼ä½äº?M5Unified.cpp ç?board_M5Tab5 è¡ã?
//
// âââââââ¬âââââââââââââ¬ââââââââââââââââââââââââââââââââââââââââââââââââââââââââ?
// â?Pin â? ESP32-P4  â?Function                                              â?
// â?å¼è â? ESP32-P4  â?åè½                                                  â?
// âââââââ¼âââââââââââââ¼ââââââââââââââââââââââââââââââââââââââââââââââââââââââââ?
// â? 1  â? GND       â?Ground (power rail)                                   â?
// â? 1  â? GND       â?å°çº¿ï¼çµæºè½¨ï¼?                                       â?
// â? 2  â? GPIO16    â?General-purpose IO                                    â?
// â? 2  â? GPIO16    â?éç¨è¾å¥è¾åº                                          â?
// â? 3  â? GND       â?Ground (power rail)                                   â?
// â? 3  â? GND       â?å°çº¿ï¼çµæºè½¨ï¼?                                       â?
// â? 4  â? GPIO17    â?General-purpose IO                                    â?
// â? 4  â? GPIO17    â?éç¨è¾å¥è¾åº                                          â?
// â? 5  â? 3.3 V     â?3.3 V supply output (power rail)                      â?
// â? 5  â? 3.3 V     â?3.3 V çµæºè¾åºï¼çµæºè½¨ï¼?                             â?
// â? 6  â? GND       â?Ground (power rail)                                   â?
// â? 6  â? GND       â?å°çº¿ï¼çµæºè½¨ï¼?                                       â?
// â? 7  â? GPIO18    â?SPI MOSI                                              â?
// â? 7  â? GPIO18    â?SPI MOSI                                              â?
// â? 8  â? GPIO45    â?SPI CS                                                â?
// â? 8  â? GPIO45    â?SPI CS                                                â?
// â? 9  â? GPIO19    â?SPI MISO                                              â?
// â? 9  â? GPIO19    â?SPI MISO                                              â?
// â?10  â? GPIO52    â?SPI CLK                                               â?
// â?10  â? GPIO52    â?SPI CLK                                               â?
// â?11  â? GPIO5     â?SPI DC / general IO                                   â?
// â?11  â? GPIO5     â?SPI DC / éç¨è¾å¥è¾åº                                 â?
// â?12  â? NC        â?Not connected                                         â?
// â?12  â? NC        â?æªè¿æ?                                               â?
// â?13  â? GPIO38    â?UART0 RXD                                             â?
// â?13  â? GPIO38    â?UART0 RXD                                             â?
// â?14  â? GPIO37    â?UART0 TXD                                             â?
// â?14  â? GPIO37    â?UART0 TXD                                             â?
// â?15  â? GPIO7     â?UART1 RXD                                             â?
// â?15  â? GPIO7     â?UART1 RXD                                             â?
// â?16  â? GPIO6     â?UART1 TXD                                             â?
// â?16  â? GPIO6     â?UART1 TXD                                             â?
// â?17  â? GPIO31    â?SYS I2C SDA (shared with on-board peripherals)        â?
// â?17  â? GPIO31    â?SYS I2C SDAï¼ä¸æ¿è½½å¤è®¾å±ç¨ï¼?                         â?
// â?18  â? GPIO32    â?SYS I2C SCL (shared with on-board peripherals)        â?
// â?18  â? GPIO32    â?SYS I2C SCLï¼ä¸æ¿è½½å¤è®¾å±ç¨ï¼?                         â?
// â?19  â? GPIO3     â?General-purpose IO                                    â?
// â?19  â? GPIO3     â?éç¨è¾å¥è¾åº                                          â?
// â?20  â? GPIO4     â?General-purpose IO                                    â?
// â?20  â? GPIO4     â?éç¨è¾å¥è¾åº                                          â?
// â?21  â? GPIO2     â?I2S MCLK (M-Bus audio path)                           â?
// â?21  â? GPIO2     â?I2S MCLKï¼M-Bus é³é¢éè·¯ï¼?                           â?
// â?22  â? GPIO48    â?I2S BCK / SCLK (M-Bus audio path)                     â?
// â?22  â? GPIO48    â?I2S BCK / SCLKï¼M-Bus é³é¢éè·¯ï¼?                     â?
// â?23  â? GPIO47    â?I2S DOUT / DATA (M-Bus audio path)                    â?
// â?23  â? GPIO47    â?I2S DOUT / DATAï¼M-Bus é³é¢éè·¯ï¼?                    â?
// â?24  â? GPIO35    â?I2S WS / LRCK (M-Bus audio path)                      â?
// â?24  â? GPIO35    â?I2S WS / LRCKï¼M-Bus é³é¢éè·¯ï¼?                      â?
// â?25  â? NC        â?Not connected                                         â?
// â?25  â? NC        â?æªè¿æ?                                               â?
// â?26  â? GPIO51    â?General-purpose IO / ADC                              â?
// â?26  â? GPIO51    â?éç¨è¾å¥è¾åº / ADC                                    â?
// â?27  â? NC        â?Not connected                                         â?
// â?27  â? NC        â?æªè¿æ?                                               â?
// â?28  â? NC        â?Not connected                                         â?
// â?28  â? NC        â?æªè¿æ?                                               â?
// â?29  â? NC        â?Not connected                                         â?
// â?29  â? NC        â?æªè¿æ?                                               â?
// â?30  â? NC        â?Not connected                                         â?
// â?30  â? NC        â?æªè¿æ?                                               â?
// âââââââ´âââââââââââââ´ââââââââââââââââââââââââââââââââââââââââââââââââââââââââ?

constexpr gpio_num_t M5TAB5_MBUS_PIN2  = GPIO_NUM_16;
constexpr gpio_num_t M5TAB5_MBUS_PIN4  = GPIO_NUM_17;
constexpr gpio_num_t M5TAB5_MBUS_PIN7  = GPIO_NUM_18;   ///< SPI MOSI / SPI MOSI
constexpr gpio_num_t M5TAB5_MBUS_PIN8  = GPIO_NUM_45;   ///< SPI CS / SPI CS
constexpr gpio_num_t M5TAB5_MBUS_PIN9  = GPIO_NUM_19;   ///< SPI MISO / SPI MISO
constexpr gpio_num_t M5TAB5_MBUS_PIN10 = GPIO_NUM_52;   ///< SPI CLK / SPI CLK
constexpr gpio_num_t M5TAB5_MBUS_PIN11 = GPIO_NUM_5;    ///< SPI DC or GPIO / SPI DC æ?GPIO
constexpr gpio_num_t M5TAB5_MBUS_PIN13 = GPIO_NUM_38;   ///< UART0 RXD / UART0 æ¥æ¶
constexpr gpio_num_t M5TAB5_MBUS_PIN14 = GPIO_NUM_37;   ///< UART0 TXD / UART0 åé?
constexpr gpio_num_t M5TAB5_MBUS_PIN15 = GPIO_NUM_7;    ///< UART1 RXD / UART1 æ¥æ¶
constexpr gpio_num_t M5TAB5_MBUS_PIN16 = GPIO_NUM_6;    ///< UART1 TXD / UART1 åé?
constexpr gpio_num_t M5TAB5_MBUS_PIN17 = GPIO_NUM_31;   ///< SYS I2C SDA (= M5TAB5_PIN_SYS_I2C_SDA) / SYS I2C SDAï¼å³ M5TAB5_PIN_SYS_I2C_SDAï¼?
constexpr gpio_num_t M5TAB5_MBUS_PIN18 = GPIO_NUM_32;   ///< SYS I2C SCL (= M5TAB5_PIN_SYS_I2C_SCL) / SYS I2C SCLï¼å³ M5TAB5_PIN_SYS_I2C_SCLï¼?
constexpr gpio_num_t M5TAB5_MBUS_PIN19 = GPIO_NUM_3;
constexpr gpio_num_t M5TAB5_MBUS_PIN20 = GPIO_NUM_4;
constexpr gpio_num_t M5TAB5_MBUS_PIN21 = GPIO_NUM_2;    ///< I2S MCLK (M-Bus audio) / I2S MCLKï¼M-Bus é³é¢ï¼?
constexpr gpio_num_t M5TAB5_MBUS_PIN22 = GPIO_NUM_48;   ///< I2S BCK (M-Bus audio) / I2S BCKï¼M-Bus é³é¢ï¼?
constexpr gpio_num_t M5TAB5_MBUS_PIN23 = GPIO_NUM_47;   ///< I2S DOUT (M-Bus audio) / I2S DOUTï¼M-Bus é³é¢ï¼?
constexpr gpio_num_t M5TAB5_MBUS_PIN24 = GPIO_NUM_35;   ///< I2S WS (M-Bus audio) / I2S WSï¼M-Bus é³é¢ï¼?
constexpr gpio_num_t M5TAB5_MBUS_PIN26 = GPIO_NUM_51;

// Aliases matching M5Stack M-Bus functional names / ä¸?M5Stack M-Bus åè½åç§°ä¸è´çå«å
constexpr gpio_num_t M5TAB5_MBUS_SPI_MOSI = M5TAB5_MBUS_PIN7;
constexpr gpio_num_t M5TAB5_MBUS_SPI_CS   = M5TAB5_MBUS_PIN8;
constexpr gpio_num_t M5TAB5_MBUS_SPI_MISO = M5TAB5_MBUS_PIN9;
constexpr gpio_num_t M5TAB5_MBUS_SPI_CLK  = M5TAB5_MBUS_PIN10;
constexpr gpio_num_t M5TAB5_MBUS_SPI_DC   = M5TAB5_MBUS_PIN11;
constexpr gpio_num_t M5TAB5_MBUS_UART0_RX = M5TAB5_MBUS_PIN13;
constexpr gpio_num_t M5TAB5_MBUS_UART0_TX = M5TAB5_MBUS_PIN14;
constexpr gpio_num_t M5TAB5_MBUS_UART1_RX = M5TAB5_MBUS_PIN15;
constexpr gpio_num_t M5TAB5_MBUS_UART1_TX = M5TAB5_MBUS_PIN16;
constexpr gpio_num_t M5TAB5_MBUS_I2C_SDA  = M5TAB5_MBUS_PIN17;  ///< Shared with SYS I2C / ä¸?SYS I2C å±ç¨
constexpr gpio_num_t M5TAB5_MBUS_I2C_SCL  = M5TAB5_MBUS_PIN18;  ///< Shared with SYS I2C / ä¸?SYS I2C å±ç¨
constexpr gpio_num_t M5TAB5_MBUS_I2S_MCLK = M5TAB5_MBUS_PIN21;
constexpr gpio_num_t M5TAB5_MBUS_I2S_BCK  = M5TAB5_MBUS_PIN22;
constexpr gpio_num_t M5TAB5_MBUS_I2S_DOUT = M5TAB5_MBUS_PIN23;
constexpr gpio_num_t M5TAB5_MBUS_I2S_WS   = M5TAB5_MBUS_PIN24;


}  // namespace m5::tab5
