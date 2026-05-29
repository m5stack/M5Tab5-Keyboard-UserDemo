#ifndef UI_PAGE_STRING_H
#define UI_PAGE_STRING_H

#include "ui_page_base.h"
#include <deque>
#include <string>

#ifdef __cplusplus

/**
 * @brief 字符模式页面
 *
 * 字符模式：物理键盘直接输出ASCII字符
 * 显示一个文本输入区域，接收物理键盘的字符串输入
 */
class StringPage : public BasePage {
public:
    StringPage();
    virtual ~StringPage();

    // 生命周期方法
    virtual void onCreate() override;
    virtual void onShow() override;
    virtual void onHide() override;

    /**
     * @brief 处理字符串输入
     * @param str_data 字符串数据
     * @param str_len 字符串长度
     * @param modifier 修饰键
     */
    void handleStringInput(const char* str_data, uint8_t str_len, uint8_t modifier);

    /**
     * @brief 追加日志信息到日志文本框
     * @param modifier 修饰键 (0:正常, 1:深蓝, 4:深灰, 其他:白色)
     * @param fmt 格式化字符串
     * @param ... 参数
     */
    void appendLog(uint8_t modifier, const char* fmt, ...);

private:
    std::deque<std::string> m_log_lines; ///< 日志行缓存 (FIFO)
    static const size_t MAX_LOG_LINES = 16;
    
    lv_obj_t* m_textarea;    ///< 文本输入区域
    lv_obj_t* m_log_ta;      ///< 只有显示的日志区域
};

#endif // __cplusplus

#endif // UI_PAGE_STRING_H
