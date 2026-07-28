#pragma once

#include "board.hpp"
#include "cmsis_os2.h"
#include <stdint.h>

// 蜂鸣器驱动 (基于 PWM 输出, 通过 board_pwm_* 接口操作)
class DrvBuzzer
{
public:
    DrvBuzzer(BoardPwmPort port, uint32_t channel);

    bool init();

    void on();
    void off();

    void setFrequency(uint32_t freq);
    void setDuty(float duty);   // 0.0~1.0

    void beep(uint32_t freq, uint32_t duration_ms);

private:
    BoardPwmPort m_port;
    uint32_t     m_channel;
};

void init_drv_buzzer();
DrvBuzzer *drv_buzzer();
