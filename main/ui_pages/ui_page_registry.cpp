/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

﻿#include "ui_page_registry.h"
#include "ui_page_manager.h"
#include "lvgl_port.h"
#include <esp_log.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ============================================================
// 布局常量
// ============================================================
#define STARTING_Y    80                           // 整体下移 (px)
#define HEADER_H      50                           // 占位按钮条高度
#define TABLE_Y       (STARTING_Y + HEADER_H + 8)  // 表格起始 y
#define CELL_W        110                          // 格子宽度 (进一步加宽，让文字更舒展)
#define CELL_H        80                           // 格子高度 (进一步加高)
#define CELL_VAL_H    (CELL_H * 0.45)              // 值区高度 (45%)
#define TABLE_INNER_W (16 * CELL_W)                // 1760
#define TABLE_INNER_H (7 * CELL_H)  // 560 (跳过 0x70~0xEF 的 8 行，将 0xF0~0xFF 移到了第 7 行，总共 7 行

// 格子背景色
#define COL_CELL_RW 0x1C3A2E  // 可读写：深绿
#define COL_CELL_R  0x1A2D3E  // 只读：深蓝

// ============================================================
// TEST 压力测试速度
//   TEST_REG_DELAY_MS  : 每个寄存器操作完成后的间隔（控制单次测试速度）
//   TEST_ITER_DELAY_MS : 每完成一轮（0x00~0xFF）后的额外延迟
//   建议：TEST_REG_DELAY_MS 50~200ms，TEST_ITER_DELAY_MS 0~10ms
//   注意：过小会导致 I2C 自旋锁竞争触发 Interrupt WDT
// ============================================================
#define TEST_REG_DELAY_MS  50
#define TEST_ITER_DELAY_MS 5

// ============================================================
// TEST 自动启动（进入页面后自动倒计时开始压力测试）
//   TEST_AUTO_START_ENABLE : 1=启用  0=禁用
//   TEST_AUTO_START_SEC    : 倒计时秒数
// ============================================================
#define TEST_AUTO_START_ENABLE 0
#define TEST_AUTO_START_SEC    30

    // 操作按钮标签
    static const char* k_ph_labels[] = {"CLEAN", "READ", "WRITE", "TEST"};

// ============================================================
// 寄存器定义表
// ============================================================
typedef struct {
    uint8_t addr;
    char name[10];       // 简短名（用于调试）
    char rw[3];          // "R" / "W" / "RW"
    uint32_t color_hex;  // 格子背景色
} reg_def_t;

static const reg_def_t k_reg_defs[] = {
    // -- 系统控制 0x00~0x03 --
    {0x00, "INT_CFG", "RW", COL_CELL_RW},
    {0x01, "INT_STA", "RW", COL_CELL_RW},
    {0x02, "EVT_NUM", "RW", COL_CELL_RW},
    {0x03, "BRIGHT", "RW", COL_CELL_RW},
    // -- 配置 0x10~0x11 --
    {0x10, "KB_MODE", "RW", COL_CELL_RW},
    {0x11, "RGB_MODE", "RW", COL_CELL_RW},
    // -- 普通模式事件 0x20 --
    {0x20, "KEY_EVT", "R", COL_CELL_R},
    // -- HID 模式事件 0x30~0x31 --
    {0x30, "HID_MOD", "R", COL_CELL_R},
    {0x31, "HID_KEY", "R", COL_CELL_R},
    // -- 字符串长度 0x40 --
    {0x40, "STR_LEN", "R", COL_CELL_R},
    // -- 字符串数据 0x50~0x59 --
    {0x50, "STR_MOD", "R", COL_CELL_R},
    {0x51, "CHAR0", "R", COL_CELL_R},
    {0x52, "CHAR1", "R", COL_CELL_R},
    {0x53, "CHAR2", "R", COL_CELL_R},
    {0x54, "CHAR3", "R", COL_CELL_R},
    {0x55, "CHAR4", "R", COL_CELL_R},
    {0x56, "CHAR5", "R", COL_CELL_R},
    {0x57, "CHAR6", "R", COL_CELL_R},
    {0x58, "CHAR7", "R", COL_CELL_R},
    {0x59, "CHAR8", "R", COL_CELL_R},
    // -- RGB 颜色 0x60~0x66 (0x63 保留) --
    {0x60, "RGB1_B", "RW", COL_CELL_RW},
    {0x61, "RGB1_G", "RW", COL_CELL_RW},
    {0x62, "RGB1_R", "RW", COL_CELL_RW},
    {0x64, "RGB2_B", "RW", COL_CELL_RW},
    {0x65, "RGB2_G", "RW", COL_CELL_RW},
    {0x66, "RGB2_R", "RW", COL_CELL_RW},
    // -- 系统信息 0xFE~0xFF --
    {0xFE, "VERSION", "R", COL_CELL_R},
    {0xFF, "I2C_ADDR", "RW", COL_CELL_RW},
};
static constexpr int k_reg_count = (int)(sizeof(k_reg_defs) / sizeof(k_reg_defs[0]));

LV_FONT_DECLARE(lv_font_montserrat_14);
LV_FONT_DECLARE(lv_font_montserrat_16);
LV_FONT_DECLARE(lv_font_montserrat_24);

static const char* TAG = "RegistryPage";

RegistryPage* RegistryPage::s_instance = nullptr;

// ============================================================
// 构造 / 析构
// ============================================================
RegistryPage::RegistryPage()
    : BasePage("REGISTRY"),
      m_header_cont(nullptr),
      m_mode_menu_cont(nullptr),
      m_mode_indicator(nullptr),
      m_current_mode(0),
      m_table_cont(nullptr),
      m_keyboard(nullptr),
      m_hl_decay_timer(nullptr)
{
    memset(m_btn_ph, 0, sizeof(m_btn_ph));
    memset(m_mode_btns, 0, sizeof(m_mode_btns));
    memset(m_val_labels, 0, sizeof(m_val_labels));
    memset(m_cell_objs, 0, sizeof(m_cell_objs));
    memset(m_hl_timers, 0, sizeof(m_hl_timers));
    memset(m_popups, 0, sizeof(m_popups));
    memset(m_pop_reg_dd, 0, sizeof(m_pop_reg_dd));
    memset(m_pop_val_rol, 0, sizeof(m_pop_val_rol));
    m_active_popup      = -1;
    m_test_timer_lbl    = nullptr;
    m_test_read_lbl     = nullptr;
    m_test_write_lbl    = nullptr;
    m_test_btn          = nullptr;
    m_test_btn_lbl      = nullptr;
    m_test_stop_bar     = nullptr;
    m_test_lv_timer     = nullptr;
    m_test_task_hdl     = nullptr;
    m_test_state        = 0;  // TEST_IDLE
    m_test_elapsed      = 0;
    m_test_read_cnt     = 0;
    m_test_write_cnt    = 0;
    m_test_btn_press_ms = 0;
    m_test_long_fired   = false;
    m_test_auto_timer   = nullptr;
    m_test_auto_sec     = 0;
    s_instance          = this;
}

RegistryPage::~RegistryPage()
{
    if (m_keyboard) m_keyboard->clearRegAccessCallback();
    if (s_instance == this) s_instance = nullptr;
}

void RegistryPage::setKeyboard(m5::M5Tab5Keyboard* kb)
{
    m_keyboard = kb;
}

// ============================================================
// onCreate
// ============================================================
void RegistryPage::onCreate()
{
    ESP_LOGI(TAG, "Creating REGISTRY page");

    createContainer(PageManager::getInstance()->getParent());
    if (!m_container) {
        ESP_LOGE(TAG, "Failed to create container");
        return;
    }
    lv_obj_clear_flag(m_container, LV_OBJ_FLAG_SCROLLABLE);

    // ----------------------------------------------------------
    // 1. 右上角占位按钮条
    // ----------------------------------------------------------
    const int btn_w = 80, btn_h = 40, btn_gap = 8;
    int cont_w = 4 * btn_w + 3 * btn_gap;  // 344

    m_header_cont = lv_obj_create(m_container);
    lv_obj_set_size(m_header_cont, cont_w, HEADER_H);
    lv_obj_align(m_header_cont, LV_ALIGN_TOP_RIGHT, -10, STARTING_Y);
    lv_obj_set_style_bg_opa(m_header_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(m_header_cont, 0, 0);
    lv_obj_set_style_pad_all(m_header_cont, 0, 0);
    lv_obj_clear_flag(m_header_cont, LV_OBJ_FLAG_SCROLLABLE);
    // 容器本身不响应点击，否则会拦截子按钮的触摸事件
    // Container must NOT be clickable, otherwise it consumes touch before children receive it
    lv_obj_clear_flag(m_header_cont, LV_OBJ_FLAG_CLICKABLE);

    for (int i = 0; i < 4; i++) {
        m_btn_ph[i] = lv_btn_create(m_header_cont);
        lv_obj_set_size(m_btn_ph[i], btn_w, btn_h);
        lv_obj_set_pos(m_btn_ph[i], i * (btn_w + btn_gap), 5);
        lv_obj_set_style_bg_color(m_btn_ph[i], lv_color_hex(0x3A3A3A), 0);
        lv_obj_set_style_bg_opa(m_btn_ph[i], LV_OPA_COVER, 0);
        lv_obj_set_style_radius(m_btn_ph[i], 6, 0);
        lv_obj_add_flag(m_btn_ph[i], LV_OBJ_FLAG_CHECKABLE);  // 预留可选中状态
        lv_obj_add_event_cb(m_btn_ph[i], onPhBtn, LV_EVENT_CLICKED, (void*)(intptr_t)i);

        lv_obj_t* lbl = lv_label_create(m_btn_ph[i]);
        lv_label_set_text(lbl, k_ph_labels[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x888888), 0);
        lv_obj_center(lbl);
    }

    // ----------------------------------------------------------
    // 1.5 模式切换滑块菜单 (在占位按钮左侧)
    // ----------------------------------------------------------
    const int mode_btn_w = 80;
    const int mode_btn_h = 40;
    int mode_cont_w      = 3 * mode_btn_w;  // 240

    m_mode_menu_cont = lv_obj_create(m_container);
    lv_obj_set_size(m_mode_menu_cont, mode_cont_w, mode_btn_h);
    // 放在 m_header_cont 左侧，留出 20px 间距
    lv_obj_align(m_mode_menu_cont, LV_ALIGN_TOP_RIGHT, -10 - cont_w - 20, STARTING_Y + 5);

    // 胶囊形状背景
    lv_obj_set_style_radius(m_mode_menu_cont, mode_btn_h / 2, 0);
    lv_obj_set_style_bg_color(m_mode_menu_cont, lv_color_hex(0x303030), 0);
    lv_obj_set_style_bg_opa(m_mode_menu_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(m_mode_menu_cont, 0, 0);
    lv_obj_set_style_pad_all(m_mode_menu_cont, 0, 0);
    lv_obj_clear_flag(m_mode_menu_cont, LV_OBJ_FLAG_SCROLLABLE);

    // 滑块指示器
    m_mode_indicator = lv_obj_create(m_mode_menu_cont);
    lv_obj_set_size(m_mode_indicator, mode_btn_w, mode_btn_h);
    lv_obj_set_style_radius(m_mode_indicator, mode_btn_h / 2, 0);
    lv_obj_set_style_bg_color(m_mode_indicator, lv_color_hex(0x007AFF), 0);  // 默认蓝色
    lv_obj_set_style_bg_opa(m_mode_indicator, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(m_mode_indicator, 0, 0);
    lv_obj_add_flag(m_mode_indicator, LV_OBJ_FLAG_FLOATING);
    lv_obj_add_flag(m_mode_indicator, LV_OBJ_FLAG_IGNORE_LAYOUT);

#if LVGL_VERSION_MAJOR >= 8
    static const lv_style_prop_t props[] = {LV_STYLE_X, (lv_style_prop_t)0};
    static lv_style_transition_dsc_t trans;
    if (trans.props == NULL) {
        lv_style_transition_dsc_init(&trans, props, lv_anim_path_ease_out, 250, 0, NULL);
    }
    lv_obj_set_style_transition(m_mode_indicator, &trans, 0);
#endif

    const char* mode_labels[] = {"Normal", "HID", "String"};
    for (int i = 0; i < 3; i++) {
        m_mode_btns[i] = lv_btn_create(m_mode_menu_cont);
        lv_obj_set_size(m_mode_btns[i], mode_btn_w, mode_btn_h);
        lv_obj_set_pos(m_mode_btns[i], i * mode_btn_w, 0);
        lv_obj_set_style_bg_opa(m_mode_btns[i], LV_OPA_TRANSP, 0);  // 透明，露出底部的指示器
        lv_obj_set_style_shadow_width(m_mode_btns[i], 0, 0);
        lv_obj_add_event_cb(m_mode_btns[i], onModeBtn, LV_EVENT_CLICKED, (void*)(intptr_t)i);

        lv_obj_t* lbl = lv_label_create(m_mode_btns[i]);
        lv_label_set_text(lbl, mode_labels[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(lbl);
    }

    // 初始化指示器位置
    if (m_keyboard) {
        uint8_t current_mode = 0;
        m_keyboard->getKeyboardMode(&current_mode);
        m_current_mode = current_mode;
        if (m_current_mode > 2) m_current_mode = 0;
    }
    lv_obj_set_x(m_mode_indicator, m_current_mode * mode_btn_w);

    // 设置颜色
    lv_color_t color = lv_color_hex(0x007AFF);
    if (m_current_mode == 1)
        color = lv_color_hex(0x00C000);  // HID: Green
    else if (m_current_mode == 2)
        color = lv_color_hex(0xFF69B4);  // String: Pink
    lv_obj_set_style_bg_color(m_mode_indicator, color, 0);

    // ----------------------------------------------------------
    // 2. 寄存器表格（可滚动）
    // ----------------------------------------------------------
    int table_y = TABLE_Y;
    int scr_h   = lv_obj_get_height(m_container);
    int scr_w   = lv_obj_get_width(m_container);
    int cont_h  = scr_h - table_y - 5;
    if (cont_h < 100) cont_h = 100;

    m_table_cont = lv_obj_create(m_container);
    lv_obj_set_pos(m_table_cont, 0, table_y);
    lv_obj_set_size(m_table_cont, scr_w, cont_h);
    lv_obj_set_style_bg_color(m_table_cont, lv_color_hex(0x0E0E0E), 0);
    lv_obj_set_style_bg_opa(m_table_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(m_table_cont, 0, 0);
    lv_obj_set_style_pad_all(m_table_cont, 2, 0);
    // 允许双向滚动
    lv_obj_add_flag(m_table_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(m_table_cont, LV_DIR_ALL);
    lv_obj_set_scrollbar_mode(m_table_cont, LV_SCROLLBAR_MODE_AUTO);

    buildTable();
    buildPopups();

    ESP_LOGI(TAG, "onCreate complete, %d regs defined", k_reg_count);
}

// ============================================================
// buildTable: 仅为已定义的寄存器创建格子
// 不再创建 256 个占位格子，避免 LVGL 128KB 内存耗尽导致崩溃
// ============================================================
void RegistryPage::buildTable()
{
    // 仅遍历 k_reg_defs 中已定义的 28 个寄存器，在网格坐标位置创建格子
    for (int i = 0; i < k_reg_count; i++) {
        const reg_def_t* def = &k_reg_defs[i];
        uint8_t addr         = def->addr;
        int row              = (addr >> 4) & 0xF;
        int col              = addr & 0xF;

        // 跳过 0x70 ~ 0xEF 之间的空行，将 0xF0~0xFF 提上来
        int visual_row = row;
        int visual_col = col;
        if (row >= 0x0F) {
            // 0xFE 与 0xFF 放到 0x60 这一行 (row = 6)
            visual_row = 6;
            // 放到 0x58 与 0x59 的正下方，即列号 8 与 9
            if (addr == 0xFE) visual_col = 8;
            if (addr == 0xFF) visual_col = 9;
        }

        int x = visual_col * CELL_W + 1;
        int y = visual_row * CELL_H + 1;

        lv_obj_t* cell = lv_obj_create(m_table_cont);
        lv_obj_set_pos(cell, x, y);
        lv_obj_set_size(cell, CELL_W - 2, CELL_H - 2);
        lv_obj_set_style_bg_color(cell, lv_color_hex(def->color_hex), 0);
        lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(cell, lv_color_hex(0x507060), 0);
        lv_obj_set_style_border_width(cell, 1, 0);
        lv_obj_set_style_radius(cell, 4, 0);  // 恢复一点小圆角，让格子看起来更精致
        lv_obj_set_style_pad_all(cell, 0, 0);
        lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
        m_cell_objs[addr] = cell;

        // 值标签 (高 45%，居中，字体放大)
        lv_obj_t* val_lbl = lv_label_create(cell);
        lv_label_set_text(val_lbl, "--");
        lv_obj_set_style_text_font(val_lbl, &lv_font_montserrat_24, 0);   // 使用更大的字体
        lv_obj_set_style_text_color(val_lbl, lv_color_hex(0xFFFFFF), 0);  // 纯白，更醒目
        lv_obj_set_width(val_lbl, CELL_W - 2);
        lv_obj_set_style_text_align(val_lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_pos(val_lbl, 0, (CELL_VAL_H - 24) / 2 + 4);  // 垂直居中微调
        m_val_labels[addr] = val_lbl;

        // 地址标签（下半部分的上侧，居中）
        lv_obj_t* addr_lbl = lv_label_create(cell);
        char addr_str[8];
        snprintf(addr_str, sizeof(addr_str), "0x%02X", addr);
        lv_label_set_text(addr_lbl, addr_str);
        lv_obj_set_style_text_font(addr_lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(addr_lbl, lv_color_hex(0x88AA88), 0);  // 浅绿色
        lv_obj_set_width(addr_lbl, CELL_W - 2);
        lv_obj_set_style_text_align(addr_lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_pos(addr_lbl, 0, CELL_VAL_H + 4);

        // 名称标签（下半部分的下侧，居中）
        lv_obj_t* name_lbl = lv_label_create(cell);
        lv_label_set_text(name_lbl, def->name);
        lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(name_lbl, lv_color_hex(0xAAAAAA), 0);  // 浅灰色
        lv_obj_set_width(name_lbl, CELL_W - 2);
        lv_obj_set_style_text_align(name_lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_pos(name_lbl, 0, CELL_VAL_H + 20);

        // R/W 指示（右上角，稍微往里靠一点）
        lv_obj_t* rw_lbl = lv_label_create(cell);
        lv_label_set_text(rw_lbl, def->rw);
        lv_obj_set_style_text_font(rw_lbl, &lv_font_montserrat_14, 0);
        uint32_t rw_color = (strcmp(def->rw, "RW") == 0) ? 0x90C040 : (strcmp(def->rw, "R") == 0) ? 0x40A0C0 : 0xE07020;
        lv_obj_set_style_text_color(rw_lbl, lv_color_hex(rw_color), 0);
        lv_obj_align(rw_lbl, LV_ALIGN_TOP_RIGHT, -4, 4);
    }

    // 强制表格内容尺寸（保证可滚动到完整 16×16 网格区域）
    lv_obj_set_content_width(m_table_cont, TABLE_INNER_W + 4);
    lv_obj_set_content_height(m_table_cont, TABLE_INNER_H + 4);
}

// ============================================================
// onShow / onHide
// ============================================================
void RegistryPage::onShow()
{
    BasePage::onShow();
    memset(m_hl_timers, 0, sizeof(m_hl_timers));

    // 进入页面时隐藏所有弹窗
    m_active_popup = -1;
    for (int i = 0; i < 4; i++) {
        if (m_popups[i]) lv_obj_add_flag(m_popups[i], LV_OBJ_FLAG_HIDDEN);
    }

    if (m_keyboard) {
        // 主动读取安全（非破坏性）寄存器以初始化显示（在 LVGL 任务上下文中，无需加锁）
        static const uint8_t k_safe_regs[] = {0x00, 0x03, 0x10, 0x11, 0x60, 0x61, 0x62, 0x64, 0x65, 0x66, 0xFE, 0xFF};
        for (uint8_t r : k_safe_regs) {
            uint8_t val = 0;
            if (m_keyboard->readRegByte(r, &val) == M5_TAB5_KB_OK && m_val_labels[r]) {
                char buf[4];
                snprintf(buf, sizeof(buf), "%02X", val);
                lv_label_set_text(m_val_labels[r], buf);
                // 同步模式指示器
                if (r == 0x10 && m_mode_indicator && val <= 2) {
                    m_current_mode = val;
                    lv_obj_set_x(m_mode_indicator, val * 80);
                    lv_color_t col = lv_color_hex(0x007AFF);
                    if (val == 1)
                        col = lv_color_hex(0x00C000);
                    else if (val == 2)
                        col = lv_color_hex(0xFF69B4);
                    lv_obj_set_style_bg_color(m_mode_indicator, col, 0);
                }
            }
        }
        // 设置拦截回调：此时 _pollTaskFunc 每次 I2C 读写都会被动触发回调更新显示
        m_keyboard->setRegAccessCallback(onRegAccess, this);
    }

    // 创建高亮衰减 LVGL 定时器（每 100ms 触发一次，在 LVGL 任务上下文中安全操作 UI）
    if (!m_hl_decay_timer) {
        m_hl_decay_timer = lv_timer_create(hlDecayTimerCb, 100, this);
    }

    ESP_LOGI(TAG, "onShow: reg access callback registered");

#if TEST_AUTO_START_ENABLE
    // 自动启动倒计时：TEST_AUTO_START_SEC 秒后打开 TEST 弹窗并开始压力测试
    if (m_test_state == 0) {
        m_test_auto_sec = TEST_AUTO_START_SEC;
        if (!m_test_auto_timer) m_test_auto_timer = lv_timer_create(autoStartTimerCb, 1000, this);
        ESP_LOGI(TAG, "onShow: auto-start countdown %ds", TEST_AUTO_START_SEC);
    }
#endif
}

void RegistryPage::onHide()
{
    // 如果压力测试正在运行，先停止
    if (m_test_state != 0) {
        m_test_state = 0;  // TEST_IDLE
        if (m_test_lv_timer) {
            lv_timer_del(m_test_lv_timer);
            m_test_lv_timer = nullptr;
        }
        if (m_test_task_hdl) {
            vTaskDelete(m_test_task_hdl);
            m_test_task_hdl = nullptr;
        }
    }

    // 取消自动启动倒计时
    if (m_test_auto_timer) {
        lv_timer_del(m_test_auto_timer);
        m_test_auto_timer = nullptr;
    }

    // 离开页面时隐藏所有弹窗并关闭下拉框
    m_active_popup = -1;
    for (int i = 0; i < 4; i++) {
        if (m_popups[i]) lv_obj_add_flag(m_popups[i], LV_OBJ_FLAG_HIDDEN);
    }
    for (int i = 0; i < 3; i++) {
        if (m_pop_reg_dd[i]) lv_dropdown_close(m_pop_reg_dd[i]);
    }

    // 停止高亮衰减定时器
    if (m_hl_decay_timer) {
        lv_timer_del(m_hl_decay_timer);
        m_hl_decay_timer = nullptr;
    }

    // 清除拦截回调
    if (m_keyboard) {
        m_keyboard->clearRegAccessCallback();
    }

    BasePage::onHide();
    ESP_LOGI(TAG, "onHide: reg access callback cleared");
}

// ============================================================
// onRegAccess: 被 m5::M5Tab5Keyboard 在每次 I2C 读写成功后调用
//   运行在 _pollTaskFunc 这个 FreeRTOS 任务上下文中，必须持有 LVGL lock
// ============================================================
void RegistryPage::onRegAccess(uint8_t reg, uint8_t value, m5_tab5_reg_access_type_t type, void* user_data)
{
    RegistryPage* self = static_cast<RegistryPage*>(user_data);
    if (!self || !self->m_val_labels[reg]) return;

    if (!lvgl_port_lock(50)) return;

    char buf[4];
    snprintf(buf, sizeof(buf), "%02X", value);
    lv_label_set_text(self->m_val_labels[reg], buf);

    // 写操作高亮金色 + 5 tick；读操作高亮白色 + 2 tick
    self->m_hl_timers[reg] = (type == M5_TAB5_REG_ACCESS_WRITE) ? 5 : 2;
    lv_color_t text_color  = (type == M5_TAB5_REG_ACCESS_WRITE) ? lv_color_hex(0xFFD700) : lv_color_hex(0xFFFFFF);
    lv_obj_set_style_text_color(self->m_val_labels[reg], text_color, 0);
    if (self->m_cell_objs[reg]) {
        lv_obj_set_style_bg_color(self->m_cell_objs[reg], lv_color_hex(0x4A6A5E), 0);
    }

    // 模式寄存器写操作时同步滑块指示器
    if (reg == 0x10 && type == M5_TAB5_REG_ACCESS_WRITE && self->m_mode_indicator) {
        uint8_t new_mode = value;
        if (new_mode <= 2) {
            self->m_current_mode = new_mode;
            lv_obj_set_x(self->m_mode_indicator, new_mode * 80);
            lv_color_t color = lv_color_hex(0x007AFF);
            if (new_mode == 1)
                color = lv_color_hex(0x00C000);
            else if (new_mode == 2)
                color = lv_color_hex(0xFF69B4);
            lv_obj_set_style_bg_color(self->m_mode_indicator, color, 0);
        }
    }

    lvgl_port_unlock();
}

// ============================================================
// hlDecayTimerCb: LVGL 定时器回调（100ms，LVGL 任务上下文）
//   负责高亮倒计时递减和颜色恢复
// ============================================================
void RegistryPage::hlDecayTimerCb(lv_timer_t* timer)
{
    RegistryPage* self = static_cast<RegistryPage*>(lv_timer_get_user_data(timer));
    if (!self) return;

    for (int i = 0; i < k_reg_count; i++) {
        uint8_t reg = k_reg_defs[i].addr;
        if (self->m_hl_timers[reg] == 0) continue;

        self->m_hl_timers[reg]--;
        if (self->m_hl_timers[reg] == 0) {
            // 倒计时结束，恢复正常颜色
            if (self->m_val_labels[reg]) {
                lv_obj_set_style_text_color(self->m_val_labels[reg], lv_color_hex(0xFFFFFF), 0);
            }
            if (self->m_cell_objs[reg]) {
                lv_obj_set_style_bg_color(self->m_cell_objs[reg], lv_color_hex(k_reg_defs[i].color_hex), 0);
            }
        }
    }
}

// ============================================================
// onPhBtn: 操作按钮回调，切换对应弹窗显示/隐藏
// ============================================================
void RegistryPage::onPhBtn(lv_event_t* e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    ESP_LOGI(TAG, "onPhBtn idx=%d active=%d", idx, s_instance ? s_instance->m_active_popup : -99);
    if (!s_instance) {
        ESP_LOGW(TAG, "onPhBtn: s_instance null");
        return;
    }
    if (!s_instance->m_popups[idx]) {
        ESP_LOGW(TAG, "onPhBtn: popup[%d] null", idx);
        return;
    }

    int prev = s_instance->m_active_popup;

    // 先关闭所有弹窗和下拉列表
    for (int i = 0; i < 4; i++) {
        if (s_instance->m_popups[i]) lv_obj_add_flag(s_instance->m_popups[i], LV_OBJ_FLAG_HIDDEN);
    }
    for (int i = 0; i < 3; i++) {
        if (s_instance->m_pop_reg_dd[i]) lv_dropdown_close(s_instance->m_pop_reg_dd[i]);
    }
    s_instance->m_active_popup = -1;

    // 若点击的不是当前已打开的弹窗，则打开（再次点击同一按钮则保持关闭，切换效果）
    if (prev != idx) {
        lv_obj_t* pop = s_instance->m_popups[idx];
        lv_obj_clear_flag(pop, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(pop);
        s_instance->m_active_popup = idx;
        ESP_LOGI(TAG, "onPhBtn: popup[%d] shown", idx);
    } else {
        ESP_LOGI(TAG, "onPhBtn: popup[%d] closed", idx);
    }
}

// ============================================================
// onModeBtn: 模式切换按钮回调
// ============================================================
void RegistryPage::onModeBtn(lv_event_t* e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (s_instance && s_instance->m_keyboard) {
        s_instance->m_current_mode = idx;
        // setMode() 同时写硬件寄存器并更新 _currentMode 内部状态，
        // 确保 _handleInterrupt() 以正确模式读取事件数据
        // setMode() writes hw reg AND updates _currentMode so _handleInterrupt() reads correct registers
        s_instance->m_keyboard->setMode((m5_tab5_kb_mode_t)idx);

        // 更新指示器位置
        if (s_instance->m_mode_indicator) {
            lv_obj_set_x(s_instance->m_mode_indicator, idx * 80);  // mode_btn_w = 80

            // 更新颜色
            lv_color_t color = lv_color_hex(0x007AFF);  // Normal: Blue
            if (idx == 1)
                color = lv_color_hex(0x00C000);  // HID: Green
            else if (idx == 2)
                color = lv_color_hex(0xFF69B4);  // String: Pink
            lv_obj_set_style_bg_color(s_instance->m_mode_indicator, color, 0);
        }
        ESP_LOGI(TAG, "Keyboard mode switched to %d", idx);
    }
}

// ============================================================
// buildPopups: 在右上角操作按钮区正下方创建 4 个操作弹窗
//   CLEAN(0) READ(1) WRITE(2) TEST(3)
//   弹窗默认隐藏，点击对应按钮切换显示/隐藏
// ============================================================
void RegistryPage::buildPopups()
{
    // m_container 使用 lv_pct(100)，必须先触发布局计算，否则 get_width 返回 0
    lv_obj_update_layout(m_container);
    int scr_w = lv_obj_get_width(m_container);
    ESP_LOGI(TAG, "buildPopups: scr_w=%d", scr_w);

    static const int PW      = 340;                         // 弹窗宽度
    static const int TITLE_H = 34;                          // 标题栏高度
    static const int PAD     = 10;                          // 内边距
    int PX                   = scr_w - 10 - PW;             // 右对齐，与操作按钮条对齐
    static const int PY      = STARTING_Y + HEADER_H + 12;  // 紧贴按钮下方

    // 构建下拉框选项字符串（static 保证生命周期覆盖整个页面）
    static char s_dd_all[620];    // ALL + 所有寄存器
    static char s_dd_noall[600];  // 仅寄存器列表（WRITE 不含 ALL）
    {
        int pos = snprintf(s_dd_all, sizeof(s_dd_all), "ALL");
        for (int i = 0; i < k_reg_count; i++) {
            pos +=
                snprintf(s_dd_all + pos, sizeof(s_dd_all) - pos, "\n0x%02X %s", k_reg_defs[i].addr, k_reg_defs[i].name);
        }
    }
    {
        int pos = 0;
        for (int i = 0; i < k_reg_count; i++) {
            if (i > 0) s_dd_noall[pos++] = '\n';
            pos += snprintf(s_dd_noall + pos, sizeof(s_dd_noall) - pos, "0x%02X %s", k_reg_defs[i].addr,
                            k_reg_defs[i].name);
        }
    }

    // WRITE 值滚轮选项 (0-F)
    static const char* K_ROL = "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\nA\nB\nC\nD\nE\nF";

    static const char* k_titles[] = {"CLEAN", "READ", "WRITE", "TEST"};
    // 弹窗高度：CLEAN=165, READ=165, WRITE=262, TEST=80
    static const int k_heights[] = {165, 165, 262, 220};

    for (int pi = 0; pi < 4; pi++) {
        // ---- 弹窗面板 ----
        lv_obj_t* pop = lv_obj_create(m_container);
        lv_obj_set_pos(pop, PX, PY);
        lv_obj_set_size(pop, PW, k_heights[pi]);
        lv_obj_set_style_bg_color(pop, lv_color_hex(0x121828), 0);
        lv_obj_set_style_bg_opa(pop, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(pop, lv_color_hex(0x3A7A5A), 0);
        lv_obj_set_style_border_width(pop, 2, 0);
        lv_obj_set_style_radius(pop, 8, 0);
        lv_obj_set_style_pad_all(pop, 0, 0);
        lv_obj_clear_flag(pop, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(pop, LV_OBJ_FLAG_HIDDEN);
        m_popups[pi] = pop;

        // ---- 标题栏----
        lv_obj_t* tbar = lv_obj_create(pop);
        lv_obj_set_pos(tbar, 0, 0);
        lv_obj_set_size(tbar, PW, TITLE_H);
        lv_obj_set_style_bg_color(tbar, lv_color_hex(0x1A3A28), 0);
        lv_obj_set_style_bg_opa(tbar, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(tbar, 0, 0);
        lv_obj_set_style_radius(tbar, 0, 0);
        lv_obj_set_style_pad_all(tbar, 0, 0);
        lv_obj_clear_flag(tbar, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* lbl_t = lv_label_create(tbar);
        lv_label_set_text(lbl_t, k_titles[pi]);
        lv_obj_set_style_text_font(lbl_t, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(lbl_t, lv_color_hex(0xCCFFCC), 0);
        lv_obj_align(lbl_t, LV_ALIGN_LEFT_MID, PAD, 0);

        // X 关闭按钮
        lv_obj_t* btn_x = lv_btn_create(tbar);
        lv_obj_set_size(btn_x, 26, 26);
        lv_obj_align(btn_x, LV_ALIGN_RIGHT_MID, -4, 0);
        lv_obj_set_style_bg_color(btn_x, lv_color_hex(0x7A3535), 0);
        lv_obj_set_style_radius(btn_x, 4, 0);
        lv_obj_set_style_shadow_width(btn_x, 0, 0);
        lv_obj_set_style_pad_all(btn_x, 0, 0);
        lv_obj_add_event_cb(btn_x, onPopupClose, LV_EVENT_CLICKED, (void*)(intptr_t)pi);
        lv_obj_t* lbl_x = lv_label_create(btn_x);
        lv_label_set_text(lbl_x, "X");
        lv_obj_set_style_text_font(lbl_x, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl_x, lv_color_hex(0xFFDDDD), 0);
        lv_obj_center(lbl_x);

        // ---- 内容区----
        int cy = TITLE_H + PAD;  // 内容起始 y

        if (pi == 3) {
            // ---- TEST 内容 ----
            // 计时器
            lv_obj_t* lbl_time = lv_label_create(pop);
            lv_label_set_text(lbl_time, "0000:00");
            lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_24, 0);
            lv_obj_set_style_text_color(lbl_time, lv_color_hex(0x88FFCC), 0);
            lv_obj_set_pos(lbl_time, PAD, cy);
            m_test_timer_lbl = lbl_time;

            // 读次数
            lv_obj_t* lbl_r = lv_label_create(pop);
            lv_label_set_text(lbl_r, "Read:  0");
            lv_obj_set_style_text_font(lbl_r, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(lbl_r, lv_color_hex(0x88CCFF), 0);
            lv_obj_set_pos(lbl_r, PAD, cy + 34);
            m_test_read_lbl = lbl_r;

            // 写次数
            lv_obj_t* lbl_w = lv_label_create(pop);
            lv_label_set_text(lbl_w, "Write: 0");
            lv_obj_set_style_text_font(lbl_w, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(lbl_w, lv_color_hex(0xFFCC88), 0);
            lv_obj_set_pos(lbl_w, PAD, cy + 56);
            m_test_write_lbl = lbl_w;

            // START/PAUSE/RESUME 按鈕
            lv_obj_t* btn = lv_btn_create(pop);
            lv_obj_set_size(btn, 140, 42);
            lv_obj_set_pos(btn, (PW - 140) / 2, cy + 84);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x1A4A2A), 0);
            lv_obj_set_style_radius(btn, 8, 0);
            lv_obj_set_style_shadow_width(btn, 0, 0);
            m_test_btn = btn;

            lv_obj_t* lbl_btn = lv_label_create(btn);
            lv_label_set_text(lbl_btn, "START");
            lv_obj_set_style_text_font(lbl_btn, &lv_font_montserrat_16, 0);
            lv_obj_set_style_text_color(lbl_btn, lv_color_hex(0xFFFFFF), 0);
            lv_obj_center(lbl_btn);
            m_test_btn_lbl = lbl_btn;

            // 长按进度条（按下时出现，3秒充满后停止）
            lv_obj_t* bar = lv_bar_create(pop);
            lv_obj_set_size(bar, PW - 2 * PAD, 12);
            lv_obj_set_pos(bar, PAD, cy + 136);
            lv_bar_set_range(bar, 0, 3000);
            lv_bar_set_value(bar, 0, LV_ANIM_OFF);
            lv_obj_set_style_bg_color(bar, lv_color_hex(0x1A2030), 0);
            lv_obj_set_style_bg_color(bar, lv_color_hex(0xFF4444), LV_PART_INDICATOR);
            lv_obj_set_style_radius(bar, 4, 0);
            lv_obj_set_style_radius(bar, 4, LV_PART_INDICATOR);
            lv_obj_add_flag(bar, LV_OBJ_FLAG_HIDDEN);
            m_test_stop_bar = bar;

            // 提示文字
            lv_obj_t* hint = lv_label_create(pop);
            lv_label_set_text(hint, "Long press 3s = STOP");
            lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(hint, lv_color_hex(0x445566), 0);
            lv_obj_set_pos(hint, PAD, cy + 156);

            lv_obj_add_event_cb(btn, onTestBtn, LV_EVENT_PRESSED, nullptr);
            lv_obj_add_event_cb(btn, onTestBtn, LV_EVENT_PRESSING, nullptr);
            lv_obj_add_event_cb(btn, onTestBtn, LV_EVENT_RELEASED, nullptr);
            lv_obj_add_event_cb(btn, onTestBtn, LV_EVENT_CLICKED, nullptr);
            lv_obj_add_event_cb(btn, onTestBtn, LV_EVENT_PRESS_LOST, nullptr);
            continue;
        }

        // ---- 寄存器下拉框（CLEAN/READ/WRITE 通用） ---
        lv_obj_t* lbl_reg = lv_label_create(pop);
        lv_label_set_text(lbl_reg, "Register:");
        lv_obj_set_style_text_font(lbl_reg, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl_reg, lv_color_hex(0x88AAAA), 0);
        lv_obj_set_pos(lbl_reg, PAD, cy);

        lv_obj_t* dd = lv_dropdown_create(pop);
        lv_dropdown_set_options(dd, (pi == 2) ? s_dd_noall : s_dd_all);
        lv_obj_set_size(dd, PW - 2 * PAD, 38);
        lv_obj_set_pos(dd, PAD, cy + 18);
        lv_obj_set_style_bg_color(dd, lv_color_hex(0x1A2A3E), 0);
        lv_obj_set_style_text_color(dd, lv_color_hex(0xDDEEFF), 0);
        lv_obj_set_style_border_color(dd, lv_color_hex(0x3A6A8A), 0);
        lv_obj_set_style_text_font(dd, &lv_font_montserrat_14, 0);
        lv_obj_set_style_shadow_width(dd, 0, 0);
        // 下拉列表暗色主题
        lv_obj_t* ddl = lv_dropdown_get_list(dd);
        if (ddl) {
            lv_obj_set_style_bg_color(ddl, lv_color_hex(0x1A2A3E), 0);
            lv_obj_set_style_text_color(ddl, lv_color_hex(0xDDEEFF), 0);
            lv_obj_set_style_border_color(ddl, lv_color_hex(0x3A6A8A), 0);
            lv_obj_set_style_max_height(ddl, 200, 0);
        }
        m_pop_reg_dd[pi <= 1 ? pi : 2] = dd;

        if (pi == 0 || pi == 1) {
            // CLEAN / READ：下拉框 + 执行按钮
            int btn_y         = cy + 18 + 38 + 14;
            lv_obj_t* btn_act = lv_btn_create(pop);
            lv_obj_set_size(btn_act, 130, 40);
            lv_obj_set_pos(btn_act, (PW - 130) / 2, btn_y);
            lv_obj_set_style_bg_color(btn_act, (pi == 0) ? lv_color_hex(0x5A2828) : lv_color_hex(0x1A4A2A), 0);
            lv_obj_set_style_radius(btn_act, 6, 0);
            lv_obj_set_style_shadow_width(btn_act, 0, 0);
            lv_obj_t* lbl_act = lv_label_create(btn_act);
            lv_label_set_text(lbl_act, k_titles[pi]);
            lv_obj_set_style_text_font(lbl_act, &lv_font_montserrat_16, 0);
            lv_obj_set_style_text_color(lbl_act, lv_color_hex(0xFFFFFF), 0);
            lv_obj_center(lbl_act);
            if (pi == 0)
                lv_obj_add_event_cb(btn_act, onCleanExec, LV_EVENT_CLICKED, nullptr);
            else
                lv_obj_add_event_cb(btn_act, onReadExec, LV_EVENT_CLICKED, nullptr);

        } else {
            // WRITE：下拉框 + 高低 nibble 滚轮 + WRITE 按钮
            int val_y = cy + 18 + 38 + 12;

            lv_obj_t* lbl_v = lv_label_create(pop);
            lv_label_set_text(lbl_v, "Value: 0x");
            lv_obj_set_style_text_font(lbl_v, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(lbl_v, lv_color_hex(0x88AAAA), 0);
            lv_obj_set_pos(lbl_v, PAD, val_y + 34);  // 与滚轮垂直居中

            lv_obj_t* rol_hi = lv_roller_create(pop);
            lv_roller_set_options(rol_hi, K_ROL, LV_ROLLER_MODE_NORMAL);
            lv_roller_set_visible_row_count(rol_hi, 3);
            lv_obj_set_size(rol_hi, 52, 90);
            lv_obj_set_pos(rol_hi, PAD + 75, val_y);
            lv_obj_set_style_bg_color(rol_hi, lv_color_hex(0x1A2A3E), 0);
            lv_obj_set_style_text_color(rol_hi, lv_color_hex(0xDDEEFF), 0);
            lv_obj_set_style_text_font(rol_hi, &lv_font_montserrat_16, 0);
            m_pop_val_rol[0] = rol_hi;

            lv_obj_t* rol_lo = lv_roller_create(pop);
            lv_roller_set_options(rol_lo, K_ROL, LV_ROLLER_MODE_NORMAL);
            lv_roller_set_visible_row_count(rol_lo, 3);
            lv_obj_set_size(rol_lo, 52, 90);
            lv_obj_set_pos(rol_lo, PAD + 75 + 58, val_y);
            lv_obj_set_style_bg_color(rol_lo, lv_color_hex(0x1A2A3E), 0);
            lv_obj_set_style_text_color(rol_lo, lv_color_hex(0xDDEEFF), 0);
            lv_obj_set_style_text_font(rol_lo, &lv_font_montserrat_16, 0);
            m_pop_val_rol[1] = rol_lo;

            lv_obj_t* btn_wr = lv_btn_create(pop);
            lv_obj_set_size(btn_wr, 130, 40);
            lv_obj_set_pos(btn_wr, (PW - 130) / 2, val_y + 90 + 10);
            lv_obj_set_style_bg_color(btn_wr, lv_color_hex(0x1A3A6A), 0);
            lv_obj_set_style_radius(btn_wr, 6, 0);
            lv_obj_set_style_shadow_width(btn_wr, 0, 0);
            lv_obj_t* lbl_wr = lv_label_create(btn_wr);
            lv_label_set_text(lbl_wr, "WRITE");
            lv_obj_set_style_text_font(lbl_wr, &lv_font_montserrat_16, 0);
            lv_obj_set_style_text_color(lbl_wr, lv_color_hex(0xFFFFFF), 0);
            lv_obj_center(lbl_wr);
            lv_obj_add_event_cb(btn_wr, onWriteExec, LV_EVENT_CLICKED, nullptr);
        }
    }
}

// ============================================================
// onPopupClose: 弹窗 X 按钮回调
// ============================================================
void RegistryPage::onPopupClose(lv_event_t* e)
{
    int pi = (int)(intptr_t)lv_event_get_user_data(e);
    if (!s_instance) return;
    if (s_instance->m_popups[pi]) lv_obj_add_flag(s_instance->m_popups[pi], LV_OBJ_FLAG_HIDDEN);
    s_instance->m_active_popup = -1;
    // 关闭该弹窗对应的下拉框（前 pi<3 有下拉框）
    int di = (pi <= 1) ? pi : (pi == 2 ? 2 : -1);
    if (di >= 0 && s_instance->m_pop_reg_dd[di]) lv_dropdown_close(s_instance->m_pop_reg_dd[di]);
}

// ============================================================
// onCleanExec: 清除选中寄存器的显示值（重置为 "--"）
// ============================================================
void RegistryPage::onCleanExec(lv_event_t* e)
{
    if (!s_instance || !s_instance->m_pop_reg_dd[0]) return;
    uint16_t sel = lv_dropdown_get_selected(s_instance->m_pop_reg_dd[0]);

    // 在 LVGL 任务上下文内（已持有锁），直接操作 UI
    auto reset_one = [](uint8_t reg) {
        if (s_instance->m_val_labels[reg]) {
            lv_label_set_text(s_instance->m_val_labels[reg], "--");
            lv_obj_set_style_text_color(s_instance->m_val_labels[reg], lv_color_hex(0xFFFFFF), 0);
        }
        s_instance->m_hl_timers[reg] = 0;
        if (s_instance->m_cell_objs[reg]) {
            for (int j = 0; j < k_reg_count; j++) {
                if (k_reg_defs[j].addr == reg) {
                    lv_obj_set_style_bg_color(s_instance->m_cell_objs[reg], lv_color_hex(k_reg_defs[j].color_hex), 0);
                    break;
                }
            }
        }
    };

    if (sel == 0) {  // ALL
        for (int i = 0; i < k_reg_count; i++) reset_one(k_reg_defs[i].addr);
    } else {
        int ri = (int)sel - 1;
        if (ri >= 0 && ri < k_reg_count) reset_one(k_reg_defs[ri].addr);
    }
}

// ============================================================
// onReadExec: 立即读取选中寄存器，刷新显示
//   在 LVGL 任务中，临时禁用拦截回调避免二次加锁死锁
// ============================================================
void RegistryPage::onReadExec(lv_event_t* e)
{
    if (!s_instance || !s_instance->m_keyboard || !s_instance->m_pop_reg_dd[1]) return;
    uint16_t sel = lv_dropdown_get_selected(s_instance->m_pop_reg_dd[1]);

    // 临时禁用拦截回调：readRegByte() 会触发 onRegAccess 尝试加锁，而此处已在 LVGL 锁内
    s_instance->m_keyboard->clearRegAccessCallback();

    auto do_read_one = [](uint8_t reg) {
        uint8_t val = 0;
        if (s_instance->m_keyboard->readRegByte(reg, &val) != M5_TAB5_KB_OK) return;
        if (!s_instance->m_val_labels[reg]) return;
        char buf[4];
        snprintf(buf, sizeof(buf), "%02X", val);
        lv_label_set_text(s_instance->m_val_labels[reg], buf);
        s_instance->m_hl_timers[reg] = 3;
        lv_obj_set_style_text_color(s_instance->m_val_labels[reg], lv_color_hex(0xAADDFF), 0);  // 蓝白：主色 READ
        if (s_instance->m_cell_objs[reg]) {
            lv_obj_set_style_bg_color(s_instance->m_cell_objs[reg], lv_color_hex(0x2A4A5A), 0);
        }
    };

    if (sel == 0) {  // ALL
        for (int i = 0; i < k_reg_count; i++) do_read_one(k_reg_defs[i].addr);
    } else {
        int ri = (int)sel - 1;
        if (ri >= 0 && ri < k_reg_count) do_read_one(k_reg_defs[ri].addr);
    }

    // 恢复拦截回调
    s_instance->m_keyboard->setRegAccessCallback(onRegAccess, s_instance);
}

// ============================================================
// onWriteExec: 向指定寄存器写入滚轮选定的十六进制值
// ============================================================
void RegistryPage::onWriteExec(lv_event_t* e)
{
    if (!s_instance || !s_instance->m_keyboard) return;
    if (!s_instance->m_pop_reg_dd[2] || !s_instance->m_pop_val_rol[0] || !s_instance->m_pop_val_rol[1]) return;

    uint16_t reg_idx = lv_dropdown_get_selected(s_instance->m_pop_reg_dd[2]);
    if (reg_idx >= (uint16_t)k_reg_count) return;
    uint8_t reg = k_reg_defs[reg_idx].addr;

    uint16_t hi = lv_roller_get_selected(s_instance->m_pop_val_rol[0]);  // nibble 0-F
    uint16_t lo = lv_roller_get_selected(s_instance->m_pop_val_rol[1]);  // nibble 0-F
    uint8_t val = (uint8_t)((hi << 4) | lo);

    // 临时禁用拦截回调，在 LVGL 上下文中直接更新 UI
    s_instance->m_keyboard->clearRegAccessCallback();
    s_instance->m_keyboard->writeRegByte(reg, val);

    if (s_instance->m_val_labels[reg]) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02X", val);
        lv_label_set_text(s_instance->m_val_labels[reg], buf);
        s_instance->m_hl_timers[reg] = 5;
        lv_obj_set_style_text_color(s_instance->m_val_labels[reg], lv_color_hex(0xFFD700), 0);
        if (s_instance->m_cell_objs[reg]) {
            lv_obj_set_style_bg_color(s_instance->m_cell_objs[reg], lv_color_hex(0x4A6A5E), 0);
        }
        // KB_MODE 写操作同步模式滑块
        if (reg == 0x10 && val <= 2 && s_instance->m_mode_indicator) {
            s_instance->m_current_mode = val;
            lv_obj_set_x(s_instance->m_mode_indicator, val * 80);
            lv_color_t col = lv_color_hex(0x007AFF);
            if (val == 1)
                col = lv_color_hex(0x00C000);
            else if (val == 2)
                col = lv_color_hex(0xFF69B4);
            lv_obj_set_style_bg_color(s_instance->m_mode_indicator, col, 0);
        }
    }

    // 恢复拦截回调
    s_instance->m_keyboard->setRegAccessCallback(onRegAccess, s_instance);
}

// ============================================================
// testTimerCb: LVGL 定时器，每 1s 刷新测试 UI（LVGL 任务上下文）
// ============================================================
void RegistryPage::testTimerCb(lv_timer_t* t)
{
    RegistryPage* self = (RegistryPage*)lv_timer_get_user_data(t);
    if (!self || self->m_test_state != 1) return;

    uint32_t cur         = self->m_test_elapsed + 1;
    self->m_test_elapsed = cur;
    uint32_t rc          = self->m_test_read_cnt;
    uint32_t wc          = self->m_test_write_cnt;

    char buf[32];
    if (self->m_test_timer_lbl) {
        snprintf(buf, sizeof(buf), "%04" PRIu32 ":%02" PRIu32, cur / 60, cur % 60);
        lv_label_set_text(self->m_test_timer_lbl, buf);
    }
    if (self->m_test_read_lbl) {
        snprintf(buf, sizeof(buf), "Read:  %" PRIu32, rc);
        lv_label_set_text(self->m_test_read_lbl, buf);
    }
    if (self->m_test_write_lbl) {
        snprintf(buf, sizeof(buf), "Write: %" PRIu32, wc);
        lv_label_set_text(self->m_test_write_lbl, buf);
    }
}

// ============================================================
// testTaskFunc: FreeRTOS 压力测试任务
//   遍历 0x00~0xFF（仅 k_reg_defs 中已定义的寄存器，按地址升序）
//   RW 且地址 != 0xFF：先写后读
//   R 或地址 == 0xFF ：直接读
// ============================================================
void RegistryPage::testTaskFunc(void* arg)
{
    RegistryPage* self     = (RegistryPage*)arg;
    m5::M5Tab5Keyboard* kb = self->m_keyboard;

    // 预建地址类型查找表：0=未定义 1=只读 2=可读写
    static uint8_t s_type[256];
    // 写后不校验的寄存器（硬件自锁位，写入值不会原样保留）
    static const uint8_t k_no_verify[] = {0x00, 0x01, 0x02, 0x03, 0x10, 0x11};
    static bool s_no_verify[256];
    static bool s_inited = false;
    if (!s_inited) {
        memset(s_type, 0, sizeof(s_type));
        memset(s_no_verify, 0, sizeof(s_no_verify));
        for (int i = 0; i < k_reg_count; i++) {
            uint8_t a = k_reg_defs[i].addr;
            bool rw   = (k_reg_defs[i].rw[0] == 'R' && k_reg_defs[i].rw[1] == 'W');
            s_type[a] = (uint8_t)((rw && a != 0xFF) ? 2 : 1);
        }
        for (int i = 0; i < (int)(sizeof(k_no_verify)); i++) s_no_verify[k_no_verify[i]] = true;
        s_inited = true;
    }

    uint8_t val;
    uint8_t wval = 0x01;  // 写值游标：0x01~0xFF 循环，体现整体递增规律

    while (true) {
        if (self->m_test_state != 1) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        if (!kb) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        // 0x00 ~ 0xFF 顺序遍历
        for (int addr = 0x00; addr <= 0xFF; addr++) {
            if (self->m_test_state != 1) break;
            uint8_t reg = (uint8_t)addr;
            uint8_t tp  = s_type[reg];
            if (tp == 0) continue;  // 未定义，跳过

            if (tp == 2) {            // RW：先写后读再校验
                uint8_t sent = wval;  // 记录本次实际值
                bool w_ok    = (kb->writeRegByte(reg, sent) == M5_TAB5_KB_OK);
                wval         = (wval >= 0xFF) ? 0x01 : (wval + 1);  // 写值递增，0x01~0xFF 循环
                if (w_ok) self->m_test_write_cnt = self->m_test_write_cnt + 1;
                // 写和读之间主动 yield，避免连续占用 I2C 自旋锁
                vTaskDelay(pdMS_TO_TICKS(5));
                // 写成功后读回校验（跳过硬件自锁寄存器）
                if (w_ok && !s_no_verify[reg] && kb->readRegByte(reg, &val) == M5_TAB5_KB_OK) {
                    self->m_test_read_cnt = self->m_test_read_cnt + 1;
                    if (val != sent) ESP_LOGW(TAG, "VERIFY FAIL reg=0x%02X wrote=0x%02X read=0x%02X", reg, sent, val);
                } else if (w_ok && s_no_verify[reg] && kb->readRegByte(reg, &val) == M5_TAB5_KB_OK) {
                    self->m_test_read_cnt = self->m_test_read_cnt + 1;  // 仍计读次数，不比较
                }
            } else {  // R-only：直接读
                if (kb->readRegByte(reg, &val) == M5_TAB5_KB_OK) self->m_test_read_cnt = self->m_test_read_cnt + 1;
            }
            // 每个寄存器操作后固定间隔，防止 I2C 总线加速后连续爆读
            vTaskDelay(pdMS_TO_TICKS(TEST_REG_DELAY_MS));
        }
        vTaskDelay(pdMS_TO_TICKS(TEST_ITER_DELAY_MS));
    }
}

// ============================================================
// onTestBtn: 压力测试按钮事件回调
//   PRESSED    ：记录按下时刻；非 IDLE 时显示进度条
//   PRESSING   ：更新进度条；满 3s 触发 STOP
//   RELEASED   ：短按：IDLE→START, RUNNING→PAUSE, PAUSED→RESUME
//   PRESS_LOST ：恢复进度条隐藏
// ============================================================
void RegistryPage::onTestBtn(lv_event_t* e)
{
    if (!s_instance) return;
    RegistryPage* self   = s_instance;
    lv_event_code_t code = lv_event_get_code(e);
    lv_event_stop_bubbling(e);  // 阻断冒泡，防止父容器二次触发

    ESP_LOGI(TAG, "onTestBtn code=%d state=%d", (int)code, self->m_test_state);

    // STOP 内联逻辑（lambda 可访问私有成员）
    auto do_stop = [self, &code]() {
        self->m_test_state = 0;
        if (self->m_test_lv_timer) {
            lv_timer_del(self->m_test_lv_timer);
            self->m_test_lv_timer = nullptr;
        }
        if (self->m_test_task_hdl) {
            vTaskDelete(self->m_test_task_hdl);
            self->m_test_task_hdl = nullptr;
        }
        self->m_test_elapsed   = 0;
        self->m_test_read_cnt  = 0;
        self->m_test_write_cnt = 0;
        if (self->m_test_timer_lbl) lv_label_set_text(self->m_test_timer_lbl, "0000:00");
        if (self->m_test_read_lbl) lv_label_set_text(self->m_test_read_lbl, "Read:  0");
        if (self->m_test_write_lbl) lv_label_set_text(self->m_test_write_lbl, "Write: 0");
        if (self->m_test_btn) lv_obj_set_style_bg_color(self->m_test_btn, lv_color_hex(0x1A4A2A), 0);
        if (self->m_test_btn_lbl) lv_label_set_text(self->m_test_btn_lbl, "START");
        if (self->m_test_stop_bar) lv_obj_add_flag(self->m_test_stop_bar, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(TAG, "Pressure test STOPPED");
    };

    if (code == LV_EVENT_PRESSED) {
        self->m_test_btn_press_ms = lv_tick_get();
        self->m_test_long_fired   = false;
        if (self->m_test_state != 0 && self->m_test_stop_bar) {
            lv_bar_set_value(self->m_test_stop_bar, 0, LV_ANIM_OFF);
            lv_obj_clear_flag(self->m_test_stop_bar, LV_OBJ_FLAG_HIDDEN);
        }

    } else if (code == LV_EVENT_PRESSING) {
        if (self->m_test_long_fired || self->m_test_state == 0) return;
        uint32_t elapsed = lv_tick_elaps(self->m_test_btn_press_ms);
        if (self->m_test_stop_bar) lv_bar_set_value(self->m_test_stop_bar, (int32_t)elapsed, LV_ANIM_OFF);
        if (elapsed >= 3000 && !self->m_test_long_fired) {
            self->m_test_long_fired = true;
            do_stop();
        }

    } else if (code == LV_EVENT_PRESS_LOST) {
        if (self->m_test_stop_bar) lv_obj_add_flag(self->m_test_stop_bar, LV_OBJ_FLAG_HIDDEN);

    } else if (code == LV_EVENT_RELEASED) {
        // 仅隐藏进度条；START/PAUSE/RESUME 在 CLICKED 中处理
        if (self->m_test_stop_bar) lv_obj_add_flag(self->m_test_stop_bar, LV_OBJ_FLAG_HIDDEN);

    } else if (code == LV_EVENT_CLICKED) {
        // 长按触发了 STOP，忽略此处的 CLICKED
        if (self->m_test_long_fired) return;

        if (self->m_test_state == 0) {
            // --- START ---
            self->m_test_elapsed   = 0;
            self->m_test_read_cnt  = 0;
            self->m_test_write_cnt = 0;
            self->m_test_state     = 1;
            if (self->m_test_timer_lbl) lv_label_set_text(self->m_test_timer_lbl, "0000:00");
            if (self->m_test_read_lbl) lv_label_set_text(self->m_test_read_lbl, "Read:  0");
            if (self->m_test_write_lbl) lv_label_set_text(self->m_test_write_lbl, "Write: 0");
            if (!self->m_test_lv_timer) self->m_test_lv_timer = lv_timer_create(testTimerCb, 1000, self);
            if (!self->m_test_task_hdl) xTaskCreate(testTaskFunc, "press_test", 4096, self, 1, &self->m_test_task_hdl);
            if (self->m_test_btn) lv_obj_set_style_bg_color(self->m_test_btn, lv_color_hex(0x1A7A2A), 0);
            if (self->m_test_btn_lbl) lv_label_set_text(self->m_test_btn_lbl, "PAUSE");
            ESP_LOGI(TAG, "Pressure test STARTED");

        } else if (self->m_test_state == 1) {
            // --- PAUSE ---
            self->m_test_state = 2;
            if (self->m_test_lv_timer) lv_timer_pause(self->m_test_lv_timer);
            if (self->m_test_btn) lv_obj_set_style_bg_color(self->m_test_btn, lv_color_hex(0x5A4A1A), 0);
            if (self->m_test_btn_lbl) lv_label_set_text(self->m_test_btn_lbl, "RESUME");
            ESP_LOGI(TAG, "Pressure test PAUSED R=%" PRIu32 " W=%" PRIu32, self->m_test_read_cnt,
                     self->m_test_write_cnt);

        } else if (self->m_test_state == 2) {
            // --- RESUME ---
            self->m_test_state = 1;
            if (self->m_test_lv_timer) lv_timer_resume(self->m_test_lv_timer);
            if (self->m_test_btn) lv_obj_set_style_bg_color(self->m_test_btn, lv_color_hex(0x1A7A2A), 0);
            if (self->m_test_btn_lbl) lv_label_set_text(self->m_test_btn_lbl, "PAUSE");
            ESP_LOGI(TAG, "Pressure test RESUMED");
        }
    }
}

// ============================================================
// autoStartTimerCb: 自动启动倒计时回调（每 1s 触发）
//   倒计时归零后：打开 TEST 弹窗，自动启动压力测试
// ============================================================
void RegistryPage::autoStartTimerCb(lv_timer_t* t)
{
#if TEST_AUTO_START_ENABLE
    RegistryPage* self = (RegistryPage*)lv_timer_get_user_data(t);
    if (!self || self->m_test_state != 0) {
        lv_timer_del(t);
        if (self) self->m_test_auto_timer = nullptr;
        return;
    }
    self->m_test_auto_sec--;
    ESP_LOGI(TAG, "Auto-start countdown: %ds remaining", self->m_test_auto_sec);
    if (self->m_test_auto_sec > 0) return;

    // 倒计时结束，删除本定时器
    lv_timer_del(t);
    self->m_test_auto_timer = nullptr;

    // 打开 TEST 弹窗（idx=3），与手动点击 TEST 按钮一样
    for (int i = 0; i < 4; i++) {
        if (self->m_popups[i]) lv_obj_add_flag(self->m_popups[i], LV_OBJ_FLAG_HIDDEN);
    }
    if (self->m_popups[3]) {
        lv_obj_clear_flag(self->m_popups[3], LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(self->m_popups[3]);
        self->m_active_popup = 3;
    }

    // 启动压力测试（与手动点击 START 相同逻辑）
    self->m_test_elapsed   = 0;
    self->m_test_read_cnt  = 0;
    self->m_test_write_cnt = 0;
    self->m_test_state     = 1;
    if (self->m_test_timer_lbl) lv_label_set_text(self->m_test_timer_lbl, "0000:00");
    if (self->m_test_read_lbl) lv_label_set_text(self->m_test_read_lbl, "Read:  0");
    if (self->m_test_write_lbl) lv_label_set_text(self->m_test_write_lbl, "Write: 0");
    if (!self->m_test_lv_timer) self->m_test_lv_timer = lv_timer_create(testTimerCb, 1000, self);
    if (!self->m_test_task_hdl) xTaskCreate(testTaskFunc, "press_test", 4096, self, 5, &self->m_test_task_hdl);
    if (self->m_test_btn) lv_obj_set_style_bg_color(self->m_test_btn, lv_color_hex(0x1A7A2A), 0);
    if (self->m_test_btn_lbl) lv_label_set_text(self->m_test_btn_lbl, "PAUSE");
    ESP_LOGI(TAG, "Auto-start: Pressure test STARTED");
#else
    (void)t;
#endif
}
