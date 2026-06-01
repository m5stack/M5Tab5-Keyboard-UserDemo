/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

﻿#include "ui_page_factory.h"
#include "ui_page_manager.h"
#include "ui_app.h"
#include "lvgl_port.h"

LV_FONT_DECLARE(lv_font_montserrat_24);
LV_FONT_DECLARE(lv_font_montserrat_16);

static const char* TAG = "FactoryPage";

FactoryPage::FactoryPage()
    : BasePage("Factory"),
      m_placeholder(nullptr),
      m_keyboard(nullptr),
      m_kb_instance(nullptr),
      m_rgb_container(nullptr),
      m_rgb1_indicator(nullptr),
      m_rgb2_indicator(nullptr),
      m_rgb1_label(nullptr),
      m_rgb2_label(nullptr),
    m_version_label(nullptr),
      m_brightness_label(nullptr),
      m_brightness_bar(nullptr),
      m_result_container(nullptr),
      m_result_label(nullptr),
      m_rgb_timer(nullptr),
      m_current_color_index(0),
            m_manual_color_index(0),
      m_breath_value(0),
      m_breath_increasing(true),
    m_pause_counter(0),
        m_manual_override_active(false)
{
}

FactoryPage::~FactoryPage()
{
    if (m_rgb_timer) {
        lv_timer_del(m_rgb_timer);
        m_rgb_timer = nullptr;
    }

    if (m_keyboard) {
        delete m_keyboard;
        m_keyboard = nullptr;
    }
}

void FactoryPage::setKeyboardInstance(m5::M5Tab5Keyboard* kb)
{
    m_kb_instance = kb;
}

void FactoryPage::rgbTimerCallback(lv_timer_t* timer)
{
    FactoryPage* page = static_cast<FactoryPage*>(lv_timer_get_user_data(timer));
    if (page) {
        page->handleRGBTimer();
    }
}

void FactoryPage::handleRGBTimer()
{
    if (m_manual_override_active) {
        return;
    }

    const int step = 5;        // 步进值，设为5以获得2倍速度的呼吸效果
    const int max_val = 100;    // 最大亮度限制为 100

    // 呼吸效果
    if (m_breath_increasing) {
        if ((int)m_breath_value + step >= max_val) {
            m_breath_value = max_val;
            m_breath_increasing = false;
        } else {
            m_breath_value += step;
        }
    } else {
        if (m_breath_value <= step) {
            m_breath_value = 0;
            // 保持 increasing = false 以便进入停顿逻辑

            // 在亮度为0时停止
            if (m_pause_counter < 1) { // 停顿时间缩短为原来的1/5
                m_pause_counter++;
            } else {
                m_pause_counter = 0;
                m_current_color_index = (m_current_color_index + 1) % 4;
                m_breath_increasing = true;
            }
        } else {
            m_breath_value -= step;
        }
    }

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    switch (m_current_color_index) {
        case 0:
            r = m_breath_value;
            g = m_breath_value;
            b = m_breath_value;
            break;
        case 1:
            r = m_breath_value;
            break;
        case 2:
            g = m_breath_value;
            break;
        case 3:
            b = m_breath_value;
            break;
        default:
            break;
    }

    (void)ui_app_submit_kb_rgb(r, g, b, m_breath_value);

    uint8_t ui_val = (uint32_t)m_breath_value * 255 / max_val;
    uint8_t ui_r = 0;
    uint8_t ui_g = 0;
    uint8_t ui_b = 0;
    switch (m_current_color_index) {
        case 0:
            ui_r = ui_val;
            ui_g = ui_val;
            ui_b = ui_val;
            break;
        case 1:
            ui_r = ui_val;
            break;
        case 2:
            ui_g = ui_val;
            break;
        case 3:
            ui_b = ui_val;
            break;
        default:
            break;
    }

    lv_color_t color = lv_color_make(ui_r, ui_g, ui_b);
    lv_obj_set_style_bg_color(m_rgb1_indicator, color, 0);
    lv_obj_set_style_bg_color(m_rgb2_indicator, color, 0);

    char buf[32];
    snprintf(buf, sizeof(buf), "RGB1\n#%02X%02X%02X", r, g, b);
    lv_label_set_text(m_rgb1_label, buf);
    snprintf(buf, sizeof(buf), "RGB2\n#%02X%02X%02X", r, g, b);
    lv_label_set_text(m_rgb2_label, buf);

    snprintf(buf, sizeof(buf), "Brightness: %d", m_breath_value);
    lv_label_set_text(m_brightness_label, buf);
    lv_slider_set_value(m_brightness_bar, m_breath_value, LV_ANIM_OFF);
}

void FactoryPage::applyFactoryWhiteRgb(uint8_t brightness)
{
    applyFactoryManualColor(brightness, 0);
}

void FactoryPage::applyFactoryManualColor(uint8_t brightness, uint8_t color_index)
{
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    const char* color_text = "#FFFFFF";

    switch (color_index % 4) {
        case 0:
            r = 0xFF;
            g = 0xFF;
            b = 0xFF;
            color_text = "#FFFFFF";
            break;
        case 1:
            r = 0xFF;
            color_text = "#FF0000";
            break;
        case 2:
            g = 0xFF;
            color_text = "#00FF00";
            break;
        case 3:
            b = 0xFF;
            color_text = "#0000FF";
            break;
        default:
            break;
    }

    (void)ui_app_submit_kb_rgb(r, g, b, brightness);

    lv_color_t color = lv_color_make(r, g, b);
    if (m_rgb1_indicator) {
        lv_obj_set_style_bg_color(m_rgb1_indicator, color, 0);
    }
    if (m_rgb2_indicator) {
        lv_obj_set_style_bg_color(m_rgb2_indicator, color, 0);
    }
    if (m_rgb1_label) {
        char buf[24];
        snprintf(buf, sizeof(buf), "RGB1\n%s", color_text);
        lv_label_set_text(m_rgb1_label, buf);
    }
    if (m_rgb2_label) {
        char buf[24];
        snprintf(buf, sizeof(buf), "RGB2\n%s", color_text);
        lv_label_set_text(m_rgb2_label, buf);
    }
}

void FactoryPage::brightnessSliderEventCallback(lv_event_t* event)
{
    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) {
        return;
    }

    FactoryPage* page = static_cast<FactoryPage*>(lv_event_get_user_data(event));
    lv_obj_t* slider = static_cast<lv_obj_t*>(lv_event_get_target(event));
    if (!page || !slider) {
        return;
    }

    page->handleBrightnessSliderChanged(lv_slider_get_value(slider));
}

void FactoryPage::rgbIndicatorEventCallback(lv_event_t* event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    FactoryPage* page = static_cast<FactoryPage*>(lv_event_get_user_data(event));
    if (!page) {
        return;
    }

    page->handleRgbIndicatorClicked();
}

void FactoryPage::handleBrightnessSliderChanged(int32_t value)
{
    if (value < 0) {
        value = 0;
    } else if (value > 100) {
        value = 100;
    }

    m_breath_value = static_cast<uint8_t>(value);

    if (!m_manual_override_active) {
        m_manual_override_active = true;
        m_manual_color_index = 0;
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "Brightness: %d", m_breath_value);
    lv_label_set_text(m_brightness_label, buf);

    applyFactoryManualColor(m_breath_value, m_manual_color_index);
}

void FactoryPage::handleRgbIndicatorClicked()
{
    if (!m_manual_override_active) {
        m_manual_override_active = true;
        m_manual_color_index = 0;
    } else {
        m_manual_color_index = (m_manual_color_index + 1) % 4;
    }

    applyFactoryManualColor(m_breath_value, m_manual_color_index);

    if (m_brightness_bar) {
        lv_slider_set_value(m_brightness_bar, m_breath_value, LV_ANIM_OFF);
    }
    if (m_brightness_label) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Brightness: %d", m_breath_value);
        lv_label_set_text(m_brightness_label, buf);
    }
}

void FactoryPage::createRGBDisplayUI()
{
    if (!m_placeholder) return;

    // 创建主容器
    m_rgb_container = lv_obj_create(m_placeholder);
    lv_obj_set_size(m_rgb_container, lv_pct(100), lv_pct(100));
    lv_obj_center(m_rgb_container);
    // 背景设为透明（与屏幕同色），添加边框
    lv_obj_set_style_bg_opa(m_rgb_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(m_rgb_container, 4, 0); // 稍微有点圆角
    lv_obj_clear_flag(m_rgb_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(m_rgb_container, 2, 0);
    lv_obj_set_style_border_color(m_rgb_container, lv_color_hex(0x606060), 0); // 灰色边框
    lv_obj_set_style_pad_all(m_rgb_container, 0, 0);

    // 使用 Flex Row 布局
    lv_obj_set_layout(m_rgb_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(m_rgb_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(m_rgb_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(m_rgb_container, 0, 0);

    // 左侧面板
    lv_obj_t* left_panel = lv_obj_create(m_rgb_container);
    lv_obj_set_size(left_panel, lv_pct(60), lv_pct(100));
    lv_obj_set_style_bg_opa(left_panel, LV_OPA_TRANSP, 0); // 透明背景
    lv_obj_set_style_radius(left_panel, 0, 0);
    lv_obj_clear_flag(left_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(left_panel, 0, 0);
    lv_obj_set_layout(left_panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(left_panel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(left_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER); // Align Start, 紧凑布局
    lv_obj_set_style_pad_all(left_panel, 0, 0); // 去除 Padding 实现撑满

    // RGB1 Section
    {
        lv_obj_t* sec = lv_obj_create(left_panel);
        lv_obj_set_size(sec, lv_pct(50), lv_pct(100)); // 50% 宽，100% 高度
        lv_obj_clear_flag(sec, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(sec, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(sec, 0, 0);
        lv_obj_set_layout(sec, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(sec, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(sec, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_gap(sec, 8, 0);

        m_rgb1_indicator = lv_obj_create(sec);
        lv_obj_set_size(m_rgb1_indicator, 50, 50);
        lv_obj_set_style_radius(m_rgb1_indicator, 6, 0);
        lv_obj_set_style_bg_color(m_rgb1_indicator, lv_color_hex(0xFFFFFF), 0);
        lv_obj_add_flag(m_rgb1_indicator, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(m_rgb1_indicator, rgbIndicatorEventCallback, LV_EVENT_CLICKED, this);

        m_rgb1_label = lv_label_create(sec);
        lv_obj_set_style_text_color(m_rgb1_label, lv_color_hex(0xEEEEEE), 0);
        lv_obj_set_style_text_font(m_rgb1_label, &lv_font_montserrat_16, 0);
        lv_obj_set_width(m_rgb1_label, lv_pct(100)); // 定宽防抖
        lv_obj_set_style_text_align(m_rgb1_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text(m_rgb1_label, "RGB1\n#FFFFFF");
    }

    // RGB2 Section
    {
        lv_obj_t* sec = lv_obj_create(left_panel);
        lv_obj_set_size(sec, lv_pct(50), lv_pct(100)); // 50% 宽，100% 高度
        lv_obj_clear_flag(sec, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(sec, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(sec, 0, 0);
        lv_obj_set_layout(sec, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(sec, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(sec, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_gap(sec, 8, 0);

        m_rgb2_indicator = lv_obj_create(sec);
        lv_obj_set_size(m_rgb2_indicator, 50, 50);
        lv_obj_set_style_radius(m_rgb2_indicator, 6, 0);
        lv_obj_set_style_bg_color(m_rgb2_indicator, lv_color_hex(0xFFFFFF), 0);
        lv_obj_add_flag(m_rgb2_indicator, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(m_rgb2_indicator, rgbIndicatorEventCallback, LV_EVENT_CLICKED, this);

        m_rgb2_label = lv_label_create(sec);
        lv_obj_set_style_text_color(m_rgb2_label, lv_color_hex(0xEEEEEE), 0);
        lv_obj_set_style_text_font(m_rgb2_label, &lv_font_montserrat_16, 0);
        lv_obj_set_width(m_rgb2_label, lv_pct(100)); // 定宽防抖
        lv_obj_set_style_text_align(m_rgb2_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text(m_rgb2_label, "RGB2\n#FFFFFF");
    }

    // 右侧面板
    lv_obj_t* right_panel = lv_obj_create(m_rgb_container);
    lv_obj_set_size(right_panel, lv_pct(40), lv_pct(100));
    lv_obj_set_style_bg_opa(right_panel, LV_OPA_TRANSP, 0); // 透明背景
    lv_obj_set_style_radius(right_panel, 0, 0);
    lv_obj_clear_flag(right_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(right_panel, 0, 0);
    lv_obj_set_layout(right_panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(right_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right_panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(right_panel, 10, 0);

    // 亮度标签
    m_brightness_label = lv_label_create(right_panel);
    lv_obj_set_style_text_color(m_brightness_label, lv_color_hex(0xEEEEEE), 0);
    lv_obj_set_style_text_font(m_brightness_label, &lv_font_montserrat_16, 0);
    lv_obj_set_width(m_brightness_label, lv_pct(100)); // 定宽防抖
    lv_obj_set_style_text_align(m_brightness_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(m_brightness_label, "Brightness: 20");

    // 亮度条
    m_brightness_bar = lv_slider_create(right_panel);
    lv_obj_set_size(m_brightness_bar, lv_pct(80), 12);
    lv_slider_set_range(m_brightness_bar, 0, 100);
    lv_slider_set_value(m_brightness_bar, 20, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(m_brightness_bar, lv_color_hex(0x404040), LV_PART_MAIN);
    lv_obj_set_style_bg_color(m_brightness_bar, lv_color_hex(0x00FF00), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(m_brightness_bar, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(m_brightness_bar, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_radius(m_brightness_bar, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_obj_set_style_pad_all(m_brightness_bar, 6, LV_PART_KNOB);
    lv_obj_add_event_cb(m_brightness_bar, brightnessSliderEventCallback, LV_EVENT_VALUE_CHANGED, this);

    ESP_LOGI(TAG, "RGB display UI created (Full UI)");
}

void FactoryPage::createResultDisplayUI()
{
    // 创建右侧占位容器
    lv_obj_t* right_placeholder = lv_obj_create(m_container);
    // 右侧约 1/3 宽度 (总宽95% -> 1/3 约 31-32%)
    lv_obj_set_size(right_placeholder, lv_pct(32), lv_pct(25)); 
    // 右对齐，距离右边缘 2.5% (使用 3% 近似)
    lv_obj_align(right_placeholder, LV_ALIGN_TOP_RIGHT, 0, 100);
    lv_obj_set_x(right_placeholder, lv_pct(-3));
    
    lv_obj_clear_flag(right_placeholder, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(right_placeholder, 0, 0);
    lv_obj_set_style_border_width(right_placeholder, 0, 0);
    lv_obj_set_style_bg_opa(right_placeholder, LV_OPA_TRANSP, 0);

    // 创建结果容器 (带边框，透明背景)
    m_result_container = lv_obj_create(right_placeholder);
    lv_obj_set_size(m_result_container, lv_pct(100), lv_pct(100));
    lv_obj_center(m_result_container);
    lv_obj_set_style_bg_opa(m_result_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(m_result_container, 4, 0);
    lv_obj_set_style_border_width(m_result_container, 2, 0);
    lv_obj_set_style_border_color(m_result_container, lv_color_hex(0x606060), 0);
    lv_obj_clear_flag(m_result_container, LV_OBJ_FLAG_SCROLLABLE);

    // 创建结果 Label
    m_result_label = lv_label_create(m_result_container);
    lv_obj_center(m_result_label);
    lv_obj_set_style_text_font(m_result_label, &lv_font_montserrat_48, 0);
    // 当前编译配置最大仅支持24号字体，使用缩放放大2.5倍(256 * 2.5 = 640)
    // lv_obj_set_style_transform_zoom(m_result_label, 640, 0); 
    lv_label_set_text(m_result_label, "CHECK"); // 初始显示 CHECK
    lv_obj_set_style_text_color(m_result_label, lv_color_hex(0x808080), 0); // 初始灰色
}

void FactoryPage::refreshVersionLabel()
{
    if (!m_version_label) {
        return;
    }

    char buf[128];
    if (ui_app_is_kb_connected()) {
        snprintf(buf, sizeof(buf), "SW: %s_%s  FW: 0x%02X",
                 ui_app_get_sw_version(), ui_app_get_sw_build_time(), ui_app_get_fw_version());
    } else {
        snprintf(buf, sizeof(buf), "SW: %s_%s  FW: --",
                 ui_app_get_sw_version(), ui_app_get_sw_build_time());
    }
    lv_label_set_text(m_version_label, buf);
}

void FactoryPage::resetAllKeys()
{
    if (m_keyboard) {
        m_keyboard->resetAllKeys();
    }
}

void FactoryPage::checkAllKeysPressed(){
    if (!m_keyboard) return;

    // 检查 70 个按键 (5行 x 14列)
    // 注意：FactoryTestKeyboard 的 Key ID 是 row * 14 + col
    bool all_pressed = true;
    for (int i = 0; i < 70; i++) {
        if (!m_keyboard->isKeyPressed(i)) {
            all_pressed = false;
            break;
        }
    }

    if (all_pressed) {
        lv_label_set_text(m_result_label, "OK");
        lv_obj_set_style_text_color(m_result_label, lv_color_hex(0x00FF00), 0); // 绿色
    }
}

void FactoryPage::onCreate()
{
    ESP_LOGI(TAG, "Creating Factory page");

    // 创建页面容器
    createContainer(PageManager::getInstance()->getParent());

    if (!m_container) {
        ESP_LOGE(TAG, "Failed to create container");
        return;
    }

    m_version_label = lv_label_create(m_container);
    lv_obj_align(m_version_label, LV_ALIGN_TOP_LEFT, 50, 20);
    lv_obj_set_style_text_color(m_version_label, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_text_font(m_version_label, &lv_font_montserrat_16, 0);
    lv_label_set_text(m_version_label, "SW: --_--  FW: --");
    refreshVersionLabel();

    // 创建占位容器 (左侧 RGB)
    m_placeholder = lv_obj_create(m_container);
    // 左侧约 2/3 宽度 (总宽95% -> 2/3 约 63%)
    lv_obj_set_size(m_placeholder, lv_pct(63), lv_pct(25)); 
    // 左对齐，距离左边缘 2.5% (使用 3% 近似)
    lv_obj_align(m_placeholder, LV_ALIGN_TOP_LEFT, 0, 100); 
    lv_obj_set_x(m_placeholder, lv_pct(3));

    lv_obj_clear_flag(m_placeholder, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(m_placeholder, 0, 0); // 去除内边距
    lv_obj_set_style_border_width(m_placeholder, 0, 0); // 去除边框
    lv_obj_set_style_bg_opa(m_placeholder, LV_OPA_TRANSP, 0); // 透明背景

    // 创建RGB显示UI
    createRGBDisplayUI();
    
    // 创建结果显示UI (右侧)
    createResultDisplayUI();

    // 创建工厂测试键盘 (5行 x 14列)
    if (!m_keyboard) {
        m_keyboard = new FactoryTestKeyboard(5, 14);
    }

    lv_obj_t* kb_obj = m_keyboard->create(m_container);
    if (kb_obj) {
        // 禁用触摸（仅通过物理按键测试
        m_keyboard->setTouchEnabled(false);

        // 防止键盘抢夺焦点
        lv_obj_clear_flag(kb_obj, LV_OBJ_FLAG_CLICK_FOCUSABLE);

        ESP_LOGI(TAG, "Factory test keyboard created");
    } else {
        ESP_LOGE(TAG, "Failed to create factory test keyboard");
    }

    ESP_LOGI(TAG, "Factory page created successfully");
}

void FactoryPage::onShow()
{
    BasePage::onShow();
    ESP_LOGI(TAG, "Factory page shown");

    refreshVersionLabel();

    // 重置状态变量
    m_breath_value = 0;
    m_breath_increasing = true;
    m_pause_counter = 0;
    m_current_color_index = 0;
    m_manual_color_index = 0;
    m_manual_override_active = false;
    if (m_brightness_bar) {
        lv_slider_set_value(m_brightness_bar, m_breath_value, LV_ANIM_OFF);
    }
    if (m_brightness_label) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Brightness: %d", m_breath_value);
        lv_label_set_text(m_brightness_label, buf);
    }

    // 启动定时器
    if (!m_rgb_timer) {
        // 加快变化速度: 100ms -> 20ms
        m_rgb_timer = lv_timer_create(rgbTimerCallback, 20, this);
        ESP_LOGI(TAG, "RGB timer started");
    }
}

void FactoryPage::onHide()
{
    BasePage::onHide();
    ESP_LOGI(TAG, "Factory page hidden");

    // 停止定时器
    if (m_rgb_timer) {
        lv_timer_del(m_rgb_timer);
        m_rgb_timer = nullptr;
        ESP_LOGI(TAG, "RGB timer stopped");
    }

    m_manual_override_active = false;
}

void FactoryPage::simulatePhysicalKeyDown(uint16_t key_id)
{
    if (!m_keyboard) {
        ESP_LOGW(TAG, "Keyboard not created");
        return;
    }

    ESP_LOGD(TAG, "Simulating physical key down: %d", key_id);
    m_keyboard->simulatePhysicalKeyDown(key_id);
    // PRESSING 状态不检查全部完成
}

void FactoryPage::simulatePhysicalKeyLongPressing(uint16_t key_id)
{
    if (!m_keyboard) return;
    m_keyboard->simulatePhysicalKeyLongPressing(key_id);
    // 中间过渡状态，不计入全部完成检查
}

void FactoryPage::simulatePhysicalKeyShortRelease(uint16_t key_id)
{
    if (!m_keyboard) {
        ESP_LOGW(TAG, "Keyboard not created");
        return;
    }

    ESP_LOGD(TAG, "Simulating physical key short release: %d", key_id);
    m_keyboard->simulatePhysicalKeyShortRelease(key_id);

    // 检查是否所有按键已完成测试
    checkAllKeysPressed();
}

void FactoryPage::simulatePhysicalKeyLongRelease(uint16_t key_id)
{
    if (!m_keyboard) {
        ESP_LOGW(TAG, "Keyboard not created");
        return;
    }

    ESP_LOGD(TAG, "Simulating physical key long release: %d", key_id);
    m_keyboard->simulatePhysicalKeyLongRelease(key_id);

    // 检查是否所有按键已完成测试
    checkAllKeysPressed();
}


