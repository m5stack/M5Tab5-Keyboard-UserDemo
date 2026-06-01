/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#include "variants/m5tab5_variant_registry.h"
#include "drivers/m5tab5_driver_common.h"
#include "m5tab5_pinmap.h"

#include "i2c_bus.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_io.h"
#include "esp_ldo_regulator.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace m5::tab5 {

static const char* TAG = "m5tab5.autodetect";

namespace {

constexpr uint8_t TAB5_EXTIO_ADDR_LOW      = 0x43;
constexpr uint8_t TAB5_EXTIO_REG_IO_DIR    = 0x03;
constexpr uint8_t TAB5_EXTIO_REG_OUT_STATE = 0x05;
constexpr uint8_t TAB5_EXTIO_REG_OUT_HI_Z  = 0x07;

constexpr uint8_t GT911_ADDR_PRIMARY   = 0x14;
constexpr uint8_t GT911_ADDR_SECONDARY = 0x5D;

constexpr uint8_t TDDI_TOUCH_ADDR            = 0x55;
constexpr uint16_t TDDI_TOUCH_FW_VERSION_REG = 0x0000;
constexpr uint8_t ST7121_TOUCH_FW_VERSION    = 1;
constexpr uint8_t ST7123_TOUCH_FW_VERSION    = 3;

constexpr int TDDI_DSI_BUS_ID    = 0;
constexpr int TDDI_DSI_LANE_NUM  = 2;
constexpr int TDDI_DSI_LANE_MBPS = 1040;
constexpr int TDDI_DSI_LDO_CHAN  = 3;
constexpr int TDDI_DSI_LDO_MV    = 2500;

esp_err_t write_extio_reg(i2c_bus_handle_t bus, uint8_t reg, uint8_t value, TickType_t delay_ticks = pdMS_TO_TICKS(10))
{
    if (bus == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_bus_device_handle_t dev = i2c_bus_device_create(bus, TAB5_EXTIO_ADDR_LOW, 400000);
    if (dev == nullptr) {
        return ESP_ERR_NOT_FOUND;
    }

    esp_err_t err = i2c_bus_write_byte(dev, reg, value);
    i2c_bus_device_delete(&dev);
    if (err == ESP_OK && delay_ticks > 0) {
        vTaskDelay(delay_ticks);
    }
    return err;
}

esp_err_t apply_common_tddi_prerequisites(i2c_bus_handle_t bus)
{
    // Same as M5Autodetect Tab5 ST712x i2c_internal prerequisites:
    // IO_DIR=0x30, OUT_STATE=0x30, OUT_HI_Z=0x00 releases LCD_RST and TP_RST.
    ESP_RETURN_ON_ERROR(write_extio_reg(bus, TAB5_EXTIO_REG_IO_DIR, 0x30), TAG, "set IO_DIR failed");
    ESP_RETURN_ON_ERROR(write_extio_reg(bus, TAB5_EXTIO_REG_OUT_STATE, 0x30), TAG, "set OUT_STATE failed");
    ESP_RETURN_ON_ERROR(write_extio_reg(bus, TAB5_EXTIO_REG_OUT_HI_Z, 0x00), TAG, "set OUT_HI_Z failed");
    return ESP_OK;
}

bool probe_i2c_ack(uint8_t addr)
{
    i2c_master_bus_handle_t master_bus = m5tab5_get_sys_i2c_master_bus_handle();
    if (master_bus == nullptr) {
        return false;
    }
    return i2c_master_probe(master_bus, addr, 100) == ESP_OK;
}

bool probe_gt911(i2c_bus_handle_t bus)
{
    gpio_config_t int_high_cfg = {};
    int_high_cfg.pin_bit_mask  = 1ULL << M5TAB5_PIN_LCD_TOUCH_INT;
    int_high_cfg.mode          = GPIO_MODE_OUTPUT;
    int_high_cfg.pull_up_en    = GPIO_PULLUP_DISABLE;
    int_high_cfg.pull_down_en  = GPIO_PULLDOWN_DISABLE;
    int_high_cfg.intr_type     = GPIO_INTR_DISABLE;
    gpio_config(&int_high_cfg);
    gpio_set_level(M5TAB5_PIN_LCD_TOUCH_INT, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    esp_err_t prereq_err = apply_common_tddi_prerequisites(bus);
    if (prereq_err != ESP_OK) {
        ESP_LOGW(TAG, "GT911 prereq failed: %s", esp_err_to_name(prereq_err));
    }

    if (probe_i2c_ack(GT911_ADDR_PRIMARY)) {
        ESP_LOGI(TAG, "Detected GT911 touch controller at address 0x%02X.", GT911_ADDR_PRIMARY);
        gpio_reset_pin(M5TAB5_PIN_LCD_TOUCH_INT);
        return true;
    }
    if (probe_i2c_ack(GT911_ADDR_SECONDARY)) {
        ESP_LOGI(TAG, "Detected GT911 touch controller at address 0x%02X.", GT911_ADDR_SECONDARY);
        gpio_reset_pin(M5TAB5_PIN_LCD_TOUCH_INT);
        return true;
    }

    gpio_reset_pin(M5TAB5_PIN_LCD_TOUCH_INT);
    return false;
}

esp_err_t begin_tddi_dsi_probe(esp_ldo_channel_handle_t* out_phy_pwr, esp_lcd_dsi_bus_handle_t* out_dsi_bus,
                               esp_lcd_panel_io_handle_t* out_dbi_io)
{
    if (out_phy_pwr == nullptr || out_dsi_bus == nullptr || out_dbi_io == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    *out_phy_pwr  = nullptr;
    *out_dsi_bus  = nullptr;
    *out_dbi_io   = nullptr;
    esp_err_t ret = ESP_OK;

    esp_ldo_channel_config_t ldo_cfg = {};
    esp_lcd_dsi_bus_config_t bus_cfg = {};
    esp_lcd_dbi_io_config_t dbi_cfg  = {};

    ldo_cfg.chan_id    = TDDI_DSI_LDO_CHAN;
    ldo_cfg.voltage_mv = TDDI_DSI_LDO_MV;
    ESP_RETURN_ON_ERROR(esp_ldo_acquire_channel(&ldo_cfg, out_phy_pwr), TAG, "acquire DSI LDO failed");

    bus_cfg.bus_id             = TDDI_DSI_BUS_ID;
    bus_cfg.num_data_lanes     = TDDI_DSI_LANE_NUM;
    bus_cfg.phy_clk_src        = MIPI_DSI_PHY_CLK_SRC_DEFAULT;
    bus_cfg.lane_bit_rate_mbps = TDDI_DSI_LANE_MBPS;
    ESP_GOTO_ON_ERROR(esp_lcd_new_dsi_bus(&bus_cfg, out_dsi_bus), err, TAG, "new DSI bus failed");

    dbi_cfg.virtual_channel = 0;
    dbi_cfg.lcd_cmd_bits    = 8;
    dbi_cfg.lcd_param_bits  = 8;
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_dbi(*out_dsi_bus, &dbi_cfg, out_dbi_io), err, TAG, "new DBI IO failed");

    esp_lcd_panel_io_tx_param(*out_dbi_io, 0x01, nullptr, 0);
    vTaskDelay(pdMS_TO_TICKS(120));
    return ESP_OK;

err:
    if (*out_dbi_io != nullptr) {
        esp_lcd_panel_io_del(*out_dbi_io);
        *out_dbi_io = nullptr;
    }
    if (*out_dsi_bus != nullptr) {
        esp_lcd_del_dsi_bus(*out_dsi_bus);
        *out_dsi_bus = nullptr;
    }
    if (*out_phy_pwr != nullptr) {
        esp_ldo_release_channel(*out_phy_pwr);
        *out_phy_pwr = nullptr;
    }
    return ret;
}

esp_err_t read_tddi_touch_version_panel_io(uint8_t* version)
{
    if (version == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_master_bus_handle_t master_bus = m5tab5_get_sys_i2c_master_bus_handle();
    if (master_bus == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_lcd_panel_io_handle_t io         = nullptr;
    esp_lcd_panel_io_i2c_config_t io_cfg = {};
    io_cfg.dev_addr                      = TDDI_TOUCH_ADDR;
    io_cfg.scl_speed_hz                  = 400000;
    io_cfg.control_phase_bytes           = 1;
    io_cfg.lcd_cmd_bits                  = 16;
    io_cfg.flags.disable_control_phase   = 1;

    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(master_bus, &io_cfg, &io), TAG, "new TDDI touch panel IO failed");
    esp_err_t err = esp_lcd_panel_io_rx_param(io, TDDI_TOUCH_FW_VERSION_REG, version, 1);
    esp_lcd_panel_io_del(io);
    return err;
}

esp_err_t read_tddi_touch_version_raw_i2c(i2c_bus_handle_t bus, uint8_t* version)
{
    if (bus == nullptr || version == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_bus_device_handle_t dev = i2c_bus_device_create(bus, TDDI_TOUCH_ADDR, 400000);
    if (dev == nullptr) {
        return ESP_ERR_NOT_FOUND;
    }

    esp_err_t err = i2c_bus_read_reg16(dev, TDDI_TOUCH_FW_VERSION_REG, 1, version);
    i2c_bus_device_delete(&dev);
    return err;
}

esp_err_t read_tddi_touch_version(i2c_bus_handle_t bus, uint8_t* version, const char** read_path)
{
    esp_err_t err = read_tddi_touch_version_panel_io(version);
    if (err == ESP_OK) {
        if (read_path != nullptr) {
            *read_path = "panel-io";
        }
        return ESP_OK;
    }

    ESP_LOGD(TAG, "TDDI panel-io version read failed: %s", esp_err_to_name(err));
    err = read_tddi_touch_version_raw_i2c(bus, version);
    if (err == ESP_OK && read_path != nullptr) {
        *read_path = "raw-i2c";
    }
    return err;
}

}  // namespace

const m5tab5_variant_descriptor_t* m5tab5_autodetect_variant_descriptor()
{
    ESP_LOGI(TAG, "Starting automatic hardware variant detection...");

    // 1. Get/Initialize SYS I2C bus
    i2c_bus_handle_t bus = m5tab5_get_sys_i2c_bus();
    if (!bus) {
        ESP_LOGE(TAG, "SYS I2C bus not available. Defaulting to reference (ILI9881+GT911).");
        return m5tab5_get_reference_variant_descriptor();
    }

    // 2. Probe for GT911 Touch screen controller at address 0x14 or 0x5D.
    // M5Autodetect treats GT911 as an I2C ACK signal and first drives INT high
    // while releasing LCD_RST/TP_RST through the PI4IOE5V6408.
    if (probe_gt911(bus)) {
        ESP_LOGI(TAG, "Variant resolved: ILI9881 LCD + GT911 Touch (Reference)");
        return m5tab5_get_reference_variant_descriptor();
    }

    // 3. Not GT911 -> TDDI Screen. We need to distinguish ST7121 vs ST7123.
    // M5Autodetect identifies them by reading FW_VERSION reg 0x0000 from touch addr 0x55.
    ESP_LOGI(TAG, "GT911 not found. Attempting to identify ST7121/ST7123 TDDI variants...");

    esp_err_t prereq_err = apply_common_tddi_prerequisites(bus);
    if (prereq_err != ESP_OK) {
        ESP_LOGW(TAG, "TDDI prereq failed: %s", esp_err_to_name(prereq_err));
    }

    esp_ldo_channel_handle_t phy_pwr_chan = nullptr;
    esp_lcd_dsi_bus_handle_t dsi_bus      = nullptr;
    esp_lcd_panel_io_handle_t dbi_io      = nullptr;
    esp_err_t dsi_err                     = begin_tddi_dsi_probe(&phy_pwr_chan, &dsi_bus, &dbi_io);
    if (dsi_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize DSI probe path. Defaulting to ST7123.");
        return m5tab5_get_lcd_st7123_touch_st7123_variant_descriptor();
    }

    uint8_t touch_ver     = 0;
    const char* read_path = nullptr;
    bool read_success     = read_tddi_touch_version(bus, &touch_ver, &read_path) == ESP_OK;
    if (read_success) {
        ESP_LOGI(TAG, "Read TDDI touch FW version: 0x%02X via %s", touch_ver, read_path);
    }

    // Clean up temporary DSI probe resources
    if (dbi_io) esp_lcd_panel_io_del(dbi_io);
    if (dsi_bus) esp_lcd_del_dsi_bus(dsi_bus);
    if (phy_pwr_chan) esp_ldo_release_channel(phy_pwr_chan);

    if (read_success) {
        if (touch_ver == ST7121_TOUCH_FW_VERSION) {
            ESP_LOGI(TAG, "Variant resolved: ST7121 LCD + ST7121 Touch");
            return m5tab5_get_lcd_st7121_touch_st7121_variant_descriptor();
        } else if (touch_ver == ST7123_TOUCH_FW_VERSION) {
            ESP_LOGI(TAG, "Variant resolved: ST7123 LCD + ST7123 Touch");
            return m5tab5_get_lcd_st7123_touch_st7123_variant_descriptor();
        }
    }

    ESP_LOGW(TAG, "Touch probe inconclusive. Defaulting to ST7123.");
    return m5tab5_get_lcd_st7123_touch_st7123_variant_descriptor();
}

}  // namespace m5::tab5
