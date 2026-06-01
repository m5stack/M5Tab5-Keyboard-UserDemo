/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * SPDX-License-Identifier: Apache-2.0
 * ILI9881C MIPI-DSI LCD panel driver for M5Stack Tab5
 * éç¨äº?M5Stack Tab5 ç?ILI9881C MIPI-DSI LCD é¢æ¿é©±å¨
 * Standalone esp_lcd panel driver with no external dependencies beyond esp-idf.
 * ç¬ç«ç?esp_lcd é¢æ¿é©±å¨ï¼é¤ esp-idf å¤æ é¢å¤ä¾èµã?
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

/* ---- ILI9881C Panel Parameters / ILI9881C é¢æ¿åæ°ï¼?20Ã1280ï¼? éé DSIï¼?---- */
// #define ILI9881C_H_RES                  720
// #define ILI9881C_V_RES                  1280
// #define ILI9881C_HSYNC_PULSE_WIDTH      40
// #define ILI9881C_HSYNC_BACK_PORCH       140
// #define ILI9881C_HSYNC_FRONT_PORCH      40
// #define ILI9881C_VSYNC_PULSE_WIDTH      4
// #define ILI9881C_VSYNC_BACK_PORCH       20
// #define ILI9881C_VSYNC_FRONT_PORCH      20
// #define ILI9881C_DPI_CLK_MHZ            60
// #define ILI9881C_LANE_NUM               2
// #define ILI9881C_LANE_BITRATE_MBPS      730

#define ILI9881C_H_RES             720
#define ILI9881C_V_RES             1280
#define ILI9881C_HSYNC_PULSE_WIDTH 40
#define ILI9881C_HSYNC_BACK_PORCH  144
#define ILI9881C_HSYNC_FRONT_PORCH 40
#define ILI9881C_VSYNC_PULSE_WIDTH 4
#define ILI9881C_VSYNC_BACK_PORCH  20
#define ILI9881C_VSYNC_FRONT_PORCH 108
#define ILI9881C_DPI_CLK_MHZ       80
#define ILI9881C_LANE_NUM          2
#define ILI9881C_LANE_BITRATE_MBPS 1040
/**
 * @brief LCD panel initialization commands.
 *        LCD é¢æ¿åå§åå½ä»¤ã?
 */
typedef struct {
    int cmd;               /*<! The specific LCD command / å·ä½ç?LCD å½ä»¤ */
    const void *data;      /*<! Buffer that holds the command specific data /
                              å­æ¾è¯¥å½ä»¤æ°æ®çç¼å²å?*/
    size_t data_bytes;     /*<! Size of `data` in memory, in bytes / `data` çåå­å­èæ° */
    unsigned int delay_ms; /*<! Delay in milliseconds after this command /
                              æ§è¡è¯¥å½ä»¤åçå»¶æ¶ï¼æ¯«ç§ï¼?*/
} ili9881c_lcd_init_cmd_t;

/**
 * @brief LCD panel vendor configuration.
 *        LCD é¢æ¿ååéç½®ã?
 * @note  Pass this to `esp_lcd_panel_dev_config_t::vendor_config`.
 * @note  è¯¥ç»æéè¦ä¼ ç»?`esp_lcd_panel_dev_config_t::vendor_config`ã?
 */
typedef struct {
    const ili9881c_lcd_init_cmd_t *init_cmds; /*!< Pointer to the initialization command array. NULL uses the built-in
                                                 default. / åå§åå½ä»¤æ°ç»æéï¼NULL
                                                 è¡¨ç¤ºä½¿ç¨åç½®é»è®¤åºåã?*/
    uint16_t init_cmds_size; /*!< Number of commands in the array above / ä¸è¿°æ°ç»ä¸­çå½ä»¤æ°é
                              */
    struct {
        esp_lcd_dsi_bus_handle_t dsi_bus; /*!< MIPI-DSI bus handle / MIPI-DSI æ»çº¿å¥æ */
        const esp_lcd_dpi_panel_config_t
            *dpi_config;  /*!< MIPI-DPI panel configuration / MIPI-DPI é¢æ¿éç½® */
        uint8_t lane_num; /*!< Number of MIPI-DSI data lanes / MIPI-DSI æ°æ®ééæ°é */
    } mipi_config;
} ili9881c_vendor_config_t;

/**
 * @brief Create an LCD panel instance for ILI9881C.
 *        ä¸?ILI9881C åå»º LCD é¢æ¿å®ä¾ã?
 *
 * @param[in]  io              LCD panel IO handle (DBI)
 * @param[in]  panel_dev_config General panel device configuration
 * @param[out] ret_panel       Returned LCD panel handle
 * @param[in]  io              LCD é¢æ¿ IO å¥æï¼DBIï¼?
 * @param[in]  panel_dev_config éç¨é¢æ¿è®¾å¤éç½®
 * @param[out] ret_panel       è¿åç?LCD é¢æ¿å¥æ
 * @return
 *      - ESP_ERR_INVALID_ARG   if parameter is invalid
 *      - ESP_ERR_NO_MEM        if out of memory
 *      - ESP_OK                on success
 *      - ESP_ERR_INVALID_ARG   åæ°æ æ
 *      - ESP_ERR_NO_MEM        åå­ä¸è¶³
 *      - ESP_OK                æå
 */
esp_err_t esp_lcd_new_panel_ili9881c(const esp_lcd_panel_io_handle_t io,
                                     const esp_lcd_panel_dev_config_t *panel_dev_config,
                                     esp_lcd_panel_handle_t *ret_panel);

/* ---- Helper Macros for Quick Configuration / å¿«ééç½®è¾å©å® ---- */

#define ILI9881C_PANEL_BUS_DSI_2CH_CONFIG()                 \
    {                                                       \
        .bus_id             = 0,                            \
        .num_data_lanes     = ILI9881C_LANE_NUM,            \
        .phy_clk_src        = MIPI_DSI_PHY_CLK_SRC_DEFAULT, \
        .lane_bit_rate_mbps = ILI9881C_LANE_BITRATE_MBPS,   \
    }

#define ILI9881C_PANEL_IO_DBI_CONFIG() \
    {                                  \
        .virtual_channel = 0,          \
        .lcd_cmd_bits    = 8,          \
        .lcd_param_bits  = 8,          \
    }

#define ILI9881C_720_1280_PANEL_60HZ_DPI_CONFIG(px_format)       \
    {                                                            \
        .virtual_channel    = 0,                                 \
        .dpi_clk_src        = MIPI_DSI_DPI_CLK_SRC_DEFAULT,      \
        .dpi_clock_freq_mhz = ILI9881C_DPI_CLK_MHZ,              \
        .in_color_format    = (px_format),                       \
        .out_color_format   = (px_format),                       \
        .num_fbs            = 2,                                 \
        .video_timing =                                          \
            {                                                    \
                .h_size            = ILI9881C_H_RES,             \
                .v_size            = ILI9881C_V_RES,             \
                .hsync_pulse_width = ILI9881C_HSYNC_PULSE_WIDTH, \
                .hsync_back_porch  = ILI9881C_HSYNC_BACK_PORCH,  \
                .hsync_front_porch = ILI9881C_HSYNC_FRONT_PORCH, \
                .vsync_pulse_width = ILI9881C_VSYNC_PULSE_WIDTH, \
                .vsync_back_porch  = ILI9881C_VSYNC_BACK_PORCH,  \
                .vsync_front_porch = ILI9881C_VSYNC_FRONT_PORCH, \
            },                                                   \
    }

#ifdef __cplusplus
}
#endif

#endif  // SOC_MIPI_DSI_SUPPORTED
