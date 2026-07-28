#pragma once

#include "stm32h7xx_hal.h"
#include <stdint.h>

#define HW_DRV_MAX_SPI  7
#define HW_DRV_MAX_I2C  5
#define HW_DRV_MAX_UART 9
#define HW_DRV_MAX_CAN  3
#define HW_DRV_MAX_PWM  13

#define HW_SPI_INDEX_VALID(n)  ((n) >= 1 && (n) < HW_DRV_MAX_SPI)
#define HW_I2C_INDEX_VALID(n)  ((n) >= 1 && (n) < HW_DRV_MAX_I2C)
#define HW_UART_INDEX_VALID(n) ((n) >= 1 && (n) < HW_DRV_MAX_UART)
#define HW_CAN_INDEX_VALID(n)  ((n) >= 1 && (n) < HW_DRV_MAX_CAN)
#define HW_PWM_INDEX_VALID(n)  ((n) >= 1 && (n) < HW_DRV_MAX_PWM)

typedef enum
{
    HW_Result_Ok = 0,
    HW_Result_BadIndex,
    HW_Result_BadParam,
    HW_Result_Busy,
    HW_Result_Timeout,
    HW_Result_Error,
} HW_Result;

typedef enum
{
    HW_OTYPE_PUSH_PULL = 0,
    HW_OTYPE_OPEN_DRAIN = 1,
} HW_Otype;

typedef enum
{
    HW_PULL_NONE = 0,
    HW_PULL_UP = 1,
    HW_PULL_DOWN = 2,
} HW_Pull;

typedef enum
{
    HW_SPEED_LOW = 0,
    HW_SPEED_MEDIUM = 1,
    HW_SPEED_HIGH = 2,
    HW_SPEED_VERY_HIGH = 3,
} HW_Speed;

typedef struct
{
    uint8_t port;
    uint8_t pin;
    uint8_t af;
} HwPin;

#define HW_PIN(port, pin, af) ((HwPin){(port), (pin), (af)})

typedef struct
{
    uint8_t dma_num;
    uint8_t stream;
} HwDmaCh;

#define HW_DMA_NONE ((HwDmaCh){0, 0})
#define HW_DMA(dma_num, stream) ((HwDmaCh){(dma_num), (stream)})

extern GPIO_TypeDef *const HW_GpioTable[11];

void HW_GpioClockEnable(uint8_t port);
void HW_ConfigurePinAF(const HwPin *pin, uint32_t mode, uint32_t pull, uint32_t speed);
void HW_ConfigurePinOutput(const HwPin *pin, uint32_t otype, uint32_t pull, uint32_t speed);

void HW_InitCommon(void);

bool HW_RegisterDmaHandle(uint8_t dma_num, uint8_t stream, DMA_HandleTypeDef *hdma);
bool HW_UnregisterDmaHandle(uint8_t dma_num, uint8_t stream);

DMA_Stream_TypeDef *HW_GetDmaStream(uint8_t dma_num, uint8_t stream);
IRQn_Type HW_GetDmaIrqn(uint8_t dma_num, uint8_t stream);

void HW_Delay(double seconds);
