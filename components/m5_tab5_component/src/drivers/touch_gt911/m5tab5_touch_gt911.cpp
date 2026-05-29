#include "drivers/touch_gt911/m5tab5_touch_gt911.h"

#include "drivers/m5tab5_driver_common.h"
#include "m5tab5_pinmap.h"

#include "esp_check.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch_gt911.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

namespace m5::tab5 {
namespace {

static m5tab5_touch_gt911_handles_t s_handles;

esp_err_t m5tab5_touch_gt911_probe(m5tab5_runtime_t* runtime)
{
    (void)runtime;
    ESP_LOGI(m5tab5_driver_log_tag(), "touch_gt911 probe");
    return ESP_OK;
}

esp_err_t m5tab5_touch_gt911_init(m5tab5_runtime_t* runtime)
{
    const char* TAG = m5tab5_driver_log_tag();

    // 1. Get the shared SYS I2C master bus handle / 获取共享�?SYS I2C 主机总线句柄
    i2c_master_bus_handle_t i2c_bus = m5tab5_get_sys_i2c_master_bus_handle();
    if (!i2c_bus) {
        ESP_LOGE(TAG, "SYS I2C master bus not available");
        return ESP_ERR_INVALID_STATE;
    }

    // 2. Create I2C panel IO for GT911 / �?GT911 创建 I2C 面板 IO
    //    The BSP uses the backup address (0x14) instead of the default (0x5D) / BSP 使用备用地址�?x14），而不是默认地址�?x5D）�?
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    tp_io_config.dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP;
    tp_io_config.scl_speed_hz = 400000;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(i2c_bus, &tp_io_config, &s_handles.tp_io),
                        TAG, "new panel IO I2C failed");

    // 3. Create the GT911 touch handle / 创建 GT911 触摸句柄
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = 720,
        .y_max = 1280,
        .rst_gpio_num = M5TAB5_PIN_LCD_TOUCH_RST,   // NC, controlled via IO expander (ADDR_LOW P5) / NC，由 IO 扩展器控制（ADDR_LOW P5�?
        .int_gpio_num = M5TAB5_PIN_LCD_TOUCH_INT,   // GPIO23 / GPIO23 中断�?
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    ESP_RETURN_ON_ERROR(esp_lcd_touch_new_i2c_gt911(s_handles.tp_io, &tp_cfg, &s_handles.tp),
                        TAG, "new GT911 touch failed");

    // Wake the GT911 from sleep, matching the BSP flow / 按照 BSP 的流程将 GT911 从休眠中唤醒
    ESP_RETURN_ON_ERROR(esp_lcd_touch_exit_sleep(s_handles.tp), TAG, "GT911 exit sleep failed");

    ESP_LOGI(TAG, "GT911 touch panel initialized");

    runtime->touch_handle = &s_handles;
    return ESP_OK;
}

const m5tab5_device_driver_descriptor_t k_m5tab5_touch_gt911_driver = {
    .id = "m5tab5.touch.gt911",
    .probe = m5tab5_touch_gt911_probe,
    .init = m5tab5_touch_gt911_init,
};

}  // namespace

const m5tab5_device_driver_descriptor_t* m5tab5_get_touch_gt911_driver()
{
    return &k_m5tab5_touch_gt911_driver;
}

}  // namespace m5::tab5

#pragma GCC diagnostic pop