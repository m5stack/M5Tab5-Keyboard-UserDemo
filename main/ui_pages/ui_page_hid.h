/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

﻿#ifndef UI_PAGE_HID_H
#define UI_PAGE_HID_H

#include "ui_page_base.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#ifdef __cplusplus

    // 前向声明
    namespace m5
{
    class M5Tab5Keyboard;
}

/**
 * @brief HID模式页面
 *
 * HID模式：物理键盘通过USB HID输出到主机
 */
class HIDPage : public BasePage {
public:
    HIDPage();
    virtual ~HIDPage();

    // 生命周期方法
    virtual void onCreate() override;
    virtual void onShow() override;
    virtual void onHide() override;

    // 设置键盘实例
    void setKeyboard(m5::M5Tab5Keyboard* kb);

    // 处理HID输入 (Called by ui_app)
    void handleHIDInput(uint8_t modifier, uint8_t key_code);

private:
    lv_obj_t* m_label;  ///< 状态标签
    // Removed: m_status_label, m_count_label

    lv_obj_t* m_ctrl_container;  ///< 控制区域容器
    lv_obj_t* m_roller_port;     ///< 端口选择滚动条
    lv_obj_t* m_btn_start;       ///< 启动/停止按钮
    lv_obj_t* m_btn_label;       ///< 按钮文本
    lv_obj_t* m_ta_log;          ///< 日志文本框

    m5::M5Tab5Keyboard* m_keyboard;  ///< 硬件键盘实例
    TaskHandle_t m_hid_task;         ///< HID按键读取任务句柄
    bool m_task_running;             ///< 任务运行标志
    uint32_t m_key_count;            ///< 按键发送计数]
    bool m_is_usb_active;            ///< USB是否已激活

    static HIDPage* s_instance;  ///< 单例指针，用于回调

    // 静态任务函数
    static void hidTaskFunction(void* param);
    static void startBtnEventHandler(lv_event_t* e);
    static void logCallback(const char* msg, uint32_t color);

    // 更新UI显示
    void updateStatusDisplay();
    void onStartBtnClicked();
    void appendLog(const char* msg, uint32_t color);
};

#endif  // __cplusplus

#endif  // UI_PAGE_HID_H
