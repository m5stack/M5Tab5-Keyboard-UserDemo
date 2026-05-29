#pragma once

#include "esp_err.h"

#include "m5tab5_variant_ids.h"

namespace m5::tab5 {

struct m5tab5_runtime_t {
    void* display_handle = nullptr;
    void* touch_handle = nullptr;
    void* ioexpander_handle = nullptr;       ///< IO expander ADDR_LOW 0x43 (RF_PTH, SPK, LCD, TP, CAM, EXT5V) / IO 扩展�?ADDR_LOW 0x43（RF_PTH、SPK、LCD、TP、CAM、EXT5V�?
    void* ioexpander_high_handle = nullptr;  ///< IO expander ADDR_HIGH 0x44 (WLAN_PWR_EN, CHG, USB5V, PWROFF) / IO 扩展�?ADDR_HIGH 0x44（WLAN_PWR_EN、CHG、USB5V、PWROFF�?
};

using m5tab5_display_init_fn = esp_err_t (*)(m5tab5_runtime_t* runtime);
using m5tab5_driver_probe_fn = esp_err_t (*)(m5tab5_runtime_t* runtime);
using m5tab5_driver_init_fn = esp_err_t (*)(m5tab5_runtime_t* runtime);
using m5tab5_variant_probe_fn = bool (*)();

struct m5tab5_display_driver_descriptor_t {
    const char* id;
    m5tab5_display_init_fn init;
};

struct m5tab5_device_driver_descriptor_t {
    const char* id;
    m5tab5_driver_probe_fn probe;
    m5tab5_driver_init_fn init;
};

struct m5tab5_variant_descriptor_t {
    M5TAB5_VariantId variant_id;
    const char* id;
    const char* description;
    const m5tab5_display_driver_descriptor_t* display;
    const m5tab5_device_driver_descriptor_t* touch;
    const m5tab5_device_driver_descriptor_t* ioexpander;
    m5tab5_variant_probe_fn probe;
};

struct m5tab5_component_config_t {
    M5TAB5_VariantId variant_id = M5TAB5_VARIANT_AUTO;
    bool enable_optional_drivers = true;
};

}  // namespace m5::tab5