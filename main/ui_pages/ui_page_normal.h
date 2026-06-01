/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef UI_PAGE_NORMAL_H
#define UI_PAGE_NORMAL_H

#include "ui_page_base.h"
#include "m5tab5_keyboard.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"

#ifdef __cplusplus

    /**
     * @brief 正常模式页面
     *
     * 提供完整的QWERTY键盘输入功能
     * 支持大小写切换、符号模式、按键重复等
     */
    class NormalPage : public BasePage {
public:
    NormalPage();
    virtual ~NormalPage();

    // 生命周期方法
    virtual void onCreate() override;
    virtual void onShow() override;
    virtual void onHide() override;

    /**
     * @brief 处理物理按键事件
     * @param key_id 按键ID (0-69)
     * @param pressed true=按下, false=释放
     *
     * 处理物理键盘的按下和释放事件，实现按键重复功能
     */
    void handlePhysicalKey(uint16_t key_id, bool pressed);

    /**
     * @brief 获取键盘对象
     * @return M5Tab5Keyboard指针
     */
    M5Tab5Keyboard* getKeyboard() const
    {
        return m_keyboard;
    }

private:
    lv_obj_t* m_textarea;        ///< 文本输入区域
    M5Tab5Keyboard* m_keyboard;  ///< M5Tab5键盘对象

    // 按键重复逻辑
    TimerHandle_t m_repeat_timer;      ///< 重复定时器
    uint16_t m_repeat_key_id;          ///< 当前重复的按键ID
    bool m_repeat_first_wait;          ///< 是否在初始延迟阶段
    SemaphoreHandle_t m_repeat_mutex;  ///< 保护重复状态

    /**
     * @brief 重复定时器回调函数
     * @param xTimer 定时器句柄
     */
    static void repeatTimerCallback(TimerHandle_t xTimer);
};

#endif  // __cplusplus

#endif  // UI_PAGE_NORMAL_H
