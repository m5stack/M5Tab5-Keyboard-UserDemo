#include "ui_page_manager.h"
#include "lvgl_port.h"
#include <string.h>
#include <stdarg.h>

static const char* TAG = "PageManager";

// 静态成员初始化
PageManager* PageManager::s_instance = nullptr;

PageManager::PageManager()
    : m_current_page(nullptr),
      m_current_page_id(-1),
      m_parent(nullptr),
      m_global_menu(nullptr),
      m_global_log(nullptr)
{
    // 初始化页面数据
    for (int i = 0; i < PAGE_ID_MAX; i++) {
        m_pages[i] = nullptr;
    }

    ESP_LOGI(TAG, "PageManager created");
}

PageManager::~PageManager()
{
    ESP_LOGI(TAG, "PageManager destroying");

    // 销毁所有页面
    for (int i = 0; i < PAGE_ID_MAX; i++) {
        if (m_pages[i]) {
            delete m_pages[i];
            m_pages[i] = nullptr;
        }
    }

    // 注意：全局组件（菜单、log）由外部管理，这里不删除
}

PageManager* PageManager::getInstance()
{
    if (s_instance == nullptr) {
        s_instance = new PageManager();
    }
    return s_instance;
}

void PageManager::destroyInstance()
{
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
        ESP_LOGI(TAG, "PageManager instance destroyed");
    }
}

void PageManager::registerPage(int page_id, BasePage* page)
{
    if (page_id < 0 || page_id >= PAGE_ID_MAX) {
        ESP_LOGE(TAG, "Invalid page_id: %d", page_id);
        return;
    }

    if (!page) {
        ESP_LOGE(TAG, "Page is null for page_id: %d", page_id);
        return;
    }

    if (m_pages[page_id]) {
        ESP_LOGW(TAG, "Page %d already registered, replacing", page_id);
        delete m_pages[page_id];
    }

    m_pages[page_id] = page;
    ESP_LOGI(TAG, "Page %d (%s) registered", page_id, page->getName());
}

bool PageManager::switchToPage(int page_id)
{
    if (page_id < 0 || page_id >= PAGE_ID_MAX) {
        ESP_LOGE(TAG, "Invalid page_id: %d", page_id);
        return false;
    }

    // 检查是否切换到当前页面
    if (page_id == m_current_page_id && m_current_page && m_current_page->isVisible()) {
        ESP_LOGI(TAG, "Already on page %d, skipping", page_id);
        return false;
    }

    ESP_LOGI(TAG, "Switching from page %d to page %d", m_current_page_id, page_id);

    // 1. 隐藏当前页面
    if (m_current_page) {
        ESP_LOGI(TAG, "Hiding current page: %s", m_current_page->getName());
        m_current_page->hide();
    }

    // 2. 获取目标页面
    BasePage* target_page = m_pages[page_id];
    if (!target_page) {
        ESP_LOGE(TAG, "Page %d not registered", page_id);
        return false;
    }

    // 3. 懒加载：首次访问时创建
    if (!target_page->isCreated()) {
        ESP_LOGI(TAG, "Page %d (%s) not created, creating now", page_id, target_page->getName());

        if (!m_parent) {
            ESP_LOGE(TAG, "Parent object not set, cannot create page");
            return false;
        }

        target_page->onCreate();

        if (!target_page->isCreated()) {
            ESP_LOGE(TAG, "Failed to create page %d", page_id);
            return false;
        }
    }

    // 4. 显示目标页面
    ESP_LOGI(TAG, "Showing target page: %s", target_page->getName());
    target_page->show();

    // 5. 更新当前页面指针
    m_current_page = target_page;
    m_current_page_id = page_id;

    // 6. 确保全局组件在最前面
    if (m_global_menu) {
        lv_obj_move_foreground(m_global_menu);
    }
    if (m_global_log) {
        lv_obj_move_foreground(m_global_log);
    }

    ESP_LOGI(TAG, "Page switch completed: now on page %d (%s)", page_id, target_page->getName());
    return true;
}

void PageManager::updateLog(const char* fmt, ...)
{
    if (!m_global_log) {
        return;
    }

    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    // 注意：调用此函数前应该已经获取了LVGL锁
    lv_label_set_text(m_global_log, buf);
}
