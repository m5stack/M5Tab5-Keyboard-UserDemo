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
constexpr gpio_num_t M5TAB5_PIN_LCD_TOUCH_RST = GPIO_NUM_NC;   // TP_RST controlled via IO expander (ADDR_LOW P5) / TP_RST Áî?IO Êâ©Â±ïÂô®ÊéßÂà∂ÔºàADDR_LOW P5Ôº?
constexpr gpio_num_t M5TAB5_PIN_LCD_TOUCH_INT = GPIO_NUM_23;   // TP_INT direct GPIO / TP_INT Áõ¥Êé•ËøûÊé•Âà?GPIO

constexpr gpio_num_t M5TAB5_PIN_SDMMC_DET = GPIO_NUM_NC;
constexpr gpio_num_t M5TAB5_PIN_SDMMC_CLK = GPIO_NUM_43;
constexpr gpio_num_t M5TAB5_PIN_SDMMC_CMD = GPIO_NUM_44;
constexpr gpio_num_t M5TAB5_PIN_SDMMC_D0 = GPIO_NUM_39;
constexpr gpio_num_t M5TAB5_PIN_SDMMC_D1 = GPIO_NUM_40;
constexpr gpio_num_t M5TAB5_PIN_SDMMC_D2 = GPIO_NUM_41;
constexpr gpio_num_t M5TAB5_PIN_SDMMC_D3 = GPIO_NUM_42;

// ‚îÄ‚îÄ ESP32-C6 Module GPIO Assignments / ESP32-C6 Ê®°ÁªÑ GPIO ÂàÜÈÖç ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ
// Physical connector pins 8-16; left = P4 signal, right = C6 GPIO.
// Áâ©ÁêÜËøûÊé•Âô®ÁöÑ 8 Âà?16 ËÑö‰ΩçÔºåÂ∑¶‰æß‰∏∫ P4 ‰ø°Âè∑ÔºåÂè≥‰æß‰∏∫ C6 GPIO„Ä?
//   pin 8  SDIO2_D3        ‚Ü?C6 GPIO8
//   8 ËÑ? SDIO2_D3         ‚Ü?C6 GPIO8
//   pin 9  VDDPST          ‚Ü?power (no GPIO)
//   9 ËÑ? VDDPST           ‚Ü?ÁîµÊ∫êÔºàÊó† GPIOÔº?
//   pin 10 SDIO2_D2        ‚Ü?C6 GPIO9
//   10 ËÑ?SDIO2_D2         ‚Ü?C6 GPIO9
//   pin 11 SDIO2_D1        ‚Ü?C6 GPIO10
//   11 ËÑ?SDIO2_D1         ‚Ü?C6 GPIO10
//   pin 12 SDIO2_D0        ‚Ü?C6 GPIO11
//   12 ËÑ?SDIO2_D0         ‚Ü?C6 GPIO11
//   pin 13 SDIO2_CLK       ‚Ü?C6 GPIO12
//   13 ËÑ?SDIO2_CLK        ‚Ü?C6 GPIO12
//   pin 14 SDIO2_CMD       ‚Ü?C6 GPIO13
//   14 ËÑ?SDIO2_CMD        ‚Ü?C6 GPIO13
//   pin 15 RF_C6_IO2       ‚Ü?C6 GPIO14 (C6 internal RF path control, not driven from P4)
//   15 ËÑ?RF_C6_IO2        ‚Ü?C6 GPIO14ÔºàC6 ÂÜÖÈÉ®Â∞ÑÈ¢ëË∑ØÂæÑÊéßÂà∂Ôºå‰∏çÁî?P4 È©±Âä®Ôº?
//   pin 16 SOC_EXTRF_RST   ‚Ü?C6 GPIO15 (reset input on C6)
//   16 ËÑ?SOC_EXTRF_RST    ‚Ü?C6 GPIO15ÔºàC6 Â§ç‰ΩçËæìÂÖ•Ôº?
constexpr int M5TAB5_C6_GPIO_SDIO_D3  = 8;
constexpr int M5TAB5_C6_GPIO_SDIO_D2  = 9;
constexpr int M5TAB5_C6_GPIO_SDIO_D1  = 10;
constexpr int M5TAB5_C6_GPIO_SDIO_D0  = 11;
constexpr int M5TAB5_C6_GPIO_SDIO_CLK = 12;
constexpr int M5TAB5_C6_GPIO_SDIO_CMD = 13;
constexpr int M5TAB5_C6_GPIO_RF_CTL   = 14;  ///< C6 internal RF path (RF_C6_IO2)
constexpr int M5TAB5_C6_GPIO_RST      = 15;  ///< SOC_EXTRF_RST ‚Ä?reset input on C6

// P4 GPIO48 is not physically connected to C6 RST; the C6 is power-cycled via the IO expander
// through WLAN_PWR_EN (ADDR_HIGH 0x44 P0).
// P4 GPIO48 ÂÆûÈôÖÂπ∂Êú™ËøûÊé•Âà?C6 Áö?RSTÔºõC6 ÈÄöËøá IO Êâ©Â±ïÂô®ÁöÑ WLAN_PWR_ENÔºàADDR_HIGH 0x44 P0ÔºâÊñ≠ÁîµÈáçÂêØ„Ä?
// GPIO48 is assigned as a dummy value to satisfy esp-hosted's internal assert and is toggled
// only if SDIO card_init fails unexpectedly.
// GPIO48 ‰ªÖ‰Ωú‰∏∫Âç†‰ΩçÂÄºÁî®‰∫éÊª°Ë∂?esp-hosted ÁöÑÂÜÖÈÉ®Êñ≠Ë®ÄÔºåÂè™ÊúâÂú® SDIO card_init ÂºÇÂ∏∏Â§±Ë¥•Êó∂Êâç‰ºöË¢´ÁøªËΩ¨„Ä?
constexpr gpio_num_t M5TAB5_P4_GPIO_C6_RST_DUMMY = GPIO_NUM_48;

// ‚îÄ‚îÄ RF Antenna Selection / Â∞ÑÈ¢ëÂ§©Á∫øÈÄâÊã© ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ
// Controlled via IO expander ADDR_LOW 0x43, P0.
// ÈÄöËøá IO Êâ©Â±ïÂô?ADDR_LOW 0x43 Áö?P0 ÊéßÂà∂„Ä?
// M5TAB5_EXTIO_ADDR_LOW_RF_PTH_L_INT_H_EXT: LOW = internal, HIGH = external.
// M5TAB5_EXTIO_ADDR_LOW_RF_PTH_L_INT_H_EXTÔºöLOW Ë°®Á§∫ÂÜÖÁΩÆÂ§©Á∫øÔºåHIGH Ë°®Á§∫Â§ñÁΩÆÂ§©Á∫ø„Ä?
enum m5tab5_rf_antenna_t : int {
	M5TAB5_RF_ANTENNA_INTERNAL = 0,  ///< Built-in PCB antenna (IO expander P0 = LOW) / ÂÜÖÁΩÆ PCB Â§©Á∫øÔºàIO Êâ©Â±ïÂô?P0 = LOWÔº?
	M5TAB5_RF_ANTENNA_EXTERNAL = 1,  ///< External U.FL / SMA antenna (IO expander P0 = HIGH) / Â§ñÁΩÆ U.FL / SMA Â§©Á∫øÔºàIO Êâ©Â±ïÂô?P0 = HIGHÔº?
};

// ‚îÄ‚îÄ M-Bus 30-Pin Expansion Connector / M-Bus 30 ÈíàÊâ©Â±ïËøûÊé•Âô® ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ
// Physical pin layout (two rows, edge connector):
// Áâ©ÁêÜÂºïËÑöÂ∏ÉÂ±Ä‰∏∫ËæπÁºòËøûÊé•Âô®ÂèåÊéíÂΩ¢ÂºèÔº?
//   odd  = 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29
//   Â•áÊï∞ËÑ?= 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29
//   even = 2, 4, 6, 8,10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30
//   ÂÅ∂Êï∞ËÑ?= 2, 4, 6, 8,10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30
//
// Pins 1 and 3-6 are power or ground rails, so they have no GPIO number (GPIO_NUM_NC).
// 1 ËÑö‰∏é 3 Âà?6 ËÑö‰∏∫ÁîµÊ∫êÊàñÂú∞Á∫øÔºåÂõ†Ê≠§Ê≤°ÊúâÂØπÂ∫î GPIO ÁºñÂè∑ÔºàGPIO_NUM_NCÔºâ„Ä?
// Derivation: m5::M5Unified::_pin_table_mbus[], board_M5Tab5 row in M5Unified.cpp.
// Êù•Ê∫êÔºöm5::M5Unified::_pin_table_mbus[]Ôºå‰Ωç‰∫?M5Unified.cpp Áö?board_M5Tab5 Ë°å„Ä?
//
// ‚îå‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚î¨‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚î¨‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚î?
// ‚î?Pin ‚î? ESP32-P4  ‚î?Function                                              ‚î?
// ‚î?ÂºïËÑö ‚î? ESP32-P4  ‚î?ÂäüËÉΩ                                                  ‚î?
// ‚îú‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îº‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îº‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚î?
// ‚î? 1  ‚î? GND       ‚î?Ground (power rail)                                   ‚î?
// ‚î? 1  ‚î? GND       ‚î?Âú∞Á∫øÔºàÁîµÊ∫êËΩ®Ôº?                                       ‚î?
// ‚î? 2  ‚î? GPIO16    ‚î?General-purpose IO                                    ‚î?
// ‚î? 2  ‚î? GPIO16    ‚î?ÈÄöÁî®ËæìÂÖ•ËæìÂá∫                                          ‚î?
// ‚î? 3  ‚î? GND       ‚î?Ground (power rail)                                   ‚î?
// ‚î? 3  ‚î? GND       ‚î?Âú∞Á∫øÔºàÁîµÊ∫êËΩ®Ôº?                                       ‚î?
// ‚î? 4  ‚î? GPIO17    ‚î?General-purpose IO                                    ‚î?
// ‚î? 4  ‚î? GPIO17    ‚î?ÈÄöÁî®ËæìÂÖ•ËæìÂá∫                                          ‚î?
// ‚î? 5  ‚î? 3.3 V     ‚î?3.3 V supply output (power rail)                      ‚î?
// ‚î? 5  ‚î? 3.3 V     ‚î?3.3 V ÁîµÊ∫êËæìÂá∫ÔºàÁîµÊ∫êËΩ®Ôº?                             ‚î?
// ‚î? 6  ‚î? GND       ‚î?Ground (power rail)                                   ‚î?
// ‚î? 6  ‚î? GND       ‚î?Âú∞Á∫øÔºàÁîµÊ∫êËΩ®Ôº?                                       ‚î?
// ‚î? 7  ‚î? GPIO18    ‚î?SPI MOSI                                              ‚î?
// ‚î? 7  ‚î? GPIO18    ‚î?SPI MOSI                                              ‚î?
// ‚î? 8  ‚î? GPIO45    ‚î?SPI CS                                                ‚î?
// ‚î? 8  ‚î? GPIO45    ‚î?SPI CS                                                ‚î?
// ‚î? 9  ‚î? GPIO19    ‚î?SPI MISO                                              ‚î?
// ‚î? 9  ‚î? GPIO19    ‚î?SPI MISO                                              ‚î?
// ‚î?10  ‚î? GPIO52    ‚î?SPI CLK                                               ‚î?
// ‚î?10  ‚î? GPIO52    ‚î?SPI CLK                                               ‚î?
// ‚î?11  ‚î? GPIO5     ‚î?SPI DC / general IO                                   ‚î?
// ‚î?11  ‚î? GPIO5     ‚î?SPI DC / ÈÄöÁî®ËæìÂÖ•ËæìÂá∫                                 ‚î?
// ‚î?12  ‚î? NC        ‚î?Not connected                                         ‚î?
// ‚î?12  ‚î? NC        ‚î?Êú™ËøûÊé?                                               ‚î?
// ‚î?13  ‚î? GPIO38    ‚î?UART0 RXD                                             ‚î?
// ‚î?13  ‚î? GPIO38    ‚î?UART0 RXD                                             ‚î?
// ‚î?14  ‚î? GPIO37    ‚î?UART0 TXD                                             ‚î?
// ‚î?14  ‚î? GPIO37    ‚î?UART0 TXD                                             ‚î?
// ‚î?15  ‚î? GPIO7     ‚î?UART1 RXD                                             ‚î?
// ‚î?15  ‚î? GPIO7     ‚î?UART1 RXD                                             ‚î?
// ‚î?16  ‚î? GPIO6     ‚î?UART1 TXD                                             ‚î?
// ‚î?16  ‚î? GPIO6     ‚î?UART1 TXD                                             ‚î?
// ‚î?17  ‚î? GPIO31    ‚î?SYS I2C SDA (shared with on-board peripherals)        ‚î?
// ‚î?17  ‚î? GPIO31    ‚î?SYS I2C SDAÔºà‰∏éÊùøËΩΩÂ§ñËÆæÂÖ±Áî®Ôº?                         ‚î?
// ‚î?18  ‚î? GPIO32    ‚î?SYS I2C SCL (shared with on-board peripherals)        ‚î?
// ‚î?18  ‚î? GPIO32    ‚î?SYS I2C SCLÔºà‰∏éÊùøËΩΩÂ§ñËÆæÂÖ±Áî®Ôº?                         ‚î?
// ‚î?19  ‚î? GPIO3     ‚î?General-purpose IO                                    ‚î?
// ‚î?19  ‚î? GPIO3     ‚î?ÈÄöÁî®ËæìÂÖ•ËæìÂá∫                                          ‚î?
// ‚î?20  ‚î? GPIO4     ‚î?General-purpose IO                                    ‚î?
// ‚î?20  ‚î? GPIO4     ‚î?ÈÄöÁî®ËæìÂÖ•ËæìÂá∫                                          ‚î?
// ‚î?21  ‚î? GPIO2     ‚î?I2S MCLK (M-Bus audio path)                           ‚î?
// ‚î?21  ‚î? GPIO2     ‚î?I2S MCLKÔºàM-Bus Èü≥È¢ëÈÄöË∑ØÔº?                           ‚î?
// ‚î?22  ‚î? GPIO48    ‚î?I2S BCK / SCLK (M-Bus audio path)                     ‚î?
// ‚î?22  ‚î? GPIO48    ‚î?I2S BCK / SCLKÔºàM-Bus Èü≥È¢ëÈÄöË∑ØÔº?                     ‚î?
// ‚î?23  ‚î? GPIO47    ‚î?I2S DOUT / DATA (M-Bus audio path)                    ‚î?
// ‚î?23  ‚î? GPIO47    ‚î?I2S DOUT / DATAÔºàM-Bus Èü≥È¢ëÈÄöË∑ØÔº?                    ‚î?
// ‚î?24  ‚î? GPIO35    ‚î?I2S WS / LRCK (M-Bus audio path)                      ‚î?
// ‚î?24  ‚î? GPIO35    ‚î?I2S WS / LRCKÔºàM-Bus Èü≥È¢ëÈÄöË∑ØÔº?                      ‚î?
// ‚î?25  ‚î? NC        ‚î?Not connected                                         ‚î?
// ‚î?25  ‚î? NC        ‚î?Êú™ËøûÊé?                                               ‚î?
// ‚î?26  ‚î? GPIO51    ‚î?General-purpose IO / ADC                              ‚î?
// ‚î?26  ‚î? GPIO51    ‚î?ÈÄöÁî®ËæìÂÖ•ËæìÂá∫ / ADC                                    ‚î?
// ‚î?27  ‚î? NC        ‚î?Not connected                                         ‚î?
// ‚î?27  ‚î? NC        ‚î?Êú™ËøûÊé?                                               ‚î?
// ‚î?28  ‚î? NC        ‚î?Not connected                                         ‚î?
// ‚î?28  ‚î? NC        ‚î?Êú™ËøûÊé?                                               ‚î?
// ‚î?29  ‚î? NC        ‚î?Not connected                                         ‚î?
// ‚î?29  ‚î? NC        ‚î?Êú™ËøûÊé?                                               ‚î?
// ‚î?30  ‚î? NC        ‚î?Not connected                                         ‚î?
// ‚î?30  ‚î? NC        ‚î?Êú™ËøûÊé?                                               ‚î?
// ‚îî‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚î¥‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚î¥‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚îÄ‚î?

constexpr gpio_num_t M5TAB5_MBUS_PIN2  = GPIO_NUM_16;
constexpr gpio_num_t M5TAB5_MBUS_PIN4  = GPIO_NUM_17;
constexpr gpio_num_t M5TAB5_MBUS_PIN7  = GPIO_NUM_18;   ///< SPI MOSI / SPI MOSI
constexpr gpio_num_t M5TAB5_MBUS_PIN8  = GPIO_NUM_45;   ///< SPI CS / SPI CS
constexpr gpio_num_t M5TAB5_MBUS_PIN9  = GPIO_NUM_19;   ///< SPI MISO / SPI MISO
constexpr gpio_num_t M5TAB5_MBUS_PIN10 = GPIO_NUM_52;   ///< SPI CLK / SPI CLK
constexpr gpio_num_t M5TAB5_MBUS_PIN11 = GPIO_NUM_5;    ///< SPI DC or GPIO / SPI DC Êà?GPIO
constexpr gpio_num_t M5TAB5_MBUS_PIN13 = GPIO_NUM_38;   ///< UART0 RXD / UART0 Êé•Êî∂
constexpr gpio_num_t M5TAB5_MBUS_PIN14 = GPIO_NUM_37;   ///< UART0 TXD / UART0 ÂèëÈÄ?
constexpr gpio_num_t M5TAB5_MBUS_PIN15 = GPIO_NUM_7;    ///< UART1 RXD / UART1 Êé•Êî∂
constexpr gpio_num_t M5TAB5_MBUS_PIN16 = GPIO_NUM_6;    ///< UART1 TXD / UART1 ÂèëÈÄ?
constexpr gpio_num_t M5TAB5_MBUS_PIN17 = GPIO_NUM_31;   ///< SYS I2C SDA (= M5TAB5_PIN_SYS_I2C_SDA) / SYS I2C SDAÔºàÂç≥ M5TAB5_PIN_SYS_I2C_SDAÔº?
constexpr gpio_num_t M5TAB5_MBUS_PIN18 = GPIO_NUM_32;   ///< SYS I2C SCL (= M5TAB5_PIN_SYS_I2C_SCL) / SYS I2C SCLÔºàÂç≥ M5TAB5_PIN_SYS_I2C_SCLÔº?
constexpr gpio_num_t M5TAB5_MBUS_PIN19 = GPIO_NUM_3;
constexpr gpio_num_t M5TAB5_MBUS_PIN20 = GPIO_NUM_4;
constexpr gpio_num_t M5TAB5_MBUS_PIN21 = GPIO_NUM_2;    ///< I2S MCLK (M-Bus audio) / I2S MCLKÔºàM-Bus Èü≥È¢ëÔº?
constexpr gpio_num_t M5TAB5_MBUS_PIN22 = GPIO_NUM_48;   ///< I2S BCK (M-Bus audio) / I2S BCKÔºàM-Bus Èü≥È¢ëÔº?
constexpr gpio_num_t M5TAB5_MBUS_PIN23 = GPIO_NUM_47;   ///< I2S DOUT (M-Bus audio) / I2S DOUTÔºàM-Bus Èü≥È¢ëÔº?
constexpr gpio_num_t M5TAB5_MBUS_PIN24 = GPIO_NUM_35;   ///< I2S WS (M-Bus audio) / I2S WSÔºàM-Bus Èü≥È¢ëÔº?
constexpr gpio_num_t M5TAB5_MBUS_PIN26 = GPIO_NUM_51;

// Aliases matching M5Stack M-Bus functional names / ‰∏?M5Stack M-Bus ÂäüËÉΩÂêçÁß∞‰∏ÄËá¥ÁöÑÂà´Âêç
constexpr gpio_num_t M5TAB5_MBUS_SPI_MOSI = M5TAB5_MBUS_PIN7;
constexpr gpio_num_t M5TAB5_MBUS_SPI_CS   = M5TAB5_MBUS_PIN8;
constexpr gpio_num_t M5TAB5_MBUS_SPI_MISO = M5TAB5_MBUS_PIN9;
constexpr gpio_num_t M5TAB5_MBUS_SPI_CLK  = M5TAB5_MBUS_PIN10;
constexpr gpio_num_t M5TAB5_MBUS_SPI_DC   = M5TAB5_MBUS_PIN11;
constexpr gpio_num_t M5TAB5_MBUS_UART0_RX = M5TAB5_MBUS_PIN13;
constexpr gpio_num_t M5TAB5_MBUS_UART0_TX = M5TAB5_MBUS_PIN14;
constexpr gpio_num_t M5TAB5_MBUS_UART1_RX = M5TAB5_MBUS_PIN15;
constexpr gpio_num_t M5TAB5_MBUS_UART1_TX = M5TAB5_MBUS_PIN16;
constexpr gpio_num_t M5TAB5_MBUS_I2C_SDA  = M5TAB5_MBUS_PIN17;  ///< Shared with SYS I2C / ‰∏?SYS I2C ÂÖ±Áî®
constexpr gpio_num_t M5TAB5_MBUS_I2C_SCL  = M5TAB5_MBUS_PIN18;  ///< Shared with SYS I2C / ‰∏?SYS I2C ÂÖ±Áî®
constexpr gpio_num_t M5TAB5_MBUS_I2S_MCLK = M5TAB5_MBUS_PIN21;
constexpr gpio_num_t M5TAB5_MBUS_I2S_BCK  = M5TAB5_MBUS_PIN22;
constexpr gpio_num_t M5TAB5_MBUS_I2S_DOUT = M5TAB5_MBUS_PIN23;
constexpr gpio_num_t M5TAB5_MBUS_I2S_WS   = M5TAB5_MBUS_PIN24;


}  // namespace m5::tab5
