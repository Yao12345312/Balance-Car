#include "drv_uart.hpp"
#include "drv_common.hpp"

#include "cmsis_os2.h"
#include "board.hpp"

#include "stm32h743xx.h"
#include <string.h>

static const osMutexAttr_t s_recursiveMutexAttr = {.attr_bits = osMutexRecursive};

//静态实例
static UART_CLASS uart_instances[HW_DRV_MAX_UART];
static bool uart_inited[HW_DRV_MAX_UART] = {};

//UART外设指针表
static USART_TypeDef *const usartTable[HW_DRV_MAX_UART] = {0, USART1, USART2, USART3, UART4, UART5, USART6, UART7, UART8};

//UART中断表
static const IRQn_Type usartIrqn[HW_DRV_MAX_UART] = {
    (IRQn_Type)0, USART1_IRQn, USART2_IRQn, USART3_IRQn, UART4_IRQn, UART5_IRQn, USART6_IRQn, UART7_IRQn, UART8_IRQn,
};

//RCC时钟使能函数
static void uart_clk_enable(uint8_t num)
{
    switch (num)
    {
        case 1: __HAL_RCC_USART1_CLK_ENABLE(); break;
        case 2: __HAL_RCC_USART2_CLK_ENABLE(); break;
        case 3: __HAL_RCC_USART3_CLK_ENABLE(); break;
        case 4: __HAL_RCC_UART4_CLK_ENABLE(); break;
        case 5: __HAL_RCC_UART5_CLK_ENABLE(); break;
        case 6: __HAL_RCC_USART6_CLK_ENABLE(); break;
        case 7: __HAL_RCC_UART7_CLK_ENABLE(); break;
        case 8: __HAL_RCC_UART8_CLK_ENABLE(); break;
        default: break;
    }
}

static uint32_t uart_dma_request(uint8_t num, bool tx)
{
    switch (num)
    {
        case 1: return tx ? DMA_REQUEST_USART1_TX : DMA_REQUEST_USART1_RX;
        case 2: return tx ? DMA_REQUEST_USART2_TX : DMA_REQUEST_USART2_RX;
        case 3: return tx ? DMA_REQUEST_USART3_TX : DMA_REQUEST_USART3_RX;
        case 4: return tx ? DMA_REQUEST_UART4_TX : DMA_REQUEST_UART4_RX;
        case 5: return tx ? DMA_REQUEST_UART5_TX : DMA_REQUEST_UART5_RX;
        case 6: return tx ? DMA_REQUEST_USART6_TX : DMA_REQUEST_USART6_RX;
        case 7: return tx ? DMA_REQUEST_UART7_TX : DMA_REQUEST_UART7_RX;
        case 8: return tx ? DMA_REQUEST_UART8_TX : DMA_REQUEST_UART8_RX;
        default: return 0xFFFFFFFFu;
    }
}

template <int N> void uartDrvTxCpltCb(UART_HandleTypeDef *) { uart_instances[N].onTxCplt(); }
template <int N> void uartDrvRxCpltCb(UART_HandleTypeDef *) { uart_instances[N].onRxCplt(); }
template <int N> void uartDrvErrorCb(UART_HandleTypeDef *) { uart_instances[N].onError(); }
template <int N> void uartDrvAbortCb(UART_HandleTypeDef *) {}
template <int N> void uartDrvRxEventCb(UART_HandleTypeDef *, uint16_t pos) { uart_instances[N].onRxEvent(pos); }
template <int N> void uartDrvIrq()
{
    if (!uart_inited[N])
        return;
    UART_CLASS *u = &uart_instances[N];
    UART_HandleTypeDef *h = u->handle();

    // RX DMA 模式下自行处理 IDLE (HAL_UART_Receive_DMA 不接管 IDLE)。
    // 必须在 HAL_UART_IRQHandler 之前检测并清除 IDLE 标志, 否则 HAL 会把它当作
    // 一次接收完成事件并 abort DMA —— DMA 一旦被 abort, 就会出现重新武装的窗口,
    // 该窗口内到达的字节会触发 ORE 被丢弃, 导致长帧 (如 COMMAND_LONG) CRC 失败。
    if (u->useRxDma && __HAL_UART_GET_FLAG(h, UART_FLAG_IDLE))
    {
        __HAL_UART_CLEAR_IDLEFLAG(h);
        uint16_t remain = __HAL_DMA_GET_COUNTER(h->hdmarx);
        uint16_t pos = (uint16_t)(UART_DRV_RX_DMA_BUF - remain);
        u->onRxEvent(pos);
    }

    HAL_UART_IRQHandler(h);
}

void UART_CLASS::feedRx(const uint8_t *data, uint16_t len)
{
    if (len == 0)
        return;
    for (uint16_t i = 0; i < len; i++)
    {
        uint16_t next = (uint16_t)((rxHead + 1u) % UART_DRV_RX_RING);
        if (next == rxTail)
            break;
        rxRing[rxHead] = data[i];
        rxHead = next;
        osSemaphoreRelease(rxSem);
    }
}

// 仅拷贝字节到环形缓冲, 不释放信号量 —— 供 DMA 回调使用
uint16_t UART_CLASS::copyToRing(const uint8_t *data, uint16_t len)
{
    uint16_t fed = 0;
    for (uint16_t i = 0; i < len; i++)
    {
        uint16_t next = (uint16_t)((rxHead + 1u) % UART_DRV_RX_RING);
        if (next == rxTail)
            break;
        rxRing[rxHead] = data[i];
        rxHead = next;
        fed++;
    }
    return fed;
}

void UART_CLASS::onTxCplt()
{
    osSemaphoreRelease(txCpltSem);
}

void UART_CLASS::onRxEvent(uint16_t pos)
{
    if (!useRxDma)
        return;
    SCB_InvalidateDCache_by_Addr((uint32_t *)rxDmaBuf, UART_DRV_RX_DMA_BUF);

    // 不重启 DMA —— 重启会使窗口期内到达的字节触发 ORE 被丢弃。
    uint16_t last = rxLastConsumed;
    uint16_t fed = 0;
    if (pos >= last)
    {
        // 未回绕: 拷贝 [last, pos)
        fed = copyToRing(&rxDmaBuf[last], (uint16_t)(pos - last));
    }
    else
    {
        // DMA 已回绕: 先拷贝尾部 [last, SIZE), 再拷贝头部 [0, pos)
        uint16_t tail = (uint16_t)(UART_DRV_RX_DMA_BUF - last);
        fed = copyToRing(&rxDmaBuf[last], tail);
        fed += copyToRing(&rxDmaBuf[0], pos);
    }
    rxLastConsumed = pos;

    for (uint16_t i = 0; i < fed; i++)
        osSemaphoreRelease(rxSem);
}

void UART_CLASS::onRxCplt()
{
    if (useRxDma)
    {
        // CIRCULAR 模式: DMA 缓冲满后硬件自动回绕继续接收, HAL 只回调不停传输。
        // 拷贝自上次消费以来的尾部 [last, SIZE), 然后把消费指针归零 (DMA 已回到头部)。
        SCB_InvalidateDCache_by_Addr((uint32_t *)rxDmaBuf, UART_DRV_RX_DMA_BUF);
        uint16_t last = rxLastConsumed;
        uint16_t fed = 0;
        if (UART_DRV_RX_DMA_BUF > last)
            fed = copyToRing(&rxDmaBuf[last], (uint16_t)(UART_DRV_RX_DMA_BUF - last));
        rxLastConsumed = 0;

        for (uint16_t i = 0; i < fed; i++)
            osSemaphoreRelease(rxSem);
    }
    else
    {
        feedRx(&rxItByte, 1);
        HAL_UART_Receive_IT(&huart, &rxItByte, 1);
    }
}

void UART_CLASS::onError()
{
    if (useRxDma)
    {
        // ORE/FE/NE 等错误发生前, DMA 可能已搬运了正在接收帧的中间字节。
        // 必须先取出 (含回绕) 再重新武装接收, 否则该帧丢中间字节 -> CRC 失败。
        // 错误属于异常路径, 此处必须重启 DMA 以恢复接收 (非 IDLE 正常路径)。
        SCB_InvalidateDCache_by_Addr((uint32_t *)rxDmaBuf, UART_DRV_RX_DMA_BUF);
        uint16_t last = rxLastConsumed;
        uint16_t remaining = __HAL_DMA_GET_COUNTER(&hdmaRx);
        uint16_t pos = (uint16_t)(UART_DRV_RX_DMA_BUF - remaining);
        uint16_t fed = 0;
        if (pos >= last)
        {
            fed = copyToRing(&rxDmaBuf[last], (uint16_t)(pos - last));
        }
        else
        {
            uint16_t tail = (uint16_t)(UART_DRV_RX_DMA_BUF - last);
            fed = copyToRing(&rxDmaBuf[last], tail);
            fed += copyToRing(&rxDmaBuf[0], pos);
        }

        HAL_UART_DMAStop(&huart);
        rxLastConsumed = 0;
        HAL_UART_Receive_DMA(&huart, rxDmaBuf, UART_DRV_RX_DMA_BUF);
        __HAL_UART_ENABLE_IT(&huart, UART_IT_IDLE);

        for (uint16_t i = 0; i < fed; i++)
            osSemaphoreRelease(rxSem);
    }
    else
    {
        HAL_UART_Receive_IT(&huart, &rxItByte, 1);
    }
}

void UART_CLASS::startRx()
{
    if (useRxDma)
    {
        // 用 HAL_UART_Receive_DMA + CIRCULAR, DMA 持续运行、永不解除武装。
        // IDLE 由 uartDrvIrq 自行检测并搬运数据 —— 不用 HAL_UARTEx_ReceiveToIdle_DMA,
        // 因为后者会在每次 IDLE 时 abort DMA, 产生丢字节的重新武装窗口。
        rxLastConsumed = 0;
        HAL_UART_Receive_DMA(&huart, rxDmaBuf, UART_DRV_RX_DMA_BUF);
        __HAL_UART_ENABLE_IT(&huart, UART_IT_IDLE);
    }
    else
    {
        HAL_UART_Receive_IT(&huart, &rxItByte, 1);
    }
}

void UART_CLASS::registerCallbacks()
{
#define UART_SETUP_CB(N)                                                                                                                                                                                \
    do {                                                                                                                                                                                                \
        HAL_UART_RegisterCallback(&huart, HAL_UART_TX_COMPLETE_CB_ID, uartDrvTxCpltCb<N>);                                                                                                              \
        HAL_UART_RegisterCallback(&huart, HAL_UART_RX_COMPLETE_CB_ID, uartDrvRxCpltCb<N>);                                                                                                              \
        HAL_UART_RegisterCallback(&huart, HAL_UART_ERROR_CB_ID, uartDrvErrorCb<N>);                                                                                                                     \
        HAL_UART_RegisterCallback(&huart, HAL_UART_ABORT_COMPLETE_CB_ID, uartDrvAbortCb<N>);                                                                                                            \
        HAL_UART_RegisterRxEventCallback(&huart, uartDrvRxEventCb<N>);                                                                                                                                  \
    } while (0)

    switch (uartNum)
    {
        case 1: UART_SETUP_CB(1); break;
        case 2: UART_SETUP_CB(2); break;
        case 3: UART_SETUP_CB(3); break;
        case 4: UART_SETUP_CB(4); break;
        case 5: UART_SETUP_CB(5); break;
        case 6: UART_SETUP_CB(6); break;
        case 7: UART_SETUP_CB(7); break;
        case 8: UART_SETUP_CB(8); break;
        default: break;
    }
#undef UART_SETUP_CB
}

bool UART_CLASS::init(uint8_t num, const UART_Config *cfg)
{
    if (!HW_UART_INDEX_VALID(num) || !cfg)
        return false;
    uartNum = num;
    useTxDma = false;
    useRxDma = false;
    rxLastConsumed = 0;
    rxItByte = 0;

    HW_ConfigurePinAF(&cfg->tx, GPIO_MODE_AF_PP, cfg->pull ? cfg->pull : GPIO_PULLUP, cfg->speed ? cfg->speed : GPIO_SPEED_FREQ_VERY_HIGH);
    HW_ConfigurePinAF(&cfg->rx, GPIO_MODE_AF_PP, cfg->pull ? cfg->pull : GPIO_PULLUP, cfg->speed ? cfg->speed : GPIO_SPEED_FREQ_VERY_HIGH);

    uart_clk_enable(num);
    HW_Delay(0.001);

    huart.Instance = usartTable[num];
    huart.Init.BaudRate = cfg->baudrate ? cfg->baudrate : 115200;
    huart.Init.WordLength = cfg->word_length ? cfg->word_length : UART_WORDLENGTH_8B;
    huart.Init.StopBits = cfg->stop_bits ? cfg->stop_bits : UART_STOPBITS_1;
    huart.Init.Parity = cfg->parity ? cfg->parity : UART_PARITY_NONE;
    huart.Init.Mode = UART_MODE_TX_RX;
    huart.Init.HwFlowCtl = cfg->hw_flow_ctl ? cfg->hw_flow_ctl : UART_HWCONTROL_NONE;
    huart.Init.OverSampling = UART_OVERSAMPLING_16;
    huart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    if (HAL_UART_Init(&huart) != HAL_OK)
        return false;
    if (cfg->swap)
    {
        __HAL_UART_DISABLE(&huart);
        MODIFY_REG(huart.Instance->CR2, USART_CR2_SWAP, USART_CR2_SWAP);
        __HAL_UART_ENABLE(&huart);
    }

    uint32_t txReq = uart_dma_request(num, true);
    if (cfg->tx_dma.dma_num >= 1 && cfg->tx_dma.dma_num <= 2 && cfg->tx_dma.stream <= 7 && txReq != 0xFFFFFFFFu)
    {
        hdmaTx.Instance = HW_GetDmaStream(cfg->tx_dma.dma_num, cfg->tx_dma.stream);
        hdmaTx.Init.Request = txReq;
        hdmaTx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdmaTx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdmaTx.Init.MemInc = DMA_MINC_ENABLE;
        hdmaTx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdmaTx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdmaTx.Init.Mode = DMA_NORMAL;
        hdmaTx.Init.Priority = DMA_PRIORITY_MEDIUM;
        hdmaTx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdmaTx) != HAL_OK)
            return false;
        HW_RegisterDmaHandle(cfg->tx_dma.dma_num, cfg->tx_dma.stream, &hdmaTx);
        HAL_NVIC_SetPriority(HW_GetDmaIrqn(cfg->tx_dma.dma_num, cfg->tx_dma.stream), cfg->irq_priority, 0);
        HAL_NVIC_EnableIRQ(HW_GetDmaIrqn(cfg->tx_dma.dma_num, cfg->tx_dma.stream));
        __HAL_LINKDMA(&huart, hdmatx, hdmaTx);
        useTxDma = true;
    }

    uint32_t rxReq = uart_dma_request(num, false);
    if (cfg->rx_dma.dma_num >= 1 && cfg->rx_dma.dma_num <= 2 && cfg->rx_dma.stream <= 7 && rxReq != 0xFFFFFFFFu)
    {
        hdmaRx.Instance = HW_GetDmaStream(cfg->rx_dma.dma_num, cfg->rx_dma.stream);
        hdmaRx.Init.Request = rxReq;
        hdmaRx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdmaRx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdmaRx.Init.MemInc = DMA_MINC_ENABLE;
        hdmaRx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdmaRx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdmaRx.Init.Mode = DMA_CIRCULAR;
        hdmaRx.Init.Priority = DMA_PRIORITY_MEDIUM;
        hdmaRx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdmaRx) != HAL_OK)
            return false;
        HW_RegisterDmaHandle(cfg->rx_dma.dma_num, cfg->rx_dma.stream, &hdmaRx);
        HAL_NVIC_SetPriority(HW_GetDmaIrqn(cfg->rx_dma.dma_num, cfg->rx_dma.stream), cfg->irq_priority, 0);
        HAL_NVIC_EnableIRQ(HW_GetDmaIrqn(cfg->rx_dma.dma_num, cfg->rx_dma.stream));
        __HAL_LINKDMA(&huart, hdmarx, hdmaRx);
        useRxDma = true;
    }
	
    txMutex = osMutexNew(&s_recursiveMutexAttr);
    rxMutex = osMutexNew(NULL);
    txCpltSem = osSemaphoreNew(1, 0, NULL);
    rxSem = osSemaphoreNew(UART_DRV_RX_RING, 0, NULL);
    rxHead = 0;
    rxTail = 0;

    registerCallbacks();

    HAL_NVIC_SetPriority(usartIrqn[num], cfg->irq_priority, 0);

    startRx();

    uart_inited[num] = true;

    HAL_NVIC_EnableIRQ(usartIrqn[num]);

    return true;
}

void UART_CLASS::deinit()
{
    HAL_NVIC_DisableIRQ(usartIrqn[uartNum]);
    HAL_UART_DeInit(&huart);
    if (useTxDma)
        HAL_DMA_DeInit(&hdmaTx);
    if (useRxDma)
        HAL_DMA_DeInit(&hdmaRx);
}

bool UART_CLASS::lock(double sync_wait)
{
    uint32_t ticks = (sync_wait < 0) ? osWaitForever : (uint32_t)(sync_wait * (double)osKernelGetTickFreq());
    return osMutexAcquire(txMutex, ticks) == osOK;
}

void UART_CLASS::unlock()
{
    osMutexRelease(txMutex);
}

bool UART_CLASS::resetRx(double sync_wait)
{
    uint32_t ticks = (sync_wait < 0) ? osWaitForever : (uint32_t)(sync_wait * (double)osKernelGetTickFreq());
    if (osMutexAcquire(rxMutex, ticks) != osOK)
        return false;
    while (rxTail != rxHead)
    {
        if (osSemaphoreAcquire(rxSem, 0) != osOK)
            break;
        rxTail = (uint16_t)((rxTail + 1u) % UART_DRV_RX_RING);
    }
    osMutexRelease(rxMutex);
    return true;
}

bool UART_CLASS::waitTxSent(double wait)
{
    uint32_t timeout = (wait < 0) ? osWaitForever : (uint32_t)(wait * (double)osKernelGetTickFreq());
    uint32_t start = osKernelGetTickCount();
    while (huart.gState != HAL_UART_STATE_READY)
    {
        if (timeout != osWaitForever && (osKernelGetTickCount() - start) >= timeout)
            return false;
        if (osKernelGetState() != osKernelRunning)
            return false;
        osDelay(1);
    }
    return true;
}

bool UART_CLASS::setBaudRate(uint32_t baudrate, double sync_wait)
{
    if (!lock(sync_wait))
        return false;

    //停止所有 DMA 传输 (TX + RX), 让 hdmatx/hdmarx->State 回到 READY
    HAL_UART_DMAStop(&huart);

    //等待最后一位数据发完
    uint32_t tickstart = HAL_GetTick();
    while (__HAL_UART_GET_FLAG(&huart, UART_FLAG_TC) == RESET)
    {
        if ((HAL_GetTick() - tickstart) > 100)
            break;
    }

    //DeInit UART 外设 (清 CR1/CR2/CR3, gState->RESET, RxState->RESET)
    HAL_UART_DeInit(&huart);

    //设置新波特率
    huart.Init.BaudRate = baudrate;

    //重新初始化 UART
    HAL_StatusTypeDef st = HAL_UART_Init(&huart);

    //重新注册 + 重启接收
    if (st == HAL_OK)
    {
        //丢弃波特率切换前的残留字节
        //复位环形缓冲与计数信号量, 避免 head/tail/sem 不同步或脏数据
        //污染后续 MAVLink 解析
        rxHead = 0;
        rxTail = 0;
        rxLastConsumed = 0;
        while (osSemaphoreAcquire(rxSem, 0) == osOK) {}

        registerCallbacks();
        startRx();
    }

    unlock();
    return st == HAL_OK;
}

UART_DrvResult UART_CLASS::write(const uint8_t *data, uint16_t size, double send_wait, double sync_wait)
{
    if (size == 0 || data == 0)
        return UART_Drv_BadParam;

    uint32_t sync_ticks = (sync_wait < 0) ? osWaitForever : (uint32_t)(sync_wait * (double)osKernelGetTickFreq());
    uint32_t send_ticks = (send_wait < 0) ? osWaitForever : (uint32_t)(send_wait * (double)osKernelGetTickFreq());

    if (osMutexAcquire(txMutex, sync_ticks) != osOK)
        return UART_Drv_Timeout;

    UART_DrvResult r = UART_Drv_Ok;
    if (useTxDma && size <= UART_DRV_TX_DMA_BUF)
    {
        memcpy(txDmaBuf, data, size);
        uint32_t clean = (uint32_t)((size + 31u) & ~31u);
        if (clean == 0) clean = 32;
        SCB_CleanDCache_by_Addr((uint32_t *)txDmaBuf, clean);

        osSemaphoreAcquire(txCpltSem, 0);
        HAL_StatusTypeDef st = HAL_UART_Transmit_DMA(&huart, txDmaBuf, size);
        if (st != HAL_OK)
        {
            r = (st == HAL_TIMEOUT) ? UART_Drv_Timeout : UART_Drv_Error;
        }
        else if (osSemaphoreAcquire(txCpltSem, send_ticks) != osOK)
        {
            r = UART_Drv_Timeout;
        }
    }
    else
    {
        uint32_t ms = (send_wait < 0) ? HAL_MAX_DELAY : (uint32_t)(send_wait * 1000);
        HAL_StatusTypeDef st = HAL_UART_Transmit(&huart, (uint8_t *)data, size, ms);
        if (st != HAL_OK)
            r = (st == HAL_TIMEOUT) ? UART_Drv_Timeout : UART_Drv_Error;
    }

    unlock();
    return r;
}

uint16_t UART_CLASS::read(uint8_t *data, uint16_t size, double rc_wait, double sync_wait)
{
    if (size == 0 || data == 0)
        return 0;

    uint32_t sync_ticks = (sync_wait < 0) ? osWaitForever : (uint32_t)(sync_wait * (double)osKernelGetTickFreq());
    uint32_t rc_ticks = (rc_wait < 0) ? osWaitForever : (uint32_t)(rc_wait * (double)osKernelGetTickFreq());

    if (osMutexAcquire(rxMutex, sync_ticks) != osOK)
        return 0;

    uint16_t got = 0;
    while (got < size)
    {
        uint32_t t = (got == 0) ? rc_ticks : 0;
        if (osSemaphoreAcquire(rxSem, t) != osOK)
            break;
        data[got++] = rxRing[rxTail];
        rxTail = (uint16_t)((rxTail + 1u) % UART_DRV_RX_RING);
    }

    osMutexRelease(rxMutex);
    return got;
}

extern "C" void USART1_IRQHandler(void) { uartDrvIrq<1>(); }
extern "C" void USART2_IRQHandler(void) { uartDrvIrq<2>(); }
extern "C" void USART3_IRQHandler(void) { uartDrvIrq<3>(); }
extern "C" void UART4_IRQHandler(void) { uartDrvIrq<4>(); }
extern "C" void UART5_IRQHandler(void) { uartDrvIrq<5>(); }
extern "C" void USART6_IRQHandler(void) { uartDrvIrq<6>(); }
extern "C" void UART7_IRQHandler(void) { uartDrvIrq<7>(); }
extern "C" void UART8_IRQHandler(void) { uartDrvIrq<8>(); }

bool register_uart(uint8_t uart_num, const UART_Config *cfg)
{
    if (!HW_UART_INDEX_VALID(uart_num) || !cfg)
        return false;
    if (uart_inited[uart_num])
        return false;
    if (!uart_instances[uart_num].init(uart_num, cfg))
        return false;
    uart_inited[uart_num] = true;
    return true;
}

UART_CLASS *get_uart_instance(uint8_t uart_num)
{
    if (HW_UART_INDEX_VALID(uart_num) && uart_inited[uart_num])
        return &uart_instances[uart_num];
    return 0;
}

void init_drv_uart(void)
{
    for (uint8_t i = 0; i < BOARD_UART_COUNT; ++i)
    {
        if (board_uart_ports[i].hw_num == 0 || board_uart_ports[i].config == 0)
            continue;
        register_uart(board_uart_ports[i].hw_num, board_uart_ports[i].config);
    }
}
