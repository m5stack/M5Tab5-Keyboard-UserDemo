#include "ui_page_hid.h"
#include "ui_page_manager.h"
#include "m5_tab5_keyboard.h"
#include "lvgl_port.h"
#include "usb_init.h"
#include <esp_log.h>
#include <cstdio>

LV_FONT_DECLARE(lv_font_montserrat_24);
LV_FONT_DECLARE(lv_font_montserrat_16);

static const char* TAG = "HIDPage";

HIDPage* HIDPage::s_instance = nullptr;

HIDPage::HIDPage()
    : BasePage("HID"),
      m_label(nullptr),
      // m_status_label(nullptr),
      // m_count_label(nullptr),
      m_ctrl_container(nullptr),
      m_roller_port(nullptr),
      m_btn_start(nullptr),
      m_btn_label(nullptr),
      m_ta_log(nullptr),
      m_keyboard(nullptr),
      m_hid_task(nullptr),
      m_task_running(false),
      m_key_count(0),
      m_is_usb_active(false)
{
    s_instance = this;
}

HIDPage::~HIDPage()
{
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

void HIDPage::setKeyboard(m5::M5Tab5Keyboard* kb)
{
    m_keyboard = kb;
}

void HIDPage::onCreate()
{
    ESP_LOGI(TAG, "Creating HID page");

    // 创建页面容器
    createContainer(PageManager::getInstance()->getParent());

    if (!m_container) {
        ESP_LOGE(TAG, "Failed to create container");
        return;
    }

    // 创建标题标签
    m_label = lv_label_create(m_container);
    lv_label_set_text(m_label, "HID Keyboard Mode");
    lv_obj_set_style_text_font(m_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_align(m_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(m_label, LV_ALIGN_TOP_MID, 0, 10);

    // 创建控制区域容器 
    m_ctrl_container = lv_obj_create(m_container);
    // 高度 25% (仿照 StringPage 输入框高度)
    lv_obj_set_size(m_ctrl_container, lv_pct(95), lv_pct(25)); 
    lv_obj_align(m_ctrl_container, LV_ALIGN_TOP_MID, 0, 100);
    
    lv_obj_set_style_bg_color(m_ctrl_container, lv_color_hex(0x202020), 0);
    lv_obj_set_style_bg_opa(m_ctrl_container, LV_OPA_0, 0); // 透明
    lv_obj_set_style_border_width(m_ctrl_container, 0, 0);
    lv_obj_clear_flag(m_ctrl_container, LV_OBJ_FLAG_SCROLLABLE);

    // 端口选择滚动条
    m_roller_port = lv_roller_create(m_ctrl_container);
    lv_roller_set_options(m_roller_port, 
                          "USB2.0 PHY (HS)\n"
                          "USBC OTG (FS)",
                          LV_ROLLER_MODE_NORMAL);
    
    // 5%, 左右布局
    lv_obj_set_width(m_roller_port, lv_pct(45)); 
    lv_roller_set_visible_row_count(m_roller_port, 2); 
    lv_obj_align(m_roller_port, LV_ALIGN_LEFT_MID, 10, 0);
    
    // 样式设置
    lv_obj_set_style_text_font(m_roller_port, &lv_font_montserrat_24, 0); 
    lv_obj_set_style_bg_color(m_roller_port, lv_color_hex(0x202020), 0);
    lv_obj_set_style_text_color(m_roller_port, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_bg_color(m_roller_port, lv_palette_main(LV_PALETTE_BLUE), LV_PART_SELECTED);
    lv_obj_set_style_text_color(m_roller_port, lv_color_hex(0xFFFFFF), LV_PART_SELECTED);
    lv_obj_set_style_text_line_space(m_roller_port, 10, 0);
    lv_obj_add_flag(m_roller_port, LV_OBJ_FLAG_CLICKABLE); 

    // 启动/停止按钮
    m_btn_start = lv_btn_create(m_ctrl_container);
    lv_obj_set_size(m_btn_start, lv_pct(45), lv_pct(60));
    lv_obj_align(m_btn_start, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_add_event_cb(m_btn_start, startBtnEventHandler, LV_EVENT_CLICKED, this);
    
    m_btn_label = lv_label_create(m_btn_start);
    lv_label_set_text(m_btn_label, "Start HID");
    lv_obj_set_style_text_font(m_btn_label, &lv_font_montserrat_24, 0);
    lv_obj_center(m_btn_label);

    // 日志文本框 (完全仿照 ui_page_string 的位置和大小)
    // String 模式 Size(95%, 50%). Align to Textarea(Top:100, H:25%) -> Top log roughly at 50% screen height
    m_ta_log = lv_textarea_create(m_container);
    lv_obj_set_size(m_ta_log, lv_pct(95), lv_pct(50));
    // 修改对齐方式：跟随上方控件，而不是强制到底部
    // 这样在高分辨率屏幕上，底部会留出空间给 info_label，防止重叠
    lv_obj_align_to(m_ta_log, m_ctrl_container, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    
    // 设置只读
    lv_obj_clear_flag(m_ta_log, LV_OBJ_FLAG_CLICKABLE); 
    lv_textarea_set_cursor_click_pos(m_ta_log, false);
    
    // 样式设置
    lv_textarea_set_placeholder_text(m_ta_log, "USB HID Log...");
    lv_obj_set_style_text_font(m_ta_log, &lv_font_montserrat_16, 0);
    lv_obj_set_style_bg_color(m_ta_log, lv_color_hex(0x101010), 0);
    lv_obj_set_style_text_color(m_ta_log, lv_color_hex(0xFFFFFF), 0);

    // 启用多色支持
    lv_obj_t* label = lv_textarea_get_label(m_ta_log);
    lv_label_set_recolor(label, true);

    // 底部说明标签 (仿照 ui_page_string)
    lv_obj_t* info_label = lv_label_create(m_container);
    lv_label_set_text(info_label, "HID Mode: USB Keyboard Output");
    lv_obj_set_style_text_color(info_label, lv_color_hex(0xAAAAAA), 0);
    // String页是 -10
    lv_obj_align(info_label, LV_ALIGN_BOTTOM_MID, 0, -10);

    ESP_LOGI(TAG, "HID page created successfully");
}

void HIDPage::onShow()
{
    BasePage::onShow();
    ESP_LOGI(TAG, "HID page shown");

    if (m_keyboard) {
        // 进入本页面，如果HID已经启动了，确保是HID模式；如果没启动，先保持NORMAL
        if (m_is_usb_active) {
             m_keyboard->setMode(M5_TAB5_KB_MODE_HID);
        } else {
             m_keyboard->setMode(M5_TAB5_KB_MODE_NORMAL);
        }
    }
    
    // 注册日志回调 (每次Show都注册，确保能收到log)
    usb_hid_set_log_cb(logCallback);
    
    updateStatusDisplay();
}

void HIDPage::onHide()
{
    BasePage::onHide();
    ESP_LOGI(TAG, "HID page hidden");

    if (m_keyboard) {
        m_keyboard->setMode(M5_TAB5_KB_MODE_NORMAL);
    }
    
    // 离开页面自动停止
    if (m_is_usb_active) {
        onStartBtnClicked(); 
    }
    
    usb_hid_set_log_cb(NULL);
}

void HIDPage::startBtnEventHandler(lv_event_t* e)
{
    HIDPage* page = (HIDPage*)lv_event_get_user_data(e);
    if (page) {
        page->onStartBtnClicked();
    }
}

void HIDPage::onStartBtnClicked()
{
    if (m_is_usb_active) {
        // STOP Logic
        ESP_LOGI(TAG, "Stopping HID...");
        appendLog("Stopping HID...", 0xFFFF00);

        // 停止任务
        if (m_hid_task) {
            m_task_running = false;
            // 简单等待，实际中可能需要信号量
            vTaskDelay(pdMS_TO_TICKS(150));
            m_hid_task = nullptr;
        }

        // 恢复键盘模式
        if (m_keyboard) {
            m_keyboard->setMode(M5_TAB5_KB_MODE_NORMAL);
        }

        // 反初始化USB
        usb_hid_keyboard_deinit();

        // 更新UI
        lvgl_port_lock(0);
        lv_label_set_text(m_btn_label, "Start HID");
        // 恢复下拉框可用
        if (m_roller_port) {
            lv_obj_clear_state(m_roller_port, LV_STATE_DISABLED);
            lv_obj_add_flag(m_roller_port, LV_OBJ_FLAG_CLICKABLE);
        }
        
        // lv_label_set_text(m_status_label, "Status: Stopped");
        // lv_obj_set_style_text_color(m_status_label, lv_color_hex(0xCCCCCC), 0);
        lvgl_port_unlock();

        m_is_usb_active = false;

    } else {
        // START Logic
        uint16_t selected = lv_roller_get_selected(m_roller_port);
        usb_port_t port = (selected == 0) ? USB_PORT_1_HIGH_SPEED : USB_PORT_0_FULL_SPEED;
        
        ESP_LOGI(TAG, "Starting HID on port %d...", port);
        
        // 更新UI
        lvgl_port_lock(0);
        if (m_roller_port) {
            lv_obj_add_state(m_roller_port, LV_STATE_DISABLED);
        }
        lv_label_set_text(m_btn_label, "Stop HID");
        lvgl_port_unlock();

        // 初始化USB
        esp_err_t init_ret = usb_hid_keyboard_init(port);
        if (init_ret != ESP_OK) {
             appendLog(esp_err_to_name(init_ret), 0xFF0000);
             lvgl_port_lock(0);
             if (m_roller_port) lv_obj_clear_state(m_roller_port, LV_STATE_DISABLED);
             lv_label_set_text(m_btn_label, "Start HID");
             lvgl_port_unlock();
             return;
        }

        // 初始化键盘
        if (m_keyboard) {
            m_keyboard->setMode(M5_TAB5_KB_MODE_HID);
        }

        // 启动任务
        m_key_count = 0;
        m_task_running = true;
        xTaskCreate(hidTaskFunction, "hid_task", 4096, this, 5, &m_hid_task);
        
        m_is_usb_active = true;
        updateStatusDisplay();
    }
}

void HIDPage::logCallback(const char* msg, uint32_t color)
{
    if (s_instance) {
        s_instance->appendLog(msg, color);
    }
}

void HIDPage::appendLog(const char* msg, uint32_t color)
{
    if (!m_ta_log) return;
    
    lvgl_port_lock(0);
    
    char color_hex[8];
    snprintf(color_hex, sizeof(color_hex), "#%06lX", (unsigned long)(color & 0xFFFFFF));
    
    // 使用 LVGL recolor 格式: #RRGGBB text#
    char formatted_msg[256];
    // 使用 > 作为前缀，保持简约
    snprintf(formatted_msg, sizeof(formatted_msg), "%s > %s#\n", color_hex, msg);
    
    lv_textarea_add_text(m_ta_log, formatted_msg);
    lvgl_port_unlock();
}

void HIDPage::handleHIDInput(uint8_t modifier, uint8_t key_code)
{
    // Debug log to verify input reception
    ESP_LOGI(TAG, "handleHIDInput: mod=0x%02X, key=0x%02X, active=%d", modifier, key_code, m_is_usb_active);

    if (m_is_usb_active) {
         esp_err_t ret = usb_hid_send_report(modifier, key_code);
         if (ret == ESP_OK) {
             m_key_count++;
             // Log detailed key info: Mod: 0x00 Key: 0x04 (A)
             char log_buf[64];
             snprintf(log_buf, sizeof(log_buf), "Key Sent: Mod=0x%02X Key=0x%02X", modifier, key_code);
             appendLog(log_buf, 0x00FF00); // Green for success
         } else {
             ESP_LOGE(TAG, "usb_hid_send_report failed: %s", esp_err_to_name(ret));
             char err_buf[64];
             snprintf(err_buf, sizeof(err_buf), "Send Fail: %s (Key=0x%02X)", esp_err_to_name(ret), key_code);
             appendLog(err_buf, 0xFF0000); // Red for error
         }
    } else {
        // Log skipped input
        // char log_buf[64];
        // snprintf(log_buf, sizeof(log_buf), "Skip: USB Inactive (Key=0x%02X)", key_code);
        // appendLog(log_buf, 0x808080); // Grey for skipped
        ESP_LOGW(TAG, "HID Input received but USB not active");
    }
}

void HIDPage::hidTaskFunction(void* param)
{
    HIDPage* page = static_cast<HIDPage*>(param);
    
    uint32_t last_status_update = 0;
    bool last_usb_status = false;
    bool status_announced = false;

    while (page->m_task_running) {
        // 检查USB连接状态
        bool usb_mounted = usb_hid_is_mounted();
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

        if (now - last_status_update > 500 || usb_mounted != last_usb_status) {
            last_status_update = now;
            if (!status_announced || usb_mounted != last_usb_status) {
                page->appendLog(usb_mounted ? "USB mounted, HID ready" : "Waiting for USB host mount...", 
                                usb_mounted ? 0x00FF00 : 0xFFFF00);
                status_announced = true;
            }
            last_usb_status = usb_mounted;
            page->updateStatusDisplay();
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // 任务退出时
    vTaskDelete(NULL);
}

void HIDPage::updateStatusDisplay()
{
    if (!m_is_usb_active) return;
}


