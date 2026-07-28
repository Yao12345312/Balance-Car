#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// 编译期时钟常量 —— 与 TimeBase.cpp 里 SystemClock_Config() 的配置一一对应
//
// 时钟树 (HSI=64MHz, VOS1, SYSCLK=400MHz):
//   HSI 64M -> PLL1: M=4 N=50 P=2 -> SYSCLK=400MHz
//   D1CPRE=/1 -> CPU    =400MHz
//   HPRE =/2  -> HCLK   =200MHz (AXI/AHB)
//   D1PPRE=/2 -> APB3   =100MHz
//   D2PPRE1=/2-> APB1   =100MHz  (I2C1-3 / USART2-8 / SPI2-3)
//   D2PPRE2=/2-> APB2   =100MHz  (SPI1/4-5 / USART1/6 / TIM1/8/15-17)
//   D3PPRE=/2 -> APB4   =100MHz  (I2C4 / SPI6 / LPTIM / BDMA)
//
// 修改 SystemClock_Config() 时必须同步修改这里的宏
// =============================================================================

#define SYSCLK         400000000
#define HCLK           200000000

#define APB1CLK        100000000
#define APB1TIMERCLK   200000000
#define APB2CLK        100000000
#define APB2TIMERCLK   200000000
#define APB3CLK        100000000
#define APB4CLK        100000000
#define D3PCLK         100000000

#define USART16CLK     100000000
#define USART234578CLK 100000000

// 初始化时钟树 + HAL 库 + HAL 时间基准 (TIM7)
// 必须在 board_init_hardware / osKernelStart 之前调用
void init_TimeBase(void);

#ifdef __cplusplus
}
#endif
