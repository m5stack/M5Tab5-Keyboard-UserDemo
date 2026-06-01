/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

﻿#ifndef UI_PAGE_REGISTRY_H
#define UI_PAGE_REGISTRY_H

#include "ui_page_base.h"
#include "m5_tab5_keyboard.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#ifdef __cplusplus

    /**
     * @brief 寄存器实时查看页面
     *
     * 布局：
     *   右上角：模式切换滑块菜单 + 占位按钮条
     *   主区域：16×16 寄存器地图表格（可滚动）
     *           每格上部显示实际值，下部显示地址/名称/RW 属性
     *
     * 刷新机制：由 m5::M5Tab5Keyboard 寄存器访问拦截回调驱动——
     *   onShow 时调用 setRegAccessCallback 注册拦截回调，每次底层 I2C 读写完成后触发
     *   onRegAccess 回调更新 LVGL 标签；onHide 时清除回调。
     */
    class RegistryPage : public BasePage {
public:
    RegistryPage();
    virtual ~RegistryPage();

    virtual void onCreate() override;
    virtual void onShow() override;
    virtual void onHide() override;

    void setKeyboard(m5::M5Tab5Keyboard* kb);

private:
    // ---- 顶部占位按钮条 ----
    lv_obj_t* m_header_cont;
    lv_obj_t* m_btn_ph[4];  ///< 占位按钮 ×4

    // ---- 模式切换滑块菜单 ----
    lv_obj_t* m_mode_menu_cont;
    lv_obj_t* m_mode_indicator;
    lv_obj_t* m_mode_btns[3];
    int m_current_mode;

    // ---- 寄存器表 ----
    lv_obj_t* m_table_cont;       ///< 外层可滚动容器
    lv_obj_t* m_val_labels[256];  ///< 值标签：未定义寄存器为 nullptr
    lv_obj_t* m_cell_objs[256];   ///< 格子容器：未定义为 nullptr

    // ---- 状态 ----
    m5::M5Tab5Keyboard* m_keyboard;

    uint8_t m_hl_timers[256];      ///< 高亮倒计时（LVGL decay timer 递减）
    lv_timer_t* m_hl_decay_timer;  ///< 统一高亮衰减 LVGL 定时器

    // ---- 操作弹窗 ----
    lv_obj_t* m_popups[4];       ///< CLEAN/READ/WRITE/TEST 弹窗面板
    lv_obj_t* m_pop_reg_dd[3];   ///< 寄存器下拉框：[0]=CLEAN [1]=READ [2]=WRITE
    lv_obj_t* m_pop_val_rol[2];  ///< WRITE 值滚轮：[0]=高nibble [1]=低nibble
    int m_active_popup;          ///< 当前显示的弹窗索引，-1=全关

    // ---- 压力测试 ----
    lv_obj_t* m_test_timer_lbl;          ///< 时间显示 "0000:00"
    lv_obj_t* m_test_read_lbl;           ///< 读次数标签
    lv_obj_t* m_test_write_lbl;          ///< 写次数标签
    lv_obj_t* m_test_btn;                ///< START/PAUSE/RESUME 按鈕
    lv_obj_t* m_test_btn_lbl;            ///< 按鈕文字
    lv_obj_t* m_test_stop_bar;           ///< 长按进度条
    lv_timer_t* m_test_lv_timer;         ///< LVGL 1s 时钟更新
    TaskHandle_t m_test_task_hdl;        ///< 压力测试 FreeRTOS 任务句柄
    volatile int m_test_state;           ///< 0=IDLE 1=RUNNING 2=PAUSED
    volatile uint32_t m_test_elapsed;    ///< 已经秒数
    volatile uint32_t m_test_read_cnt;   ///< 读次数
    volatile uint32_t m_test_write_cnt;  ///< 写次数
    uint32_t m_test_btn_press_ms;        ///< 按下时刻 lv_tick_get()
    bool m_test_long_fired;              ///< 已触发长按停止
    lv_timer_t* m_test_auto_timer;       ///< 自动启动倒计时 LVGL 定时器（nullptr=禁用）
    int m_test_auto_sec;                 ///< 剩余倒计时秒数

    static RegistryPage* s_instance;

    // ---- 内部方法 ----
    void buildTable();
    void buildPopups();

    static void onRegAccess(uint8_t reg, uint8_t value, m5_tab5_reg_access_type_t type, void* user_data);
    static void hlDecayTimerCb(lv_timer_t* timer);
    static void onPhBtn(lv_event_t* e);
    static void onModeBtn(lv_event_t* e);
    static void onPopupClose(lv_event_t* e);
    static void onCleanExec(lv_event_t* e);
    static void onReadExec(lv_event_t* e);
    static void onWriteExec(lv_event_t* e);
    static void onTestBtn(lv_event_t* e);
    static void testTimerCb(lv_timer_t* t);
    static void testTaskFunc(void* arg);
    static void autoStartTimerCb(lv_timer_t* t);
};

#endif  // __cplusplus

#endif  // UI_PAGE_REGISTRY_H
