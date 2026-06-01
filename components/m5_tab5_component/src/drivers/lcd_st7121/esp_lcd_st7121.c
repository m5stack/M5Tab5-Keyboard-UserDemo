/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * SPDX-License-Identifier: Apache-2.0
 * ST7121 MIPI-DSI LCD panel driver for M5Stack Tab5
 * 适用于 M5Stack Tab5 的 ST7121 MIPI-DSI LCD 面板驱动
 * Standalone esp_lcd panel driver with no external dependencies beyond esp-idf.
 * 独立的 esp_lcd 面板驱动，除 esp-idf 外无额外依赖。
 */

#include "soc/soc_caps.h"

#if SOC_MIPI_DSI_SUPPORTED
#include "esp_check.h"
#include "esp_log.h"
#include "esp_lcd_panel_commands.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_vendor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_lcd_st7121.h"

typedef struct {
    esp_lcd_panel_io_handle_t io;
    int reset_gpio_num;
    uint8_t madctl_val;
    uint8_t colmod_val;
    const st7121_lcd_init_cmd_t *init_cmds;
    uint16_t init_cmds_size;
    uint8_t lane_num;
    struct {
        unsigned int reset_level : 1;
    } flags;
    // Original functions of the MIPI DPI panel / MIPI DPI 面板原始函数指针
    esp_err_t (*del)(esp_lcd_panel_t *panel);
    esp_err_t (*init)(esp_lcd_panel_t *panel);
} st7121_panel_t;

static const char *TAG = "st7121";

static esp_err_t panel_st7121_del(esp_lcd_panel_t *panel);
static esp_err_t panel_st7121_init(esp_lcd_panel_t *panel);
static esp_err_t panel_st7121_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_st7121_invert_color(esp_lcd_panel_t *panel, bool invert_color_data);
static esp_err_t panel_st7121_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y);
static esp_err_t panel_st7121_swap_xy(esp_lcd_panel_t *panel, bool swap_xy);
static esp_err_t panel_st7121_disp_on_off(esp_lcd_panel_t *panel, bool on_off);
static esp_err_t panel_st7121_sleep(esp_lcd_panel_t *panel, bool sleep);

/* ---- ST7121 Vendor-Specific Init Sequence / ST7121 厂商专用初始化序列 ---- */
static const st7121_lcd_init_cmd_t vendor_specific_init_default[] = {
    {0x60, (uint8_t[]){0x71, 0x21, 0xA2}, 3, 0},
    {0x60, (uint8_t[]){0x71, 0x21, 0xA3}, 3, 0},
    {0x60, (uint8_t[]){0x71, 0x21, 0xA4}, 3, 0},
    {0x78, (uint8_t[]){0x21}, 1, 0},
    {0x79, (uint8_t[]){0xEF}, 1, 0},
    {0xA4, (uint8_t[]){0x31}, 1, 0},
    {0xB7, (uint8_t[]){0x00, 0x00, 0x5F, 0x5F, 0x44, 0x1A}, 6, 0},
    {0xB0, (uint8_t[]){0x22, 0x6B, 0x11, 0x89, 0x25, 0x43, 0x43}, 7, 0},
    {0xBF, (uint8_t[]){0xA7, 0xA7}, 2, 0},
    {0xA5, (uint8_t[]){0xF0, 0x03}, 2, 0},
    {0xD7, (uint8_t[]){0x10, 0x2C, 0x14, 0x2A, 0x80, 0x80}, 6, 0},
    {0x90, (uint8_t[]){0x71, 0x23, 0x5A, 0x20, 0x24, 0x11, 0x21}, 7, 0},
    {0xA3, (uint8_t[]){0x80, 0x01, 0x8C, 0xFF, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x00, 0x00,
                       0x1E, 0x5C, 0x1E, 0x80, 0x10, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46,
                       0x00, 0x00, 0x1E, 0x5C, 0x1E, 0x80, 0x10, 0xEF, 0x58, 0x00, 0x00, 0x00, 0xFF},
     39, 0},
    {0xA6, (uint8_t[]){0x0A, 0x00, 0x24, 0x71, 0x36, 0x00, 0x00, 0x00, 0x68, 0x68, 0x91, 0xFF, 0x00, 0x24,
                       0x71, 0x37, 0x00, 0x00, 0x00, 0x68, 0x68, 0x91, 0xFF, 0x00, 0x24, 0x71, 0x00, 0x00,
                       0x00, 0x00, 0x68, 0x68, 0x91, 0xFF, 0x00, 0x2C, 0x71, 0x00, 0x01, 0x00, 0x00, 0x68,
                       0x68, 0xFF, 0xFF, 0x00, 0x08, 0x80, 0x08, 0x80, 0x06, 0x00, 0x00, 0x00, 0x00},
     55, 0},
    {0xA7, (uint8_t[]){0x1A, 0x1A, 0xC0, 0x64, 0x40, 0x04, 0x15, 0x40, 0x00, 0x40, 0x00, 0x68, 0x68, 0x91, 0xFF,
                       0x08, 0x80, 0x64, 0x40, 0x26, 0x37, 0x40, 0x00, 0x00, 0x00, 0x68, 0x68, 0x91, 0xFF, 0x08,
                       0x80, 0x64, 0x40, 0x8C, 0x9D, 0x40, 0x00, 0x00, 0x00, 0x68, 0x68, 0x91, 0xFF, 0x08, 0x80,
                       0x64, 0x40, 0xAE, 0xBF, 0x00, 0x00, 0x20, 0x00, 0x68, 0x68, 0x91, 0xFF, 0x08, 0x80, 0x79},
     60, 0},
    {0xAC, (uint8_t[]){0x1D, 0x18, 0x19, 0x1D, 0x18, 0x19, 0x04, 0x1C, 0x1D, 0x08, 0x0A, 0x10, 0x12, 0x0C, 0x0E,
                       0x14, 0x16, 0x00, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x18, 0x19, 0x1D, 0x18, 0x19, 0x06, 0x1C,
                       0x1D, 0x09, 0x0B, 0x11, 0x13, 0x0D, 0x0F, 0x15, 0x17, 0x02, 0x1D, 0x1D, 0x1D, 0x1D},
     44, 0},
    {0xAD, (uint8_t[]){0x0C, 0x40, 0x46, 0x00, 0x07, 0x4B, 0x4B, 0xFF, 0xFF, 0xF0, 0x40, 0x0E, 0x01,
                       0x07, 0x42, 0x42, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF},
     25, 0},
    {0xAE, (uint8_t[]){0xF0, 0xFF, 0x03, 0xF0, 0xFF, 0x03, 0x00}, 7, 0},
    {0xB2,
     (uint8_t[]){0x15, 0x19, 0x05, 0x23, 0x49, 0x2D, 0x03, 0x2E, 0x5C, 0xD2, 0xFF, 0x10, 0x60, 0xFD, 0x20, 0xC0, 0x00},
     17, 0},
    {0xE8, (uint8_t[]){0x20, 0x60, 0x04, 0x8E, 0x8E, 0x3E, 0x04, 0xDC, 0xDC, 0x3E, 0x06, 0xFA, 0x26, 0x3E}, 14, 0},
    {0x75, (uint8_t[]){0x03, 0x04}, 2, 0},
    {0xE7, (uint8_t[]){0x4B, 0x00, 0x00, 0xBE, 0x4B, 0x8C, 0x20, 0x1A, 0xF0, 0x7D, 0x14, 0x7D, 0x14, 0x7D,
                       0x14, 0x7D, 0x14, 0xFF, 0x00, 0x32, 0x30, 0x73, 0x00, 0x00, 0xC8, 0x6A, 0xFF, 0x5A,
                       0x64, 0x38, 0x88, 0x15, 0xB1, 0x01, 0x01, 0x64, 0x01, 0x01, 0x7C, 0xFF, 0x1A, 0x51},
     42, 0},
    {0xE1, (uint8_t[]){0x0C, 0x0C}, 2, 0},
    {0xEA, (uint8_t[]){0x15, 0x00, 0x01}, 3, 0},
    // Gamma / 伽马参数
    {0xC8, (uint8_t[]){0x00, 0x00, 0x04, 0x08, 0x10, 0x00, 0x1F, 0x01, 0x39, 0x3E, 0x00, 0x78, 0x06,
                       0xE2, 0x02, 0x11, 0x33, 0x01, 0x7A, 0x0D, 0x21, 0xC4, 0x0B, 0x19, 0x08, 0x32,
                       0xA0, 0x08, 0x1A, 0x0A, 0xF3, 0x7F, 0x0E, 0xC5, 0xE8, 0x03, 0xFF},
     37, 0},
    {0xC9, (uint8_t[]){0x00, 0x00, 0x04, 0x08, 0x10, 0x00, 0x1F, 0x01, 0x39, 0x3E, 0x00, 0x78, 0x06,
                       0xE2, 0x02, 0x11, 0x33, 0x01, 0x7A, 0x0D, 0x21, 0xC4, 0x0B, 0x19, 0x08, 0x32,
                       0xA0, 0x08, 0x1A, 0x0A, 0xF3, 0x7F, 0x0E, 0xC5, 0xE8, 0x03, 0xFF},
     37, 0},
    {0x60, (uint8_t[]){0x71, 0x21, 0x00}, 3, 0},
    {0x11, NULL, 0, 100},
    {0x29, NULL, 0, 0},
    {0x35, (uint8_t[]){0x00}, 1, 0},
};

esp_err_t esp_lcd_new_panel_st7121(const esp_lcd_panel_io_handle_t io,
                                   const esp_lcd_panel_dev_config_t *panel_dev_config,
                                   esp_lcd_panel_handle_t *ret_panel)
{
    ESP_RETURN_ON_FALSE(io && panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, TAG, "invalid arguments");
    st7121_vendor_config_t *vendor_config = (st7121_vendor_config_t *)panel_dev_config->vendor_config;
    ESP_RETURN_ON_FALSE(vendor_config && vendor_config->mipi_config.dpi_config && vendor_config->mipi_config.dsi_bus,
                        ESP_ERR_INVALID_ARG, TAG, "invalid vendor config");

    esp_err_t ret          = ESP_OK;
    st7121_panel_t *st7121 = (st7121_panel_t *)calloc(1, sizeof(st7121_panel_t));
    ESP_RETURN_ON_FALSE(st7121, ESP_ERR_NO_MEM, TAG, "no mem for st7121 panel");

    if (panel_dev_config->reset_gpio_num >= 0) {
        gpio_config_t io_conf = {
            .mode         = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << panel_dev_config->reset_gpio_num,
        };
        ESP_GOTO_ON_ERROR(gpio_config(&io_conf), err, TAG, "configure GPIO for RST line failed");
    }

    switch (panel_dev_config->rgb_ele_order) {
        case LCD_RGB_ELEMENT_ORDER_RGB:
            st7121->madctl_val = 0;
            break;
        case LCD_RGB_ELEMENT_ORDER_BGR:
            st7121->madctl_val |= LCD_CMD_BGR_BIT;
            break;
        default:
            ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported color space");
            break;
    }

    switch (panel_dev_config->bits_per_pixel) {
        case 16:
            st7121->colmod_val = 0x55;
            break;
        case 18:
            st7121->colmod_val = 0x66;
            break;
        case 24:
            st7121->colmod_val = 0x77;
            break;
        default:
            ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported pixel width");
            break;
    }

    st7121->io                = io;
    st7121->init_cmds         = vendor_config->init_cmds;
    st7121->init_cmds_size    = vendor_config->init_cmds_size;
    st7121->lane_num          = vendor_config->mipi_config.lane_num;
    st7121->reset_gpio_num    = panel_dev_config->reset_gpio_num;
    st7121->flags.reset_level = panel_dev_config->flags.reset_active_high;

    // Create the MIPI DPI panel / 创建 MIPI DPI 面板
    ESP_GOTO_ON_ERROR(
        esp_lcd_new_panel_dpi(vendor_config->mipi_config.dsi_bus, vendor_config->mipi_config.dpi_config, ret_panel),
        err, TAG, "create MIPI DPI panel failed");
    ESP_LOGD(TAG, "new MIPI DPI panel @%p", *ret_panel);

    // Save the original functions of the MIPI DPI panel / 保存 MIPI DPI 面板原始函数
    st7121->del  = (*ret_panel)->del;
    st7121->init = (*ret_panel)->init;
    // Overwrite them with ST7121-specific functions / 用 ST7121 专用函数覆盖
    (*ret_panel)->del          = panel_st7121_del;
    (*ret_panel)->init         = panel_st7121_init;
    (*ret_panel)->reset        = panel_st7121_reset;
    (*ret_panel)->mirror       = panel_st7121_mirror;
    (*ret_panel)->swap_xy      = panel_st7121_swap_xy;
    (*ret_panel)->invert_color = panel_st7121_invert_color;
    (*ret_panel)->disp_on_off  = panel_st7121_disp_on_off;
    (*ret_panel)->disp_sleep   = panel_st7121_sleep;
    (*ret_panel)->user_data    = st7121;
    ESP_LOGD(TAG, "new st7121 panel @%p", st7121);

    return ESP_OK;

err:
    if (st7121) {
        if (panel_dev_config->reset_gpio_num >= 0) {
            gpio_reset_pin(panel_dev_config->reset_gpio_num);
        }
        free(st7121);
    }
    return ret;
}

static esp_err_t panel_st7121_del(esp_lcd_panel_t *panel)
{
    st7121_panel_t *st7121 = (st7121_panel_t *)panel->user_data;

    if (st7121->reset_gpio_num >= 0) {
        gpio_reset_pin(st7121->reset_gpio_num);
    }
    st7121->del(panel);
    ESP_LOGD(TAG, "del st7121 panel @%p", st7121);
    free(st7121);

    return ESP_OK;
}

static esp_err_t panel_st7121_init(esp_lcd_panel_t *panel)
{
    st7121_panel_t *st7121                 = (st7121_panel_t *)panel->user_data;
    esp_lcd_panel_io_handle_t io           = st7121->io;
    const st7121_lcd_init_cmd_t *init_cmds = NULL;
    uint16_t init_cmds_size                = 0;

    // NOTE: Do NOT send MADCTL/COLMOD before vendor init sequence.
    // ST7121 requires entering vendor command pages first (cmd 0x60 with keys).
    // Sending standard commands before that will be ignored or cause issues.
    // 注意：不要在厂商初始化序列之前发送 MADCTL/COLMOD。
    // ST7121 需要先进入厂商命令页（cmd 0x60 + 密钥），
    // 在此之前发送标准命令会被忽略或导致问题。

    // Vendor-specific initialization / 厂商专用初始化
    if (st7121->init_cmds) {
        init_cmds      = st7121->init_cmds;
        init_cmds_size = st7121->init_cmds_size;
    } else {
        init_cmds      = vendor_specific_init_default;
        init_cmds_size = sizeof(vendor_specific_init_default) / sizeof(st7121_lcd_init_cmd_t);
    }

    bool is_cmd_overwritten = false;
    for (int i = 0; i < init_cmds_size; i++) {
        if (init_cmds[i].data_bytes > 0) {
            switch (init_cmds[i].cmd) {
                case LCD_CMD_MADCTL:
                    is_cmd_overwritten = true;
                    st7121->madctl_val = ((uint8_t *)init_cmds[i].data)[0];
                    break;
                case LCD_CMD_COLMOD:
                    is_cmd_overwritten = true;
                    st7121->colmod_val = ((uint8_t *)init_cmds[i].data)[0];
                    break;
                default:
                    is_cmd_overwritten = false;
                    break;
            }
            if (is_cmd_overwritten) {
                is_cmd_overwritten = false;
                ESP_LOGW(TAG,
                         "The %02Xh command has been used and will be overwritten by external initialization sequence",
                         init_cmds[i].cmd);
            }
        }
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, init_cmds[i].cmd, init_cmds[i].data, init_cmds[i].data_bytes),
                            TAG, "send command failed");
        vTaskDelay(pdMS_TO_TICKS(init_cmds[i].delay_ms));
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    ESP_LOGD(TAG, "send init commands success");

    ESP_RETURN_ON_ERROR(st7121->init(panel), TAG, "init MIPI DPI panel failed");

    return ESP_OK;
}

static esp_err_t panel_st7121_reset(esp_lcd_panel_t *panel)
{
    st7121_panel_t *st7121       = (st7121_panel_t *)panel->user_data;
    esp_lcd_panel_io_handle_t io = st7121->io;

    if (st7121->reset_gpio_num >= 0) {
        gpio_set_level(st7121->reset_gpio_num, !st7121->flags.reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(st7121->reset_gpio_num, st7121->flags.reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(st7121->reset_gpio_num, !st7121->flags.reset_level);
        vTaskDelay(pdMS_TO_TICKS(120));
    } else if (io) {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_SWRESET, NULL, 0), TAG, "send command failed");
        vTaskDelay(pdMS_TO_TICKS(120));
    }

    return ESP_OK;
}

static esp_err_t panel_st7121_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    st7121_panel_t *st7121       = (st7121_panel_t *)panel->user_data;
    esp_lcd_panel_io_handle_t io = st7121->io;
    ESP_RETURN_ON_FALSE(io, ESP_ERR_INVALID_STATE, TAG, "invalid panel IO");

    uint8_t command = invert_color_data ? LCD_CMD_INVON : LCD_CMD_INVOFF;
    return esp_lcd_panel_io_tx_param(io, command, NULL, 0);
}

/**
 * @brief ST7121 TDDI panel — hardware mirror via MADCTL is NOT supported.
 *        ST7121 TDDI 面板不支持通过 MADCTL 实现硬件镜像。
 *        Use LVGL software rotation instead.
 *        请改用 LVGL 的软件旋转能力。
 */
static esp_err_t panel_st7121_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    return ESP_ERR_NOT_SUPPORTED;
}

static esp_err_t panel_st7121_swap_xy(esp_lcd_panel_t *panel, bool swap_xy)
{
    return ESP_ERR_NOT_SUPPORTED;
}

static esp_err_t panel_st7121_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
{
    st7121_panel_t *st7121       = (st7121_panel_t *)panel->user_data;
    esp_lcd_panel_io_handle_t io = st7121->io;
    ESP_RETURN_ON_FALSE(io, ESP_ERR_INVALID_STATE, TAG, "invalid panel IO");

    uint8_t command = on_off ? LCD_CMD_DISPON : LCD_CMD_DISPOFF;
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, command, NULL, 0), TAG, "send command failed");

    if (on_off) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    return ESP_OK;
}

static esp_err_t panel_st7121_sleep(esp_lcd_panel_t *panel, bool sleep)
{
    st7121_panel_t *st7121       = (st7121_panel_t *)panel->user_data;
    esp_lcd_panel_io_handle_t io = st7121->io;
    ESP_RETURN_ON_FALSE(io, ESP_ERR_INVALID_STATE, TAG, "invalid panel IO");

    uint8_t command = sleep ? LCD_CMD_SLPIN : LCD_CMD_SLPOUT;
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, command, NULL, 0), TAG, "send command failed");
    vTaskDelay(pdMS_TO_TICKS(120));

    return ESP_OK;
}

#endif  // SOC_MIPI_DSI_SUPPORTED
