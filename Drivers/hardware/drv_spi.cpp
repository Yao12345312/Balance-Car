#include "drv_spi.hpp"
#include "drv_common.hpp"

#include "cmsis_os2.h"
#include "board.hpp"

static const osMutexAttr_t s_recursiveMutexAttr = {.attr_bits = osMutexRecursive};

#include "stm32h743xx.h"
#include <string.h>

static SPI_CLASS spi_instances[HW_DRV_MAX_SPI];
static bool spi_inited[HW_DRV_MAX_SPI] = {};

static SPI_TypeDef *const spiTable[HW_DRV_MAX_SPI] = {0, SPI1, SPI2, SPI3, SPI4, SPI5, SPI6};

static void spi_clk_enable(uint8_t num)
{
    switch (num)
    {
        case 1: __HAL_RCC_SPI1_CLK_ENABLE(); break;
        case 2: __HAL_RCC_SPI2_CLK_ENABLE(); break;
        case 3: __HAL_RCC_SPI3_CLK_ENABLE(); break;
        case 4: __HAL_RCC_SPI4_CLK_ENABLE(); break;
        case 5: __HAL_RCC_SPI5_CLK_ENABLE(); break;
        case 6: __HAL_RCC_SPI6_CLK_ENABLE(); break;
        default: break;
    }
}

static uint32_t spi_dma_request(uint8_t num, bool tx)
{
    switch (num)
    {
        case 1: return tx ? DMA_REQUEST_SPI1_TX : DMA_REQUEST_SPI1_RX;
        case 2: return tx ? DMA_REQUEST_SPI2_TX : DMA_REQUEST_SPI2_RX;
        case 3: return tx ? DMA_REQUEST_SPI3_TX : DMA_REQUEST_SPI3_RX;
        case 4: return tx ? DMA_REQUEST_SPI4_TX : DMA_REQUEST_SPI4_RX;
        case 5: return tx ? DMA_REQUEST_SPI5_TX : DMA_REQUEST_SPI5_RX;
        default: return 0xFFFFFFFFu;
    }
}

template <int N> void spiDrvTxCpltCb(SPI_HandleTypeDef *) { spi_instances[N].onTransferCplt(); }
template <int N> void spiDrvRxCpltCb(SPI_HandleTypeDef *) { spi_instances[N].onTransferCplt(); }
template <int N> void spiDrvTxRxCpltCb(SPI_HandleTypeDef *) { spi_instances[N].onTransferCplt(); }
template <int N> void spiDrvAbortCpltCb(SPI_HandleTypeDef *) { spi_instances[N].onTransferCplt(); }
template <int N> void spiDrvErrorCb(SPI_HandleTypeDef *) { spi_instances[N].onTransferErr(); }

void SPI_CLASS::notify(SPI_DrvResult r)
{
    lastResult = r;
    osSemaphoreRelease(xferSem);
}

void SPI_CLASS::onTransferCplt() { notify(SPI_Drv_Ok); }
void SPI_CLASS::onTransferErr() { notify(SPI_Drv_Error); }

bool SPI_CLASS::init(uint8_t num, const SPI_Config *cfg)
{
    if (!HW_SPI_INDEX_VALID(num) || !cfg)
        return false;
    if (!cfg->sck.port && !cfg->sck.pin && !cfg->mosi.port)
        return false;

    spiNum = num;
    useDma = false;
    lastResult = SPI_Drv_Ok;

    spi_clk_enable(num);
    HW_Delay(0.001);

    HW_ConfigurePinAF(&cfg->sck, GPIO_MODE_AF_PP, cfg->pull, cfg->speed);
    //MISO 仅在配置了有效引脚时才初始化
    if (cfg->miso.port || cfg->miso.pin || cfg->miso.af)
        HW_ConfigurePinAF(&cfg->miso, GPIO_MODE_AF_PP, cfg->pull, cfg->speed);
    HW_ConfigurePinAF(&cfg->mosi, GPIO_MODE_AF_PP, cfg->pull, cfg->speed);

    hspi.Instance = spiTable[num];
    hspi.Init.Mode = SPI_MODE_MASTER;
    hspi.Init.Direction = SPI_DIRECTION_2LINES;
    hspi.Init.DataSize = cfg->data_size ? cfg->data_size : SPI_DATASIZE_8BIT;
    hspi.Init.CLKPolarity = (cfg->mode == SPI_MODE_0 || cfg->mode == SPI_MODE_1) ? SPI_POLARITY_LOW : SPI_POLARITY_HIGH;
    hspi.Init.CLKPhase = (cfg->mode == SPI_MODE_0 || cfg->mode == SPI_MODE_2) ? SPI_PHASE_1EDGE : SPI_PHASE_2EDGE;
    hspi.Init.NSS = cfg->nss ? cfg->nss : SPI_NSS_SOFT;
    hspi.Init.BaudRatePrescaler = cfg->baudrate_prescaler ? cfg->baudrate_prescaler : SPI_BAUDRATEPRESCALER_32;
    hspi.Init.FirstBit = cfg->firstbit ? cfg->firstbit : SPI_FIRSTBIT_MSB;
    hspi.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi.Init.CRCPolynomial = 7;
    hspi.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hspi.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    hspi.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
    hspi.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    hspi.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    hspi.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
    hspi.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
    hspi.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;
    hspi.Init.IOSwap = SPI_IO_SWAP_DISABLE;
    if (HAL_SPI_Init(&hspi) != HAL_OK)
        return false;

    uint32_t txReq = spi_dma_request(num, true);
    if (cfg->tx_dma.dma_num >= 1 && cfg->tx_dma.dma_num <= 2 && cfg->tx_dma.stream <= 7 &&
        cfg->rx_dma.dma_num >= 1 && cfg->rx_dma.dma_num <= 2 && cfg->rx_dma.stream <= 7 &&
        txReq != 0xFFFFFFFFu)
    {
        hdmaTx.Instance = HW_GetDmaStream(cfg->tx_dma.dma_num, cfg->tx_dma.stream);
        hdmaTx.Init.Request = txReq;
        hdmaTx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdmaTx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdmaTx.Init.MemInc = DMA_MINC_ENABLE;
        hdmaTx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdmaTx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdmaTx.Init.Mode = DMA_NORMAL;
        hdmaTx.Init.Priority = DMA_PRIORITY_HIGH;
        hdmaTx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdmaTx) != HAL_OK)
            return false;
        HW_RegisterDmaHandle(cfg->tx_dma.dma_num, cfg->tx_dma.stream, &hdmaTx);
        HAL_NVIC_SetPriority(HW_GetDmaIrqn(cfg->tx_dma.dma_num, cfg->tx_dma.stream), cfg->irq_priority, 0);
        HAL_NVIC_EnableIRQ(HW_GetDmaIrqn(cfg->tx_dma.dma_num, cfg->tx_dma.stream));
        __HAL_LINKDMA(&hspi, hdmatx, hdmaTx);

        hdmaRx.Instance = HW_GetDmaStream(cfg->rx_dma.dma_num, cfg->rx_dma.stream);
        hdmaRx.Init.Request = spi_dma_request(num, false);
        hdmaRx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdmaRx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdmaRx.Init.MemInc = DMA_MINC_ENABLE;
        hdmaRx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdmaRx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdmaRx.Init.Mode = DMA_NORMAL;
        hdmaRx.Init.Priority = DMA_PRIORITY_HIGH;
        hdmaRx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdmaRx) != HAL_OK)
            return false;
        HW_RegisterDmaHandle(cfg->rx_dma.dma_num, cfg->rx_dma.stream, &hdmaRx);
        HAL_NVIC_SetPriority(HW_GetDmaIrqn(cfg->rx_dma.dma_num, cfg->rx_dma.stream), cfg->irq_priority, 0);
        HAL_NVIC_EnableIRQ(HW_GetDmaIrqn(cfg->rx_dma.dma_num, cfg->rx_dma.stream));
        __HAL_LINKDMA(&hspi, hdmarx, hdmaRx);

        useDma = true;
    }

    mutex = osMutexNew(&s_recursiveMutexAttr);
    xferSem = osSemaphoreNew(1, 0, NULL);

#define SPI_SETUP_CB(N)                                                                                                                                                                                 \
    do {                                                                                                                                                                                                \
        HAL_SPI_RegisterCallback(&hspi, HAL_SPI_TX_COMPLETE_CB_ID, spiDrvTxCpltCb<N>);                                                                                                                  \
        HAL_SPI_RegisterCallback(&hspi, HAL_SPI_RX_COMPLETE_CB_ID, spiDrvRxCpltCb<N>);                                                                                                                  \
        HAL_SPI_RegisterCallback(&hspi, HAL_SPI_TX_RX_COMPLETE_CB_ID, spiDrvTxRxCpltCb<N>);                                                                                                             \
        HAL_SPI_RegisterCallback(&hspi, HAL_SPI_ABORT_CB_ID, spiDrvAbortCpltCb<N>);                                                                                                                     \
        HAL_SPI_RegisterCallback(&hspi, HAL_SPI_ERROR_CB_ID, spiDrvErrorCb<N>);                                                                                                                         \
    } while (0)

    switch (num)
    {
        SPI_SETUP_CB(1);
        SPI_SETUP_CB(2);
        SPI_SETUP_CB(3);
        SPI_SETUP_CB(4);
        SPI_SETUP_CB(5);
        SPI_SETUP_CB(6);
        default: break;
    }
#undef SPI_SETUP_CB

    return true;
}

void SPI_CLASS::deinit()
{
    HAL_SPI_DeInit(&hspi);
    if (useDma)
    {
        HAL_DMA_DeInit(&hdmaTx);
        HAL_DMA_DeInit(&hdmaRx);
    }
    useDma = false;
}

bool SPI_CLASS::lock(double timeout)
{
    uint32_t ticks = (timeout < 0) ? osWaitForever : (uint32_t)(timeout * (double)osKernelGetTickFreq());
    return osMutexAcquire(mutex, ticks) == osOK;
}

void SPI_CLASS::unlock()
{
    osMutexRelease(mutex);
}

static uint32_t spi_to_ticks(double timeout)
{
    return (timeout < 0) ? osWaitForever : (uint32_t)(timeout * (double)osKernelGetTickFreq());
}

SPI_DrvResult SPI_CLASS::transmitReceive(const uint8_t *tx_data, uint8_t *rx_data, uint16_t size, double timeout)
{
    if (size == 0 || (tx_data == 0 && rx_data == 0))
        return SPI_Drv_BadParam;
    if (size > SPI_DMA_BUF_SIZE)
        return SPI_Drv_BadParam;

    if (!lock(timeout))
        return SPI_Drv_Timeout;

    static uint8_t dummyTx[SPI_DMA_BUF_SIZE];
    const uint8_t *tx = tx_data ? tx_data : dummyTx;

    memcpy(txBounce, tx, size);

    // cache 维护仅在 DMA 模式下需要: DMA 绕过 CPU cache 直接访存, 必须保证 cache 一致性
    // 非 DMA 模式下 CPU 自己读写 bounce buffer, cache 天然一致, 调 cache 维护属于
    // ARMv7-M "constrained unpredictable" 行为 (尤其 D-cache 禁用时会触发 imprecise bus fault)
    if (useDma)
    {
        __DSB();
        SCB_CleanDCache_by_Addr((uint32_t *)txBounce, SPI_DMA_BUF_SIZE);
        SCB_InvalidateDCache_by_Addr((uint32_t *)rxBounce, SPI_DMA_BUF_SIZE);
    }

    osSemaphoreAcquire(xferSem, 0);
    lastResult = SPI_Drv_Error;
    HAL_StatusTypeDef st;
    if (useDma)
        st = HAL_SPI_TransmitReceive_DMA(&hspi, txBounce, rxBounce, size);
    else
        st = HAL_SPI_TransmitReceive(&hspi, (uint8_t *)txBounce, rxBounce, size, (timeout < 0 ? HAL_MAX_DELAY : (uint32_t)(timeout * 1000)));

    SPI_DrvResult result = SPI_Drv_Ok;
    if (st != HAL_OK)
    {
        result = (st == HAL_TIMEOUT) ? SPI_Drv_Timeout : SPI_Drv_Error;
    }
    else if (useDma)
    {
        if (osSemaphoreAcquire(xferSem, spi_to_ticks(timeout)) != osOK)
            result = SPI_Drv_Timeout;
        else
            result = lastResult;
    }

    if (result == SPI_Drv_Ok)
    {
        if (useDma)
            SCB_InvalidateDCache_by_Addr((uint32_t *)rxBounce, SPI_DMA_BUF_SIZE);
        if (rx_data)
            memcpy(rx_data, rxBounce, size);
    }

    unlock();
    return result;
}

SPI_DrvResult SPI_CLASS::write(const uint8_t *tx_data, uint16_t size, double timeout)
{
    //OLED 等只写设备: 用纯 HAL_SPI_Transmit (与参考工程一致), 避免全双工模式下
    //MISO 悬空导致 HAL_SPI_TransmitReceive 状态机异常而传输失败 (黑屏)
    if (size == 0 || tx_data == 0)
        return SPI_Drv_BadParam;
    if (size > SPI_DMA_BUF_SIZE)
        return SPI_Drv_BadParam;
    if (!lock(timeout))
        return SPI_Drv_Timeout;

    memcpy(txBounce, tx_data, size);
    if (useDma)
    {
        __DSB();
        SCB_CleanDCache_by_Addr((uint32_t *)txBounce, SPI_DMA_BUF_SIZE);
        osSemaphoreAcquire(xferSem, 0);
        lastResult = SPI_Drv_Error;
        HAL_StatusTypeDef st = HAL_SPI_Transmit_DMA(&hspi, txBounce, size);
        SPI_DrvResult result = SPI_Drv_Ok;
        if (st != HAL_OK)
            result = (st == HAL_TIMEOUT) ? SPI_Drv_Timeout : SPI_Drv_Error;
        else if (osSemaphoreAcquire(xferSem, spi_to_ticks(timeout)) != osOK)
            result = SPI_Drv_Timeout;
        else
            result = lastResult;
        unlock();
        return result;
    }
    else
    {
        uint32_t ms = (timeout < 0) ? HAL_MAX_DELAY : (uint32_t)(timeout * 1000.0) + 1u;
        HAL_StatusTypeDef st = HAL_SPI_Transmit(&hspi, txBounce, size, ms);
        unlock();
        if (st == HAL_OK)      return SPI_Drv_Ok;
        if (st == HAL_TIMEOUT) return SPI_Drv_Timeout;
        return SPI_Drv_Error;
    }
}

SPI_DrvResult SPI_CLASS::read(uint8_t *rx_data, uint16_t size, double timeout)
{
    return transmitReceive(0, rx_data, size, timeout);
}

bool register_spi(uint8_t spi_num, const SPI_Config *cfg)
{
    if (!HW_SPI_INDEX_VALID(spi_num) || !cfg)
        return false;
    if (spi_inited[spi_num])
        return false;
    if (!spi_instances[spi_num].init(spi_num, cfg))
        return false;
    spi_inited[spi_num] = true;
    return true;
}

SPI_CLASS *get_spi_instance(uint8_t spi_num)
{
    if (HW_SPI_INDEX_VALID(spi_num) && spi_inited[spi_num])
        return &spi_instances[spi_num];
    return 0;
}

void init_drv_spi(void)
{
    for (uint8_t i = 0; i < BOARD_SPI_COUNT; ++i)
    {
        if (board_spi_ports[i].hw_num == 0 || board_spi_ports[i].config == 0)
            continue;
        register_spi(board_spi_ports[i].hw_num, board_spi_ports[i].config);
    }
}
