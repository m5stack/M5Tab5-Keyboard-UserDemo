/*
 * SPDX-FileCopyrightText: 2025-2026 exia
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * @file lvgl_keyboard.h
 * @brief LVGL 键盘基类
 * 
 * 提供键盘对象的基础框架，包括热键系统、按键禁用、文本模拟等功能
 * 所有具体的键盘实现都应继承此类
 */

#ifndef lvgl_KEYBOARD_H
#define lvgl_KEYBOARD_H

#include "lvgl.h"
#include <vector>

// 包含按键定义（使用 kb_key_id_t 枚举）
#include "m5tab5_keyboard_def.h"

#ifdef __cplusplus

// ============================================================================
// 热键系统定义
// ============================================================================

/**
 * @brief 热键事件类型
 */
typedef enum {
    HOTKEY_EVENT_PRESSED = 0,   // 按键按下
    HOTKEY_EVENT_RELEASED,      // 按键释放
    HOTKEY_EVENT_CLICKED,       // 按键点击
    HOTKEY_EVENT_BEFORE_INPUT   // 字符输入前
} hotkey_event_type_t;

/**
 * @brief 热键事件结构体
 */
typedef struct {
    uint16_t key_id;              // 按键 ID
    const char* key_label;        // 按键标签（如 "a", "ESC"）
    const char* key_value;        // 按键值（如 "a", "\x1B"）
    hotkey_event_type_t event;    // 事件类型
    bool ctrl_pressed;            // CTRL 是否按下
    bool alt_pressed;             // ALT 是否按下
    bool shift_pressed;           // Shift(Aa) 是否激活
    void* user_data;              // 用户自定义数据
} hotkey_event_t;

/**
 * @brief 热键回调函数签名
 * @param event 热键事件
 * @param user_data 用户数据
 * @return 返回 true 表示拦截事件（阻止默认行为），false 表示继续处理
 */
typedef bool (*hotkey_callback_t)(const hotkey_event_t* event, void* user_data);

/**
 * @brief 热键注册结构（内部使用）
 */
typedef struct {
    int hotkey_id;                // 热键 ID（用于注销）
    uint16_t key_id;              // 按键 ID (0xFFFF 表示匹配所有)
    hotkey_event_type_t event;    // 事件类型
    bool require_ctrl;            // 是否需要 CTRL
    bool require_alt;             // 是否需要 ALT
    bool require_shift;           // 是否需要 Shift
    bool require_ctrl_only;       // 是否只需要 CTRL (排除 ALT)
    bool require_alt_only;        // 是否只需要 ALT (排除 CTRL)
    hotkey_callback_t callback;   // 回调函数
    void* user_data;              // 用户数据
} hotkey_registration_t;

// ============================================================================

/**
 * @brief LVGL 键盘基类
 * 
 * 提供键盘的通用接口和热键系统实现
 * 所有具体的键盘类都应继承此基类
 */
class LvglKeyboard {
public:
    /**
     * @brief 构造函数
     */
    LvglKeyboard();
    
    /**
     * @brief 析构函数
     */
    virtual ~LvglKeyboard();

    /**
     * @brief 创建键盘 UI 对象
     * @param parent 父对象
     * @return 键盘容器对象指针
     */
    virtual lv_obj_t* create(lv_obj_t* parent) = 0;

    /**
     * @brief 设置目标文本区域
     * @param ta 文本区域对象
     */
    virtual void setTextArea(lv_obj_t* ta);

    /**
     * @brief 设置按键禁用状态
     * @param key_id 按键 ID
     * @param disabled 禁用状态
     */
    virtual void setKeyDisabled(uint16_t key_id, bool disabled) = 0;

    /**
     * @brief 获取底层 LVGL 容器对象
     */
    lv_obj_t* getContainer() const { return m_kb_cont; }

    /**
     * @brief 布局约束（虚函数，带默认值）
     */
    virtual float getPreferredAspectRatio() const { return 2.9f; }  // 首选宽高比
    virtual float getMinAspectRatio() const { return 2.0f; }        // 最小宽高比
    virtual float getMaxAspectRatio() const { return 4.0f; }        // 最大宽高比
    virtual float getMaxScreenHeightPct() const { return 0.60f; }   // 最大屏幕高度占比

    /**
     * @brief 虚拟键盘模拟 API - 模拟单字符输入
     * @param key_char 要模拟的字符（例如 "a", "1", "!"）
     */
    virtual void simulateCharacter(const char* key_char) = 0;

    /**
     * @brief 虚拟键盘模拟 API - 模拟字符串输入
     * @param str 要模拟的字符串
     * @param interval_ms 字符间隔时间（毫秒）
     */
    virtual void simulateString(const char* str, uint32_t interval_ms = 100) = 0;

    // ========================================================================
    // 热键系统 API
    // ========================================================================

    /**
     * @brief 注册热键回调
     * 
     * 注册一个热键回调函数，当指定的按键和修饰键组合被触发时调用
     * 
     * @param key_id 按键 ID（使用 kb_key_id_t 枚举，或 HOTKEY_ANY_KEY 匹配所有按键）
     * @param event 事件类型（PRESSED/RELEASED/CLICKED/BEFORE_INPUT）
     * @param callback 回调函数指针，返回 true 表示拦截事件，false 表示继续处理
     * @param user_data 用户自定义数据，会传递给回调函数（可选，默认 nullptr）
     * @param require_ctrl 是否需要 CTRL 按下（默认 false）
     * @param require_alt 是否需要 ALT 按下（默认 false）
     * @param require_shift 是否需要 Shift 激活（默认 false）
     * @param require_ctrl_only 是否只需要 CTRL，排除 Ctrl+Alt 组合（默认 false）
     * @param require_alt_only 是否只需要 ALT，排除 Ctrl+Alt 组合（默认 false）
     * 
     * @return 热键 ID（用于注销），失败返回 -1
     * 
     * @note 修饰键匹配规则：
     *   - require_ctrl=true, require_alt=true: 需要同时按下 Ctrl+Alt
     *   - require_ctrl=true, require_ctrl_only=true: 只需要 Ctrl（不能有 Alt）
     *   - require_alt=true, require_alt_only=true: 只需要 Alt（不能有 Ctrl）
     * 
     * @see unregisterHotkey(), clearAllHotkeys()
     * 
     * @code
     * // 注册 Ctrl+S 保存热键
     * bool save_callback(const hotkey_event_t* event, void* user_data) {
     *     printf("保存文件\n");
     *     return true;  // 拦截事件
     * }
     * int id = kb.registerHotkey(KB_KEY_S, HOTKEY_EVENT_PRESSED, 
     *                            save_callback, nullptr, true, false, false, true, false);
     * @endcode
     */
    int registerHotkey(
        uint16_t key_id,
        hotkey_event_type_t event,
        hotkey_callback_t callback,
        void* user_data = nullptr,
        bool require_ctrl = false,
        bool require_alt = false,
        bool require_shift = false,
        bool require_ctrl_only = false,
        bool require_alt_only = false
    );

    /**
     * @brief 注销热键回调
     * 
     * 根据热键 ID 注销之前注册的热键回调
     * 
     * @param hotkey_id 热键 ID（registerHotkey 返回值）
     * 
     * @return 成功返回 true，失败返回 false
     * 
     * @see registerHotkey(), clearAllHotkeys()
     */
    bool unregisterHotkey(int hotkey_id);

    /**
     * @brief 清除所有热键回调
     * 
     * 移除所有已注册的热键回调
     * 
     * @see registerHotkey(), unregisterHotkey()
     */
    void clearAllHotkeys();

    /**
     * @brief 启用/禁用热键系统
     * 
     * 控制热键系统的启用状态。禁用后，热键不会触发回调
     * 
     * @param enabled true=启用热键系统，false=禁用热键系统
     * 
     * @see isHotkeyEnabled()
     */
    void setHotkeyEnabled(bool enabled);

    /**
     * @brief 获取热键系统启用状态
     * 
     * @return true=热键系统已启用，false=热键系统已禁用
     * 
     * @see setHotkeyEnabled()
     */
    bool isHotkeyEnabled() const { return m_hotkey_enabled; }

protected:
    lv_obj_t * m_kb_cont;      // 键盘容器对象
    lv_obj_t * m_textarea;     // 目标文本区域对象

    // 通用辅助函数：处理标准输入
    void sendTextToTextarea(const char* text);

    // ========================================================================
    // 热键系统成员变量
    // ========================================================================
    std::vector<hotkey_registration_t> m_hotkey_registrations;  // 热键注册列表
    int m_next_hotkey_id;      // 下一个热键 ID
    bool m_hotkey_enabled;     // 热键系统启用状态

    // 修饰键状态
    bool m_ctrl_pressed;       // CTRL 键按下状态
    bool m_alt_pressed;        // ALT 键按下状态

    /**
     * @brief 触发热键回调
     * @param event 热键事件
     * @return 返回 true 表示事件被拦截
     */
    bool triggerHotkey(const hotkey_event_t* event);
};

#endif // __cplusplus

#endif // lvgl_KEYBOARD_H
