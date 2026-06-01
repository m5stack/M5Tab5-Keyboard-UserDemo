/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * SPDX-License-Identifier: Apache-2.0
 * ST7123 MIPI-DSI touch panel driver (TDDI) for M5Stack Tab5
 * 适用于 M5Stack Tab5 的 ST7123 MIPI-DSI 触摸面板驱动（TDDI）
 * Adapted from the esp-iot-solution esp_lcd_touch_st7123 example.
 * 改编自 esp-iot-solution 的 esp_lcd_touch_st7123 示例。
 */

#include <string.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"

#include "drivers/touch_st7123/esp_lcd_touch_st7123.h"

/* ST7123 Touch Register Addresses / ST7123 触摸寄存器地址 */
#define ST7123_FW_VERSION_REG     (0x0000)
#define ST7123_FW_REVISION_REG    (0x000C)
#define ST7123_MAX_X_COORD_H_REG  (0x0005)
#define ST7123_MAX_X_COORD_L_REG  (0x0006)
#define ST7123_MAX_Y_COORD_H_REG  (0x0007)
#define ST7123_MAX_Y_COORD_L_REG  (0x0008)
#define ST7123_MAX_TOUCHES_REG    (0x0009)
#define ST7123_ADV_INFO_REG       (0x0010)
#define ST7123_REPORT_COORD_0_REG (0x0014)

static const char *TAG = "ST7123_touch";

static esp_err_t st7123_read_data(esp_lcd_touch_handle_t tp);
static bool st7123_get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num,
                          uint8_t max_point_num);
static esp_err_t st7123_del(esp_lcd_touch_handle_t tp);
static esp_err_t st7123_reset(esp_lcd_touch_handle_t tp);
static esp_err_t st7123_i2c_read(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t *data, uint8_t len);
static esp_err_t st7123_read_fw_info(esp_lcd_touch_handle_t tp);

esp_err_t esp_lcd_touch_new_i2c_st7123(const esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *config,
                                       esp_lcd_touch_handle_t *tp)
{
    ESP_RETURN_ON_FALSE(io, ESP_ERR_INVALID_ARG, TAG, "Invalid io");
    ESP_RETURN_ON_FALSE(config, ESP_ERR_INVALID_ARG, TAG, "Invalid config");
    ESP_RETURN_ON_FALSE(tp, ESP_ERR_INVALID_ARG, TAG, "Invalid touch handle");

    esp_err_t ret                 = ESP_OK;
    esp_lcd_touch_handle_t st7123 = (esp_lcd_touch_handle_t)calloc(1, sizeof(esp_lcd_touch_t));
    ESP_GOTO_ON_FALSE(st7123, ESP_ERR_NO_MEM, err, TAG, "Touch handle malloc failed");

    /* Communication interface / 通信接口 */
    st7123->io = io;
    /* Callbacks / 回调函数 */
    st7123->read_data = st7123_read_data;
    st7123->get_xy    = st7123_get_xy;
    st7123->del       = st7123_del;
    /* Mutex / 互斥锁 */
    st7123->data.lock.owner = portMUX_FREE_VAL;
    /* Save config / 保存配置 */
    memcpy(&st7123->config, config, sizeof(esp_lcd_touch_config_t));

    /* Configure interrupt pin / 配置中断引脚 */
    if (config->int_gpio_num != GPIO_NUM_NC) {
        const gpio_config_t int_gpio_config = {
            .pin_bit_mask = BIT64(config->int_gpio_num),
            .mode         = GPIO_MODE_INPUT,
            .pull_up_en   = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = (config->levels.interrupt) ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE,
        };
        ESP_GOTO_ON_ERROR(gpio_config(&int_gpio_config), err, TAG, "GPIO intr config failed");

        if (config->interrupt_callback) {
            esp_lcd_touch_register_interrupt_callback(st7123, config->interrupt_callback);
        }
    }

    /* Configure reset pin / 配置复位引脚 */
    if (config->rst_gpio_num != GPIO_NUM_NC) {
        const gpio_config_t rst_gpio_config = {
            .pin_bit_mask = BIT64(config->rst_gpio_num),
            .mode         = GPIO_MODE_OUTPUT,
            .pull_up_en   = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_DISABLE,
        };
        ESP_GOTO_ON_ERROR(gpio_config(&rst_gpio_config), err, TAG, "GPIO reset config failed");
    }

    /* Reset controller / 复位控制器 */
    ESP_GOTO_ON_ERROR(st7123_reset(st7123), err, TAG, "Reset failed");
    /* Read firmware info / 读取固件信息 */
    ESP_GOTO_ON_ERROR(st7123_read_fw_info(st7123), err, TAG, "Read FW info failed");

    ESP_LOGI(TAG, "ST7123 touch panel initialized");

    *tp = st7123;
    return ESP_OK;

err:
    if (st7123) {
        st7123_del(st7123);
    }
    ESP_LOGE(TAG, "Initialization failed!");
    return ret;
}

static esp_err_t st7123_read_data(esp_lcd_touch_handle_t tp)
{
    typedef struct {
        uint8_t x_h : 6;
        uint8_t reserved_6 : 1;
        uint8_t valid : 1;
        uint8_t x_l;
        uint8_t y_h;
        uint8_t y_l;
        uint8_t area;
        uint8_t intensity;
        uint8_t reserved_49_55;
    } touch_report_t;

    typedef struct {
        uint8_t reserved_0_1 : 2;
        uint8_t with_prox : 1;
        uint8_t with_coord : 1;
        uint8_t prox_status : 3;
        uint8_t rst_chip : 1;
    } adv_info_t;

    touch_report_t touch_report[10];
    adv_info_t adv_info = {0};
    uint8_t max_touches = 0;

    ESP_RETURN_ON_ERROR(st7123_i2c_read(tp, ST7123_ADV_INFO_REG, (uint8_t *)&adv_info, 1), TAG,
                        "Read advanced info failed");

    if (adv_info.with_coord) {
        ESP_RETURN_ON_ERROR(st7123_i2c_read(tp, ST7123_MAX_TOUCHES_REG, &max_touches, 1), TAG,
                            "Read max touches failed");
        if (max_touches > 10) {
            max_touches = 10;
        }
        ESP_RETURN_ON_ERROR(st7123_i2c_read(tp, ST7123_REPORT_COORD_0_REG, (uint8_t *)&touch_report[0],
                                            sizeof(touch_report_t) * max_touches),
                            TAG, "Read report failed");
    }

    portENTER_CRITICAL(&tp->data.lock);
    int j = 0;
    for (int i = 0; (i < max_touches) && (j < CONFIG_ESP_LCD_TOUCH_MAX_POINTS); i++) {
        if (!touch_report[i].valid) {
            continue;
        }
        tp->data.coords[j].x        = (uint16_t)(touch_report[i].x_h << 8) | touch_report[i].x_l;
        tp->data.coords[j].y        = (uint16_t)(touch_report[i].y_h << 8) | touch_report[i].y_l;
        tp->data.coords[j].strength = touch_report[i].area;
        j++;
    }
    tp->data.points = j;
    portEXIT_CRITICAL(&tp->data.lock);

    return ESP_OK;
}

static bool st7123_get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num,
                          uint8_t max_point_num)
{
    portENTER_CRITICAL(&tp->data.lock);
    *point_num = (tp->data.points > max_point_num) ? max_point_num : tp->data.points;
    for (size_t i = 0; i < *point_num; i++) {
        x[i] = tp->data.coords[i].x;
        y[i] = tp->data.coords[i].y;
        if (strength) {
            strength[i] = tp->data.coords[i].strength;
        }
    }
    tp->data.points = 0;
    portEXIT_CRITICAL(&tp->data.lock);

    return (*point_num > 0);
}

static esp_err_t st7123_del(esp_lcd_touch_handle_t tp)
{
    if (tp->config.int_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.int_gpio_num);
    }
    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.rst_gpio_num);
    }
    free(tp);
    return ESP_OK;
}

static esp_err_t st7123_reset(esp_lcd_touch_handle_t tp)
{
    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        vTaskDelay(pdMS_TO_TICKS(10));
        ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, tp->config.levels.reset), TAG,
                            "GPIO set level failed");
        vTaskDelay(pdMS_TO_TICKS(10));
        ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, !tp->config.levels.reset), TAG,
                            "GPIO set level failed");
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return ESP_OK;
}

static esp_err_t st7123_i2c_read(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t *data, uint8_t len)
{
    ESP_RETURN_ON_FALSE((len == 0) || (data != NULL), ESP_ERR_INVALID_ARG, TAG, "Invalid data");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_rx_param(tp->io, reg, data, len), TAG, "Read param failed");
    return ESP_OK;
}

static esp_err_t st7123_read_fw_info(esp_lcd_touch_handle_t tp)
{
    uint8_t version     = 0;
    uint8_t revision[4] = {0};
    struct {
        uint8_t max_x_h;
        uint8_t max_x_l;
        uint8_t max_y_h;
        uint8_t max_y_l;
        uint8_t max_touches;
    } info;

    ESP_RETURN_ON_ERROR(st7123_i2c_read(tp, ST7123_FW_VERSION_REG, &version, 1), TAG, "Read version failed");
    ESP_RETURN_ON_ERROR(st7123_i2c_read(tp, ST7123_FW_REVISION_REG, (uint8_t *)&revision[0], 4), TAG,
                        "Read revision failed");
    ESP_RETURN_ON_ERROR(st7123_i2c_read(tp, ST7123_MAX_X_COORD_H_REG, (uint8_t *)&info, sizeof(info)), TAG,
                        "Read info failed");

    ESP_LOGI(TAG, "FW version: %d(%d.%d.%d.%d), Max.X: %d, Max.Y: %d, Max.Touches: %d", version, revision[0],
             revision[1], revision[2], revision[3], ((uint16_t)info.max_x_h << 8) | info.max_x_l,
             ((uint16_t)info.max_y_h << 8) | info.max_y_l, info.max_touches);

    return ESP_OK;
}
