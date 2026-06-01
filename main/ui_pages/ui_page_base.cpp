/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#include "ui_page_base.h"

static const char* TAG = "BasePage";

BasePage::BasePage(const char* name) : m_container(nullptr), m_name(name), m_created(false), m_visible(false)
{
    ESP_LOGI(TAG, "Page '%s' constructed", m_name);
}

BasePage::~BasePage()
{
    ESP_LOGI(TAG, "Page '%s' destructing", m_name);
    onDestroy();
}

void BasePage::onShow()
{
    ESP_LOGI(TAG, "Page '%s' showing", m_name);
    m_visible = true;
}

void BasePage::onHide()
{
    ESP_LOGI(TAG, "Page '%s' hiding", m_name);
    m_visible = false;
}

void BasePage::onDestroy()
{
    if (m_container) {
        ESP_LOGI(TAG, "Page '%s' destroying container", m_name);
        lv_obj_del(m_container);
        m_container = nullptr;
    }
    m_created = false;
    m_visible = false;
}

void BasePage::show()
{
    if (!m_container) {
        ESP_LOGW(TAG, "Page '%s' container is null, cannot show", m_name);
        return;
    }

    lv_obj_clear_flag(m_container, LV_OBJ_FLAG_HIDDEN);
    onShow();
}

void BasePage::hide()
{
    if (!m_container) {
        ESP_LOGW(TAG, "Page '%s' container is null, cannot hide", m_name);
        return;
    }

    lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
    onHide();
}

void BasePage::createContainer(lv_obj_t* parent)
{
    if (m_container) {
        ESP_LOGW(TAG, "Page '%s' container already exists", m_name);
        return;
    }

    ESP_LOGI(TAG, "Page '%s' creating container", m_name);

    // 创建页面容器
    m_container = lv_obj_create(parent);

    // 全屏布局
    lv_obj_set_size(m_container, lv_pct(100), lv_pct(100));
    lv_obj_align(m_container, LV_ALIGN_TOP_LEFT, 0, 0);

    // 透明背景，继承父容器颜色
    lv_obj_set_style_bg_opa(m_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(m_container, 0, 0);
    lv_obj_set_style_pad_all(m_container, 0, 0);

    // 禁用滚动
    lv_obj_clear_flag(m_container, LV_OBJ_FLAG_SCROLLABLE);

    // 禁用点击 (让容器作为布局容器，不拦截点击事件)
    lv_obj_clear_flag(m_container, LV_OBJ_FLAG_CLICKABLE);

    // 初始隐藏
    lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);

    m_created = true;

    ESP_LOGI(TAG, "Page '%s' container created successfully", m_name);
}
