#pragma once

#include "drv_common.hpp"
#include "stm32h7xx_hal.h"
#include "cmsis_os2.h"
#include <stdint.h>

//PWM 通道选择位 (start/stop 支持位或, setPulse/setDuty 传单通道)
#define PWM_CH_1   (1u << 0)
#define PWM_CH_2   (1u << 1)
#define PWM_CH_3   (1u << 2)
#define PWM_CH_4   (1u << 3)
#define PWM_CH_ALL (PWM_CH_1 | PWM_CH_2 | PWM_CH_3 | PWM_CH_4)

typedef enum
{
    PWM_Drv_Ok = 0,
    PWM_Drv_BadIndex,
    PWM_Drv_BadParam,
    PWM_Drv_Timeout,
    PWM_Drv_Busy,
    PWM_Drv_Error,
} PWM_DrvResult;

struct PWM_Config
{
    HwPin   ch1;            //通道1 引脚, port==0 表示未使用
    HwPin   ch2;            //通道2
    HwPin   ch3;            //通道3
    HwPin   ch4;            //通道4

    uint32_t pulse1;        //各通道初始比较值 (CCR)
    uint32_t pulse2;
    uint32_t pulse3;
    uint32_t pulse4;

    uint32_t prescaler;     //预分频 (PSC)
    uint32_t period;        //自动重装载值 (ARR)
    uint32_t counter_mode;  //TIM_COUNTERMODE_UP / DOWN, 0 = 默认 UP
    uint32_t clockdivision; //TIM_CLOCKDIVISION_DIV1 等, 0 = 默认 DIV1

    uint32_t pull;          //GPIO 上下拉
    uint32_t speed;         //GPIO 速度

    uint8_t irq_priority;   //保留 
};

class PWM_CLASS
{
  public:
    bool init(uint8_t pwm_num, const PWM_Config *cfg);
    void deinit();

    PWM_DrvResult start(uint32_t channels);                    //channels: PWM_CH_x 位或
    PWM_DrvResult stop(uint32_t channels);
    PWM_DrvResult setPulse(uint32_t channel, uint32_t pulse);  //单通道 (PWM_CH_x)
    PWM_DrvResult setDuty(uint32_t channel, float duty);       //duty 0.0~1.0
    PWM_DrvResult setFrequency(uint32_t prescaler, uint32_t period); //更新 PSC/ARR (运行时改频率)

    bool lock(double timeout = -1.0);
    void unlock();

    uint8_t  number() const { return pwmNum; }
    uint32_t period() const { return htim.Init.Period; }
    uint32_t channels() const { return channelsUsed; }
    TIM_HandleTypeDef *handle() { return &htim; }

  private:
    TIM_HandleTypeDef htim;
    osMutexId_t mutex;
    uint8_t  pwmNum;
    uint32_t channelsUsed;
};

bool register_pwm(uint8_t pwm_num, const PWM_Config *cfg);
PWM_CLASS *get_pwm_instance(uint8_t pwm_num);
void init_drv_pwm(void);
