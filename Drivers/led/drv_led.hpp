#pragma once

#include "board.hpp"
#include "cmsis_os2.h"
#include <stdint.h>

// RGB LED PWM 驱动 (基于 board_pwm_* 接口)
// 使用一个定时器的 CH2/CH3/CH4 三路 PWM 分别驱动 R/G/B
// 百分比 0-100 内部映射为占空比 0.0~1.0 (由 board_pwm_set_duty 转为 CCR)
class DrvLed
{
public:
    // port: RGB 所在的逻辑 PWM 端口
    DrvLed(BoardPwmPort port);

    // 启动三路 PWM 通道并熄灭 LED
    bool init();

    // 设置 RGB 占空比 (百分比, 总和无需严格等于 100, 各路独立 0-100)
    void setRGB(uint8_t red, uint8_t green, uint8_t blue);

    // 设置 RGB 并以指定频率闪烁 (在独立任务中运行)
    // frequency_hz == 0 表示常亮不闪烁
    void setRGBBlink(uint8_t red, uint8_t green, uint8_t blue, uint8_t frequency_hz);

    // 停止闪烁 (熄灭 LED)
    void stopBlink();

    // 当前是否正在闪烁
    bool isBlinking();

    void setRed(uint8_t value);     // 红色百分比 (0-100)
    void setGreen(uint8_t value);   // 绿色百分比 (0-100)
    void setBlue(uint8_t value);    // 蓝色百分比 (0-100)

    void turnOff();                 // 熄灭全部 LED

private:
    static void blinkTaskFunc(void *parameter);
    void blinkTask();

    BoardPwmPort      m_port;
    osThreadId_t      m_blinkTaskHandle;
    uint8_t           m_blinkRed;
    uint8_t           m_blinkGreen;
    uint8_t           m_blinkBlue;
    uint16_t          m_blinkHalfPeriodMs;
    volatile bool     m_blinkRunning;
    bool              m_isInitialized;

    // 通道映射 (与参考工程一致: CH4=R, CH3=G, CH2=B)
    static constexpr uint32_t RED_CHANNEL   = PWM_CH_4;
    static constexpr uint32_t GREEN_CHANNEL = PWM_CH_3;
    static constexpr uint32_t BLUE_CHANNEL  = PWM_CH_2;

    // 百分比 (0-100) -> 占空比 (0.0~1.0)
    static float percentToDuty(uint8_t percent);
};

// 全局访问 (需调用方先 init_drv_led(BOARD_PWM_LED) 传入已配置的端口)
void init_drv_led(BoardPwmPort port);
DrvLed *drv_led();
