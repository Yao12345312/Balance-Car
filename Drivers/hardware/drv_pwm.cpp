#include "drv_pwm.hpp"
#include "drv_common.hpp"

#include "cmsis_os2.h"
#include "board.hpp"

#include "stm32h743xx.h"

static const osMutexAttr_t s_recursiveMutexAttr = {.attr_bits = osMutexRecursive};

static PWM_CLASS pwm_instances[HW_DRV_MAX_PWM];
static bool pwm_inited[HW_DRV_MAX_PWM] = {};

//逻辑 PWM 号 -> 物理定时器
// 1=TIM1 2=TIM2 3=TIM3 4=TIM4 5=TIM5 6=TIM8
// 7=TIM12 8=TIM13 9=TIM14 10=TIM15 11=TIM16 12=TIM17
//注: TIM6/TIM7 为基本定时器 (无 PWM 输出) 已排除; TIM7 由 HAL 时间基准占用
static TIM_TypeDef *const timTable[HW_DRV_MAX_PWM] = {
    0, TIM1, TIM2, TIM3, TIM4, TIM5, TIM8,
    TIM12, TIM13, TIM14, TIM15, TIM16, TIM17,
};

static const uint32_t s_halChannel[4] = {
    TIM_CHANNEL_1, TIM_CHANNEL_2, TIM_CHANNEL_3, TIM_CHANNEL_4,
};

static void tim_clk_enable(uint8_t num)
{
    switch (num)
    {
        case 1:  __HAL_RCC_TIM1_CLK_ENABLE();  break;
        case 2:  __HAL_RCC_TIM2_CLK_ENABLE();  break;
        case 3:  __HAL_RCC_TIM3_CLK_ENABLE();  break;
        case 4:  __HAL_RCC_TIM4_CLK_ENABLE();  break;
        case 5:  __HAL_RCC_TIM5_CLK_ENABLE();  break;
        case 6:  __HAL_RCC_TIM8_CLK_ENABLE();  break;
        case 7:  __HAL_RCC_TIM12_CLK_ENABLE(); break;
        case 8:  __HAL_RCC_TIM13_CLK_ENABLE(); break;
        case 9:  __HAL_RCC_TIM14_CLK_ENABLE(); break;
        case 10: __HAL_RCC_TIM15_CLK_ENABLE(); break;
        case 11: __HAL_RCC_TIM16_CLK_ENABLE(); break;
        case 12: __HAL_RCC_TIM17_CLK_ENABLE(); break;
        default: break;
    }
}

bool PWM_CLASS::init(uint8_t num, const PWM_Config *cfg)
{
    if (!HW_PWM_INDEX_VALID(num) || !cfg)
        return false;

    const HwPin chArr[4] = { cfg->ch1, cfg->ch2, cfg->ch3, cfg->ch4 };
    const uint32_t pulseArr[4] = { cfg->pulse1, cfg->pulse2, cfg->pulse3, cfg->pulse4 };

    bool any = false;
    for (int i = 0; i < 4; i++)
        if (chArr[i].af != 0) { any = true; break; }
    if (!any)
        return false;

    pwmNum = num;
    channelsUsed = 0;

    tim_clk_enable(num);
    HW_Delay(0.001);

    //配置所有启用通道的 GPIO 复用
    for (int i = 0; i < 4; i++)
    {
        if (chArr[i].af == 0)
            continue;
        HW_ConfigurePinAF(&chArr[i], GPIO_MODE_AF_PP, cfg->pull, cfg->speed);
    }

    htim.Instance = timTable[num];
    htim.Init.Prescaler = cfg->prescaler;
    htim.Init.Period = cfg->period;
    htim.Init.CounterMode = cfg->counter_mode ? cfg->counter_mode : TIM_COUNTERMODE_UP;
    htim.Init.ClockDivision = cfg->clockdivision ? cfg->clockdivision : TIM_CLOCKDIVISION_DIV1;
    htim.Init.RepetitionCounter = 0;
    //ARR 预装载必须关闭: 蜂鸣器等需要在运行时修改频率 (ARR) 的设备,
    htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim) != HAL_OK)
        return false;

    //逐通道配置 PWM 输出比较
    for (int i = 0; i < 4; i++)
    {
        if (chArr[i].af == 0)
            continue;

        TIM_OC_InitTypeDef oc = {0};
        oc.OCMode = TIM_OCMODE_PWM1;
        oc.Pulse = pulseArr[i];
        oc.OCPolarity = TIM_OCPOLARITY_HIGH;
        oc.OCFastMode = TIM_OCFAST_DISABLE;
        if (HAL_TIM_PWM_ConfigChannel(&htim, &oc, s_halChannel[i]) != HAL_OK)
            return false;
        channelsUsed |= (1u << i);
    }

    mutex = osMutexNew(&s_recursiveMutexAttr);
    return true;
}

void PWM_CLASS::deinit()
{
    HAL_TIM_PWM_DeInit(&htim);
}

bool PWM_CLASS::lock(double timeout)
{
    uint32_t ticks = (timeout < 0) ? osWaitForever : (uint32_t)(timeout * (double)osKernelGetTickFreq());
    return osMutexAcquire(mutex, ticks) == osOK;
}

void PWM_CLASS::unlock()
{
    osMutexRelease(mutex);
}

PWM_DrvResult PWM_CLASS::start(uint32_t channels)
{
    if (!channels)
        return PWM_Drv_BadParam;
    if ((channels & channelsUsed) != channels)
        return PWM_Drv_BadParam;

    if (!lock())
        return PWM_Drv_Busy;

    PWM_DrvResult r = PWM_Drv_Ok;
    for (int i = 0; i < 4; i++)
    {
        if (channels & (1u << i))
        {
            if (HAL_TIM_PWM_Start(&htim, s_halChannel[i]) != HAL_OK)
            {
                r = PWM_Drv_Error;
                break;
            }
        }
    }
    unlock();
    return r;
}

PWM_DrvResult PWM_CLASS::stop(uint32_t channels)
{
    if (!channels)
        return PWM_Drv_BadParam;
    if ((channels & channelsUsed) != channels)
        return PWM_Drv_BadParam;

    if (!lock())
        return PWM_Drv_Busy;

    PWM_DrvResult r = PWM_Drv_Ok;
    for (int i = 0; i < 4; i++)
    {
        if (channels & (1u << i))
        {
            if (HAL_TIM_PWM_Stop(&htim, s_halChannel[i]) != HAL_OK)
                r = PWM_Drv_Error;
        }
    }
    unlock();
    return r;
}

PWM_DrvResult PWM_CLASS::setPulse(uint32_t channel, uint32_t pulse)
{
    if (channel == 0)
        return PWM_Drv_BadParam;
    for (int i = 0; i < 4; i++)
    {
        if (channel == (1u << i))
        {
            if (!(channelsUsed & channel))
                return PWM_Drv_BadParam;
            __HAL_TIM_SET_COMPARE(&htim, s_halChannel[i], pulse);
            return PWM_Drv_Ok;
        }
    }
    return PWM_Drv_BadParam;
}

PWM_DrvResult PWM_CLASS::setDuty(uint32_t channel, float duty)
{
    if (duty < 0.0f) duty = 0.0f;
    if (duty > 1.0f) duty = 1.0f;
    uint32_t pulse = (uint32_t)(duty * (float)(htim.Init.Period + 1u) + 0.5f);
    return setPulse(channel, pulse);
}

PWM_DrvResult PWM_CLASS::setFrequency(uint32_t prescaler, uint32_t period)
{
    if (!lock())
        return PWM_Drv_Busy;

    htim.Init.Prescaler = prescaler;
    htim.Init.Period = period;
    __HAL_TIM_SET_PRESCALER(&htim, prescaler);
    __HAL_TIM_SET_AUTORELOAD(&htim, period);

    unlock();
    return PWM_Drv_Ok;
}

bool register_pwm(uint8_t pwm_num, const PWM_Config *cfg)
{
    if (!HW_PWM_INDEX_VALID(pwm_num) || !cfg)
        return false;
    if (pwm_inited[pwm_num])
        return false;
    if (!pwm_instances[pwm_num].init(pwm_num, cfg))
        return false;
    pwm_inited[pwm_num] = true;
    return true;
}

PWM_CLASS *get_pwm_instance(uint8_t pwm_num)
{
    if (HW_PWM_INDEX_VALID(pwm_num) && pwm_inited[pwm_num])
        return &pwm_instances[pwm_num];
    return 0;
}

void init_drv_pwm(void)
{
    for (uint8_t i = 0; i < BOARD_PWM_COUNT; ++i)
    {
        if (board_pwm_ports[i].hw_num == 0 || board_pwm_ports[i].config == 0)
            continue;
        register_pwm(board_pwm_ports[i].hw_num, board_pwm_ports[i].config);
    }
}
