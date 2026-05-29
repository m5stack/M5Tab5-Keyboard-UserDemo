#ifndef UI_APP_H
#define UI_APP_H

#include <esp_err.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "ui_pages/ui_top_menu.h"

/**
 * @brief Create the main Application UI
 * @param kb_handle Pointer to the physical keyboard instance (m5::M5Tab5Keyboard*)
 * @param menu_scope_mask Bitmask for menu items to display (M5Tab5MenuScope)
 * @return ESP_OK on success
 */
esp_err_t lvgl_ui_create(void* kb_handle, int menu_scope_mask = 0xFFFF);

/**
 * @brief 获取当前程序 SW 版本号
 */
const char* ui_app_get_sw_version(void);

/**
 * @brief 获取当前程序 SW 编译时间
 */
const char* ui_app_get_sw_build_time(void);

/**
 * @brief 获取当前缓存的从机键盘固件版本号
 */
uint8_t ui_app_get_fw_version(void);

/**
 * @brief 查询键盘当前是否连接
 * @return true 已连接 / false 未连接
 */
bool ui_app_is_kb_connected(void);

/**
 * @brief 查询当前是否适合执行物理键盘 RGB 输出
 * @return true 可输出 / false 不可输出
 */
bool ui_app_is_kb_rgb_ready(void);

/**
 * @brief 提交最新的物理键盘 RGB 目标值
 *
 * 使用单槽覆盖队列，只保留最新一帧，避免在热插拔窗口堆积陈旧 RGB 命令。
 * @return true 提交成功 / false 提交失败
 */
bool ui_app_submit_kb_rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);

#ifdef __cplusplus
}

// C++ only declarations
#include "m5_tab5_keyboard.h"

#endif

#endif // UI_APP_H
