#ifndef UI_PAGE_BASE_H
#define UI_PAGE_BASE_H

#include "lvgl.h"
#include <esp_log.h>

#ifdef __cplusplus

/**
 * @brief 页面基类 - 所有页面的抽象基类
 *
 * 提供页面的生命周期管理和基本功能
 */
class BasePage {
public:
    /**
     * @brief 构造函数
     * @param name 页面名称
     */
    BasePage(const char* name);

    /**
     * @brief 虚析构函数
     */
    virtual ~BasePage();

    // ========== 生命周期方法 ==========

    /**
     * @brief 创建页面UI（纯虚函数，子类必须实现）
     *
     * 在此方法中创建页面的所有UI组件
     */
    virtual void onCreate() = 0;

    /**
     * @brief 页面显示时调用
     *
     * 页面从隐藏状态切换到显示状态时调用
     * 可以在此方法中恢复页面状态、启动动画等
     */
    virtual void onShow();

    /**
     * @brief 页面隐藏时调用
     *
     * 页面从显示状态切换到隐藏状态时调用
     * 可以在此方法中暂停动画、保存状态等
     */
    virtual void onHide();

    /**
     * @brief 销毁页面
     *
     * 释放页面占用的所有资源
     */
    virtual void onDestroy();

    // ========== 页面管理方法 ==========

    /**
     * @brief 获取页面容器对象
     * @return 页面容器的LVGL对象指针
     */
    lv_obj_t* getContainer() const { return m_container; }

    /**
     * @brief 获取页面名称
     * @return 页面名称字符串
     */
    const char* getName() const { return m_name; }

    /**
     * @brief 检查页面是否已创建
     * @return true=已创建 false=未创建
     */
    bool isCreated() const { return m_created; }

    /**
     * @brief 检查页面是否可见
     * @return true=可见, false=隐藏
     */
    bool isVisible() const { return m_visible; }

    // ========== 显示/隐藏方法 ==========

    /**
     * @brief 显示页面
     *
     * 清除HIDDEN标志，使页面可见
     */
    void show();

    /**
     * @brief 隐藏页面
     *
     * 添加HIDDEN标志，使页面不可见
     */
    void hide();

protected:
    lv_obj_t* m_container;    ///< 页面容器对象
    const char* m_name;       ///< 页面名称
    bool m_created;           ///< 是否已创建
    bool m_visible;           ///< 是否可见

    /**
     * @brief 创建页面容器
     * @param parent 父对象（通常是主screen）
     *
     * 创建一个全屏透明容器作为页面的根容器
     */
    void createContainer(lv_obj_t* parent);
};

#endif // __cplusplus

#endif // UI_PAGE_BASE_H
