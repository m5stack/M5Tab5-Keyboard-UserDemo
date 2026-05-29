/*
 * SPDX-FileCopyrightText: 2025-2026 exia
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * @file factorytest_keyboard.h
 * @brief 工厂测试键盘实现
 * 
 * 提供物理按键测试功能，支持可配置的行列数，用于验证硬件按键功能
 */

#ifndef FACTORYTEST_KEYBOARD_H
#define FACTORYTEST_KEYBOARD_H

#include "lvgl_keyboard.h"
#include <stdint.h>

#ifdef __cplusplus

/**
 * @brief 工厂测试键盘
 * 
 * 用于测试实体物理键盘，每个按键显示编号
 * 支持可视化按键测试，按键按下后会改变颜色
 */
class FactoryTestKeyboard : public LvglKeyboard {
public:
    /**
     * @brief 构造函数
     * 
     * @param rows 键盘行数，默认 5
     * @param cols 键盘列数，默认 14
     */
    FactoryTestKeyboard(uint8_t rows = 5, uint8_t cols = 14);
    
    /**
     * @brief 析构函数
     */
    virtual ~FactoryTestKeyboard();

    /**
     * @brief 创建键盘 UI
     * 
     * @param parent 父 LVGL 对象
     * @return 键盘容器对象指针
     */
    virtual lv_obj_t* create(lv_obj_t* parent) override;
    
    /**
     * @brief 设置按键禁用状态
     * 
     * @param key_id 按键 ID (0 到 rows*cols-1)
     * @param disabled true=禁用，false=启用
     */
    virtual void setKeyDisabled(uint16_t key_id, bool disabled) override;

    // ========================================================================
    // 虚拟键盘模拟 API
    // ========================================================================
    
    /**
     * @brief 模拟单字符输入
     * 
     * @param key_char 要模拟的字符
     */
    virtual void simulateCharacter(const char* key_char) override;
    
    /**
     * @brief 模拟字符串输入
     * 
     * @param str 要模拟的字符串
     * @param interval_ms 字符间间隔时间（毫秒），默认 100ms
     */
    virtual void simulateString(const char* str, uint32_t interval_ms = 100) override;

    // ========================================================================
    // 布局约束
    // ========================================================================
    
    /**
     * @brief 获取首选宽高比
     * @return 首选宽高比
     */
    virtual float getPreferredAspectRatio() const override { return 2.9f; }
    
    /**
     * @brief 获取最小宽高比
     * @return 最小宽高比
     */
    virtual float getMinAspectRatio() const override { return 2.0f; }
    
    /**
     * @brief 获取最大宽高比
     * @return 最大宽高比
     */
    virtual float getMaxAspectRatio() const override { return 4.0f; }
    
    /**
     * @brief 获取最大屏幕高度占比
     * @return 最大屏幕高度占比值（0.0-1.0）
     */
    virtual float getMaxScreenHeightPct() const override { return 0.60f; }

    /**
     * @brief 设置是否启用触摸
     * @param enabled true=启用触摸, false=禁用触摸(默认)
     */
    void setTouchEnabled(bool enabled);

    /**
     * @brief 按键物理按下（按下未松手）- 深灰色 + click 显示，无 OK
     * @param key_id 按键ID (0到rows*cols-1)
     */
    void simulatePhysicalKeyDown(uint16_t key_id);

    /**
     * @brief 按下中超过长按门槛（仍未松手）- 切换深灰色 + long
     * @param key_id 按键ID (0到rows*cols-1)
     */
    void simulatePhysicalKeyLongPressing(uint16_t key_id);

    /**
     * @brief 按键短按松开 - 深绿色 + OK，无 long
     * @param key_id 按键ID (0到rows*cols-1)
     */
    void simulatePhysicalKeyShortRelease(uint16_t key_id);

    /**
     * @brief 按键长按松开 - 深绿色 + OK + hold
     * @param key_id 按键ID (0到rows*cols-1)
     */
    void simulatePhysicalKeyLongRelease(uint16_t key_id);

    /**
     * @brief 检查按键是否已完成测试（短按或长按均算作已完成）
     * @param key_id 按键ID
     * @return true=已完成测试，false=未完成
     */
    bool isKeyPressed(uint16_t key_id) const;

    /**
     * @brief 重置所有按键状态
     */
    void resetAllKeys();

    /**
     * @brief 取消所有「按下中」的按键（深灰 pending 状态），已完成（绿色）的保留
     *
     * 键盘断线重连后调用，消除因丢失 release 事件而卡在 pressing 状态的按键
     */
    void cancelPressingKeys();

protected:
    uint8_t m_rows;           // 键盘行数
    uint8_t m_cols;           // 键盘列数
    uint16_t m_key_count;     // 按键总数
    bool m_touch_enabled;     // 触摸启用状态

    // 按键按下中状态，true=按下未松开
    bool* m_key_pressing;

    // 按下中已达到长按门槛（依然未松手）
    bool* m_key_long_pressing;

    // 按键短按完成: true=短按松开(深绿+OK,无hold)
    bool* m_key_pressed;

    // 按键长按完成: true=长按松开(深绿+OK+hold)
    bool* m_key_hold_pressed;

    // 按键按钮对象数组
    lv_obj_t** m_key_buttons;

    // hold 标签对象数组
    lv_obj_t** m_key_hold_labels;

    // 网格描述符数组 (需要持久化，LVGL 会持续访问)
    int32_t* m_col_dsc;       // 列描述符
    int32_t* m_row_dsc;       // 行描述符

    // 事件处理
    static void eventHandlerStatic(lv_event_t* e);  // 静态事件处理器
    void handleKeyEvent(lv_event_t* e);             // 处理按键事件

    // 更新按键颜色
    void updateKeyColor(uint16_t key_id);           // 根据状态更新按键颜色
};

#endif // __cplusplus

#endif // FACTORYTEST_KEYBOARD_H
