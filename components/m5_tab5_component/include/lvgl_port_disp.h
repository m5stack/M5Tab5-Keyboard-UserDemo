/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * @file lvgl_port_disp.h
 * @brief LVGL 显示驱动接口
 *
 * 提供三种显示接口：
 * 1. lvgl_port_add_disp()     - 默认接口 (I2C/SPI/I8080)
 * 2. lvgl_port_add_disp_dsi() - MIPI-DSI 接口
 * 3. lvgl_port_add_disp_rgb() - RGB 并行接口
 *
 * 核心特性：
 * - PPA 硬件加速旋转(ESP32-P4)
 * - 区域绘制 (partial update)
 * - 双缓冲 + vsync 防撕裂
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 旋转配置
 */
typedef struct {
    bool swap_xy;  /*!< LCD 屏幕 X/Y 交换 (在 esp_lcd 驱动中) */
    bool mirror_x; /*!< LCD 屏幕 X 镜像 (在 esp_lcd 驱动中) */
    bool mirror_y; /*!< LCD 屏幕 Y 镜像 (在 esp_lcd 驱动中) */
} lvgl_rotation_cfg_t;

/**
 * @brief 显示配置结构 (通用)
 */
typedef struct {
    esp_lcd_panel_io_handle_t io_handle;   /*!< LCD 面板 IO 句柄 */
    esp_lcd_panel_handle_t panel_handle;   /*!< LCD 面板句柄 */
    esp_lcd_panel_handle_t control_handle; /*!< LCD 面板控制句柄 (可选) */

    uint32_t buffer_size; /*!< 屏幕缓冲区大小 (像素数 */
    bool double_buffer;   /*!< 是否分配双缓�?*/
    uint32_t trans_size;  /*!< 传输缓冲区大小 (可选) 用于 SRAM 中转) */

    uint32_t hres; /*!< LCD 水平分辨率*/
    uint32_t vres; /*!< LCD 垂直分辨率*/

    bool monochrome; /*!< 是否为单色显�?(1bit/pixel) */

    lvgl_rotation_cfg_t rotation;   /*!< 屏幕旋转配置 (仅硬件状态) */
    lv_color_format_t color_format; /*!< 颜色格式 */

    struct {
        unsigned int buff_dma : 1;     /*!< LVGL 缓冲区使用 DMA 内存 */
        unsigned int buff_spiram : 1;  /*!< LVGL 缓冲区使用 PSRAM */
        unsigned int sw_rotate : 1;    /*!< 使用软件旋转 (较慢) 或 PPA (如果可用)
                                        */
        unsigned int swap_bytes : 1;   /*!< RGB565 字节交换 */
        unsigned int full_refresh : 1; /*!< 始终全屏刷新 */
        unsigned int direct_mode : 1;  /*!<
                                          使用全屏缓冲区，直接绘制到绝对坐�?*/
    } flags;
} lvgl_disp_cfg_t;

/**
 * @brief RGB 显示特定配置
 */
typedef struct {
    struct {
        unsigned int bb_mode : 1;       /*!< 使用 bounce buffer 模式 */
        unsigned int avoid_tearing : 1; /*!< 使用内部 RGB 缓冲区作�?LVGL
                                           绘图缓冲区以避免撕裂 */
    } flags;
} lvgl_disp_rgb_cfg_t;

/**
 * @brief MIPI-DSI 显示特定配置
 */
typedef struct {
    lv_display_rotation_t sw_rotation; /*!< 软件旋转角度 (PPA 加�? */
    struct {
        unsigned int avoid_tearing : 1; /*!< 使用内部 DSI 缓冲区作�?LVGL
                                           绘图缓冲区以避免撕裂 */
        unsigned int use_ppa : 1;       /*!< 使用 PPA 硬件加速旋转*/
    } flags;
} lvgl_disp_dsi_cfg_t;

/**
 * @brief 添加 I2C/SPI/I8080 显示�?LVGL
 *
 * @param disp_cfg 显示配置结构
 * @return lv_display_t* 成功返回 LVGL 显示句柄，失败返�?NULL
 */
lv_display_t *lvgl_port_add_disp(const lvgl_disp_cfg_t *disp_cfg);

/**
 * @brief 添加 MIPI-DSI 显示�?LVGL
 *
 * @note 支持 PPA 硬件加速旋转+ 区域绘制 + avoid_tearing
 *
 * @param disp_cfg 显示配置结构
 * @param dsi_cfg MIPI-DSI 特定配置
 * @return lv_display_t* 成功返回 LVGL 显示句柄，失败返�?NULL
 */
lv_display_t *lvgl_port_add_disp_dsi(const lvgl_disp_cfg_t *disp_cfg, const lvgl_disp_dsi_cfg_t *dsi_cfg);

/**
 * @brief 添加 RGB 并行显示�?LVGL
 *
 * @param disp_cfg 显示配置结构
 * @param rgb_cfg RGB 特定配置
 * @return lv_display_t* 成功返回 LVGL 显示句柄，失败返�?NULL
 */
lv_display_t *lvgl_port_add_disp_rgb(const lvgl_disp_cfg_t *disp_cfg, const lvgl_disp_rgb_cfg_t *rgb_cfg);

/**
 * @brief �?LVGL 移除显示
 *
 * @param disp 显示句柄
 * @return ESP_OK 成功
 */
esp_err_t lvgl_port_remove_disp(lv_display_t *disp);

/**
 * @brief 通知 LVGL 数据已刷新到 LCD
 *
 * @param disp LVGL 显示句柄
 */
void lvgl_port_flush_ready(lv_display_t *disp);

#ifdef __cplusplus
}
#endif
