/*
 * SPDX-FileCopyrightText: 2025-2026 exia
 *
 * SPDX-License-Identifier: MIT
 */

#include "factorytest_keyboard.h"
#include <esp_log.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

LV_FONT_DECLARE(lv_font_montserrat_24);
LV_FONT_DECLARE(lv_font_montserrat_16);

static const char* TAG = "FactoryTestKeyboard";

// 颜色定义
#define FT_COLOR_BG_NORMAL      0xD0D0D0  // 未按键 - 浅灰
#define FT_COLOR_BG_SHORT_PRESS 0x404040  // 短按按下 - 深灰
#define FT_COLOR_BG_LONG_PRESS  0x003800  // 长按按下 - 深绿
#define FT_COLOR_TXT_NORMAL     0x000000  // 文字颜色(黑色)
#define FT_COLOR_TXT_WHITE      0xFFFFFF  // 白色文字

FactoryTestKeyboard::FactoryTestKeyboard(uint8_t rows, uint8_t cols)
    : m_rows(rows),
      m_cols(cols),
      m_key_count(rows * cols),
      m_touch_enabled(false),  // 默认禁用触摸
      m_key_pressing(nullptr),
      m_key_long_pressing(nullptr),
      m_key_pressed(nullptr),
      m_key_hold_pressed(nullptr),
      m_key_buttons(nullptr),
      m_key_hold_labels(nullptr),
      m_col_dsc(nullptr),
      m_row_dsc(nullptr)
{
    // 分配按下中状态数组
    m_key_pressing = (bool*)calloc(m_key_count, sizeof(bool));

    // 分配长按进行中状态数组（已达门槛但未松手）
    m_key_long_pressing = (bool*)calloc(m_key_count, sizeof(bool));

    // 分配短按完成状态数组
    m_key_pressed = (bool*)calloc(m_key_count, sizeof(bool));

    // 分配长按完成状态数组
    m_key_hold_pressed = (bool*)calloc(m_key_count, sizeof(bool));

    // 分配按键按钮对象数组
    m_key_buttons = (lv_obj_t**)calloc(m_key_count, sizeof(lv_obj_t*));

    // 分配次标签对象数组
    m_key_hold_labels = (lv_obj_t**)calloc(m_key_count, sizeof(lv_obj_t*));

    // 分配网格描述符数组 (需要持久化)
    m_col_dsc = (int32_t*)malloc((m_cols + 1) * sizeof(int32_t));
    m_row_dsc = (int32_t*)malloc((m_rows + 1) * sizeof(int32_t));

    if (!m_key_pressing || !m_key_long_pressing || !m_key_pressed || !m_key_hold_pressed || !m_key_buttons ||
        !m_key_hold_labels || !m_col_dsc || !m_row_dsc) {
        ESP_LOGE(TAG, "Failed to allocate memory for keyboard arrays");
    }
}

FactoryTestKeyboard::~FactoryTestKeyboard()
{
    if (m_key_pressing) {
        free(m_key_pressing);
        m_key_pressing = nullptr;
    }

    if (m_key_long_pressing) {
        free(m_key_long_pressing);
        m_key_long_pressing = nullptr;
    }

    if (m_key_pressed) {
        free(m_key_pressed);
        m_key_pressed = nullptr;
    }

    if (m_key_hold_pressed) {
        free(m_key_hold_pressed);
        m_key_hold_pressed = nullptr;
    }

    if (m_key_buttons) {
        free(m_key_buttons);
        m_key_buttons = nullptr;
    }

    if (m_key_hold_labels) {
        free(m_key_hold_labels);
        m_key_hold_labels = nullptr;
    }

    if (m_col_dsc) {
        free(m_col_dsc);
        m_col_dsc = nullptr;
    }

    if (m_row_dsc) {
        free(m_row_dsc);
        m_row_dsc = nullptr;
    }
}

void FactoryTestKeyboard::setTouchEnabled(bool enabled)
{
    m_touch_enabled = enabled;
    ESP_LOGI(TAG, "Touch %s", enabled ? "enabled" : "disabled");
}

bool FactoryTestKeyboard::isKeyPressed(uint16_t key_id) const
{
    if (key_id >= m_key_count || !m_key_pressed) {
        return false;
    }
    // 完成测试 = 短按完成 / 长按完成
    return m_key_pressed[key_id] || (m_key_hold_pressed && m_key_hold_pressed[key_id]);
}

void FactoryTestKeyboard::resetAllKeys()
{
    if (!m_key_pressed) return;

    for (uint16_t i = 0; i < m_key_count; i++) {
        if (m_key_pressing) m_key_pressing[i] = false;
        if (m_key_long_pressing) m_key_long_pressing[i] = false;
        m_key_pressed[i] = false;
        if (m_key_hold_pressed) m_key_hold_pressed[i] = false;
        updateKeyColor(i);
    }

    ESP_LOGI(TAG, "All keys reset");
}

void FactoryTestKeyboard::cancelPressingKeys()
{
    if (!m_key_pressing) return;

    for (uint16_t i = 0; i < m_key_count; i++) {
        if (m_key_pressing[i] || (m_key_long_pressing && m_key_long_pressing[i])) {
            m_key_pressing[i] = false;
            if (m_key_long_pressing) m_key_long_pressing[i] = false;
            // 已完成的状态（m_key_pressed / m_key_hold_pressed）保持不变
            updateKeyColor(i);
        }
    }
}

void FactoryTestKeyboard::updateKeyColor(uint16_t key_id)
{
    if (key_id >= m_key_count || !m_key_buttons || !m_key_buttons[key_id]) {
        return;
    }

    lv_obj_t* btn        = m_key_buttons[key_id];
    lv_obj_t* label_ok   = lv_obj_get_child(btn, 0);
    lv_obj_t* label_hold = (m_key_hold_labels) ? m_key_hold_labels[key_id] : nullptr;

    if (m_key_hold_pressed && m_key_hold_pressed[key_id]) {
        // 状态 LONG_PRESS: 深绿, 显示 OK + long
        lv_obj_set_style_bg_color(btn, lv_color_hex(FT_COLOR_BG_LONG_PRESS), 0);
        if (label_ok) {
            lv_obj_clear_flag(label_ok, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(label_ok, "OK");
            lv_obj_set_style_text_color(label_ok, lv_color_hex(FT_COLOR_TXT_WHITE), 0);
        }
        if (label_hold) {
            lv_label_set_text(label_hold, "long");
            lv_obj_clear_flag(label_hold, LV_OBJ_FLAG_HIDDEN);
        }
    } else if (m_key_pressed[key_id]) {
        // 状态 SHORT_CLICK: 深绿, 显示 OK, 隐藏 long
        lv_obj_set_style_bg_color(btn, lv_color_hex(FT_COLOR_BG_LONG_PRESS), 0);
        if (label_ok) {
            lv_obj_clear_flag(label_ok, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(label_ok, "OK");
            lv_obj_set_style_text_color(label_ok, lv_color_hex(FT_COLOR_TXT_WHITE), 0);
        }
        if (label_hold) {
            lv_obj_add_flag(label_hold, LV_OBJ_FLAG_HIDDEN);
        }
    } else if (m_key_pressing && m_key_pressing[key_id]) {
        // 状态 PRESSING: 深灰, 隐藏 label_ok
        // 未达长按门槛显示 "click"；已达长按门槛显示 "long"
        lv_obj_set_style_bg_color(btn, lv_color_hex(FT_COLOR_BG_SHORT_PRESS), 0);
        if (label_ok) {
            lv_obj_add_flag(label_ok, LV_OBJ_FLAG_HIDDEN);
        }
        if (label_hold) {
            const char* sub_text = (m_key_long_pressing && m_key_long_pressing[key_id]) ? "long" : "click";
            lv_label_set_text(label_hold, sub_text);
            lv_obj_clear_flag(label_hold, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        // 状态 IDLE: 浅灰, 显示编号, 隐藏 long
        lv_obj_set_style_bg_color(btn, lv_color_hex(FT_COLOR_BG_NORMAL), 0);
        if (label_ok) {
            lv_obj_clear_flag(label_ok, LV_OBJ_FLAG_HIDDEN);
            char label_text[8];
            snprintf(label_text, sizeof(label_text), "%d", key_id + 1);
            lv_label_set_text(label_ok, label_text);
            lv_obj_set_style_text_color(label_ok, lv_color_hex(FT_COLOR_TXT_NORMAL), 0);
        }
        if (label_hold) {
            lv_obj_add_flag(label_hold, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void FactoryTestKeyboard::simulatePhysicalKeyDown(uint16_t key_id)
{
    if (key_id >= m_key_count || !m_key_pressing) {
        ESP_LOGW(TAG, "Invalid key_id: %d", key_id);
        return;
    }

    // 已处于 PRESSING 中，忽略重复下按
    if (m_key_pressing[key_id]) {
        ESP_LOGD(TAG, "Key %d already pressing, ignoring down", key_id);
        return;
    }

    // 任何状态（IDLE/SHORT_CLICK/LONG_PRESS）都可重新进入 PRESSING
    // 清除旧状态，设置 pressing
    m_key_pressed[key_id] = false;
    if (m_key_hold_pressed) m_key_hold_pressed[key_id] = false;
    if (m_key_long_pressing) m_key_long_pressing[key_id] = false;
    m_key_pressing[key_id] = true;
    updateKeyColor(key_id);

    ESP_LOGI(TAG, "Key %d pressing (dark gray + click)", key_id);
}

void FactoryTestKeyboard::simulatePhysicalKeyShortRelease(uint16_t key_id)
{
    if (key_id >= m_key_count || !m_key_pressed) {
        ESP_LOGW(TAG, "Invalid key_id: %d", key_id);
        return;
    }

    // 只处理 PRESSING 状态的松开
    if (!m_key_pressing || !m_key_pressing[key_id]) {
        ESP_LOGD(TAG, "Key %d not in pressing state, ignoring short release", key_id);
        return;
    }

    // PRESSING -> SHORT_CLICK: 深绿 + OK，隐藏 long
    m_key_pressing[key_id] = false;
    m_key_pressed[key_id]  = true;
    if (m_key_hold_pressed) m_key_hold_pressed[key_id] = false;
    updateKeyColor(key_id);

    ESP_LOGI(TAG, "Key %d short-click (dark green + OK)", key_id);
}

void FactoryTestKeyboard::simulatePhysicalKeyLongPressing(uint16_t key_id)
{
    if (key_id >= m_key_count || !m_key_pressing || !m_key_long_pressing) {
        ESP_LOGW(TAG, "Invalid key_id: %d", key_id);
        return;
    }

    // 只在 PRESSING 状态且尚未标记 long_pressing 时生效
    if (!m_key_pressing[key_id] || m_key_long_pressing[key_id]) {
        return;
    }

    // 标记已达长按门槛，更新显示为 "long"
    m_key_long_pressing[key_id] = true;
    updateKeyColor(key_id);

    ESP_LOGI(TAG, "Key %d long-pressing (dark gray + long)", key_id);
}

void FactoryTestKeyboard::simulatePhysicalKeyLongRelease(uint16_t key_id)
{
    if (key_id >= m_key_count || !m_key_pressed || !m_key_hold_pressed) {
        ESP_LOGW(TAG, "Invalid key_id: %d", key_id);
        return;
    }

    // 只处理 PRESSING 状态的松开
    if (!m_key_pressing || !m_key_pressing[key_id]) {
        ESP_LOGD(TAG, "Key %d not in pressing state, ignoring long release", key_id);
        return;
    }

    // PRESSING -> LONG_PRESS: 深绿 + OK + long
    m_key_pressing[key_id]     = false;
    m_key_pressed[key_id]      = true;
    m_key_hold_pressed[key_id] = true;
    updateKeyColor(key_id);

    ESP_LOGI(TAG, "Key %d long-release (dark green + OK + long)", key_id);
}

void FactoryTestKeyboard::handleKeyEvent(lv_event_t* e)
{
    // 如果触摸被禁用，直接返回
    if (!m_touch_enabled) {
        return;
    }

    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) {
        return;
    }

    // 获取按键ID
    uint16_t key_id = (uint16_t)(uintptr_t)lv_event_get_user_data(e);

    // 如果已经按下，则忽略
    if (m_key_pressed[key_id]) {
        ESP_LOGD(TAG, "Key %d already pressed, ignoring click", key_id);
        return;
    }

    // 标记为已按下并上报
    m_key_pressed[key_id] = true;
    updateKeyColor(key_id);

    ESP_LOGI(TAG, "Key %d clicked and marked", key_id);
}

void FactoryTestKeyboard::eventHandlerStatic(lv_event_t* e)
{
    lv_obj_t* btn  = (lv_obj_t*)lv_event_get_target(e);
    lv_obj_t* cont = lv_obj_get_parent(btn);

    if (!cont) return;

    FactoryTestKeyboard* kb = (FactoryTestKeyboard*)lv_obj_get_user_data(cont);
    if (kb) {
        kb->handleKeyEvent(e);
    }
}

lv_obj_t* FactoryTestKeyboard::create(lv_obj_t* parent)
{
    m_kb_cont = lv_obj_create(parent);

    // 附加this指针到容器
    lv_obj_set_user_data(m_kb_cont, this);

    // 计算动态尺寸
    lv_disp_t* disp = lv_obj_get_disp(parent);
    if (!disp) disp = lv_disp_get_default();
    int32_t screen_w = lv_disp_get_hor_res(disp);
    int32_t screen_h = lv_disp_get_ver_res(disp);

    // 目标宽度 98%
    int32_t target_w = (int32_t)(screen_w * 0.98f);

    // 根据宽高比计算高度
    float ratio = getPreferredAspectRatio();
    if (ratio < 0.1f) ratio = 1.0f;
    int32_t target_h = (int32_t)(target_w / ratio);

    // 应用最小/最大宽高比限制
    float min_r           = getMinAspectRatio();
    float max_r           = getMaxAspectRatio();
    int32_t h_allowed_max = (int32_t)(target_w / min_r);
    int32_t h_allowed_min = (int32_t)(target_w / max_r);

    if (target_h > h_allowed_max) target_h = h_allowed_max;
    if (target_h < h_allowed_min) target_h = h_allowed_min;

    // 应用最大屏幕高度限制
    int32_t screen_h_limit = (int32_t)(screen_h * getMaxScreenHeightPct());
    if (target_h > screen_h_limit) {
        target_h = screen_h_limit;
    }

    lv_obj_set_size(m_kb_cont, target_w, target_h);
    lv_obj_align(m_kb_cont, LV_ALIGN_BOTTOM_MID, 0, -5);

    lv_obj_set_style_bg_color(m_kb_cont, lv_color_hex(0x181818), 0);
    lv_obj_set_style_radius(m_kb_cont, 10, 0);
    lv_obj_set_style_border_width(m_kb_cont, 0, 0);
    lv_obj_set_style_pad_gap(m_kb_cont, 4, 0);
    lv_obj_clear_flag(m_kb_cont, LV_OBJ_FLAG_GESTURE_BUBBLE);

    // 填充列描述符
    for (uint8_t i = 0; i < m_cols; i++) {
        m_col_dsc[i] = LV_GRID_FR(1);
    }
    m_col_dsc[m_cols] = LV_GRID_TEMPLATE_LAST;

    // 填充行描述符
    for (uint8_t i = 0; i < m_rows; i++) {
        m_row_dsc[i] = LV_GRID_FR(1);
    }
    m_row_dsc[m_rows] = LV_GRID_TEMPLATE_LAST;

    lv_obj_set_layout(m_kb_cont, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(m_kb_cont, m_col_dsc, m_row_dsc);

    // 创建按键
    for (uint16_t i = 0; i < m_key_count; i++) {
        uint8_t row = i / m_cols;
        uint8_t col = i % m_cols;

#if LVGL_VERSION_MAJOR >= 9
        lv_obj_t* btn = lv_button_create(m_kb_cont);
#else
        lv_obj_t* btn = lv_btn_create(m_kb_cont);
#endif

        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);

        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(FT_COLOR_BG_NORMAL), 0);
        lv_obj_set_style_bg_grad_dir(btn, LV_GRAD_DIR_NONE, 0);  // 禁用渐变
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);           // 强制不透明
        lv_obj_set_style_border_width(btn, 0, 0);                // 移除边框
        lv_obj_set_style_shadow_width(btn, 0, 0);                // 移除阴影
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_GESTURE_BUBBLE);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);

        // 保存按钮引用
        m_key_buttons[i] = btn;

        // 配置按钮操作区域: flex列布局以展示主标签 (OK/编号) 和 hold
        lv_obj_set_layout(btn, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(btn, 2, 0);
        lv_obj_set_style_pad_gap(btn, 1, 0);  // OK 与 hold 之间的间距

        // 创建主标签（初始显示编号，按下后显示 OK）
        char label_text[8];
        snprintf(label_text, sizeof(label_text), "%d", i + 1);

        lv_obj_t* label_ok = lv_label_create(btn);
        lv_label_set_text(label_ok, label_text);
        lv_obj_set_style_text_color(label_ok, lv_color_hex(FT_COLOR_TXT_NORMAL), 0);
        lv_obj_set_style_text_font(label_ok, &lv_font_montserrat_24, 0);

        // 创建 long 标签（小字体，初始隐藏）
        lv_obj_t* label_hold = lv_label_create(btn);
        lv_label_set_text(label_hold, "long");
        lv_obj_set_style_text_color(label_hold, lv_color_hex(FT_COLOR_TXT_WHITE), 0);
        lv_obj_set_style_text_font(label_hold, &lv_font_montserrat_16, 0);
        lv_obj_add_flag(label_hold, LV_OBJ_FLAG_HIDDEN);  // 初始隐藏

        // 存储 hold(long) 标签引用
        if (m_key_hold_labels) {
            m_key_hold_labels[i] = label_hold;
        }

        // 添加事件处理 - 传递按键ID作为user_data
        lv_obj_add_event_cb(btn, eventHandlerStatic, LV_EVENT_CLICKED, (void*)(uintptr_t)i);

        // 如果按键之前已被测试过，恢复颜色状态
        if (m_key_pressed[i]) {
            updateKeyColor(i);
        }
    }

    ESP_LOGI(TAG, "Factory test keyboard created: %dx%d (%d keys), touch %s", m_rows, m_cols, m_key_count,
             m_touch_enabled ? "enabled" : "disabled");

    return m_kb_cont;
}

void FactoryTestKeyboard::setKeyDisabled(uint16_t key_id, bool disabled)
{
    // 工厂测试键盘不支持禁用单个按键
    ESP_LOGW(TAG, "setKeyDisabled not supported for factory test keyboard");
}

void FactoryTestKeyboard::simulateCharacter(const char* key_char)
{
    // 工厂测试键盘不支持字符模式
    ESP_LOGW(TAG, "simulateCharacter not supported for factory test keyboard");
}

void FactoryTestKeyboard::simulateString(const char* str, uint32_t interval_ms)
{
    // 工厂测试键盘不支持字符串模拟
    ESP_LOGW(TAG, "simulateString not supported for factory test keyboard");
}
