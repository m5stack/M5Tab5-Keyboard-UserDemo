/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef UI_PAGE_MANAGER_H
#define UI_PAGE_MANAGER_H

#include "ui_page_base.h"
#include "lvgl.h"
#include <esp_log.h>

#ifdef __cplusplus

/**
 * @brief 页面ID枚举
 */
typedef enum {
    PAGE_ID_FACTORY  = 0,  ///< 工厂测试页面
    PAGE_ID_NORMAL   = 1,  ///< 正常模式页面
    PAGE_ID_STRING   = 2,  ///< 字符模式页面
    PAGE_ID_HID      = 3,  ///< HID模式页面
    PAGE_ID_REGISTRY = 4,  ///< 注册表/寄存器读写页面
    PAGE_ID_MAX      = 5   ///< 页面总数
} page_id_t;

/**
 * @brief 页面管理器 - 单例模式
 *
 * 负责管理所有页面的创建、切换、销毁
 * 管理全局组件（菜单、log标签）
 */
class PageManager {
public:
    /**
     * @brief 获取单例实例
     * @return PageManager单例指针
     */
    static PageManager* getInstance();

    /**
     * @brief 销毁单例实例
     */
    static void destroyInstance();

    // ========== 页面注册 ==========

    /**
     * @brief 注册页面
     * @param page_id 页面ID
     * @param page 页面对象指针
     */
    void registerPage(int page_id, BasePage* page);

    // ========== 页面切换 ==========

    /**
     * @brief 切换到指定页面
     * @param page_id 目标页面ID
     * @return true 如果成功切换到新页面; false 如果已经在该页面或切换失败
     *
     * 流程：
     * 1. 隐藏当前页面
     * 2. 检查目标页面是否已创建，未创建则创建
     * 3. 显示目标页面
     * 4. 更新当前页面指针
     */
    bool switchToPage(int page_id);

    // ========== 获取当前页面 ==========

    /**
     * @brief 获取当前页面对象
     * @return 当前页面指针
     */
    BasePage* getCurrentPage() const
    {
        return m_current_page;
    }

    /**
     * @brief 获取当前页面ID
     * @return 当前页面ID
     */
    int getCurrentPageId() const
    {
        return m_current_page_id;
    }

    // ========== 全局组件管理 ==========

    /**
     * @brief 设置全局菜单对象
     * @param menu 菜单对象指针
     */
    void setGlobalMenu(lv_obj_t* menu)
    {
        m_global_menu = menu;
    }

    /**
     * @brief 设置全局log标签对象
     * @param log log标签对象指针
     */
    void setGlobalLog(lv_obj_t* log)
    {
        m_global_log = log;
    }

    /**
     * @brief 获取全局log标签对象
     * @return log标签对象指针
     */
    lv_obj_t* getGlobalLog() const
    {
        return m_global_log;
    }

    /**
     * @brief 获取全局菜单对象
     * @return 菜单对象指针
     */
    lv_obj_t* getGlobalMenu() const
    {
        return m_global_menu;
    }

    /**
     * @brief 更新全局log标签文本
     * @param fmt 格式化字符串
     * @param ... 可变参数
     */
    void updateLog(const char* fmt, ...);

    /**
     * @brief 设置页面容器的父对象
     * @param parent 父对象（通常是主screen）
     */
    void setParent(lv_obj_t* parent)
    {
        m_parent = parent;
    }

    /**
     * @brief 获取页面容器的父对象
     * @return 父对象指针
     */
    lv_obj_t* getParent() const
    {
        return m_parent;
    }

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    PageManager();

    /**
     * @brief 私有析构函数（单例模式）
     */
    ~PageManager();

    // 禁止拷贝和赋值
    PageManager(const PageManager&)            = delete;
    PageManager& operator=(const PageManager&) = delete;

    static PageManager* s_instance;  ///< 单例实例指针

    BasePage* m_pages[PAGE_ID_MAX];  ///< 页面数组
    BasePage* m_current_page;        ///< 当前页面指针
    int m_current_page_id;           ///< 当前页面ID

    lv_obj_t* m_parent;       ///< 页面容器的父对象
    lv_obj_t* m_global_menu;  ///< 全局菜单对象
    lv_obj_t* m_global_log;   ///< 全局log标签对象
};

#endif  // __cplusplus

#endif  // UI_PAGE_MANAGER_H
