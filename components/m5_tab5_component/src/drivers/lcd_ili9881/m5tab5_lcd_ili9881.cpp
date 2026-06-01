/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#include "drivers/lcd_ili9881/m5tab5_lcd_ili9881.h"
#include "drivers/lcd_ili9881/esp_lcd_ili9881c.h"

#include "drivers/m5tab5_driver_common.h"
#include "m5tab5_pinmap.h"

#include "esp_check.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_ldo_regulator.h"
#include "driver/ledc.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

namespace m5::tab5 {
namespace {

constexpr int MIPI_DSI_PHY_PWR_LDO_CHAN        = 3;
constexpr int MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV  = 2500;
constexpr ledc_channel_t LCD_BACKLIGHT_LEDC_CH = LEDC_CHANNEL_1;
constexpr uint32_t LCD_BACKLIGHT_DUTY_ON       = (1U << 12) - 1;

static m5tab5_lcd_ili9881_handles_t s_handles;

esp_err_t enable_dsi_phy_power()
{
    static esp_ldo_channel_handle_t phy_pwr_chan = nullptr;
    if (phy_pwr_chan) return ESP_OK;

    esp_ldo_channel_config_t ldo_cfg = {
        .chan_id    = MIPI_DSI_PHY_PWR_LDO_CHAN,
        .voltage_mv = MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV,
    };
    return esp_ldo_acquire_channel(&ldo_cfg, &phy_pwr_chan);
}

esp_err_t backlight_init()
{
    const ledc_timer_config_t timer_cfg = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_12_BIT,
        .timer_num       = LEDC_TIMER_0,
        .freq_hz         = 5000,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_cfg), m5tab5_driver_log_tag(), "ledc timer config failed");

    const ledc_channel_config_t ch_cfg = {
        .gpio_num   = M5TAB5_PIN_LCD_BACKLIGHT,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LCD_BACKLIGHT_LEDC_CH,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&ch_cfg), m5tab5_driver_log_tag(), "ledc channel config failed");
    ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, LCD_BACKLIGHT_LEDC_CH, LCD_BACKLIGHT_DUTY_ON),
                        m5tab5_driver_log_tag(), "ledc set duty failed");
    return ledc_update_duty(LEDC_LOW_SPEED_MODE, LCD_BACKLIGHT_LEDC_CH);
}

esp_err_t m5tab5_lcd_ili9881_init(m5tab5_runtime_t* runtime)
{
    const char* TAG = m5tab5_driver_log_tag();

    // 1. Backlight / 背光
    ESP_RETURN_ON_ERROR(backlight_init(), TAG, "backlight init failed");

    // 2. DSI PHY power / DSI PHY 供电
    ESP_RETURN_ON_ERROR(enable_dsi_phy_power(), TAG, "DSI PHY power failed");

    // 3. Create MIPI DSI bus / 创建 MIPI DSI 总线
    esp_lcd_dsi_bus_config_t bus_cfg = ILI9881C_PANEL_BUS_DSI_2CH_CONFIG();
    ESP_RETURN_ON_ERROR(esp_lcd_new_dsi_bus(&bus_cfg, &s_handles.mipi_dsi_bus), TAG, "new DSI bus failed");

    // 4. Create DBI panel IO / 创建 DBI 面板 IO
    esp_lcd_dbi_io_config_t dbi_cfg = ILI9881C_PANEL_IO_DBI_CONFIG();
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_dbi(s_handles.mipi_dsi_bus, &dbi_cfg, &s_handles.io), TAG,
                        "new panel IO failed");

    // 5. Create the ILI9881C panel / 创建 ILI9881C 面板
    esp_lcd_dpi_panel_config_t dpi_cfg = ILI9881C_720_1280_PANEL_60HZ_DPI_CONFIG(LCD_COLOR_FMT_RGB565);

    ili9881c_vendor_config_t vendor_cfg = {
        .init_cmds      = nullptr,  // use built-in default / 使用内置默认初始化序�?
        .init_cmds_size = 0,
        .mipi_config =
            {
                .dsi_bus    = s_handles.mipi_dsi_bus,
                .dpi_config = &dpi_cfg,
                .lane_num   = ILI9881C_LANE_NUM,
            },
    };

    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = GPIO_NUM_NC,  // LCD_RST controlled via IO expander / LCD_RST �?IO 扩展器控�?
        .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB,
        .data_endian    = LCD_RGB_DATA_ENDIAN_BIG,
        .bits_per_pixel = 16,
        .vendor_config  = &vendor_cfg,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_ili9881c(s_handles.io, &panel_cfg, &s_handles.panel), TAG,
                        "new ili9881c panel failed");

    // 6. Reset -> Init -> Display ON / 复位 -> 初始�?-> 打开显示
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_handles.panel), TAG, "panel reset failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_handles.panel), TAG, "panel init failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(s_handles.panel, true), TAG, "display on failed");

    ESP_LOGI(TAG, "ILI9881C 720x1280 MIPI-DSI display initialized");

    runtime->display_handle = &s_handles;
    return ESP_OK;
}

const m5tab5_display_driver_descriptor_t k_m5tab5_lcd_ili9881_driver = {
    .id   = "m5tab5.lcd.ili9881",
    .init = m5tab5_lcd_ili9881_init,
};

}  // namespace

const m5tab5_display_driver_descriptor_t* m5tab5_get_lcd_ili9881_driver()
{
    return &k_m5tab5_lcd_ili9881_driver;
}

}  // namespace m5::tab5

#pragma GCC diagnostic pop