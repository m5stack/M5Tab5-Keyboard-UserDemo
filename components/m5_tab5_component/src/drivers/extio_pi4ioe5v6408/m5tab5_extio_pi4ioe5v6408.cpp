/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 *
 * PI4IOE5V6408 I/O expander driver using espressif/i2c_bus.
 * 基于 espressif/i2c_bus ?PI4IOE5V6408 IO 扩展器驱动实现?
 * Register semantics derived from the PI4IOE5V6408 datasheet.
 * 寄存器语义依?PI4IOE5V6408 数据手册整理?
 */

#include "drivers/extio_pi4ioe5v6408/m5tab5_extio_pi4ioe5v6408.h"

#include "drivers/m5tab5_driver_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace m5::tab5 {
namespace {

static const char* TAG = "m5tab5.extio.pi4io";

// ── Low-Level Register Helpers / 底层寄存器辅助函?─────────────────────────

static esp_err_t pi4io_write_reg(i2c_bus_device_handle_t dev, uint8_t reg, uint8_t value)
{
    return i2c_bus_write_byte(dev, reg, value);
}

static esp_err_t pi4io_read_reg(i2c_bus_device_handle_t dev, uint8_t reg, uint8_t* value)
{
    return i2c_bus_read_byte(dev, reg, value);
}

static esp_err_t pi4io_modify_bit(i2c_bus_device_handle_t dev, uint8_t reg, uint8_t bit, bool set)
{
    uint8_t val   = 0;
    esp_err_t err = pi4io_read_reg(dev, reg, &val);
    if (err != ESP_OK) return err;
    if (set) {
        val |= (1u << bit);
    } else {
        val &= ~(1u << bit);
    }
    return pi4io_write_reg(dev, reg, val);
}

// ── Descriptor Integration / 描述符集?─────────────────────────────────────

static m5tab5_extio_pi4ioe5v6408_t s_dev_ctx;       // ADDR_LOW 0x43 / 低地址芯片 0x43
static m5tab5_extio_pi4ioe5v6408_t s_dev_ctx_high;  // ADDR_HIGH 0x44 / 高地址芯片 0x44

esp_err_t m5tab5_extio_pi4ioe5v6408_descriptor_probe(m5tab5_runtime_t* runtime)
{
    (void)runtime;
    i2c_bus_handle_t bus = m5tab5_get_sys_i2c_bus();
    if (!bus) {
        ESP_LOGE(TAG, "probe: SYS I2C bus not available");
        return ESP_ERR_INVALID_STATE;
    }
    // Create a temporary device to check for ACK at the default address. /
    // 创建一个临时设备句柄，用于检测默认地址是否?ACK 响应?
    i2c_bus_device_handle_t probe_dev = i2c_bus_device_create(bus, 0x43, 400000);
    if (!probe_dev) {
        ESP_LOGW(TAG, "probe: i2c_bus_device_create failed");
        return ESP_ERR_NOT_FOUND;
    }
    uint8_t id_reg = 0;
    esp_err_t err  = i2c_bus_read_byte(probe_dev, M5TAB5_PI4IO_REG_DEVICE_ID, &id_reg);
    i2c_bus_device_delete(&probe_dev);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "probe: no ACK at 0x43 (%s)", esp_err_to_name(err));
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "probe ok: PI4IOE5V6408 addr=0x43 id_reg=0x%02X", id_reg);
    return ESP_OK;
}

esp_err_t m5tab5_extio_pi4ioe5v6408_descriptor_init(m5tab5_runtime_t* runtime)
{
    i2c_bus_handle_t bus = m5tab5_get_sys_i2c_bus();
    if (!bus) {
        ESP_LOGE(TAG, "init: SYS I2C bus not available");
        return ESP_ERR_INVALID_STATE;
    }
    m5tab5_extio_pi4ioe5v6408_config_t cfg = {};
    cfg.i2c_bus_handle                     = bus;
    cfg.i2c_addr                           = 0x43;
    cfg.scl_speed_hz                       = 400000;
    cfg.int_pin                            = GPIO_NUM_NC;
    esp_err_t err                          = m5tab5_extio_pi4ioe5v6408_init(&cfg, &s_dev_ctx);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "init: m5tab5_extio_pi4ioe5v6408_init failed: %s", esp_err_to_name(err));
        return err;
    }

    // ── Tab5-Specific Pin Configuration / Tab5 专用引脚配置 ─────────────────
    // PI4IOE5V6408 at 0x43 (low-address chip):
    // 地址?0x43 ?PI4IOE5V6408（低地址芯片）：
    //   P0 = RF_PTH_L_INT_H_EXT  (output, default LOW)
    //   P0 = RF_PTH_L_INT_H_EXT  （输出，默认 LOW?
    //   P1 = SPK_EN              (output, HIGH, speaker enabled)
    //   P1 = SPK_EN              （输出，HIGH，扬声器使能?
    //   P2 = EXT5V_EN            (output, HIGH, external 5 V enabled)
    //   P2 = EXT5V_EN            （输出，HIGH，外?5 V 使能?
    //   P3 = unused              (output, LOW)
    //   P3 = unused              （输出，LOW?
    //   P4 = LCD_RST             (output, HIGH, LCD out of reset)
    //   P4 = LCD_RST             （输出，HIGH，LCD 处于非复位状态）
    //   P5 = TP_RST              (output, HIGH, touch out of reset)
    //   P5 = TP_RST              （输出，HIGH，触摸控制器处于非复位状态）
    //   P6 = CAM_RST             (output, HIGH, camera out of reset)
    //   P6 = CAM_RST             （输出，HIGH，摄像头处于非复位状态）
    //   P7 = HP_DET              (input)
    //   P7 = HP_DET              （输入）
    // IO_DIR: 1 means output. P0-P6 are outputs, P7 is input -> 0b0111_1111 = 0x7F.
    // IO_DIR? 表示输出。P0 ?P6 为输出，P7 为输入，对应 0b0111_1111 = 0x7F?
    err = pi4io_write_reg(s_dev_ctx.i2c_dev, M5TAB5_PI4IO_REG_IO_DIR, 0x7F);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "init: IO_DIR write failed: %s", esp_err_to_name(err));
        return err;
    }
    // OUT_HI_Z: 1 means Hi-Z. Drive all outputs actively -> 0x00. / OUT_HI_Z ?1
    // 表示高阻态，这里将全部输出脚主动驱动，因此写?0x00?
    err = pi4io_write_reg(s_dev_ctx.i2c_dev, M5TAB5_PI4IO_REG_OUT_HI_Z, 0x00);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "init: OUT_HI_Z write failed: %s", esp_err_to_name(err));
        return err;
    }
    // OUT_STATE: P2=H P4=H P5=H P6=H -> 0b0111_0100 = 0x74.
    // OUT_STATE：P2=H、P4=H、P5=H、P6=H，对?0b0111_0100 = 0x74?
    // P1 = SPK_EN stays LOW, so the audio subsystem remains on hold and the amplifier is off by default.
    // P1 = SPK_EN 保持 LOW，因此音频子系统处于保持状态，功放默认关闭?
    // P0 = RF_PTH stays LOW, which selects the internal antenna.
    // P0 = RF_PTH 保持 LOW，对应选择内置天线?
    // LCD_RST, TP_RST, and CAM_RST are released from reset; EXT5V_EN is HIGH.
    // LCD_RST、TP_RST、CAM_RST 均已释放复位；EXT5V_EN ?HIGH?
    err = pi4io_write_reg(s_dev_ctx.i2c_dev, M5TAB5_PI4IO_REG_OUT_STATE, 0x74);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "init: OUT_STATE write failed: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "ADDR_LOW(0x43): LCD_RST=H TP_RST=H CAM_RST=H EXT5V_EN=H SPK_EN=L");

    runtime->ioexpander_handle = &s_dev_ctx;

    // ── ADDR_HIGH (0x44) / 高地址芯片 0x44 ───────────────────────────────────
    // Signals: WLAN_PWR_EN, CHG_EN, USB5V_EN, and PWROFF.
    // 对应信号：WLAN_PWR_EN、CHG_EN、USB5V_EN ?PWROFF?
    // PI4IOE5V6408 at 0x44:
    // 地址?0x44 ?PI4IOE5V6408?
    //   P0 = WLAN_PWR_EN     (output, LOW, C6 off; wlan_power(true) will enable it)
    //   P0 = WLAN_PWR_EN     （输出，LOW，C6 断电；wlan_power(true) 会再使能?
    //   P1 = unassigned      (output, LOW)
    //   P1 = unassigned      （输出，LOW?
    //   P2 = unassigned      (output, LOW)
    //   P2 = unassigned      （输出，LOW?
    //   P3 = USB5V_EN        (output, LOW, USB 5 V off)
    //   P3 = USB5V_EN        （输出，LOW，USB 5 V 关闭?
    //   P4 = PWROFF_PLUSE    (output, LOW, no power-off pulse)
    //   P4 = PWROFF_PLUSE    （输出，LOW，不触发关机脉冲?
    //   P5 = NCHG_QC_EN      (output, HIGH, slow or standard charge by default; active LOW for QC)
    //   P5 = NCHG_QC_EN      （输出，HIGH，默认慢?标准充电；QC 低电平有效）
    //   P6 = CHG_STAT        (input)
    //   P6 = CHG_STAT        （输入）
    //   P7 = CHG_EN          (output, HIGH, charging enabled by default)
    //   P7 = CHG_EN          （输出，HIGH，默认允许充电）
    // IO_DIR: P6 is input (0), the rest are outputs (1) -> 0b10111111 = 0xBF.
    // IO_DIR：P6 为输入（0），其余均为输出?），?0b10111111 = 0xBF?
    // OUT_STATE: P7=CHG_EN=H, P5=nCHG_QC_EN=H (QC disabled), others LOW -> 0b10100000 = 0xA0.
    // OUT_STATE：P7=CHG_EN=H，P5=nCHG_QC_EN=H（禁?QC），其余保持 LOW，即 0b10100000 = 0xA0?
    m5tab5_extio_pi4ioe5v6408_config_t high_cfg = {};
    high_cfg.i2c_bus_handle                     = bus;
    high_cfg.i2c_addr                           = 0x44;
    high_cfg.scl_speed_hz                       = 400000;
    high_cfg.int_pin                            = GPIO_NUM_NC;
    err                                         = m5tab5_extio_pi4ioe5v6408_init(&high_cfg, &s_dev_ctx_high);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "init: ADDR_HIGH init failed: %s", esp_err_to_name(err));
        return err;
    }
    err = pi4io_write_reg(s_dev_ctx_high.i2c_dev, M5TAB5_PI4IO_REG_IO_DIR, 0xBF);
    if (err != ESP_OK) return err;
    err = pi4io_write_reg(s_dev_ctx_high.i2c_dev, M5TAB5_PI4IO_REG_OUT_HI_Z, 0x00);
    if (err != ESP_OK) return err;
    err = pi4io_write_reg(s_dev_ctx_high.i2c_dev, M5TAB5_PI4IO_REG_OUT_STATE, 0xA0);
    if (err != ESP_OK) return err;
    ESP_LOGI(TAG, "ADDR_HIGH(0x44): WLAN_PWR_EN=L CHG_EN=H NCHG_QC_EN=H");

    runtime->ioexpander_high_handle = &s_dev_ctx_high;
    return ESP_OK;
}

const m5tab5_device_driver_descriptor_t k_m5tab5_extio_pi4ioe5v6408_driver = {
    .id    = "m5tab5.extio.pi4ioe5v6408",
    .probe = m5tab5_extio_pi4ioe5v6408_descriptor_probe,
    .init  = m5tab5_extio_pi4ioe5v6408_descriptor_init,
};

}  // anonymous namespace

// ── Public Driver API / 对外驱动接口 ───────────────────────────────────────

esp_err_t m5tab5_extio_pi4ioe5v6408_init(const m5tab5_extio_pi4ioe5v6408_config_t* config,
                                         m5tab5_extio_pi4ioe5v6408_t* dev)
{
    if (!config || !dev || !config->i2c_bus_handle) {
        return ESP_ERR_INVALID_ARG;
    }

    // Add the device to the I2C bus via i2c_bus. / 通过 i2c_bus 将设备加?I2C 总线?
    dev->i2c_dev = i2c_bus_device_create(config->i2c_bus_handle, config->i2c_addr, config->scl_speed_hz);
    if (!dev->i2c_dev) {
        ESP_LOGE(TAG, "i2c_bus_device_create failed (addr=0x%02X)", config->i2c_addr);
        return ESP_FAIL;
    }

    dev->int_pin  = config->int_pin;
    dev->i2c_addr = config->i2c_addr;

    // Perform a soft reset. / 执行软复位?
    esp_err_t err = m5tab5_extio_pi4ioe5v6408_soft_reset(dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "soft reset failed: %s", esp_err_to_name(err));
        i2c_bus_device_delete(&dev->i2c_dev);
        return err;
    }

    // Read the Device-ID register to verify that the chip is responding. / 读取 Device-ID 寄存器，以确认芯片有响应?
    uint8_t id_reg = 0;
    err            = pi4io_read_reg(dev->i2c_dev, M5TAB5_PI4IO_REG_DEVICE_ID, &id_reg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "device ID read failed: %s", esp_err_to_name(err));
        i2c_bus_device_delete(&dev->i2c_dev);
        return err;
    }

    uint8_t mfr_id = id_reg >> 5;
    uint8_t fw_rev = (id_reg >> 2) & 0x07;
    ESP_LOGI(TAG, "PI4IOE5V6408[0x%02X] ready: mfr=0x%X fw_rev=0x%X", config->i2c_addr, mfr_id, fw_rev);

    return ESP_OK;
}

void m5tab5_extio_pi4ioe5v6408_deinit(m5tab5_extio_pi4ioe5v6408_t* dev)
{
    if (!dev) return;
    if (dev->i2c_dev) {
        i2c_bus_device_delete(&dev->i2c_dev);
    }
    dev->int_pin  = GPIO_NUM_NC;
    dev->i2c_addr = 0;
}

esp_err_t m5tab5_extio_pi4ioe5v6408_set_pin_mode(m5tab5_extio_pi4ioe5v6408_t* dev, M5TAB5_ExtIo_PI4IOE5V6408_Pin pin,
                                                 M5TAB5_ExtIo_PI4IOE5V6408_PinMode mode)
{
    if (!dev || !dev->i2c_dev || pin >= M5TAB5_EXTIO_PI4IOE5V6408_PIN_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err;

    switch (mode) {
        case M5TAB5_EXTIO_PI4IOE5V6408_PIN_INPUT:
            err = pi4io_modify_bit(dev->i2c_dev, M5TAB5_PI4IO_REG_IO_DIR, pin, false);
            if (err != ESP_OK) return err;
            err = pi4io_modify_bit(dev->i2c_dev, M5TAB5_PI4IO_REG_OUT_HI_Z, pin, true);
            if (err != ESP_OK) return err;
            err = pi4io_modify_bit(dev->i2c_dev, M5TAB5_PI4IO_REG_PULL_ENABLE, pin, false);
            return err;

        case M5TAB5_EXTIO_PI4IOE5V6408_PIN_OUTPUT:
            err = pi4io_modify_bit(dev->i2c_dev, M5TAB5_PI4IO_REG_IO_DIR, pin, true);
            if (err != ESP_OK) return err;
            err = pi4io_modify_bit(dev->i2c_dev, M5TAB5_PI4IO_REG_OUT_HI_Z, pin, false);
            return err;

        case M5TAB5_EXTIO_PI4IOE5V6408_PIN_INPUT_PULLUP:
            err = pi4io_modify_bit(dev->i2c_dev, M5TAB5_PI4IO_REG_IO_DIR, pin, false);
            if (err != ESP_OK) return err;
            err = pi4io_modify_bit(dev->i2c_dev, M5TAB5_PI4IO_REG_OUT_HI_Z, pin, true);
            if (err != ESP_OK) return err;
            err = pi4io_modify_bit(dev->i2c_dev, M5TAB5_PI4IO_REG_PULL_ENABLE, pin, true);
            if (err != ESP_OK) return err;
            err = pi4io_modify_bit(dev->i2c_dev, M5TAB5_PI4IO_REG_PULL_SELECT, pin, true);
            return err;

        case M5TAB5_EXTIO_PI4IOE5V6408_PIN_INPUT_PULLDOWN:
            err = pi4io_modify_bit(dev->i2c_dev, M5TAB5_PI4IO_REG_IO_DIR, pin, false);
            if (err != ESP_OK) return err;
            err = pi4io_modify_bit(dev->i2c_dev, M5TAB5_PI4IO_REG_OUT_HI_Z, pin, true);
            if (err != ESP_OK) return err;
            err = pi4io_modify_bit(dev->i2c_dev, M5TAB5_PI4IO_REG_PULL_ENABLE, pin, true);
            if (err != ESP_OK) return err;
            err = pi4io_modify_bit(dev->i2c_dev, M5TAB5_PI4IO_REG_PULL_SELECT, pin, false);
            return err;

        default:
            return ESP_ERR_INVALID_ARG;
    }
}

esp_err_t m5tab5_extio_pi4ioe5v6408_write_pin(m5tab5_extio_pi4ioe5v6408_t* dev, M5TAB5_ExtIo_PI4IOE5V6408_Pin pin,
                                              bool level)
{
    if (!dev || !dev->i2c_dev || pin >= M5TAB5_EXTIO_PI4IOE5V6408_PIN_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    return pi4io_modify_bit(dev->i2c_dev, M5TAB5_PI4IO_REG_OUT_STATE, pin, level);
}

esp_err_t m5tab5_extio_pi4ioe5v6408_read_pin(m5tab5_extio_pi4ioe5v6408_t* dev, M5TAB5_ExtIo_PI4IOE5V6408_Pin pin,
                                             bool* level)
{
    if (!dev || !dev->i2c_dev || pin >= M5TAB5_EXTIO_PI4IOE5V6408_PIN_MAX || !level) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t val   = 0;
    esp_err_t err = pi4io_read_reg(dev->i2c_dev, M5TAB5_PI4IO_REG_IN_STATUS, &val);
    if (err != ESP_OK) return err;
    *level = (val >> pin) & 0x01;
    return ESP_OK;
}

esp_err_t m5tab5_extio_pi4ioe5v6408_read_all_pins(m5tab5_extio_pi4ioe5v6408_t* dev, uint8_t* out_state)
{
    if (!dev || !dev->i2c_dev || !out_state) {
        return ESP_ERR_INVALID_ARG;
    }
    return pi4io_read_reg(dev->i2c_dev, M5TAB5_PI4IO_REG_IN_STATUS, out_state);
}

esp_err_t m5tab5_extio_pi4ioe5v6408_write_all_pins(m5tab5_extio_pi4ioe5v6408_t* dev, uint8_t state)
{
    if (!dev || !dev->i2c_dev) {
        return ESP_ERR_INVALID_ARG;
    }
    return pi4io_write_reg(dev->i2c_dev, M5TAB5_PI4IO_REG_OUT_STATE, state);
}

esp_err_t m5tab5_extio_pi4ioe5v6408_soft_reset(m5tab5_extio_pi4ioe5v6408_t* dev)
{
    if (!dev || !dev->i2c_dev) {
        return ESP_ERR_INVALID_ARG;
    }
    // Writing 0xFF to the Device-ID/Control register (0x01) triggers a chip software reset.
    // ?Device-ID/Control 寄存器（0x01）写?0xFF 会触发芯片的软件复位?
    // The value 0xFF matches the reference M5Stack Tab5 BSP.
    // 这里?0xFF 与参考的 M5Stack Tab5 BSP 保持一致?
    esp_err_t err = pi4io_write_reg(dev->i2c_dev, M5TAB5_PI4IO_REG_DEVICE_ID, 0xFF);
    if (err != ESP_OK) return err;
    vTaskDelay(pdMS_TO_TICKS(
        10));  // chip resets in <1 ms; 10 ms is conservative / 芯片小于 1 ms 即可复位完成，这里保守等?10 ms
    return ESP_OK;
}

esp_err_t m5tab5_extio_pi4ioe5v6408_read_interrupt_status(m5tab5_extio_pi4ioe5v6408_t* dev, uint8_t* status)
{
    if (!dev || !dev->i2c_dev || !status) {
        return ESP_ERR_INVALID_ARG;
    }
    return pi4io_read_reg(dev->i2c_dev, M5TAB5_PI4IO_REG_INT_STATUS, status);
}

esp_err_t m5tab5_extio_pi4ioe5v6408_set_interrupt_mask(m5tab5_extio_pi4ioe5v6408_t* dev,
                                                       M5TAB5_ExtIo_PI4IOE5V6408_Pin pin, bool enable)
{
    if (!dev || !dev->i2c_dev || pin >= M5TAB5_EXTIO_PI4IOE5V6408_PIN_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    // INT_MASK register: 0 means interrupt enabled, 1 means masked. / INT_MASK 寄存器中? 表示允许中断? 表示屏蔽?
    return pi4io_modify_bit(dev->i2c_dev, M5TAB5_PI4IO_REG_INT_MASK, pin, !enable);
}

// ── Descriptor Factory / 描述符工?────────────────────────────────────────

const m5tab5_device_driver_descriptor_t* m5tab5_get_extio_pi4ioe5v6408_driver()
{
    return &k_m5tab5_extio_pi4ioe5v6408_driver;
}

}  // namespace m5::tab5