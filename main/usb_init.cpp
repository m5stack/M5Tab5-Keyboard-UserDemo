/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "usb_init.h"

#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sdkconfig.h>
#include <stdarg.h>
#include <tinyusb.h>
#include <tusb.h>

#if CONFIG_IDF_TARGET_ESP32P4
#include "hal/usb_wrap_ll.h"
#include "soc/usb_wrap_struct.h"
#endif

static const char* TAG = "USB_HID";

static bool s_usb_mounted = false;
static bool s_usb_initialized = false;
static bool s_tab5_usbc_route_active = false;
static usb_hid_log_cb_t s_log_cb = NULL;

void usb_hid_set_log_cb(usb_hid_log_cb_t cb)
{
    s_log_cb = cb;
}

static void usb_log(uint32_t color, const char* format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (s_log_cb) {
        s_log_cb(buffer, color);
    }
    ESP_LOGI(TAG, "%s", buffer);
}

static void tab5_route_usbc_to_otg_fs(void)
{
#if CONFIG_IDF_TARGET_ESP32P4
    // Tab5 USB-C is wired to ESP32-P4 FSLS PHY0 (GPIO24/25). ESP-IDF's
    // default USB_WRAP route uses FSLS PHY1 (GPIO26/27), so switch it here.
    usb_wrap_ll_phy_select(&USB_WRAP, 0);
    s_tab5_usbc_route_active = true;
    ESP_LOGI(TAG, "USB_WRAP routed to FSLS PHY0 for Tab5 USB-C device port");
#endif
}

static void tab5_restore_default_otg_fs_route(void)
{
#if CONFIG_IDF_TARGET_ESP32P4
    if (s_tab5_usbc_route_active) {
        usb_wrap_ll_phy_select(&USB_WRAP, 1);
        s_tab5_usbc_route_active = false;
        ESP_LOGI(TAG, "USB_WRAP restored to default FSLS PHY1 route");
    }
#endif
}

// HID Keyboard Report Descriptor (Standard Keyboard)
static const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

#define HID_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

static const tusb_desc_device_t hid_device_descriptor = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_UNSPECIFIED,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0x303A,
    .idProduct = 0x4004,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01,
};

#if TUD_OPT_HIGH_SPEED
static const tusb_desc_device_qualifier_t hid_device_qualifier = {
    .bLength = sizeof(tusb_desc_device_qualifier_t),
    .bDescriptorType = TUSB_DESC_DEVICE_QUALIFIER,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_UNSPECIFIED,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .bNumConfigurations = 0x01,
    .bReserved = 0x00,
};
#endif

static const char hid_langid_descriptor[] = {0x09, 0x04};
static const char* hid_string_descriptor[] = {
    hid_langid_descriptor,
    "M5Stack",
    "Tab5 Keyboard",
    "123456",
    "HID Keyboard",
};

// TinyUSB Configuration Descriptor
static const uint8_t hid_configuration_descriptor[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, HID_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface number, string index, boot protocol, report descriptor len, EP In address, size, polling interval
    TUD_HID_DESCRIPTOR(0, 4, HID_ITF_PROTOCOL_KEYBOARD, sizeof(hid_report_descriptor), 0x81, 16, 10)
};

uint16_t tud_hid_get_report_cb(uint8_t instance,
                               uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t* buffer,
                               uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

void tud_hid_set_report_cb(uint8_t instance,
                           uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const* buffer,
                           uint16_t bufsize)
{
    (void)instance;
    (void)report_id;
    (void)report_type;

    if (bufsize >= 1) {
        uint8_t leds = buffer[0];
        usb_log(0x00FFFF, "RX LED: Caps=%d Num=%d Scr=%d",
                (leds & 0x02) ? 1 : 0,
                (leds & 0x01) ? 1 : 0,
                (leds & 0x04) ? 1 : 0);
    }
}

static void tusb_event_handler(tinyusb_event_t* event, void* arg)
{
    (void)arg;

    switch (event->id) {
    case TINYUSB_EVENT_ATTACHED:
        s_usb_mounted = true;
        usb_log(0x00FF00, "USB Device Mounted on rhport %d", event->rhport);
        break;
    case TINYUSB_EVENT_DETACHED:
        s_usb_mounted = false;
        usb_log(0xFFA500, "USB Device Unmounted from rhport %d", event->rhport);
        break;
#ifdef CONFIG_TINYUSB_SUSPEND_CALLBACK
    case TINYUSB_EVENT_SUSPENDED:
        usb_log(0xFFFF00, "USB Suspended");
        break;
#endif
#ifdef CONFIG_TINYUSB_RESUME_CALLBACK
    case TINYUSB_EVENT_RESUMED:
        usb_log(0x00FFFF, "USB Resumed");
        break;
#endif
    default:
        break;
    }
}

uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return hid_report_descriptor;
}

esp_err_t usb_hid_keyboard_init(usb_port_t port)
{
    if (s_usb_initialized) {
        usb_log(0xFFFF00, "USB HID already initialized");
        return ESP_OK;
    }

    tinyusb_port_t tusb_port = TINYUSB_PORT_FULL_SPEED_0;
    if (port == USB_PORT_1_HIGH_SPEED) {
#if TUD_OPT_HIGH_SPEED
        tusb_port = TINYUSB_PORT_HIGH_SPEED_0;
        usb_log(0xFFFFFF, "Initializing USB HID Keyboard on USB2.0 High-Speed port...");
#else
        usb_log(0xFF0000, "High-Speed TinyUSB port is not enabled in this build");
        return ESP_ERR_NOT_SUPPORTED;
#endif
    } else {
        usb_log(0xFFFFFF, "Initializing USB HID Keyboard on USB-C Full-Speed port...");
        tab5_route_usbc_to_otg_fs();
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    const tinyusb_config_t tusb_cfg = {
        .port = tusb_port,
        .phy = {
            .skip_setup = false,
            .self_powered = false,
            .vbus_monitor_io = -1,
        },
        .task = {
            .size = 4096,
            .priority = 5,
            .xCoreID = 0,
        },
        .descriptor = {
            .device = &hid_device_descriptor,
#if TUD_OPT_HIGH_SPEED
            .qualifier = &hid_device_qualifier,
#else
            .qualifier = NULL,
#endif
            .string = hid_string_descriptor,
            .string_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
            .full_speed_config = hid_configuration_descriptor,
            .high_speed_config = hid_configuration_descriptor,
        },
        .event_cb = tusb_event_handler,
        .event_arg = NULL,
    };

    esp_err_t ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        tab5_restore_default_otg_fs_route();
        usb_log(0xFF0000, "Failed to install TinyUSB driver: %s", esp_err_to_name(ret));
        return ret;
    }

    s_usb_initialized = true;
    s_usb_mounted = tud_mounted();
    usb_log(0x00FF00, "USB HID Keyboard initialized on port %d.", tusb_port);

    return ESP_OK;
}

esp_err_t usb_hid_keyboard_deinit(void)
{
    if (!s_usb_initialized) {
        usb_log(0xFFFF00, "USB HID not initialized");
        return ESP_OK;
    }

    usb_log(0xFFFFFF, "Deinitializing USB HID Keyboard");

    usb_hid_send_release();
    vTaskDelay(pdMS_TO_TICKS(50));

    esp_err_t ret = tinyusb_driver_uninstall();
    if (ret != ESP_OK) {
        usb_log(0xFF0000, "Failed to uninstall TinyUSB driver: %s", esp_err_to_name(ret));
    }

    s_usb_mounted = false;
    s_usb_initialized = false;
    tab5_restore_default_otg_fs_route();

    usb_log(0xFFFFFF, "USB HID Keyboard deinitialized");

    return ret;
}

esp_err_t usb_hid_send_report(uint8_t modifier, uint8_t keycode)
{
    if (!s_usb_initialized) {
        ESP_LOGE(TAG, "USB HID not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (!tud_mounted()) {
        static int64_t last_log = 0;
        int64_t now = esp_timer_get_time();
        if (now - last_log > 5000000) {
            ESP_LOGW(TAG, "USB not mounted (tud_mounted=false), cannot send report");
            last_log = now;
        }
        return ESP_ERR_INVALID_STATE;
    }

    if (!tud_hid_ready()) {
        ESP_LOGW(TAG, "USB HID interface not ready, cannot send report");
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t keycodes[6] = {0};
    keycodes[0] = keycode;

    bool sent = tud_hid_keyboard_report(0, modifier, keycodes);
    if (!sent) {
        ESP_LOGE(TAG, "Failed to send HID report");
        return ESP_FAIL;
    }

    usb_log(0x80FF80, "TX Report: %02X 00 %02X 00 00 00 00 00", modifier, keycode);

    return ESP_OK;
}

esp_err_t usb_hid_send_release(void)
{
    if (!s_usb_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!tud_mounted()) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!tud_hid_ready()) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t keycodes[6] = {0};
    bool sent = tud_hid_keyboard_report(0, 0, keycodes);
    if (!sent) {
        ESP_LOGE(TAG, "Failed to send release report");
        return ESP_FAIL;
    }

    usb_log(0x80FF80, "TX Release");

    return ESP_OK;
}

bool usb_hid_is_mounted(void)
{
    return s_usb_initialized && s_usb_mounted && tud_mounted();
}
