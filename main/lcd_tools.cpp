#include "lcd_tools.h"

#include "m5_tab5_component.h"
#include "lvgl_port.h"
#include "lvgl_port_disp.h"
#include "lvgl_port_touch.h"

#include <esp_log.h>

namespace {

static const char* TAG = "lcd_tools";

static lv_display_t* s_lvgl_disp = nullptr;
static lv_indev_t* s_lvgl_touch_indev = nullptr;

constexpr uint32_t LCD_H_RES = 720;
constexpr uint32_t LCD_V_RES = 1280;

lv_display_rotation_t lcd_rotation_for_variant(const m5::tab5::m5tab5_variant_descriptor_t* variant)
{
    if (variant == nullptr) {
        return LV_DISPLAY_ROTATION_90;
    }
    switch (variant->variant_id) {
        case m5::tab5::M5TAB5_VARIANT_TAB5_LCD_ST7123_TOUCH_ST7123:
        case m5::tab5::M5TAB5_VARIANT_TAB5_LCD_ST7121_TOUCH_ST7121:
        case m5::tab5::M5TAB5_VARIANT_TAB5_LCD_ILI9881_TOUCH_GT911:
        default:
            return LV_DISPLAY_ROTATION_90;
    }
}

}  // namespace

esp_err_t app_lcd_lvgl_init(m5::tab5::m5tab5_component& board)
{
    if (s_lvgl_disp != nullptr) {
        ESP_LOGW(TAG, "LVGL already initialized, skipping duplicate init");
        return ESP_OK;
    }

    esp_lcd_panel_handle_t panel_handle = board.lcd_panel();
    if (panel_handle == nullptr) {
        ESP_LOGE(TAG, "LCD panel handle unavailable, call board.begin() first");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Initializing LVGL port...");
    lvgl_port_cfg_t lvgl_cfg = {};
    lvgl_cfg.task_priority = 6;
    lvgl_cfg.task_stack = 16384;
    lvgl_cfg.task_affinity = 1;
    lvgl_cfg.task_max_sleep_ms = 500;
    lvgl_cfg.timer_period_ms = 5;
    esp_err_t ret = lvgl_port_init(&lvgl_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LVGL port: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Adding LVGL display...");
    lvgl_disp_cfg_t disp_cfg = {};
    disp_cfg.panel_handle = panel_handle;
    disp_cfg.hres = LCD_H_RES;
    disp_cfg.vres = LCD_V_RES;
    disp_cfg.buffer_size = LCD_H_RES * LCD_V_RES;
    disp_cfg.color_format = LV_COLOR_FORMAT_RGB565;
    disp_cfg.flags.full_refresh = 0;
    disp_cfg.flags.direct_mode = 1;
    disp_cfg.flags.buff_spiram = 1;

    const m5::tab5::m5tab5_variant_descriptor_t* variant = board.variant();
    lv_display_rotation_t rotation = lcd_rotation_for_variant(variant);
    disp_cfg.flags.sw_rotate = (rotation != LV_DISPLAY_ROTATION_0);

    lvgl_disp_dsi_cfg_t dsi_cfg = {};
    dsi_cfg.sw_rotation = rotation;
    dsi_cfg.flags.avoid_tearing = 1;
    dsi_cfg.flags.use_ppa = (rotation != LV_DISPLAY_ROTATION_0);

    s_lvgl_disp = lvgl_port_add_disp_dsi(&disp_cfg, &dsi_cfg);
    if (s_lvgl_disp == nullptr) {
        ESP_LOGE(TAG, "Failed to add LVGL display");
        return ESP_FAIL;
    }

    esp_lcd_touch_handle_t touch_handle = board.touch_panel();
    if (touch_handle != nullptr) {
        ESP_LOGI(TAG, "Adding LVGL touch input...");
        lvgl_touch_cfg_t touch_cfg = {};
        touch_cfg.disp = s_lvgl_disp;
        touch_cfg.handle = touch_handle;

        s_lvgl_touch_indev = lvgl_port_add_touch(&touch_cfg);
        if (s_lvgl_touch_indev == nullptr) {
            ESP_LOGW(TAG, "Failed to add LVGL touch input");
        } else if (dsi_cfg.sw_rotation != LV_DISPLAY_ROTATION_0) {
            lvgl_port_set_touch_rotation(s_lvgl_touch_indev, dsi_cfg.sw_rotation);
        }
    }

    ESP_LOGI(TAG, "LVGL initialized successfully");
    return ESP_OK;
}
