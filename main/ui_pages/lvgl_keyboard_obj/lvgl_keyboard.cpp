/*
 * SPDX-FileCopyrightText: 2025-2026 exia
 *
 * SPDX-License-Identifier: MIT
 */

#include "lvgl_keyboard.h"
#include <esp_log.h>
#include <string.h>

static const char* TAG = "LvglKeyboard";

LvglKeyboard::LvglKeyboard()
    : m_kb_cont(nullptr),
      m_textarea(nullptr),
      m_next_hotkey_id(1),
      m_hotkey_enabled(true),
      m_ctrl_pressed(false),
      m_alt_pressed(false)
{
}

LvglKeyboard::~LvglKeyboard()
{
    // Note: LVGL objects are usually managed by LVGL parent-child relationship.
    // If m_kb_cont is valid, LVGL will delete it when parent is deleted.
    clearAllHotkeys();
}

void LvglKeyboard::setTextArea(lv_obj_t* ta)
{
    m_textarea = ta;
}

void LvglKeyboard::sendTextToTextarea(const char* val)
{
    if (!m_textarea || !val) return;

    // Common logic to handle basic control chars
    char c = val[0];
    if (c == '\0') return;

    if (c == '\b') {  // Backspace
        lv_textarea_delete_char(m_textarea);
    } else if (c == '\n') {  // Enter
        lv_textarea_add_char(m_textarea, '\n');
    } else if (c == '\x7F') {  // Delete
        lv_textarea_delete_char_forward(m_textarea);
    } else if (c == '\x14') {  // LEFT
        lv_textarea_cursor_left(m_textarea);
    } else if (c == '\x13') {  // RIGHT
        lv_textarea_cursor_right(m_textarea);
    } else if (c == '\x11') {  // UP
        lv_textarea_cursor_up(m_textarea);
    } else if (c == '\x12') {  // DOWN
        lv_textarea_cursor_down(m_textarea);
    } else if (c == '\t') {  // TAB
        lv_textarea_add_char(m_textarea, ' ');
        lv_textarea_add_char(m_textarea, ' ');
    } else if (c == '\x02') {  // HOME
        lv_textarea_set_cursor_pos(m_textarea, 0);
    } else if (c == '\x03') {  // END
        lv_textarea_set_cursor_pos(m_textarea, LV_TEXTAREA_CURSOR_LAST);
    } else {
        // Normal text
        lv_textarea_add_text(m_textarea, val);
    }
}

// ============================================================================
// 热键系统实现
// ============================================================================

int LvglKeyboard::registerHotkey(uint16_t key_id, hotkey_event_type_t event, hotkey_callback_t callback,
                                 void* user_data, bool require_ctrl, bool require_alt, bool require_shift,
                                 bool require_ctrl_only, bool require_alt_only)
{
    if (!callback) {
        ESP_LOGW(TAG, "Cannot register hotkey with null callback");
        return -1;
    }

    hotkey_registration_t reg;
    reg.hotkey_id         = m_next_hotkey_id++;
    reg.key_id            = key_id;
    reg.event             = event;
    reg.require_ctrl      = require_ctrl;
    reg.require_alt       = require_alt;
    reg.require_shift     = require_shift;
    reg.require_ctrl_only = require_ctrl_only;
    reg.require_alt_only  = require_alt_only;
    reg.callback          = callback;
    reg.user_data         = user_data;

    m_hotkey_registrations.push_back(reg);

    ESP_LOGI(TAG, "Registered hotkey: id=%d, key_id=%d, event=%d, ctrl=%d, alt=%d, shift=%d, ctrl_only=%d, alt_only=%d",
             reg.hotkey_id, key_id, event, require_ctrl, require_alt, require_shift, require_ctrl_only,
             require_alt_only);

    return reg.hotkey_id;
}

bool LvglKeyboard::unregisterHotkey(int hotkey_id)
{
    for (auto it = m_hotkey_registrations.begin(); it != m_hotkey_registrations.end(); ++it) {
        if (it->hotkey_id == hotkey_id) {
            ESP_LOGI(TAG, "Unregistered hotkey: id=%d", hotkey_id);
            m_hotkey_registrations.erase(it);
            return true;
        }
    }

    ESP_LOGW(TAG, "Hotkey id=%d not found", hotkey_id);
    return false;
}

void LvglKeyboard::clearAllHotkeys()
{
    ESP_LOGI(TAG, "Cleared all hotkeys (%d total)", (int)m_hotkey_registrations.size());
    m_hotkey_registrations.clear();
}

void LvglKeyboard::setHotkeyEnabled(bool enabled)
{
    m_hotkey_enabled = enabled;
    ESP_LOGI(TAG, "Hotkey system %s", enabled ? "enabled" : "disabled");
}

bool LvglKeyboard::triggerHotkey(const hotkey_event_t* event)
{
    if (!m_hotkey_enabled || !event) {
        ESP_LOGD(TAG, "Hotkey disabled or null event");
        return false;
    }

    // ========================================================================
    // 热键触发条件验证
    // 只有以下情况才能触发热键：
    // 1. 有修饰键按下（CTRL 与 ALT）
    // 2. 按下的是特殊键（ESC）
    // 普通字符键单独按下不触发热键，避免与正常输入冲突
    // ========================================================================
    bool has_modifier   = event->ctrl_pressed || event->alt_pressed;
    bool is_special_key = (event->key_id == KB_KEY_ESC);

    ESP_LOGI(TAG, "Hotkey check: key_id=%d, ctrl=%d, alt=%d, has_mod=%d, special=%d", event->key_id,
             event->ctrl_pressed, event->alt_pressed, has_modifier, is_special_key);

    if (!has_modifier && !is_special_key) {
        // 普通键且没有修饰键，不触发热键
        ESP_LOGD(TAG, "No modifier and not special key, skip hotkey");
        return false;
    }

    bool intercepted = false;

    ESP_LOGI(TAG, "Checking %d hotkey registrations", (int)m_hotkey_registrations.size());

    for (const auto& reg : m_hotkey_registrations) {
        // 检查按键 ID 匹配
        if (reg.key_id != M5TAB5_HOTKEY_ANY_KEY && reg.key_id != event->key_id) {
            continue;
        }

        // 检查事件类型匹配
        if (reg.event != event->event) {
            continue;
        }

        // 检查修饰键匹配
        // 1. 必须满足 require_ctrl/require_alt 的要求
        if (reg.require_ctrl && !event->ctrl_pressed) continue;
        if (reg.require_alt && !event->alt_pressed) continue;
        if (reg.require_shift && !event->shift_pressed) continue;

        // 2. 检查排他性要求（ctrl_only 表示只能有 Ctrl，不能有 Alt）
        if (reg.require_ctrl_only && event->alt_pressed) continue;
        if (reg.require_alt_only && event->ctrl_pressed) continue;

        ESP_LOGI(TAG, "Hotkey matched! Calling callback for key_id=%d", event->key_id);

        // 调用回调
        if (reg.callback) {
            bool result = reg.callback(event, reg.user_data);
            if (result) {
                intercepted = true;  // 标记为已拦截
                ESP_LOGI(TAG, "Hotkey intercepted: key_id=%d, event=%d", event->key_id, event->event);
            }
        }
    }

    return intercepted;
}
