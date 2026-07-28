#include "drv_buzzer.hpp"
#include "stm32h7xx_hal.h"

// TIM4 位于 APB1, 定时器时钟 = PCLK1 * 2 = 200MHz
// 固定预分频 199 -> 200MHz/(199+1) = 1MHz 计数频率, 故频率(Hz) = 1000000 / (ARR+1)
static const uint32_t BUZZER_PRESCALER = 199;
static const uint32_t BUZZER_TICK_HZ   = 1000000U;

DrvBuzzer::DrvBuzzer(BoardPwmPort port, uint32_t channel)
    : m_port(port), m_channel(channel)
{
}

bool DrvBuzzer::init()
{
    if (m_port == BOARD_PWM_NONE || m_channel == 0)
        return false;

    board_pwm_start(m_port, m_channel);
    off();
    return true;
}

void DrvBuzzer::on()
{
    board_pwm_set_duty(m_port, m_channel, 0.5f);
}

void DrvBuzzer::off()
{
    board_pwm_set_duty(m_port, m_channel, 0.0f);
}

void DrvBuzzer::setFrequency(uint32_t freq)
{
    if (freq == 0)
        freq = 1000;

    uint32_t arr = BUZZER_TICK_HZ / freq;
    if (arr == 0)
        arr = 1;
    arr -= 1;

    board_pwm_set_frequency(m_port, BUZZER_PRESCALER, arr);
}

void DrvBuzzer::setDuty(float duty)
{
    board_pwm_set_duty(m_port, m_channel, duty);
}

void DrvBuzzer::beep(uint32_t freq, uint32_t duration_ms)
{
    setFrequency(freq);
    setDuty(0.5f);
    osDelay(duration_ms);
    off();
}

// =============================================================================
// 全局访问函数
// =============================================================================

static DrvBuzzer *g_drv_buzzer = nullptr;

void init_drv_buzzer()
{
    if (g_drv_buzzer)
        return;

    g_drv_buzzer = new DrvBuzzer(BOARD_PWM_BUZZER, PWM_CH_3);
    if (g_drv_buzzer)
        g_drv_buzzer->init();
}

DrvBuzzer *drv_buzzer()
{
    return g_drv_buzzer;
}
