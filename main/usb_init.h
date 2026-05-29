/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef USB_INIT_H
#define USB_INIT_H

#include <esp_err.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    USB_PORT_0_FULL_SPEED = 0, // Tab5 USB-C device pins, FS TinyUSB device
    USB_PORT_1_HIGH_SPEED = 1  // Tab5 USB2.0 High-Speed PHY
} usb_port_t;

typedef void (*usb_hid_log_cb_t)(const char* msg, uint32_t color);

/**
 * @brief Set USB HID log callback.
 */
void usb_hid_set_log_cb(usb_hid_log_cb_t cb);

/**
 * @brief Initialize USB HID keyboard device.
 */
esp_err_t usb_hid_keyboard_init(usb_port_t port);

/**
 * @brief Deinitialize USB HID keyboard device.
 */
esp_err_t usb_hid_keyboard_deinit(void);

/**
 * @brief Send HID keyboard report.
 */
esp_err_t usb_hid_send_report(uint8_t modifier, uint8_t keycode);

/**
 * @brief Send all-zero report to release all keys.
 */
esp_err_t usb_hid_send_release(void);

/**
 * @brief Check whether the USB device is configured by host.
 */
bool usb_hid_is_mounted(void);

#ifdef __cplusplus
}
#endif

#endif // USB_INIT_H
