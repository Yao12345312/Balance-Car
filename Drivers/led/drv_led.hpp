#pragma once

#include "board.hpp"
#include "cmsis_os2.h"
#include <stdint.h>

// RGB LED GPIO 驱动 (基于 board_led_* 接口)
// 三色通道无 TIM 复用通道, 直接 GPIO 高低电平控制
// setRGB/setRed/... 参数非 0 即点亮该通道 (GPIO 无中间亮度)
class DrvLed
{
public:
    // 引脚绑定在 board.cpp 中完成 (R=PD5 G=PD4 B=PD3)
    DrvLed();

    // 初始化三路 GPIO 输出并熄灭 LED
    bool init();

    // 设置 RGB (各通道非 0 即点亮)
    void setRGB(uint8_t red, uint8_t green, uint8_t blue);

    // 设置 RGB 并以指定频率闪烁 (在独立任务中运行)
    // frequency_hz == 0 表示常亮不闪烁
    void setRGBBlink(uint8_t red, uint8_t green, uint8_t blue, uint8_t frequency_hz);

    // 停止闪烁 (熄灭 LED)
    void stopBlink();

    // 当前是否正在闪烁
    bool isBlinking();

    void setRed(uint8_t value);     // 红色 (非 0 点亮)
    void setGreen(uint8_t value);   // 绿色 (非 0 点亮)
    void setBlue(uint8_t value);    // 蓝色 (非 0 点亮)

    void turnOff();                 // 熄灭全部 LED

private:
    static void blinkTaskFunc(void *parameter);
    void blinkTask();

    osThreadId_t      m_blinkTaskHandle;
    uint8_t           m_blinkRed;
    uint8_t           m_blinkGreen;
    uint8_t           m_blinkBlue;
    uint16_t          m_blinkHalfPeriodMs;
    volatile bool     m_blinkRunning;
    bool              m_isInitialized;
};

// 全局访问 (内部绑定 R=PD5 G=PD4 B=PD3, 见 board.cpp)
void init_drv_led();
DrvLed *drv_led();
