/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * SPDX-License-Identifier: Apache-2.0
 * ST7121 MIPI-DSI LCD panel driver for M5Stack Tab5
 * 适用�?M5Stack Tab5 �?ST7121 MIPI-DSI LCD 面板驱动
 * Standalone esp_lcd panel driver with no external dependencies beyond esp-idf.
 * 独立�?esp_lcd 面板驱动，除 esp-idf 外无额外依赖�?
 */
#pragma once

#include <stdint.h>
#include "soc/soc_caps.h"

#if SOC_MIPI_DSI_SUPPORTED
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_mipi_dsi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- ST7121 TDDI Panel Parameters / ST7121 TDDI 面板参数�?20×1280�? 通道 DSI�?----
 */
#define ST7121_H_RES             720
#define ST7121_V_RES             1280
#define ST7121_HSYNC_PULSE_WIDTH 2
#define ST7121_HSYNC_BACK_PORCH  40
#define ST7121_HSYNC_FRONT_PORCH 40
#define ST7121_VSYNC_PULSE_WIDTH 20
#define ST7121_VSYNC_BACK_PORCH  24
#define ST7121_VSYNC_FRONT_PORCH 220
#define ST7121_DPI_CLK_MHZ       80
#define ST7121_LANE_NUM          2
#define ST7121_LANE_BITRATE_MBPS 1040

/**
 * @brief LCD panel initialization commands.
 *        LCD 面板初始化命令�?
 */
typedef struct {
    int cmd;               /*<! The specific LCD command / 具体�?LCD 命令 */
    const void *data;      /*<! Buffer that holds the command specific data /
                              存放该命令数据的缓冲�?*/
    size_t data_bytes;     /*<! Size of `data` in memory, in bytes / `data` 的内存字节数 */
    unsigned int delay_ms; /*<! Delay in milliseconds after this command /
                              执行该命令后的延时（毫秒�?*/
} st7121_lcd_init_cmd_t;

/**
 * @brief LCD panel vendor configuration.
 *        LCD 面板厂商配置�?
 * @note  Pass this to `esp_lcd_panel_dev_config_t::vendor_config`.
 * @note  该结构需要传�?`esp_lcd_panel_dev_config_t::vendor_config`�?
 */
typedef struct {
    const st7121_lcd_init_cmd_t *init_cmds; /*!< Pointer to the initialization command array. NULL uses the built-in
                                               default. / 初始化命令数组指针；NULL
                                               表示使用内置默认序列�?*/
    uint16_t init_cmds_size;                /*!< Number of commands in the array above / 上述数组中的命令数量
                                             */
    struct {
        esp_lcd_dsi_bus_handle_t dsi_bus;             /*!< MIPI-DSI bus handle / MIPI-DSI 总线句柄 */
        const esp_lcd_dpi_panel_config_t *dpi_config; /*!< MIPI-DPI panel configuration / MIPI-DPI 面板配置 */
        uint8_t lane_num; /*!< Number of MIPI-DSI data lanes / MIPI-DSI 数据通道数量 */
    } mipi_config;
} st7121_vendor_config_t;

/**
 * @brief Create an LCD panel instance for ST7121.
 *        �?ST7121 创建 LCD 面板实例�?
 *
 * @param[in]  io              LCD panel IO handle (DBI)
 * @param[in]  panel_dev_config General panel device configuration
 * @param[out] ret_panel       Returned LCD panel handle
 * @param[in]  io              LCD 面板 IO 句柄（DBI�?
 * @param[in]  panel_dev_config 通用面板设备配置
 * @param[out] ret_panel       返回�?LCD 面板句柄
 * @return
 *      - ESP_ERR_INVALID_ARG   if parameter is invalid
 *      - ESP_ERR_NO_MEM        if out of memory
 *      - ESP_OK                on success
 *      - ESP_ERR_INVALID_ARG   参数无效
 *      - ESP_ERR_NO_MEM        内存不足
 *      - ESP_OK                成功
 */
esp_err_t esp_lcd_new_panel_st7121(const esp_lcd_panel_io_handle_t io,
                                   const esp_lcd_panel_dev_config_t *panel_dev_config,
                                   esp_lcd_panel_handle_t *ret_panel);

/* ---- Helper Macros for Quick Configuration / 快速配置辅助宏 ---- */

#define ST7121_PANEL_BUS_DSI_2CH_CONFIG()                   \
    {                                                       \
        .bus_id             = 0,                            \
        .num_data_lanes     = ST7121_LANE_NUM,              \
        .phy_clk_src        = MIPI_DSI_PHY_CLK_SRC_DEFAULT, \
        .lane_bit_rate_mbps = ST7121_LANE_BITRATE_MBPS,     \
    }

#define ST7121_PANEL_IO_DBI_CONFIG() \
    {                                \
        .virtual_channel = 0,        \
        .lcd_cmd_bits    = 8,        \
        .lcd_param_bits  = 8,        \
    }

/**
 * @note  refresh_rate �?70 MHz / 802 / 1524 �?57 Hz
 * @note  刷新率约�?70 MHz / 802 / 1524 �?57 Hz
 */
#define ST7121_720_1280_PANEL_60HZ_DPI_CONFIG(px_format)       \
    {                                                          \
        .virtual_channel    = 0,                               \
        .dpi_clk_src        = MIPI_DSI_DPI_CLK_SRC_DEFAULT,    \
        .dpi_clock_freq_mhz = ST7121_DPI_CLK_MHZ,              \
        .in_color_format    = (px_format),                     \
        .out_color_format   = (px_format),                     \
        .num_fbs            = 2,                               \
        .video_timing =                                        \
            {                                                  \
                .h_size            = ST7121_H_RES,             \
                .v_size            = ST7121_V_RES,             \
                .hsync_pulse_width = ST7121_HSYNC_PULSE_WIDTH, \
                .hsync_back_porch  = ST7121_HSYNC_BACK_PORCH,  \
                .hsync_front_porch = ST7121_HSYNC_FRONT_PORCH, \
                .vsync_pulse_width = ST7121_VSYNC_PULSE_WIDTH, \
                .vsync_back_porch  = ST7121_VSYNC_BACK_PORCH,  \
                .vsync_front_porch = ST7121_VSYNC_FRONT_PORCH, \
            },                                                 \
    }

#ifdef __cplusplus
}
#endif

#endif  // SOC_MIPI_DSI_SUPPORTED
