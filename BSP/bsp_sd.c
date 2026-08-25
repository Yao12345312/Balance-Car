/*
 * bsp_sd.c - SDMMC1 low-level for the application firmware.
 *
 * Provides the hsd1 handle that bsp_driver_sd.c references, the SDMMC1 MSP
 * (GPIO/clock) setup lifted from the bootloader, and a field-initialiser that
 * must run before BSP_SD_Init().
 *
 * Note: HAL_SD_MspInit() is invoked internally by HAL_SD_Init(), so it MUST
 * stay a C-linkage symbol (this file is compiled as C).
 */
#include "stm32h7xx_hal.h"

/* The handle consumed by bsp_driver_sd.c (declared there as extern). */
SD_HandleTypeDef hsd1;

/**
 * @brief SDMMC1 low-level init: fills hsd1.Init fields only.
 *        BSP_SD_Init() afterwards performs HAL_SD_Init() + 4-bit bus config.
 *        Call sequence:  MX_SDMMC1_SD_Init();  BSP_SD_Init();
 */
void MX_SDMMC1_SD_Init(void)
{
    hsd1.Instance                  = SDMMC1;
    hsd1.Init.ClockEdge            = SDMMC_CLOCK_EDGE_RISING;
    hsd1.Init.ClockPowerSave       = SDMMC_CLOCK_POWER_SAVE_DISABLE;
    hsd1.Init.BusWide              = SDMMC_BUS_WIDE_4B;
    hsd1.Init.HardwareFlowControl  = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
    hsd1.Init.ClockDiv             = 24;   
}

/**
 * @brief SD MSP Init - GPIO + clocks, ported verbatim from the bootloader
 *        (Core/Src/stm32h7xx_hal_msp.c). Same board, same pins.
 *        PC8..PC12 = SDMMC1_D0..CK, PD2 = CMD, AF12.
 */
void HAL_SD_MspInit(SD_HandleTypeDef *hsd)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    if (hsd->Instance == SDMMC1)
    {
        /* SDMMC clock source = PLL1Q (must be enabled in the system clock setup) */
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SDMMC;
        PeriphClkInitStruct.SdmmcClockSelection  = RCC_SDMMCCLKSOURCE_PLL;
        (void)HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

        __HAL_RCC_SDMMC1_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();

        /* PC8 PC9 PC10 PC11 PC12 -> SDMMC1_D0 D1 D2 D3 CK */
        GPIO_InitStruct.Pin       = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF12_SDIO1;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        /* PD2 -> SDMMC1_CMD */
        GPIO_InitStruct.Pin = GPIO_PIN_2;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

        /* Polling-mode SD access (storage_if uses BSP_SD_ReadBlocks/WriteBlocks),
         * so the SDMMC1 IRQ is intentionally NOT enabled here. */
    }
}

void HAL_SD_MspDeInit(SD_HandleTypeDef *hsd)
{
    if (hsd->Instance == SDMMC1)
    {
        __HAL_RCC_SDMMC1_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
        HAL_GPIO_DeInit(GPIOD, GPIO_PIN_2);
    }
}
