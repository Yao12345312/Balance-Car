#include "drv_common.hpp"
#include "cmsis_os2.h"

GPIO_TypeDef *const HW_GpioTable[11] = {
    GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH, GPIOI, GPIOJ, GPIOK,
};

void HW_GpioClockEnable(uint8_t port)
{
    switch (port)
    {
        case 0: __HAL_RCC_GPIOA_CLK_ENABLE(); break;
        case 1: __HAL_RCC_GPIOB_CLK_ENABLE(); break;
        case 2: __HAL_RCC_GPIOC_CLK_ENABLE(); break;
        case 3: __HAL_RCC_GPIOD_CLK_ENABLE(); break;
        case 4: __HAL_RCC_GPIOE_CLK_ENABLE(); break;
        case 5: __HAL_RCC_GPIOF_CLK_ENABLE(); break;
        case 6: __HAL_RCC_GPIOG_CLK_ENABLE(); break;
        case 7: __HAL_RCC_GPIOH_CLK_ENABLE(); break;
        case 8: __HAL_RCC_GPIOI_CLK_ENABLE(); break;
        case 9: __HAL_RCC_GPIOJ_CLK_ENABLE(); break;
        case 10: __HAL_RCC_GPIOK_CLK_ENABLE(); break;
        default: break;
    }
}

void HW_ConfigurePinAF(const HwPin *pin, uint32_t mode, uint32_t pull, uint32_t speed)
{
    if (!pin || pin->port > 10)
        return;

    HW_GpioClockEnable(pin->port);

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = (uint16_t)(1u << pin->pin);
    gpio.Mode = mode;
    gpio.Pull = pull;
    gpio.Speed = speed;
    gpio.Alternate = pin->af;
    HAL_GPIO_Init(HW_GpioTable[pin->port], &gpio);
}

void HW_ConfigurePinOutput(const HwPin *pin, uint32_t otype, uint32_t pull, uint32_t speed)
{
    if (!pin || pin->port > 10)
        return;

    HW_GpioClockEnable(pin->port);

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = (uint16_t)(1u << pin->pin);
    gpio.Mode = (otype == HW_OTYPE_OPEN_DRAIN) ? GPIO_MODE_OUTPUT_OD : GPIO_MODE_OUTPUT_PP;
    gpio.Pull = pull;
    gpio.Speed = speed;
    HAL_GPIO_Init(HW_GpioTable[pin->port], &gpio);
}

static DMA_HandleTypeDef *s_dmaHandles[16];

static uint8_t s_dmaIndex(uint8_t dma_num, uint8_t stream)
{
    return (uint8_t)((dma_num - 1) * 8 + stream);
}

bool HW_RegisterDmaHandle(uint8_t dma_num, uint8_t stream, DMA_HandleTypeDef *hdma)
{
    if (dma_num < 1 || dma_num > 2 || stream > 7)
        return false;
    uint8_t idx = s_dmaIndex(dma_num, stream);
    if (s_dmaHandles[idx] != 0 && s_dmaHandles[idx] != hdma)
        return false;
    s_dmaHandles[idx] = hdma;
    return true;
}

bool HW_UnregisterDmaHandle(uint8_t dma_num, uint8_t stream)
{
    if (dma_num < 1 || dma_num > 2 || stream > 7)
        return false;
    s_dmaHandles[s_dmaIndex(dma_num, stream)] = 0;
    return true;
}

#define HW_DMA_IRQ_HANDLER(dma_n, stream)                                                                                                                                                               \
    extern "C" void DMA##dma_n##_Stream##stream##_IRQHandler(void)                                                                                                                                      \
    {                                                                                                                                                                                                   \
        DMA_HandleTypeDef *hdma = s_dmaHandles[s_dmaIndex(dma_n, stream)];                                                                                                                              \
        if (hdma)                                                                                                                                                                                       \
            HAL_DMA_IRQHandler(hdma);                                                                                                                                                                   \
    }

HW_DMA_IRQ_HANDLER(1, 0)
HW_DMA_IRQ_HANDLER(1, 1)
HW_DMA_IRQ_HANDLER(1, 2)
HW_DMA_IRQ_HANDLER(1, 3)
HW_DMA_IRQ_HANDLER(1, 4)
HW_DMA_IRQ_HANDLER(1, 5)
HW_DMA_IRQ_HANDLER(1, 6)
HW_DMA_IRQ_HANDLER(1, 7)
HW_DMA_IRQ_HANDLER(2, 0)
HW_DMA_IRQ_HANDLER(2, 1)
HW_DMA_IRQ_HANDLER(2, 2)
HW_DMA_IRQ_HANDLER(2, 3)
HW_DMA_IRQ_HANDLER(2, 4)
HW_DMA_IRQ_HANDLER(2, 5)
HW_DMA_IRQ_HANDLER(2, 6)
HW_DMA_IRQ_HANDLER(2, 7)

void HW_InitCommon(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();
}

static DMA_Stream_TypeDef *const s_dma1Streams[8] = {
    DMA1_Stream0, DMA1_Stream1, DMA1_Stream2, DMA1_Stream3,
    DMA1_Stream4, DMA1_Stream5, DMA1_Stream6, DMA1_Stream7,
};
static DMA_Stream_TypeDef *const s_dma2Streams[8] = {
    DMA2_Stream0, DMA2_Stream1, DMA2_Stream2, DMA2_Stream3,
    DMA2_Stream4, DMA2_Stream5, DMA2_Stream6, DMA2_Stream7,
};
static const IRQn_Type s_dmaIrqn[2][8] = {
    {DMA1_Stream0_IRQn, DMA1_Stream1_IRQn, DMA1_Stream2_IRQn, DMA1_Stream3_IRQn,
     DMA1_Stream4_IRQn, DMA1_Stream5_IRQn, DMA1_Stream6_IRQn, DMA1_Stream7_IRQn},
    {DMA2_Stream0_IRQn, DMA2_Stream1_IRQn, DMA2_Stream2_IRQn, DMA2_Stream3_IRQn,
     DMA2_Stream4_IRQn, DMA2_Stream5_IRQn, DMA2_Stream6_IRQn, DMA2_Stream7_IRQn},
};

DMA_Stream_TypeDef *HW_GetDmaStream(uint8_t dma_num, uint8_t stream)
{
    if (dma_num < 1 || dma_num > 2 || stream > 7)
        return 0;
    return (dma_num == 1) ? s_dma1Streams[stream] : s_dma2Streams[stream];
}

IRQn_Type HW_GetDmaIrqn(uint8_t dma_num, uint8_t stream)
{
    if (dma_num < 1 || dma_num > 2 || stream > 7)
        return (IRQn_Type)0;
    return s_dmaIrqn[dma_num - 1][stream];
}

void HW_Delay(double seconds)
{
    if (seconds <= 0)
        return;
    if (osKernelGetState() == osKernelRunning)
    {
        uint32_t ticks = (uint32_t)(seconds * (double)osKernelGetTickFreq());
        if (ticks == 0)
            ticks = 1;
        osDelay(ticks);
    }
    else
    {
        uint32_t ms = (uint32_t)(seconds * 1000.0 + 0.5);
        if (ms == 0)
            ms = 1;
        HAL_Delay(ms);
    }
}
