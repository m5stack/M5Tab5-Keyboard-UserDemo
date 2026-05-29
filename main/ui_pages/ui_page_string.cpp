#include "ui_page_string.h"
#include "ui_page_manager.h"
#include "lvgl_port.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

LV_FONT_DECLARE(lv_font_montserrat_16);
LV_FONT_DECLARE(lv_font_montserrat_24);

static const char* TAG = "StringPage";

StringPage::StringPage()
    : BasePage("String"),
      m_textarea(nullptr),
      m_log_ta(nullptr)
{
}

StringPage::~StringPage()
{
}

void StringPage::onCreate()
{
    ESP_LOGI(TAG, "Creating Char page");

    // 创建页面容器
    createContainer(PageManager::getInstance()->getParent());

    if (!m_container) {
        ESP_LOGE(TAG, "Failed to create container");
        return;
    }

    // 1. 创建文本输入区域（上方，用于显示输入内容）
    m_textarea = lv_textarea_create(m_container);
    lv_obj_set_size(m_textarea, lv_pct(95), lv_pct(25));
    lv_obj_align(m_textarea, LV_ALIGN_TOP_MID, 0, 100);
    lv_textarea_set_placeholder_text(m_textarea, "Char Mode Input...");
    lv_obj_set_style_text_font(m_textarea, &lv_font_montserrat_24, 0);

    // 光标设置 (输入框)
    lv_obj_set_style_bg_opa(m_textarea, LV_OPA_COVER, LV_PART_CURSOR);
    lv_obj_set_style_bg_color(m_textarea, lv_palette_main(LV_PALETTE_BLUE), LV_PART_CURSOR);
    lv_obj_set_style_anim_time(m_textarea, 400, LV_PART_CURSOR);

    // 保持输入框焦点
    lv_obj_add_event_cb(m_textarea, [](lv_event_t * e){
        lv_event_code_t code = lv_event_get_code(e);
        lv_obj_t * t = (lv_obj_t *)lv_event_get_target(e);
        if(code == LV_EVENT_DEFOCUSED) {
            lv_obj_add_state(t, LV_STATE_FOCUSED);
        }
    }, LV_EVENT_DEFOCUSED, NULL);
    lv_obj_add_state(m_textarea, LV_STATE_FOCUSED);

    // 禁用多色支持，恢复普通文本模式以便输入字符
    // lv_obj_t* ta_label = lv_textarea_get_label(m_textarea);
    // lv_label_set_recolor(ta_label, true);

    // 2. 创建日志显示区域（下方，只读）
    m_log_ta = lv_textarea_create(m_container);
    lv_obj_set_size(m_log_ta, lv_pct(95), lv_pct(50));
    lv_obj_align_to(m_log_ta, m_textarea, LV_ALIGN_OUT_BOTTOM_MID, 0, 20); // 位于输入框下方 20px
    
    // 设置为只读、不可选中、不可点击光标
    lv_textarea_set_cursor_click_pos(m_log_ta, false);
    // lv_textarea_set_editable(m_log_ta, false); // LVGL does not have this, we control it by flags
    lv_obj_clear_flag(m_log_ta, LV_OBJ_FLAG_CLICKABLE); // 禁止点击
    
    // 启用多色支持 (通过Label)
    lv_obj_t* label = lv_textarea_get_label(m_log_ta);
    lv_label_set_recolor(label, true);
    
    // 样式设置
    lv_textarea_set_placeholder_text(m_log_ta, "Log buffer...");
    lv_obj_set_style_text_font(m_log_ta, &lv_font_montserrat_16, 0);
    // 默认文字颜色设为白色，具体颜色由recolor控制
    lv_obj_set_style_text_color(m_log_ta, lv_color_hex(0xFFFFFF), 0); 
    lv_obj_set_style_bg_color(m_log_ta, lv_color_hex(0x101010), 0); // 深色背景

    // 创建说明标签
    lv_obj_t* info_label = lv_label_create(m_container);
    lv_label_set_text(info_label, "String Mode: ASCII Input & Logs");
    lv_obj_set_style_text_color(info_label, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(info_label, LV_ALIGN_BOTTOM_MID, 0, -10);

    ESP_LOGI(TAG, "String page created successfully");
}

void StringPage::onShow()
{
    BasePage::onShow();
    ESP_LOGI(TAG, "String page shown");
}

void StringPage::onHide()
{
    BasePage::onHide();
    ESP_LOGI(TAG, "String page hidden");
}

static std::string escape_hash(const char* input) {
    if (!input) return "";
    std::string s = input;
    size_t pos = 0;
    while ((pos = s.find('#', pos)) != std::string::npos) {
        // 使用 \# 转义井号，LVGL recolor 模式支持此写法
        s.replace(pos, 1, "\\#");
        pos += 2; 
    }
    return s;
}

void StringPage::handleStringInput(const char* str_data, uint8_t str_len, uint8_t modifier)
{
    if (!m_textarea || !str_data || str_len == 0) {
        return;
    }

    char safe_str[16];
    size_t copy_len = str_len;
    if (copy_len > 15) copy_len = 15;
    memcpy(safe_str, str_data, copy_len);
    safe_str[copy_len] = '\0';

    ESP_LOGD(TAG, "Handling string input: len=%d, data='%s', mod=%d", str_len, safe_str, modifier);

    // 格式化输出字符串 (无颜色)
    char disp_buf[128];

    // 0:Normal, 1:Ctrl, 4:Alt, 5:Ctrl+Alt
    if (modifier == 1) {
        snprintf(disp_buf, sizeof(disp_buf), "[Ctrl+%s]", safe_str);
    } else if (modifier == 4) {
        snprintf(disp_buf, sizeof(disp_buf), "[Alt+%s]", safe_str);
    } else if (modifier == 5) {
        snprintf(disp_buf, sizeof(disp_buf), "[Ctrl+Alt+%s]", safe_str);
    } else {
        // Normal text
        // 当mod=0且len长度超过2的也是要[%s]显示 (例如功能键字符串)
        if (str_len > 2) {
            snprintf(disp_buf, sizeof(disp_buf), "[%s]", safe_str);
        } else {
            snprintf(disp_buf, sizeof(disp_buf), "%s", safe_str);
        }
    }

    // 使用200ms超时，确保字符输入成功
    if (lvgl_port_lock(200)) {
        lv_textarea_add_text(m_textarea, disp_buf);
        lvgl_port_unlock();
    } else {
        ESP_LOGW(TAG, "Failed to acquire lock, input dropped");
    }
}

void StringPage::appendLog(uint8_t modifier, const char* fmt, ...)
{
    if (!m_log_ta) return;

    char msg_buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg_buf, sizeof(msg_buf), fmt, args);
    va_end(args);

    // 移除原有换行符
    size_t len = strlen(msg_buf);
    if (len > 0 && msg_buf[len-1] == '\n') msg_buf[len-1] = '\0';

    // 1. 转义消息中的井号
    std::string esc_msg = escape_hash(msg_buf);

    // 2. 决定颜色 (#RRGGBB)
    // 0:黄色, 1:深蓝色, 4:深灰色, 5:深蓝色, 其他:白色
    const char* color_hex = "#FFFFFF"; 
    if (modifier == 0)      color_hex = "#FFFF00"; // Yellow
    else if (modifier == 1) color_hex = "#4169E1"; // RoyalBlue
    else if (modifier == 4) color_hex = "#A9A9A9"; // DarkGray
    else if (modifier == 5) color_hex = "#087447"; // DeepBlue
    
    // 3. 组装带颜色的字符串
    char final_buf[512]; // 增加buffer防止溢出
    snprintf(final_buf, sizeof(final_buf), "%s %s#", color_hex, esc_msg.c_str()); // 注意空格

    // FIFO 处理
    m_log_lines.push_back(std::string(final_buf));
    while (m_log_lines.size() > MAX_LOG_LINES) {
        m_log_lines.pop_front();
    }

    // 重建完整日志文本
    std::string full_log;
    for (const auto& line : m_log_lines) {
        full_log += line;
        full_log += "\n";
    }

    if (lvgl_port_lock(50)) {
        lv_textarea_set_text(m_log_ta, full_log.c_str());
        lvgl_port_unlock();
    }
}


