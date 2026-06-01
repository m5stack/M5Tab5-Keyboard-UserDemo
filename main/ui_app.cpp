/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#include "ui_app.h"
#include "ui_pages/ui_page_manager.h"
#include "ui_pages/ui_page_factory.h"
#include "ui_pages/ui_page_normal.h"
#include "ui_pages/ui_page_string.h"
#include "ui_pages/ui_page_hid.h"

#include "ui_pages/ui_page_registry.h"
#include "ui_pages/ui_top_menu.h"
#include "app_build_info.h"
#include "m5tab5_keyboard.h"
#include "m5_tab5_keyboard.h"
#include <esp_log.h>
#include <esp_timer.h>
#include "lvgl_port.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <string.h>

    LV_FONT_DECLARE(lv_font_montserrat_24);
LV_FONT_DECLARE(lv_font_montserrat_16);

static const char* TAG = "ui_app";

// Event queue and task configurations
#define KB_EVENT_QUEUE_SIZE 64  // Event queue size to handle fast key presses
static QueueHandle_t kb_event_queue      = NULL;
static TaskHandle_t kb_event_task_handle = NULL;

// Factory RGB calibration variables
#define KB_RGB_QUEUE_SIZE        1
#define KB_RGB_APPLY_RETRY_COUNT 3
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t brightness;
} kb_rgb_cmd_t;

typedef struct {
    uint8_t rgb1_r;
    uint8_t rgb1_g;
    uint8_t rgb1_b;
    uint8_t rgb2_r;
    uint8_t rgb2_g;
    uint8_t rgb2_b;
    uint8_t brightness;
} kb_rgb_state_t;

static QueueHandle_t kb_rgb_queue      = NULL;
static TaskHandle_t kb_rgb_task_handle = NULL;

static uint8_t kb_firmware_version = 0;  // Keyboard firmware version

// Internal pointer to the keyboard instance
static m5::M5Tab5Keyboard* g_kb_instance = NULL;

// ========== Hotplug Monitor ==========
#define KB_HOTPLUG_CHECK_MS     1000  // Hotplug check interval
#define KB_RECONNECT_ATTEMPT_MS 2000  // Reconnection attempt interval
#define KB_DISCONNECT_THRESHOLD 3     // Disconnect attempt threshold

typedef enum {
    KB_HOTPLUG_STATE_STABLE = 0,
    KB_HOTPLUG_STATE_DISCONNECTING,
    KB_HOTPLUG_STATE_RECONNECTING,
    KB_HOTPLUG_STATE_PAUSED,
} kb_hotplug_state_t;

static bool kb_connected                            = false;  // Connection status
static int kb_i2c_fail_count                        = 0;
static TaskHandle_t kb_hotplug_task_handle          = NULL;
static volatile kb_hotplug_state_t kb_hotplug_state = KB_HOTPLUG_STATE_RECONNECTING;

// Forward declarations
static void physical_keyboard_callback(m5_tab5_key_event_t event, void* arg);
static void menu_switch_callback(int mode_int);
static void kb_apply_mode_for_current_page(void);
static void kb_rgb_task(void* arg);
static bool kb_read_rgb_state(kb_rgb_state_t* state);
static bool kb_write_rgb_state(const kb_rgb_state_t* state);
static bool kb_verify_rgb_state(const kb_rgb_state_t* expected_state);
static bool kb_restore_rgb_state(const kb_rgb_state_t* state);
static bool kb_verify_rgb_command(const kb_rgb_cmd_t* cmd);
static bool kb_apply_rgb_command(const kb_rgb_cmd_t* cmd);
static void kb_reset_rgb_queue(void);
static void kb_set_hotplug_state(kb_hotplug_state_t new_state);

// Factory page: track key press timestamps for long press detection
#define FACTORY_KEY_COUNT     70
#define FACTORY_LONG_PRESS_MS 500  // Long press detection threshold (ms)
static int64_t factory_key_press_time[FACTORY_KEY_COUNT] = {0};
static bool factory_key_long_notified[FACTORY_KEY_COUNT] = {false};

const char* ui_app_get_sw_version(void)
{
    return FIRMWARE_VERSION;
}

const char* ui_app_get_sw_build_time(void)
{
    return FIRMWARE_BUILD_TIME;
}

uint8_t ui_app_get_fw_version(void)
{
    return kb_firmware_version;
}

bool ui_app_is_kb_rgb_ready(void)
{
    return g_kb_instance != NULL && kb_connected && kb_hotplug_state == KB_HOTPLUG_STATE_STABLE &&
           g_kb_instance->isInitialized();
}

bool ui_app_submit_kb_rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    if (kb_rgb_queue == NULL || !ui_app_is_kb_rgb_ready()) {
        return false;
    }

    kb_rgb_cmd_t cmd = {
        .r          = r,
        .g          = g,
        .b          = b,
        .brightness = brightness,
    };

    return xQueueOverwrite(kb_rgb_queue, &cmd) == pdPASS;
}

static void kb_reset_rgb_queue(void)
{
    if (kb_rgb_queue != NULL) {
        xQueueReset(kb_rgb_queue);
    }
}

static void kb_set_hotplug_state(kb_hotplug_state_t new_state)
{
    if (kb_hotplug_state == new_state) {
        return;
    }

    kb_hotplug_state = new_state;

    if (new_state != KB_HOTPLUG_STATE_STABLE) {
        kb_reset_rgb_queue();
    }
}

// Helper to update log label (with LVGL lock)
static void update_log_label(const char* fmt, ...)
{
    PageManager* pm     = PageManager::getInstance();
    lv_obj_t* log_label = pm->getGlobalLog();

    if (!log_label) return;

    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    // Logs error or success messages
    if (lvgl_port_lock(5)) {
        lv_label_set_text(log_label, buf);
        lvgl_port_unlock();
    }
}

// Refresh the firmware version label on the Factory page
static void refresh_factory_version_if_visible(void)
{
    PageManager* pm = PageManager::getInstance();
    if (pm->getCurrentPageId() == PAGE_ID_FACTORY) {
        FactoryPage* fp = static_cast<FactoryPage*>(pm->getCurrentPage());
        if (fp && lvgl_port_lock(200)) {
            fp->refreshVersionLabel();
            lvgl_port_unlock();
        }
    }
}

// Event task to update UI
static void kb_event_task(void* arg)
{
    m5_tab5_key_event_t event;

    ESP_LOGI(TAG, "Keyboard event task started");

    while (1) {
        // Periodic UI refresh task
        bool got_event = (xQueueReceive(kb_event_queue, &event, pdMS_TO_TICKS(50)) == pdTRUE);

        // Query and parse pending key events
        {
            PageManager* pm_lp = PageManager::getInstance();
            if (pm_lp->getCurrentPageId() == PAGE_ID_FACTORY) {
                int64_t now_ms = esp_timer_get_time() / 1000;
                for (int i = 0; i < FACTORY_KEY_COUNT; i++) {
                    if (factory_key_press_time[i] > 0 && !factory_key_long_notified[i]) {
                        if ((now_ms - factory_key_press_time[i]) >= FACTORY_LONG_PRESS_MS) {
                            factory_key_long_notified[i] = true;
                            FactoryPage* fp              = static_cast<FactoryPage*>(pm_lp->getCurrentPage());
                            if (fp && lvgl_port_lock(50)) {
                                fp->simulatePhysicalKeyLongPressing((uint16_t)i);
                                lvgl_port_unlock();
                            }
                        }
                    }
                }
            }
        }

        if (!got_event) {
            continue;
        }

        PageManager* pm = PageManager::getInstance();
        int page_id     = pm->getCurrentPageId();

        // Handle different event types
        if (event.type == M5_TAB5_KB_MODE_NORMAL) {
            // Normal Mode: Row/Col events
            uint16_t key_id = event.row * 14 + event.col;
            ESP_LOGD(TAG, "NORMAL mode: row=%d, col=%d, key_id=%d, pressed=%d", event.row, event.col, key_id,
                     event.pressed);

            // Update Log Label with firmware version (Format based on _default_key_callback)
            update_log_label("[FW:%02X] [NORMAL] %s | Row: %d | Col: %d | Raw: 0x%02X", kb_firmware_version,
                             event.pressed ? "PRESSED " : "RELEASED", event.row, event.col, event.raw_data);

            if (page_id == PAGE_ID_FACTORY) {
                // Factory Page event processing
                if (event.pressed) {
                    // Event timestamp for long press detection
                    if (key_id < FACTORY_KEY_COUNT) {
                        factory_key_press_time[key_id]    = esp_timer_get_time() / 1000;
                        factory_key_long_notified[key_id] = false;
                    }
                    FactoryPage* factory = static_cast<FactoryPage*>(pm->getCurrentPage());
                    if (factory && lvgl_port_lock(200)) {
                        factory->simulatePhysicalKeyDown(key_id);
                        lvgl_port_unlock();
                    }
                } else {
                    // Determine single click or long press on release
                    if (key_id < FACTORY_KEY_COUNT && factory_key_press_time[key_id] > 0) {
                        int64_t held_ms                = (esp_timer_get_time() / 1000) - factory_key_press_time[key_id];
                        factory_key_press_time[key_id] = 0;
                        factory_key_long_notified[key_id] = false;
                        FactoryPage* factory              = static_cast<FactoryPage*>(pm->getCurrentPage());
                        if (factory && lvgl_port_lock(200)) {
                            if (held_ms >= FACTORY_LONG_PRESS_MS) {
                                factory->simulatePhysicalKeyLongRelease(key_id);
                            } else {
                                factory->simulatePhysicalKeyShortRelease(key_id);
                            }
                            lvgl_port_unlock();
                        }
                    }
                }
            } else if (page_id == PAGE_ID_NORMAL) {
                // Normal page logging
                NormalPage* normal = static_cast<NormalPage*>(pm->getCurrentPage());
                if (normal) {
                    normal->handlePhysicalKey(key_id, event.pressed);
                }
            }
        } else if (event.type == M5_TAB5_KB_MODE_STRING) {
            // String/Char Mode: String data events
            ESP_LOGD(TAG, "CHAR mode: len=%d, data='%s'", event.str_len, event.str_data);

            // Update Log Label with firmware version (Format based on _default_key_callback)
            update_log_label("[FW:%02X] [STRING] Mod: 0x%02X | Len: %d | Data: %s", kb_firmware_version,
                             event.str_modifier, event.str_len, event.str_data);

            if (page_id == PAGE_ID_STRING) {
                // String page logging
                StringPage* str_page = static_cast<StringPage*>(pm->getCurrentPage());
                if (str_page) {
                    str_page->handleStringInput(event.str_data, event.str_len, event.str_modifier);
                    // Dedicated log page display
                    str_page->appendLog(event.str_modifier, "[FW:%02X] [STRING] Mod: 0x%02X | Len: %d | Data: %s",
                                        kb_firmware_version, event.str_modifier, event.str_len, event.str_data);
                }
            }
        } else if (event.type == M5_TAB5_KB_MODE_HID) {
            // HID Mode: HID keycode events
            ESP_LOGD(TAG, "HID mode: modifier=0x%02X, keycode=0x%02X", event.hid_modifier, event.hid_key_code);

            // Update Log Label with firmware version (Format based on _default_key_callback)
            update_log_label("[FW:%02X] [HID] Mod: 0x%02X | KeyCode: 0x%02X", kb_firmware_version, event.hid_modifier,
                             event.hid_key_code);

            if (page_id == PAGE_ID_HID) {
                // HID page logging
                HIDPage* hid_page = static_cast<HIDPage*>(pm->getCurrentPage());
                if (hid_page) {
                    hid_page->handleHIDInput(event.hid_modifier, event.hid_key_code);
                }
            }
        }
    }
}

static bool kb_apply_rgb_command(const kb_rgb_cmd_t* cmd)
{
    if (cmd == NULL || !ui_app_is_kb_rgb_ready()) {
        return false;
    }

    kb_rgb_state_t previous_state;
    if (!kb_read_rgb_state(&previous_state)) {
        ESP_LOGE(TAG, "RGB apply aborted because previous state could not be captured: rgb=(%u,%u,%u) brightness=%u",
                 cmd->r, cmd->g, cmd->b, cmd->brightness);
        return false;
    }

    for (int attempt = 1; attempt <= KB_RGB_APPLY_RETRY_COUNT; ++attempt) {
        if (!ui_app_is_kb_rgb_ready()) {
            return false;
        }

        kb_rgb_state_t target_state = {
            .rgb1_r     = cmd->r,
            .rgb1_g     = cmd->g,
            .rgb1_b     = cmd->b,
            .rgb2_r     = cmd->r,
            .rgb2_g     = cmd->g,
            .rgb2_b     = cmd->b,
            .brightness = cmd->brightness,
        };

        if (!kb_write_rgb_state(&target_state)) {
            ESP_LOGW(TAG, "RGB apply attempt %d/%d failed while writing target state", attempt,
                     KB_RGB_APPLY_RETRY_COUNT);
        } else if (kb_verify_rgb_command(cmd)) {
            return true;
        } else {
            ESP_LOGW(TAG, "RGB apply attempt %d/%d verification mismatch: rgb=(%u,%u,%u) brightness=%u", attempt,
                     KB_RGB_APPLY_RETRY_COUNT, cmd->r, cmd->g, cmd->b, cmd->brightness);
        }

        if (attempt < KB_RGB_APPLY_RETRY_COUNT) {
            vTaskDelay(pdMS_TO_TICKS(2));
        }
    }

    ESP_LOGE(TAG, "RGB apply failed after %d attempts: rgb=(%u,%u,%u) brightness=%u", KB_RGB_APPLY_RETRY_COUNT, cmd->r,
             cmd->g, cmd->b, cmd->brightness);

    if (!kb_restore_rgb_state(&previous_state)) {
        ESP_LOGE(TAG, "RGB restore failed after apply error");
    }

    return false;
}

static bool kb_read_rgb_state(kb_rgb_state_t* state)
{
    if (state == NULL || g_kb_instance == NULL || !ui_app_is_kb_rgb_ready()) {
        return false;
    }

    if (g_kb_instance->getBothRGB(&state->rgb1_r, &state->rgb1_g, &state->rgb1_b, &state->rgb2_r, &state->rgb2_g,
                                  &state->rgb2_b) != M5_TAB5_KB_OK) {
        return false;
    }
    if (g_kb_instance->getBrightness(&state->brightness) != M5_TAB5_KB_OK) {
        return false;
    }

    return true;
}

static bool kb_write_rgb_state(const kb_rgb_state_t* state)
{
    if (state == NULL || g_kb_instance == NULL || !ui_app_is_kb_rgb_ready()) {
        return false;
    }

    // Use a single 7-byte burst write so the slave MCU receives both LED colors
    // in one I2C transaction, eliminating the timing window that causes one LED
    // to lag behind the other. Falls back to two separate writes only when the
    // two colors actually differ (e.g. during state restore after a failure).
    if (state->rgb1_r == state->rgb2_r && state->rgb1_g == state->rgb2_g && state->rgb1_b == state->rgb2_b) {
        if (g_kb_instance->setBothRGB(state->rgb1_r, state->rgb1_g, state->rgb1_b) != M5_TAB5_KB_OK) {
            return false;
        }
    } else {
        if (g_kb_instance->setRGB(0, state->rgb1_r, state->rgb1_g, state->rgb1_b) != M5_TAB5_KB_OK) {
            return false;
        }
        if (g_kb_instance->setRGB(1, state->rgb2_r, state->rgb2_g, state->rgb2_b) != M5_TAB5_KB_OK) {
            return false;
        }
    }

    if (g_kb_instance->setBrightness(state->brightness) != M5_TAB5_KB_OK) {
        return false;
    }

    return true;
}

static bool kb_verify_rgb_state(const kb_rgb_state_t* expected_state)
{
    if (expected_state == NULL) {
        return false;
    }

    kb_rgb_state_t actual_state;
    if (!kb_read_rgb_state(&actual_state)) {
        return false;
    }

    return memcmp(&actual_state, expected_state, sizeof(actual_state)) == 0;
}

static bool kb_restore_rgb_state(const kb_rgb_state_t* state)
{
    if (state == NULL) {
        return false;
    }

    for (int attempt = 1; attempt <= KB_RGB_APPLY_RETRY_COUNT; ++attempt) {
        if (!ui_app_is_kb_rgb_ready()) {
            return false;
        }

        if (kb_write_rgb_state(state) && kb_verify_rgb_state(state)) {
            return true;
        }

        ESP_LOGW(TAG, "RGB restore attempt %d/%d failed", attempt, KB_RGB_APPLY_RETRY_COUNT);
        if (attempt < KB_RGB_APPLY_RETRY_COUNT) {
            vTaskDelay(pdMS_TO_TICKS(2));
        }
    }

    return false;
}

static bool kb_verify_rgb_command(const kb_rgb_cmd_t* cmd)
{
    if (cmd == NULL) {
        return false;
    }

    kb_rgb_state_t expected_state = {
        .rgb1_r     = cmd->r,
        .rgb1_g     = cmd->g,
        .rgb1_b     = cmd->b,
        .rgb2_r     = cmd->r,
        .rgb2_g     = cmd->g,
        .rgb2_b     = cmd->b,
        .brightness = cmd->brightness,
    };

    return kb_verify_rgb_state(&expected_state);
}

static void kb_rgb_task(void* arg)
{
    (void)arg;
    kb_rgb_cmd_t cmd;

    ESP_LOGI(TAG, "Keyboard RGB task started");

    while (1) {
        if (xQueueReceive(kb_rgb_queue, &cmd, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        (void)kb_apply_rgb_command(&cmd);
    }
}

// Physical keyboard event callback
static void physical_keyboard_callback(m5_tab5_key_event_t event, void* arg)
{
    (void)arg;

    if (kb_event_queue == NULL) {
        ESP_LOGW(TAG, "Keyboard event queue not ready, event dropped!");
        return;
    }

    // Validate key event
    if (xQueueSend(kb_event_queue, &event, 0) != pdTRUE) {
        m5_tab5_key_event_t dropped;
        if (xQueueReceive(kb_event_queue, &dropped, 0) == pdTRUE) {
            if (xQueueSend(kb_event_queue, &event, 0) != pdTRUE) {
                ESP_LOGW(TAG, "Keyboard event queue full, event dropped!");
            }
        } else {
            ESP_LOGW(TAG, "Keyboard event queue full, event dropped!");
        }
    }
}

// ========== Public API ==========
bool ui_app_is_kb_connected(void)
{
    return kb_connected;
}
// Sync hardware keyboard and RGB mode based on current page
static void kb_apply_mode_for_current_page(void)
{
    if (!g_kb_instance || !g_kb_instance->isInitialized()) return;

    PageManager* pm = PageManager::getInstance();
    int page_id     = pm->getCurrentPageId();

    m5_tab5_kb_mode_t driver_mode = M5_TAB5_KB_MODE_NORMAL;
    switch (page_id) {
        case PAGE_ID_FACTORY:
        case PAGE_ID_NORMAL:
            driver_mode = M5_TAB5_KB_MODE_NORMAL;
            break;
        case PAGE_ID_HID:
            driver_mode = M5_TAB5_KB_MODE_HID;
            break;
        case PAGE_ID_STRING:
            driver_mode = M5_TAB5_KB_MODE_STRING;
            break;
        default:
            driver_mode = M5_TAB5_KB_MODE_NORMAL;
            break;
    }

    g_kb_instance->setMode(driver_mode);

    if (page_id == PAGE_ID_FACTORY) {
        g_kb_instance->setRGBMode(M5_TAB5_KB_RGB_MODE_CUSTOM);
    } else {
        g_kb_instance->setRGBMode(M5_TAB5_KB_RGB_MODE_BINDING);
    }
    ESP_LOGI(TAG, "Hotplug: applied mode %d for page %d", driver_mode, page_id);
}

// Hotplug monitor thread
static void kb_hotplug_monitor_task(void* arg)
{
    (void)arg;
    ESP_LOGI(TAG, "Keyboard hotplug monitor started");

    for (;;) {
        if (g_kb_instance == nullptr) {
            vTaskDelay(pdMS_TO_TICKS(KB_HOTPLUG_CHECK_MS));
            continue;
        }

        if (g_kb_instance->isInitialized()) {
            // --- Connected State ---
            uint8_t ver = 0;
            if (g_kb_instance->getVersion(&ver) == M5_TAB5_KB_OK) {
                // I2C communication test
                kb_i2c_fail_count = 0;
                if (!kb_connected) {
                    // State transition (Disconnected -> Connected): Restore keyboard configurations
                    kb_connected        = true;
                    kb_firmware_version = ver;
                    kb_apply_mode_for_current_page();
                    kb_set_hotplug_state(KB_HOTPLUG_STATE_STABLE);
                    update_log_label("[FW:%02X] Keyboard Connected", kb_firmware_version);
                    refresh_factory_version_if_visible();
                    ESP_LOGI(TAG, "Keyboard reconnected (init path). FW: 0x%02X", ver);
                } else if (ver != kb_firmware_version) {
                    // Check for firmware version changes
                    kb_firmware_version = ver;
                    update_log_label("[FW:%02X] Keyboard FW Updated", kb_firmware_version);
                    refresh_factory_version_if_visible();
                    ESP_LOGI(TAG, "Keyboard FW version changed: 0x%02X", ver);
                }
            } else {
                // I2C communication failed
                kb_i2c_fail_count++;
                if (kb_i2c_fail_count >= KB_DISCONNECT_THRESHOLD && kb_connected) {
                    kb_set_hotplug_state(KB_HOTPLUG_STATE_DISCONNECTING);
                    kb_connected = false;
                    ESP_LOGW(TAG, "Keyboard disconnected (after %d failures)", kb_i2c_fail_count);
                    // End monitor and release resources
                    g_kb_instance->end();
                    kb_set_hotplug_state(KB_HOTPLUG_STATE_RECONNECTING);
                    update_log_label("[FW:--] Keyboard Disconnected");
                    refresh_factory_version_if_visible();
                    // Read queue to flush stale events
                    memset(factory_key_press_time, 0, sizeof(factory_key_press_time));
                    memset(factory_key_long_notified, 0, sizeof(factory_key_long_notified));
                    PageManager* pm_dc = PageManager::getInstance();
                    if (pm_dc->getCurrentPageId() == PAGE_ID_FACTORY) {
                        FactoryPage* fp_dc = static_cast<FactoryPage*>(pm_dc->getCurrentPage());
                        if (fp_dc && lvgl_port_lock(200)) {
                            fp_dc->resetAllKeys();
                            lvgl_port_unlock();
                        }
                    }
                }
            }
            vTaskDelay(pdMS_TO_TICKS(KB_HOTPLUG_CHECK_MS));
        } else {
            // --- Disconnected State ---
            kb_set_hotplug_state(KB_HOTPLUG_STATE_RECONNECTING);
            m5_tab5_kb_err_t err =
                g_kb_instance->begin(I2C_NUM_1, M5_TAB5_KB_DEFAULT_ADDR, M5_TAB5_KB_DEFAULT_SDA, M5_TAB5_KB_DEFAULT_SCL,
                                     M5_TAB5_KB_I2C_FREQ_400K, M5_TAB5_KB_DEFAULT_INT, M5_TAB5_KB_INT_MODE_HARDWARE);

            if (err == M5_TAB5_KB_OK) {
                // Initialization successful, read version and set mode
                g_kb_instance->setKeyCallback(physical_keyboard_callback, nullptr);

                uint8_t ver = 0;
                if (g_kb_instance->getVersion(&ver) == M5_TAB5_KB_OK) {
                    kb_firmware_version = ver;
                } else {
                    kb_firmware_version = 0;
                }

                kb_connected      = true;
                kb_i2c_fail_count = 0;
                kb_apply_mode_for_current_page();
                kb_set_hotplug_state(KB_HOTPLUG_STATE_STABLE);
                update_log_label("[FW:%02X] Keyboard Connected", kb_firmware_version);
                refresh_factory_version_if_visible();
                ESP_LOGI(TAG, "Keyboard connected! FW: 0x%02X", kb_firmware_version);
            }
            vTaskDelay(pdMS_TO_TICKS(KB_RECONNECT_ATTEMPT_MS));
        }
    }
}

// Create Main UI Background
static void create_main_ui(void)
{
    // 1. Set background color
    lv_obj_t* scr = lv_scr_act();
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);             // Disable scroll
    lv_obj_add_flag(scr, LV_OBJ_FLAG_CLICKABLE);                // Enable click
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x202020), 0);  // Dark Gray
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    ESP_LOGI(TAG, "Main UI created successfully");
}

// Menu switch callback
static void menu_switch_callback(int mode_int)
{
    ESP_LOGI(TAG, "Menu switch to mode: %d", mode_int);

    PageManager* pm = PageManager::getInstance();

    // Switch page (and sync I2C settings)
    if (!lvgl_port_lock(100)) {
        ESP_LOGE(TAG, "Failed to acquire LVGL lock for page switch");
        return;
    }

    bool page_switched = pm->switchToPage(mode_int);

    lvgl_port_unlock();

    // Synchronize registers only during page change
    if (!page_switched) {
        ESP_LOGD(TAG, "Page not switched, skipping I2C operations");
        return;
    }

    // Switch working mode
    m5_tab5_kb_mode_t driver_mode = M5_TAB5_KB_MODE_NORMAL;
    switch (mode_int) {
        case PAGE_ID_FACTORY:
        case PAGE_ID_NORMAL:
            driver_mode = M5_TAB5_KB_MODE_NORMAL;
            break;
        case PAGE_ID_HID:
            driver_mode = M5_TAB5_KB_MODE_HID;
            break;
        case PAGE_ID_STRING:
            driver_mode = M5_TAB5_KB_MODE_STRING;
            break;

        case PAGE_ID_REGISTRY:
            driver_mode = M5_TAB5_KB_MODE_NORMAL;  // Registry Page
            break;
        default:
            break;
    }

    if (g_kb_instance) {
        g_kb_instance->setMode(driver_mode);
        ESP_LOGI(TAG, "Switched physical keyboard to mode %d", driver_mode);

        // RGB Mode Switch: Factory = CUSTOM, Others = BINDING
        if (mode_int == PAGE_ID_FACTORY) {
            g_kb_instance->setRGBMode(M5_TAB5_KB_RGB_MODE_CUSTOM);
            ESP_LOGI(TAG, "Switched RGB mode to CUSTOM");
        } else {
            g_kb_instance->setRGBMode(M5_TAB5_KB_RGB_MODE_BINDING);
            ESP_LOGI(TAG, "Switched RGB mode to BINDING");
        }
    } else {
        ESP_LOGW(TAG, "Keyboard instance not available for mode switch");
    }
}

// Create Application UI
esp_err_t lvgl_ui_create(void* kb_handle, int menu_scope_mask)
{
    ESP_LOGI(TAG, "Creating UI with PageManager...");

    // Store keyboard instance
    g_kb_instance = (m5::M5Tab5Keyboard*)kb_handle;

    // Initialize task sync mutex
    kb_event_queue = xQueueCreate(KB_EVENT_QUEUE_SIZE, sizeof(m5_tab5_key_event_t));
    if (kb_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create keyboard event queue");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Keyboard event queue created (size=%d)", KB_EVENT_QUEUE_SIZE);

    kb_rgb_queue = xQueueCreate(KB_RGB_QUEUE_SIZE, sizeof(kb_rgb_cmd_t));
    if (kb_rgb_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create keyboard RGB queue");
        vQueueDelete(kb_event_queue);
        kb_event_queue = NULL;
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Keyboard RGB queue created (size=%d)", KB_RGB_QUEUE_SIZE);

    // Create background event processing task
    BaseType_t ret = xTaskCreatePinnedToCore(kb_event_task, "kb_event", 4096, NULL, 7, &kb_event_task_handle, 1);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create keyboard event task");
        vQueueDelete(kb_rgb_queue);
        kb_rgb_queue = NULL;
        vQueueDelete(kb_event_queue);
        kb_event_queue = NULL;
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Keyboard event task created");

    ret = xTaskCreatePinnedToCore(kb_rgb_task, "kb_rgb", 3072, NULL, 2, &kb_rgb_task_handle, 0);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create keyboard RGB task");
        if (kb_event_task_handle != NULL) {
            vTaskDelete(kb_event_task_handle);
            kb_event_task_handle = NULL;
        }
        vQueueDelete(kb_rgb_queue);
        kb_rgb_queue = NULL;
        vQueueDelete(kb_event_queue);
        kb_event_queue = NULL;
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Keyboard RGB task created");

    // Read and cache keyboard firmware version
    if (g_kb_instance) {
        if (g_kb_instance->isInitialized()) {
            if (g_kb_instance->getVersion(&kb_firmware_version) != M5_TAB5_KB_OK) {
                kb_firmware_version = 0x00;
            }
            g_kb_instance->setKeyCallback(physical_keyboard_callback, nullptr);
            kb_connected = true;
            kb_set_hotplug_state(KB_HOTPLUG_STATE_STABLE);
            ESP_LOGI(TAG, "Keyboard connected at boot. FW: 0x%02X", kb_firmware_version);
        } else {
            kb_firmware_version = 0x00;
            kb_connected        = false;
            kb_set_hotplug_state(KB_HOTPLUG_STATE_RECONNECTING);
            ESP_LOGW(TAG, "Keyboard not connected at boot, hotplug monitor will handle");
        }
    }
    ESP_LOGI(TAG, "Keyboard firmware version: 0x%02X", kb_firmware_version);

    // Initialize UI components (requires LVGL lock)
    if (!lvgl_port_lock(1000)) {
        ESP_LOGE(TAG, "Failed to acquire LVGL lock");
        return ESP_FAIL;
    }

    // Create main UI background
    create_main_ui();

    // Get PageManager instance
    PageManager* pm = PageManager::getInstance();
    pm->setParent(lv_scr_act());

    // Create global menu
    lv_obj_t* menu = createM5Tab5Menu(lv_scr_act(), menu_switch_callback, menu_scope_mask);
    lv_obj_add_flag(menu, LV_OBJ_FLAG_FLOATING);
    lv_obj_move_foreground(menu);
    pm->setGlobalMenu(menu);
    ESP_LOGI(TAG, "Global menu created");

    // Create global log label
    lv_obj_t* log_label = lv_label_create(lv_scr_act());
    lv_obj_add_flag(log_label, LV_OBJ_FLAG_FLOATING);
    lv_obj_align(log_label, LV_ALIGN_TOP_LEFT, 50, 50);
    lv_obj_set_style_text_color(log_label, lv_color_hex(0x00FF00), 0);  // Green text
    lv_obj_set_style_text_font(log_label, &lv_font_montserrat_16, 0);
    lv_label_set_text(log_label, "Waiting for input...");
    lv_obj_move_foreground(log_label);
    pm->setGlobalLog(log_label);
    ESP_LOGI(TAG, "Global log label created");

    // Register all pages
    FactoryPage* factoryPage = new FactoryPage();
    factoryPage->setKeyboardInstance(g_kb_instance);
    pm->registerPage(PAGE_ID_FACTORY, factoryPage);
    pm->registerPage(PAGE_ID_NORMAL, new NormalPage());
    pm->registerPage(PAGE_ID_STRING, new StringPage());
    HIDPage* hidPage = new HIDPage();
    hidPage->setKeyboard(g_kb_instance);
    pm->registerPage(PAGE_ID_HID, hidPage);

    RegistryPage* registryPage = new RegistryPage();
    registryPage->setKeyboard(g_kb_instance);
    pm->registerPage(PAGE_ID_REGISTRY, registryPage);
    ESP_LOGI(TAG, "All pages registered");

    // Determine initial page and switch
    // Priority follows UI render order: Normal -> String -> HID -> BLE -> Registry -> Factory
    // Factory is intentionally last as it is a hidden mode that requires unlock.
    int initial_page = -1;

    if (menu_scope_mask & (1 << PAGE_ID_NORMAL)) {
        initial_page = PAGE_ID_NORMAL;
    } else if (menu_scope_mask & (1 << PAGE_ID_STRING)) {
        initial_page = PAGE_ID_STRING;
    } else if (menu_scope_mask & (1 << PAGE_ID_HID)) {
        initial_page = PAGE_ID_HID;
    } else if (menu_scope_mask & (1 << PAGE_ID_REGISTRY)) {
        initial_page = PAGE_ID_REGISTRY;
    } else if (menu_scope_mask & (1 << PAGE_ID_FACTORY)) {
        initial_page = PAGE_ID_FACTORY;
    }

    if (initial_page != -1) {
        ESP_LOGI(TAG, "Switching to initial page: %d", initial_page);
        // Ensure HW state is consistent with initial page
        menu_switch_callback(initial_page);

        // Update menu indicator position if needed (it defaults to 0)
        // Note: We can't easily update the menu indicator visual position here without access to menu internal context,
        // but since createM5Tab5Menu logic handles hidden items, it usually aligns correctly.
    } else {
        ESP_LOGW(TAG, "No valid initial page found in scope mask 0x%X", menu_scope_mask);
    }

    lvgl_port_unlock();

    // Create hotplug monitor task
    BaseType_t hp_ret =
        xTaskCreatePinnedToCore(kb_hotplug_monitor_task, "kb_hotplug", 3072, NULL, 3, &kb_hotplug_task_handle, 0);
    if (hp_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create keyboard hotplug monitor task");
    } else {
        ESP_LOGI(TAG, "Keyboard hotplug monitor started");
    }

    ESP_LOGI(TAG, "UI created successfully with async keyboard event handling");
    return ESP_OK;
}
