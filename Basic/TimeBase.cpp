#include "TimeBase.h"
#include "stm32h7xx_hal.h"
#include "stm32h743xx.h"

// =============================================================================
// 系统时钟配置 (替代 CubeMX 的 SystemClock_Config)
//
// 时钟树 (HSE=16MHz, VOS1, SYSCLK=400MHz):
//   HSE 16M -> PLL1: M=2 N=100 P=2 -> SYSCLK=400MHz
//   HPRE=/2  -> HCLK  =200MHz
//   APB3=/2  ->        100MHz
//   APB1=/2  ->        100MHz  (I2C1-3 / USART2-8 / SPI2-3 / TIM2-7,12-14)
//   APB2=/2  ->        100MHz  (SPI1/4-5 / USART1/6 / TIM1/8/15-17)
//   APB4=/2  ->        100MHz  (I2C4 / SPI6 / LPTIM / BDMA)
//
// 修改后必须同步更新 TimeBase.h 里的编译期常量宏
// =============================================================================

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    RCC_OscInitStruct.OscillatorType       = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState             = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState         = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource        = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM             = 2;
    RCC_OscInitStruct.PLL.PLLN             = 100;
    RCC_OscInitStruct.PLL.PLLP             = 2;
    RCC_OscInitStruct.PLL.PLLQ             = 4;
    RCC_OscInitStruct.PLL.PLLR             = 2;
    RCC_OscInitStruct.PLL.PLLRGE           = RCC_PLL1VCIRANGE_3;
    RCC_OscInitStruct.PLL.PLLVCOSEL        = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLFRACN         = 0;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        while (1) {}

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
                                       RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
        while (1) {}
}

// =============================================================================
// HAL 时间基准 (TIM7) —— SysTick作为FreeRTOS时钟源
// =============================================================================

static TIM_HandleTypeDef htim7;

static uint32_t TIM7_GetClockFreq(void)
{
    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    uint32_t prescaler = READ_BIT(RCC->D2CFGR, RCC_D2CFGR_D2PPRE1);
    if (prescaler != RCC_D2CFGR_D2PPRE1_DIV1)
        return pclk1 * 2U;
    return pclk1;
}

extern "C" HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    uint32_t tim_clock = TIM7_GetClockFreq();
    uint32_t prescaler = (tim_clock / 1000000U) - 1U;
    uint32_t period    = (1000000U / 1000U) - 1U;

    htim7.Instance               = TIM7;
    htim7.Init.Prescaler         = prescaler;
    htim7.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim7.Init.Period            = period;
    htim7.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    __HAL_RCC_TIM7_CLK_ENABLE();

    if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
        return HAL_ERROR;

    HAL_NVIC_SetPriority(TIM7_IRQn, TickPriority, 0);
    HAL_NVIC_EnableIRQ(TIM7_IRQn);

    if (HAL_TIM_Base_Start_IT(&htim7) != HAL_OK)
        return HAL_ERROR;

    uwTickPrio = TickPriority;
    return HAL_OK;
}

extern "C" void HAL_SuspendTick(void)
{
    __HAL_TIM_DISABLE_IT(&htim7, TIM_IT_UPDATE);
    __HAL_TIM_DISABLE(&htim7);
}

extern "C" void HAL_ResumeTick(void)
{
    __HAL_TIM_ENABLE(&htim7);
    __HAL_TIM_ENABLE_IT(&htim7, TIM_IT_UPDATE);
}

extern "C" void TIM7_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim7);
}

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM7)
        HAL_IncTick();
}

// =============================================================================
// FreeRTOS 运行时统计计数器 —— DWT CYCCNT
// =============================================================================

extern "C" void RunTimeStats_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

extern "C" uint32_t RunTimeStats_GetCounter(void)
{
    return DWT->CYCCNT;
}

// =============================================================================
// 统一入口: 时钟树 + HAL 初始化 + HAL 时间基准
// =============================================================================

void init_TimeBase(void)
{
    HAL_Init();
	
    SystemClock_Config();
	
    HAL_EnableCompensationCell();
	//DWT计数器初始化
	RunTimeStats_Init();
}
