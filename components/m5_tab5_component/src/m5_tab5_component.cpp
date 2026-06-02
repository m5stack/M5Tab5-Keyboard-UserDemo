/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#include "m5_tab5_component.h"
#include "m5tab5_tools.h"

#include "sdkconfig.h"

#include "esp_check.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "drivers/m5tab5_driver_common.h"
#include "variants/m5tab5_variant_registry.h"
#include "drivers/extio_pi4ioe5v6408/m5tab5_extio_pi4ioe5v6408.h"
#include "drivers/lcd_ili9881/m5tab5_lcd_ili9881.h"
#include "drivers/lcd_st7121/m5tab5_lcd_st7121.h"
#include "drivers/lcd_st7123/m5tab5_lcd_st7123.h"
#include "drivers/rtc_rx8130/m5tab5_rtc_rx8130.h"
#include "drivers/ina226/m5tab5_ina226.h"
#include "drivers/touch_gt911/m5tab5_touch_gt911.h"
#include "drivers/touch_st7121/m5tab5_touch_st7121.h"
#include "drivers/touch_st7123/m5tab5_touch_st7123.h"

#include <new>

namespace m5::tab5 {
namespace {

const m5tab5_variant_descriptor_t* m5tab5_resolve_variant(const m5tab5_component_config_t& config)
{
    if (config.variant_id != M5TAB5_VARIANT_AUTO) {
        return m5tab5_get_variant_descriptor(config.variant_id);
    }

#if CONFIG_M5_TAB5_VARIANT_SELECT_REFERENCE
    return m5tab5_get_default_variant_descriptor();
#else
    return m5tab5_detect_variant_descriptor();
#endif
}

esp_err_t m5tab5_init_display(const m5tab5_variant_descriptor_t* variant, m5tab5_runtime_t* runtime)
{
    if (variant == nullptr || variant->display == nullptr || variant->display->init == nullptr) {
        return ESP_ERR_NOT_SUPPORTED;
    }

    return variant->display->init(runtime);
}

esp_err_t m5tab5_init_optional_driver(const m5tab5_device_driver_descriptor_t* driver, m5tab5_runtime_t* runtime)
{
    if (driver == nullptr) {
        return ESP_OK;
    }
    if (driver->probe != nullptr) {
        esp_err_t err = driver->probe(runtime);
        if (err != ESP_OK) {
            return err;
        }
    }
    if (driver->init == nullptr) {
        return ESP_OK;
    }
    return driver->init(runtime);
}

esp_err_t m5tab5_pulse_display_reset(m5tab5_runtime_t* runtime)
{
    const char* TAG = m5tab5_driver_log_tag();

    if (runtime == nullptr || runtime->ioexpander_handle == nullptr) {
        return ESP_OK;
    }

    auto* dev = static_cast<m5tab5_extio_pi4ioe5v6408_t*>(runtime->ioexpander_handle);

    // Match UserDemo bsp_reset_tp() exactly:
    //   gpio_reset_pin(GPIO23) ?GPIO_MODE_DISABLE + internal pullup
    //   TP_RST LOW 100ms ?TP_RST HIGH ?wait 100ms
    // GPIO23 floats HIGH via internal pullup ?GT911 latches address 0x14.
    // INT SYNC is NOT done here; it will be done in exit_sleep after GT911 init.
    // ?UserDemo bsp_reset_tp() 完全一致：
    //   GPIO23 通过内部上拉浮空高电??GT911 锁定地址 0x14
    //   INT SYNC 不在这里做，而是?GT911 init 后的 exit_sleep 中执行?
    gpio_reset_pin(M5TAB5_PIN_LCD_TOUCH_INT);

    ESP_LOGI(TAG, "reset pulse: LCD_RST and TP_RST before display init");
    ESP_RETURN_ON_ERROR(m5tab5_extio_pi4ioe5v6408_write_pin(
                            dev, static_cast<M5TAB5_ExtIo_PI4IOE5V6408_Pin>(M5TAB5_EXTIO_ADDR_LOW_LCD_RST), false),
                        TAG, "drive LCD_RST low failed");
    ESP_RETURN_ON_ERROR(m5tab5_extio_pi4ioe5v6408_write_pin(
                            dev, static_cast<M5TAB5_ExtIo_PI4IOE5V6408_Pin>(M5TAB5_EXTIO_ADDR_LOW_TP_RST), false),
                        TAG, "drive TP_RST low failed");
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_RETURN_ON_ERROR(m5tab5_extio_pi4ioe5v6408_write_pin(
                            dev, static_cast<M5TAB5_ExtIo_PI4IOE5V6408_Pin>(M5TAB5_EXTIO_ADDR_LOW_LCD_RST), true),
                        TAG, "drive LCD_RST high failed");
    ESP_RETURN_ON_ERROR(m5tab5_extio_pi4ioe5v6408_write_pin(
                            dev, static_cast<M5TAB5_ExtIo_PI4IOE5V6408_Pin>(M5TAB5_EXTIO_ADDR_LOW_TP_RST), true),
                        TAG, "drive TP_RST high failed");
    vTaskDelay(pdMS_TO_TICKS(100));

    return ESP_OK;
}

}  // namespace

esp_err_t m5tab5_component::begin(const m5tab5_component_config_t& config)
{
    active_config_  = config;
    runtime_        = {};
    active_variant_ = m5tab5_resolve_variant(config);
    if (active_variant_ == nullptr) {
        return ESP_ERR_NOT_FOUND;
    }

    esp_err_t err = ESP_OK;

#if CONFIG_M5_TAB5_ENABLE_OPTIONAL_DRIVER_INIT
    // Initialize the IO expander before the display because PI4IOE5V6408 controls LCD_RST (P4)
    // and TP_RST (P5) via its output pins.
    // 必须先初始化 IO 扩展器，再初始化显示，因?PI4IOE5V6408 通过输出引脚控制 LCD_RST（P4）和 TP_RST（P5）?
    // The LCD must not be initialized while those lines are floating or held low by the chip's
    // power-on Hi-Z default.
    // 当这些线仍处于悬空状态，或因芯片上电默认高阻而被拉低时，不能初始?LCD?
    if (config.enable_optional_drivers) {
        err = m5tab5_init_optional_driver(active_variant_->ioexpander, &runtime_);
        if (err != ESP_OK) {
            return err;
        }

        err = m5tab5_pulse_display_reset(&runtime_);
        if (err != ESP_OK) {
            return err;
        }
    }
#endif

    err = m5tab5_init_display(active_variant_, &runtime_);
    if (err != ESP_OK) {
        return err;
    }

#if !CONFIG_M5_TAB5_ENABLE_OPTIONAL_DRIVER_INIT
    return ESP_OK;
#endif

    if (!config.enable_optional_drivers) {
        return ESP_OK;
    }

    // Initialize touch after the LCD because ST7123 is a TDDI chip whose I2C touch interface
    // becomes active only after the DSI display has been initialized.
    // 触摸初始化必须放?LCD 之后，因?ST7123 属于 TDDI 芯片，只有在 DSI 显示初始化完成后，其 I2C 触摸接口才会生效?
    err = m5tab5_init_optional_driver(active_variant_->touch, &runtime_);
    if (err != ESP_OK) {
        return err;
    }

    return ESP_OK;
}

const m5tab5_variant_descriptor_t* m5tab5_component::variant() const
{
    return active_variant_;
}

const m5tab5_component_config_t& m5tab5_component::config() const
{
    return active_config_;
}

const m5tab5_runtime_t& m5tab5_component::runtime() const
{
    return runtime_;
}

esp_lcd_panel_handle_t m5tab5_component::lcd_panel() const
{
    if (runtime_.display_handle == nullptr || active_variant_ == nullptr) {
        return nullptr;
    }

    switch (active_variant_->variant_id) {
        case M5TAB5_VARIANT_TAB5_LCD_ILI9881_TOUCH_GT911:
            return static_cast<const m5tab5_lcd_ili9881_handles_t*>(runtime_.display_handle)->panel;
        case M5TAB5_VARIANT_TAB5_LCD_ST7121_TOUCH_ST7121:
            return static_cast<const m5tab5_lcd_st7121_handles_t*>(runtime_.display_handle)->panel;
        case M5TAB5_VARIANT_TAB5_LCD_ST7123_TOUCH_ST7123:
            return static_cast<const m5tab5_lcd_st7123_handles_t*>(runtime_.display_handle)->panel;
        default:
            return nullptr;
    }
}

esp_lcd_touch_handle_t m5tab5_component::touch_panel() const
{
    if (runtime_.touch_handle == nullptr || active_variant_ == nullptr) {
        return nullptr;
    }

    switch (active_variant_->variant_id) {
        case M5TAB5_VARIANT_TAB5_LCD_ILI9881_TOUCH_GT911:
            return static_cast<const m5tab5_touch_gt911_handles_t*>(runtime_.touch_handle)->tp;
        case M5TAB5_VARIANT_TAB5_LCD_ST7121_TOUCH_ST7121:
            return static_cast<const m5tab5_touch_st7121_handles_t*>(runtime_.touch_handle)->tp;
        case M5TAB5_VARIANT_TAB5_LCD_ST7123_TOUCH_ST7123:
            return static_cast<const m5tab5_touch_st7123_handles_t*>(runtime_.touch_handle)->tp;
        default:
            return nullptr;
    }
}

esp_err_t m5tab5_component::lvgl_init()
{
    if (active_variant_ == nullptr || runtime_.display_handle == nullptr) {
        ESP_LOGE(m5tab5_driver_log_tag(), "lvgl_init: component not initialized -- call begin() first");
        return ESP_ERR_INVALID_STATE;
    }

    return m5tab5_lvgl_init(runtime_, &lv_display_, &lv_touch_indev_);
}

esp_err_t m5tab5_component::wlan_power(bool enable)
{
    static const char* TAG = "m5tab5.wlan";

    if (runtime_.ioexpander_high_handle == nullptr) {
        ESP_LOGE(TAG, "ioexpander_high_handle not available -- call begin() with enable_optional_drivers=true first");
        return ESP_ERR_INVALID_STATE;
    }

    auto* dev     = static_cast<m5tab5_extio_pi4ioe5v6408_t*>(runtime_.ioexpander_high_handle);
    esp_err_t ret = m5tab5_extio_pi4ioe5v6408_set_pin_mode(
        dev, static_cast<M5TAB5_ExtIo_PI4IOE5V6408_Pin>(M5TAB5_EXTIO_ADDR_HIGH_WLAN_PWR_EN),
        M5TAB5_EXTIO_PI4IOE5V6408_PIN_OUTPUT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "set_pin_mode WLAN_PWR_EN failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = m5tab5_extio_pi4ioe5v6408_write_pin(
        dev, static_cast<M5TAB5_ExtIo_PI4IOE5V6408_Pin>(M5TAB5_EXTIO_ADDR_HIGH_WLAN_PWR_EN), enable);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "WLAN_PWR_EN -> %s", enable ? "ON" : "OFF");
    }
    return ret;
}

esp_err_t m5tab5_component::ext5v_enable(bool enable)
{
    static const char* TAG = "m5tab5.ext5v";

    if (runtime_.ioexpander_handle == nullptr) {
        ESP_LOGE(TAG, "ioexpander_handle not available -- call begin() with enable_optional_drivers=true first");
        return ESP_ERR_INVALID_STATE;
    }
    auto* dev     = static_cast<m5tab5_extio_pi4ioe5v6408_t*>(runtime_.ioexpander_handle);
    esp_err_t ret = m5tab5_extio_pi4ioe5v6408_write_pin(
        dev, static_cast<M5TAB5_ExtIo_PI4IOE5V6408_Pin>(M5TAB5_EXTIO_ADDR_LOW_EXT5V_EN), enable);
    if (ret == ESP_OK) ESP_LOGI(TAG, "EXT5V (Grove 5V rail) -> %s", enable ? "ON" : "OFF");
    return ret;
}

esp_err_t m5tab5_component::usb5v_enable(bool enable)
{
    static const char* TAG = "m5tab5.usb5v";

    if (runtime_.ioexpander_high_handle == nullptr) {
        ESP_LOGE(TAG, "ioexpander_high_handle not available -- call begin() first");
        return ESP_ERR_INVALID_STATE;
    }
    auto* dev     = static_cast<m5tab5_extio_pi4ioe5v6408_t*>(runtime_.ioexpander_high_handle);
    esp_err_t ret = m5tab5_extio_pi4ioe5v6408_write_pin(
        dev, static_cast<M5TAB5_ExtIo_PI4IOE5V6408_Pin>(M5TAB5_EXTIO_ADDR_HIGH_USB5V_EN), enable);
    if (ret == ESP_OK) ESP_LOGI(TAG, "USB5V_EN -> %s", enable ? "ON" : "OFF");
    return ret;
}

esp_err_t m5tab5_component::set_charge_fast(bool fast)
{
    static const char* TAG = "m5tab5.chg";

    if (runtime_.ioexpander_high_handle == nullptr) {
        ESP_LOGE(TAG, "ioexpander_high_handle not available -- call begin() first");
        return ESP_ERR_INVALID_STATE;
    }
    auto* dev = static_cast<m5tab5_extio_pi4ioe5v6408_t*>(runtime_.ioexpander_high_handle);
    // nCHG_QC_EN is active LOW: LOW means fast charging (QC), HIGH means slow or standard charging.
    // nCHG_QC_EN 为低电平有效：LOW 表示快速充电（QC），HIGH 表示慢速或标准充电?
    esp_err_t ret = m5tab5_extio_pi4ioe5v6408_write_pin(
        dev, static_cast<M5TAB5_ExtIo_PI4IOE5V6408_Pin>(M5TAB5_EXTIO_ADDR_HIGH_NCHG_QC_EN),
        !fast);  // fast=true -> pin LOW; fast=false -> pin HIGH / fast=true 时输?LOW，fast=false 时输?HIGH
    if (ret == ESP_OK) ESP_LOGI(TAG, "charge mode -> %s", fast ? "FAST (QC)" : "SLOW (standard)");
    return ret;
}

esp_err_t m5tab5_component::charge_enable(bool enable)
{
    static const char* TAG = "m5tab5.chg";

    if (runtime_.ioexpander_high_handle == nullptr) {
        ESP_LOGE(TAG, "ioexpander_high_handle not available -- call begin() first");
        return ESP_ERR_INVALID_STATE;
    }
    auto* dev = static_cast<m5tab5_extio_pi4ioe5v6408_t*>(runtime_.ioexpander_high_handle);
    esp_err_t ret = m5tab5_extio_pi4ioe5v6408_write_pin(
        dev, static_cast<M5TAB5_ExtIo_PI4IOE5V6408_Pin>(M5TAB5_EXTIO_ADDR_HIGH_CHG_EN), enable);
    if (ret == ESP_OK) ESP_LOGI(TAG, "CHG_EN -> %s", enable ? "ENABLED" : "DISABLED");
    return ret;
}

esp_err_t m5tab5_component::set_rf_antenna(m5tab5_rf_antenna_t ant)
{
    if (runtime_.ioexpander_handle == nullptr) return ESP_ERR_INVALID_STATE;
    auto* dev     = static_cast<m5tab5_extio_pi4ioe5v6408_t*>(runtime_.ioexpander_handle);
    bool high     = (ant == M5TAB5_RF_ANTENNA_EXTERNAL);
    esp_err_t ret = m5tab5_extio_pi4ioe5v6408_write_pin(
        dev, static_cast<M5TAB5_ExtIo_PI4IOE5V6408_Pin>(M5TAB5_EXTIO_ADDR_LOW_RF_PTH_L_INT_H_EXT), high);
    if (ret == ESP_OK) ESP_LOGI("m5tab5.rf", "RF antenna -> %s", high ? "EXTERNAL" : "INTERNAL");
    return ret;
}

// ── RX8130 RTC / RX8130 实时时钟 ────────────────────────────────────────────

esp_err_t m5tab5_component::rtc_init()
{
    static const char* TAG = "m5tab5.rtc";

    if (rtc_handle_) {
        // Already initialized; keep the API idempotent. / 已经初始化，保持接口幂等?
        return ESP_OK;
    }

    i2c_bus_handle_t bus = m5tab5_get_sys_i2c_bus();
    if (!bus) {
        ESP_LOGE(TAG, "SYS I2C bus not available ?call begin() first");
        return ESP_ERR_INVALID_STATE;
    }

    auto* ctx = new (std::nothrow) m5tab5_rtc_rx8130_t{};
    if (!ctx) return ESP_ERR_NO_MEM;

    esp_err_t err = m5tab5_rtc_rx8130_init(bus, ctx);
    if (err != ESP_OK) {
        delete ctx;
        return err;
    }

    rtc_handle_ = ctx;
    return ESP_OK;
}

esp_err_t m5tab5_component::rtc_get_datetime(m5tab5_rtc_datetime_t* out) const
{
    if (!rtc_handle_) return ESP_ERR_INVALID_STATE;
    return m5tab5_rtc_rx8130_get_datetime(static_cast<const m5tab5_rtc_rx8130_t*>(rtc_handle_), out);
}

esp_err_t m5tab5_component::rtc_set_datetime(const m5tab5_rtc_datetime_t* dt)
{
    if (!rtc_handle_) return ESP_ERR_INVALID_STATE;
    return m5tab5_rtc_rx8130_set_datetime(static_cast<m5tab5_rtc_rx8130_t*>(rtc_handle_), dt);
}

bool m5tab5_component::rtc_volt_low() const
{
    if (!rtc_handle_) return false;
    return m5tab5_rtc_rx8130_volt_low(static_cast<const m5tab5_rtc_rx8130_t*>(rtc_handle_));
}
// ── Power Management / 电源管理 ─────────────────────────────────────────────

void m5tab5_component::power_off()
{
    static const char* TAG = "m5tab5.power";

    ESP_LOGI(TAG, "power_off: toggling PWROFF_PLUSE (ADDR_HIGH 0x44 P4) x10");

    if (runtime_.ioexpander_high_handle != nullptr) {
        auto* dev = static_cast<m5tab5_extio_pi4ioe5v6408_t*>(runtime_.ioexpander_high_handle);
        for (int i = 0; i < 10; ++i) {
            m5tab5_extio_pi4ioe5v6408_write_pin(
                dev, static_cast<M5TAB5_ExtIo_PI4IOE5V6408_Pin>(M5TAB5_EXTIO_ADDR_HIGH_PWROFF_PLUSE),
                static_cast<bool>(i & 1));
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    } else {
        ESP_LOGW(TAG, "power_off: IO expander not available, entering deep sleep");
    }

    esp_deep_sleep_start();
    // Never reached. / 不会执行到这里?
    __builtin_unreachable();
}

void m5tab5_component::deep_sleep(uint64_t wakeup_us)
{
    if (wakeup_us > 0) {
        esp_sleep_enable_timer_wakeup(wakeup_us);
    } else {
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
    }
    esp_deep_sleep_start();
    // Never reached. / 不会执行到这里?
    __builtin_unreachable();
}

void m5tab5_component::light_sleep(uint64_t wakeup_us)
{
    if (wakeup_us > 0) {
        esp_sleep_enable_timer_wakeup(wakeup_us);
    } else {
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
    }
    esp_light_sleep_start();
}
// ── INA226 Power Monitor / INA226 电压电流监测 ─────────────────────────────

esp_err_t m5tab5_component::ina226_init()
{
    static const char* TAG = "m5tab5.ina226";

    if (ina226_handle_) {
        return ESP_OK;  // Idempotent / 幂等返回
    }

    i2c_bus_handle_t bus = m5tab5_get_sys_i2c_bus();
    if (!bus) {
        ESP_LOGE(TAG, "SYS I2C bus not available ?call begin() first");
        return ESP_ERR_INVALID_STATE;
    }

    auto* ctx = new (std::nothrow) m5tab5_ina226_t{};
    if (!ctx) return ESP_ERR_NO_MEM;

    m5tab5_ina226_config_t cfg = {};
    cfg.bus                    = bus;
    cfg.i2c_addr               = INA226_I2C_ADDR_TAB5;
    cfg.freq_hz                = 400000;
    cfg.shunt_ohms             = 0.005f;
    cfg.max_current_a          = 2.0f;
    cfg.averaging              = Ina226Avg::AVG_16;
    cfg.bus_ct                 = Ina226ConvTime::MS_1_1;
    cfg.shunt_ct               = Ina226ConvTime::MS_1_1;
    cfg.mode                   = Ina226Mode::SHUNT_BUS_CONT;

    esp_err_t err = m5tab5_ina226_init(&cfg, ctx);
    if (err != ESP_OK) {
        delete ctx;
        return err;
    }

    ina226_handle_ = ctx;
    return ESP_OK;
}

esp_err_t m5tab5_component::ina226_read(ina226_reading_t* out)
{
    if (!ina226_handle_ || !out) return ESP_ERR_INVALID_STATE;
    auto* dev = static_cast<m5tab5_ina226_t*>(ina226_handle_);

    esp_err_t err = m5tab5_ina226_read_bus_voltage(dev, &out->bus_voltage_v);
    if (err != ESP_OK) return err;
    err = m5tab5_ina226_read_current(dev, &out->current_a);
    if (err != ESP_OK) return err;
    return m5tab5_ina226_read_power(dev, &out->power_w);
}

}  // namespace m5::tab5