/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef UI_TOP_MENU_H
#define UI_TOP_MENU_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 菜单范围枚举
 *
 * 用于控制菜单栏显示哪些模式选项
 */
typedef enum {
    M5TAB5_MENU_SCOPE_NONE     = 0,         ///< 无菜单项
    M5TAB5_MENU_SCOPE_NORMAL   = (1 << 1),  ///< 普通模式
    M5TAB5_MENU_SCOPE_STRING   = (1 << 2),  ///< 字符串模式（CHAR）
    M5TAB5_MENU_SCOPE_HID      = (1 << 3),  ///< HID 模式
    M5TAB5_MENU_SCOPE_REGISTRY = (1 << 4),  ///< 注册表/寄存器读写模式
    M5TAB5_MENU_SCOPE_FACTORY  = (1 << 0),  ///< 工厂测试模式
    M5TAB5_MENU_SCOPE_ALL      = 0xFFFF     ///< 所有模式
} M5Tab5MenuScope;

/**
 * @brief 创建顶部菜单
 *
 * 创建一个包含模式切换按钮的菜单栏
 *
 * @param parent 父 LVGL 对象
 * @param switch_callback 模式切换回调函数（接收模式索引：0=FACTORY, 1=Normal, 2=HID, 3=Char），可选
 * @paramscope_mask 菜单范围掩码，使用 M5Tab5MenuScope 枚举组合，默认显示所有
 *
 * @return 菜单容器对象指针
 *
 * @code
 * // 创建只显示普通模式和字符串模式的菜单
 * lv_obj_t* menu = createM5Tab5Menu(screen, nullptr,
 * M5TAB5_MENU_SCOPE_NORMAL | M5TAB5_MENU_SCOPE_STRING);
 * @endcode
 */
lv_obj_t* createM5Tab5Menu(lv_obj_t* parent, void (*switch_callback)(int) = nullptr,
                           int scope_mask = M5TAB5_MENU_SCOPE_ALL);

#ifdef __cplusplus
}
#endif

#endif  // UI_TOP_MENU_H
