/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

﻿#include "ui_page_normal.h"
#include "ui_page_manager.h"
#include "lvgl_port.h"
#include <string.h>

LV_FONT_DECLARE(lv_font_montserrat_24);

static const char* TAG = "NormalPage";

// 热键提示弹窗定时器回调
static void hotkeyToastTimerCb(lv_timer_t* timer)
{
    lv_obj_t* toast = (lv_obj_t*)lv_timer_get_user_data(timer);
    if (toast) {
        lv_obj_del(toast);
    }
}

// 显示热键提示弹窗
static void showHotkeyToast(lv_obj_t* parent, const char* message)
{
    if (!parent || !message) return;

    // 创建弹窗容器
    lv_obj_t* toast = lv_obj_create(parent);
    lv_obj_set_size(toast, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(toast, LV_ALIGN_TOP_RIGHT, -50, 205);

    // 设置样式
    lv_obj_set_style_bg_color(toast, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_bg_opa(toast, LV_OPA_90, 0);
    lv_obj_set_style_border_width(toast, 0, 0);
    lv_obj_set_style_radius(toast, 10, 0);
    lv_obj_set_style_pad_all(toast, 15, 0);
    lv_obj_set_style_shadow_width(toast, 20, 0);
    lv_obj_set_style_shadow_color(toast, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(toast, LV_OPA_30, 0);

    // 创建文本标签
    lv_obj_t* label = lv_label_create(toast);
    lv_label_set_text(label, message);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
    lv_obj_center(label);

    // 创建定时器，1秒后删除弹窗
    lv_timer_t* timer = lv_timer_create(hotkeyToastTimerCb, 1000, toast);
    lv_timer_set_repeat_count(timer, 1);
}

// 通用热键回调函数
static bool hotkeyCallback(const hotkey_event_t* event, void* user_data)
{
    lv_obj_t* textarea = (lv_obj_t*)user_data;
    if (!textarea) return true;

    // 获取父容器
    lv_obj_t* parent = lv_obj_get_parent(textarea);
    if (!parent) return true;

    // 根据按键ID and 修饰键组合显示不同的提示
    char message[64];
    char key_name_upper[32];
    const char* key_name = event->key_label ? event->key_label : "?";

    // 将按键名称转换为大写（如果是字母）
    size_t key_len = strnlen(key_name, sizeof(key_name_upper) - 1);
    for (size_t i = 0; i < key_len; i++) {
        if (key_name[i] >= 'a' && key_name[i] <= 'z') {
            key_name_upper[i] = key_name[i] - 'a' + 'A';
        } else {
            key_name_upper[i] = key_name[i];
        }
    }
    key_name_upper[key_len] = '\0';

    if (event->ctrl_pressed && event->alt_pressed) {
        // Ctrl+Alt 组合
        snprintf(message, sizeof(message), "Ctrl+Alt+%s", key_name_upper);
    } else if (event->ctrl_pressed) {
        // Ctrl 组合
        snprintf(message, sizeof(message), "Ctrl+%s", key_name_upper);
    } else if (event->alt_pressed) {
        // Alt 组合
        snprintf(message, sizeof(message), "Alt+%s", key_name_upper);
    } else if (event->key_id == KB_KEY_ESC) {
        // ESC 键
        snprintf(message, sizeof(message), "ESC");
    } else {
        snprintf(message, sizeof(message), "Hotkey: %s", key_name_upper);
    }

    ESP_LOGI(TAG, "Hotkey triggered: %s", message);
    showHotkeyToast(parent, message);

    return true; // 拦截事件
}

NormalPage::NormalPage()
    : BasePage("Normal"),
      m_textarea(nullptr),
      m_keyboard(nullptr),
      m_repeat_timer(nullptr),
      m_repeat_key_id(0),
      m_repeat_first_wait(true),
      m_repeat_mutex(nullptr)
{
}

NormalPage::~NormalPage()
{
    // 删除定时器
    if (m_repeat_timer) {
        xTimerDelete(m_repeat_timer, 0);
        m_repeat_timer = nullptr;
    }

    if (m_repeat_mutex) {
        vSemaphoreDelete(m_repeat_mutex);
        m_repeat_mutex = nullptr;
    }

    // 删除键盘
    if (m_keyboard) {
        delete m_keyboard;
        m_keyboard = nullptr;
    }
}

void NormalPage::onCreate()
{
    ESP_LOGI(TAG, "Creating Normal page");

    // 创建页面容器
    createContainer(PageManager::getInstance()->getParent());

    if (!m_container) {
        ESP_LOGE(TAG, "Failed to create container");
        return;
    }

    // 创建文本输入区域
    m_textarea = lv_textarea_create(m_container);
    lv_obj_set_size(m_textarea, lv_pct(95), lv_pct(25));
    lv_obj_align(m_textarea, LV_ALIGN_TOP_MID, 0, 100);
    lv_textarea_set_placeholder_text(m_textarea, "Type here...");
    lv_textarea_set_max_length(m_textarea, 512);  // 限制最大长度，防止 lv_textarea_add_text O(n) 性能劣化导致 WDT

    // 设置较大字体
    lv_obj_set_style_text_font(m_textarea, &lv_font_montserrat_24, 0);

    // 光标始终闪烁
    lv_obj_set_style_bg_opa(m_textarea, LV_OPA_COVER, LV_PART_CURSOR);
    lv_obj_set_style_bg_color(m_textarea, lv_palette_main(LV_PALETTE_BLUE), LV_PART_CURSOR);
    lv_obj_set_style_anim_time(m_textarea, 400, LV_PART_CURSOR);

    // 保持焦点
    lv_obj_add_event_cb(m_textarea, [](lv_event_t * e){
        lv_event_code_t code = lv_event_get_code(e);
        lv_obj_t * t = (lv_obj_t *)lv_event_get_target(e);
        if(code == LV_EVENT_DEFOCUSED) {
            lv_obj_add_state(t, LV_STATE_FOCUSED);
        }
    }, LV_EVENT_DEFOCUSED, NULL);

    // 初始焦点
    lv_obj_add_state(m_textarea, LV_STATE_FOCUSED);

    // 创建M5Tab5键盘
    if (!m_keyboard) {
        m_keyboard = new M5Tab5Keyboard();
    }

    lv_obj_t* kb_obj = m_keyboard->create(m_container);
    if (kb_obj) {
        // 绑定到textarea
        m_keyboard->setTextArea(m_textarea);

        // 设置全局实例（用于外部访问）
        setM5Tab5KeyboardInstance(m_keyboard);

        // 防止键盘抢夺焦点
        lv_obj_clear_flag(kb_obj, LV_OBJ_FLAG_CLICK_FOCUSABLE);

        // 注册热键回调示例
        ESP_LOGI(TAG, "Registering hotkeys...");

        // 示例1: Ctrl+C - 复制操作
        m_keyboard->registerHotkey(KB_KEY_C, HOTKEY_EVENT_PRESSED, hotkeyCallback, m_textarea, true, false, false, true, false);

        // 示例2: Ctrl+V - 粘贴操作
        m_keyboard->registerHotkey(KB_KEY_V, HOTKEY_EVENT_PRESSED, hotkeyCallback, m_textarea, true, false, false, true, false);

        // 示例3: Ctrl+S - 保存操作
        m_keyboard->registerHotkey(KB_KEY_S, HOTKEY_EVENT_PRESSED, hotkeyCallback, m_textarea, true, false, false, true, false);

        // 示例4: Alt+F - 文件菜单
        m_keyboard->registerHotkey(KB_KEY_F, HOTKEY_EVENT_PRESSED, hotkeyCallback, m_textarea, false, true, false, false, true);

        // 示例5: Ctrl+Alt+D - 调试模式
        m_keyboard->registerHotkey(KB_KEY_D, HOTKEY_EVENT_PRESSED, hotkeyCallback, m_textarea, true, true, false, false, false);

        // 临时禁用触摸
        // uint32_t child_cnt = lv_obj_get_child_cnt(kb_obj);
        // for(uint32_t i=0; i<child_cnt; i++) {
        //     lv_obj_t* btn = lv_obj_get_child(kb_obj, i);
        //     lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        // }

        // 示例6: ESC - 退出 / 取消操作
        m_keyboard->registerHotkey(KB_KEY_ESC, HOTKEY_EVENT_PRESSED, hotkeyCallback, m_textarea, false, false, false, false, false);

        ESP_LOGI(TAG, "Hotkeys registered successfully");
        ESP_LOGI(TAG, "M5Tab5 keyboard created");
    } else {
        ESP_LOGE(TAG, "Failed to create M5Tab5 keyboard");
    }

    // 创建重复定时器（懒加载）
    if (!m_repeat_timer) {
        m_repeat_timer = xTimerCreate("kb_rep", pdMS_TO_TICKS(100), pdTRUE, this, repeatTimerCallback);
    }

    if (!m_repeat_mutex) {
        m_repeat_mutex = xSemaphoreCreateMutex();
        if (!m_repeat_mutex) {
            ESP_LOGW(TAG, "Failed to create repeat mutex");
        }
    }

    ESP_LOGI(TAG, "Normal page created successfully");
}

void NormalPage::onShow()
{
    BasePage::onShow();
    ESP_LOGI(TAG, "Normal page shown");

    // 恢复全局键盘实例
    if (m_keyboard) {
        setM5Tab5KeyboardInstance(m_keyboard);
    }
}

void NormalPage::onHide()
{
    BasePage::onHide();
    ESP_LOGI(TAG, "Normal page hidden");

    // 停止重复定时器
    if (m_repeat_timer) {
        xTimerStop(m_repeat_timer, 0);
    }
}

void NormalPage::handlePhysicalKey(uint16_t key_id, bool pressed)
{
    if (!m_keyboard) {
        ESP_LOGW(TAG, "Keyboard not created");
        return;
    }

    ESP_LOGD(TAG, "Physical key: id=%d, pressed=%d", key_id, pressed);

    if (pressed) {
        // 按键按下：开始重复序列
        xTimerStop(m_repeat_timer, 0);

        if (m_repeat_mutex && xSemaphoreTake(m_repeat_mutex, portMAX_DELAY) == pdTRUE) {
            m_repeat_key_id = key_id;
            m_repeat_first_wait = true;
            xSemaphoreGive(m_repeat_mutex);
        } else {
            m_repeat_key_id = key_id;
            m_repeat_first_wait = true;
        }

        // 使用200ms超时，确保按键状态更新成功
        if (lvgl_port_lock(200)) {
            // 1. 视觉按下
            m_keyboard->simulateKeyPress(key_id);
            // 2. 立即输入一次
            m_keyboard->simulateInput(key_id);
            lvgl_port_unlock();
        }

        // 3. 启动重复定时器（初始延迟500ms）
        xTimerChangePeriod(m_repeat_timer, pdMS_TO_TICKS(500), 0);
        xTimerStart(m_repeat_timer, 0);

    } else {
        // 按键释放：停止重复
        xTimerStop(m_repeat_timer, 0);

        // 使用200ms超时，确保按键状态更新成功（避免卡住）
        if (lvgl_port_lock(200)) {
            // 仅视觉释放（不触发输入）
            m_keyboard->simulateKeyRelease(key_id, false);
            lvgl_port_unlock();
        }
    }
}

void NormalPage::repeatTimerCallback(TimerHandle_t xTimer)
{
    NormalPage* page = static_cast<NormalPage*>(pvTimerGetTimerID(xTimer));
    if (!page || !page->m_keyboard) {
        return;
    }

    bool first_wait = false;
    uint16_t repeat_key_id = 0;

    if (page->m_repeat_mutex) {
        if (xSemaphoreTake(page->m_repeat_mutex, 0) != pdTRUE) {
            return;
        }
        first_wait = page->m_repeat_first_wait;
        repeat_key_id = page->m_repeat_key_id;
        if (first_wait) {
            page->m_repeat_first_wait = false;
        }
        xSemaphoreGive(page->m_repeat_mutex);
    } else {
        first_wait = page->m_repeat_first_wait;
        repeat_key_id = page->m_repeat_key_id;
        if (first_wait) {
            page->m_repeat_first_wait = false;
        }
    }

    // 如果是初始延迟阶段，切换到快速重复模式
    if (first_wait) {
        xTimerChangePeriod(xTimer, pdMS_TO_TICKS(100), 0);
    }

    // 重复输入 —— 使用 timeout=0，不在 timer daemon 任务中阻塞等待锁
    // 避免 timer daemon 长时间占用 CPU 导致 LVGL 任务/idle 任务饿死
    if (lvgl_port_lock(0)) {
        page->m_keyboard->simulateInput(repeat_key_id);
        lvgl_port_unlock();
    }
}


