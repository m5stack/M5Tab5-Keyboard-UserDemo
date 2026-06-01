/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef UI_PAGE_FACTORY_H
#define UI_PAGE_FACTORY_H

#include "ui_page_base.h"
#include "factorytest_keyboard.h"
#include "m5_tab5_keyboard.h"

#ifdef __cplusplus

    /**
     * @brief 工厂测试页面
     *
     * 用于测试物理键盘的所有按键是否正常工作
     * 显示5x14网格，每个按键按下后会高亮显示
     */
    class FactoryPage : public BasePage {
public:
    FactoryPage();
    virtual ~FactoryPage();

    // 生命周期方法
    virtual void onCreate() override;
    virtual void onShow() override;
    virtual void onHide() override;

    /**
     * @brief 模拟物理按键按下（按下未松手）- 深灰色 + click 显示，无 OK
     */
    void simulatePhysicalKeyDown(uint16_t key_id);

    /**
     * @brief 按下超过长按门槛（仍未松手）- 深灰色 click 切换为 long
     */
    void simulatePhysicalKeyLongPressing(uint16_t key_id);

    /**
     * @brief 模拟物理按键短按松开 - 深绿色 + OK，无 long
     */
    void simulatePhysicalKeyShortRelease(uint16_t key_id);

    /**
     * @brief 模拟物理按键长按松开 - 深绿色 + OK + long
     */
    void simulatePhysicalKeyLongRelease(uint16_t key_id);

    /**
     * @brief 设置键盘实例（用于RGB控制）
     * @param kb 键盘实例指针
     */
    void setKeyboardInstance(m5::M5Tab5Keyboard* kb);

    /**
     * @brief 刷新左上角 SW/FW 版本显示（热插拔重连后外部调用）
     */
    void refreshVersionLabel();

    /**
     * @brief 重置所有按键测试记录（拔出键盘时调用）
     */
    void resetAllKeys();

private:
    lv_obj_t* m_placeholder;            ///< 占位容器
    FactoryTestKeyboard* m_keyboard;    ///< 工厂测试键盘对象
    m5::M5Tab5Keyboard* m_kb_instance;  ///< 物理键盘实例

    // RGB显示UI
    lv_obj_t* m_rgb_container;     ///< RGB显示容器
    lv_obj_t* m_rgb1_indicator;    ///< RGB1指示器
    lv_obj_t* m_rgb2_indicator;    ///< RGB2指示器
    lv_obj_t* m_rgb1_label;        ///< RGB1颜色标签
    lv_obj_t* m_rgb2_label;        ///< RGB2颜色标签
    lv_obj_t* m_version_label;     ///< 左上角版本标签
    lv_obj_t* m_brightness_label;  ///< 亮度标签
    lv_obj_t* m_brightness_bar;    ///< 亮度条

    // 结果显示UI
    lv_obj_t* m_result_container;  ///< 结果显示容器
    lv_obj_t* m_result_label;      ///< 结果标签

    // RGB呼吸灯效果
    lv_timer_t* m_rgb_timer;        ///< RGB更新定时器
    uint8_t m_current_color_index;  ///< 当前颜色索引
    uint8_t m_manual_color_index;   ///< 手动覆盖模式下的颜色索引
    uint8_t m_breath_value;         ///< 呼吸值
    bool m_breath_increasing;       ///< 呼吸方向
    uint8_t m_pause_counter;        ///< 停顿计数器
    bool m_manual_override_active;  ///< 当前页面是否启用手动覆盖模式

    /**
     * @brief 创建RGB显示UI
     */
    void createRGBDisplayUI();

    /**
     * @brief 创建结果显示UI
     */
    void createResultDisplayUI();

    /**
     * @brief 检查所有按键是否已按下
     */
    void checkAllKeysPressed();

    /**
     * @brief RGB定时器回调
     */
    static void rgbTimerCallback(lv_timer_t* timer);

    /**
     * @brief RGB定时器处理
     */
    void handleRGBTimer();

    /**
     * @brief 亮度滑块事件回调
     */
    static void brightnessSliderEventCallback(lv_event_t* event);

    /**
     * @brief RGB 色块点击事件回调
     */
    static void rgbIndicatorEventCallback(lv_event_t* event);

    /**
     * @brief 处理手动亮度调整
     */
    void handleBrightnessSliderChanged(int32_t value);

    /**
     * @brief 处理 RGB 色块点击后的手动颜色覆盖
     */
    void handleRgbIndicatorClicked();

    /**
     * @brief 将两个 RGB 灯设置为当前页面要求的固定白色
     */
    void applyFactoryWhiteRgb(uint8_t brightness);

    /**
     * @brief 将两个 RGB 灯设置为手动覆盖颜色，亮度保持不变
     */
    void applyFactoryManualColor(uint8_t brightness, uint8_t color_index);
};

#endif  // __cplusplus

#endif  // UI_PAGE_FACTORY_H
