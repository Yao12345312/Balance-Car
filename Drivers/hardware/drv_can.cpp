#include "drv_can.hpp"
#include "drv_common.hpp"

#include "cmsis_os2.h"
#include "board.hpp"

#include "stm32h743xx.h"
#include <string.h>

static const osMutexAttr_t s_recursiveMutexAttr = {.attr_bits = osMutexRecursive};

static CAN_CLASS can_instances[HW_DRV_MAX_CAN];
static bool can_inited[HW_DRV_MAX_CAN] = {};

static FDCAN_GlobalTypeDef *const fdcanTable[HW_DRV_MAX_CAN] = {0, FDCAN1, FDCAN2};

static const IRQn_Type fdcanIt0Irqn[HW_DRV_MAX_CAN] = {
    (IRQn_Type)0, FDCAN1_IT0_IRQn, FDCAN2_IT0_IRQn,
};
static const IRQn_Type fdcanIt1Irqn[HW_DRV_MAX_CAN] = {
    (IRQn_Type)0, FDCAN1_IT1_IRQn, FDCAN2_IT1_IRQn,
};

static void can_clk_enable(uint8_t num)
{
    switch (num)
    {
        case 1: __HAL_RCC_FDCAN_CLK_ENABLE(); break;
        case 2: __HAL_RCC_FDCAN_CLK_ENABLE(); break;
        default: break;
    }
}

template <int N> void canDrvTxFifoEmptyCb(FDCAN_HandleTypeDef *) { can_instances[N].onTxFifoEmpty(); }
template <int N> void canDrvRxNewMsgCb(FDCAN_HandleTypeDef *)     { can_instances[N].onRxNewMsg(); }
template <int N> void canDrvErrorCb(FDCAN_HandleTypeDef *)        { can_instances[N].onError(); }

template <int N> void canDrvIrq0()
{
    if (can_inited[N])
        HAL_FDCAN_IRQHandler(&can_instances[N].hfdcan);
}
template <int N> void canDrvIrq1()
{
    if (can_inited[N])
        HAL_FDCAN_IRQHandler(&can_instances[N].hfdcan);
}

void CAN_CLASS::feedRx(const CAN_Msg *msg)
{
    uint8_t next = rxHead + 1u;
    if (next >= CAN_RX_FIFO_SIZE)
        next = 0;
    if (next == rxTail)
        return;
    rxRing[rxHead] = *msg;
    rxHead = next;
    osSemaphoreRelease(rxSem);
}

void CAN_CLASS::onTxFifoEmpty()
{
    txResult = CAN_Drv_Ok;
    osSemaphoreRelease(txSem);
}

void CAN_CLASS::onRxNewMsg()
{
    FDCAN_RxHeaderTypeDef rxHeader;
    CAN_Msg msg;
    while (HAL_FDCAN_GetRxMessage(&hfdcan, FDCAN_RX_FIFO0, &rxHeader, msg.data) == HAL_OK)
    {
        msg.id       = rxHeader.Identifier;
        msg.is_ext   = (rxHeader.IdType == FDCAN_EXTENDED_ID);
        msg.is_remote= (rxHeader.RxFrameType == FDCAN_REMOTE_FRAME);
        msg.dlc      = (uint8_t)((rxHeader.DataLength >> 16U) & 0x0FU);
        feedRx(&msg);
    }
}

void CAN_CLASS::onError()
{
}

bool CAN_CLASS::init(uint8_t num, const CAN_Config *cfg)
{
    if (!HW_CAN_INDEX_VALID(num) || !cfg)
        return false;
    if (!cfg->tx.port && !cfg->tx.pin)
        return false;

    canNum = num;
    txResult = CAN_Drv_Ok;

    HW_ConfigurePinAF(&cfg->tx, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH);
    HW_ConfigurePinAF(&cfg->rx, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH);

    can_clk_enable(num);
    HW_Delay(0.001);

    hfdcan.Instance = fdcanTable[num];
    hfdcan.Init.FrameFormat          = FDCAN_FRAME_CLASSIC;
    hfdcan.Init.Mode                 = FDCAN_MODE_NORMAL;
    hfdcan.Init.AutoRetransmission   = ENABLE;
    hfdcan.Init.TransmitPause        = DISABLE;
    hfdcan.Init.ProtocolException    = DISABLE;
    hfdcan.Init.NominalPrescaler     = cfg->prescaler  ? cfg->prescaler  : 2;
    hfdcan.Init.NominalSyncJumpWidth = cfg->sjw        ? cfg->sjw        : 1;
    hfdcan.Init.NominalTimeSeg1      = cfg->time_seg1  ? cfg->time_seg1  : 12;
    hfdcan.Init.NominalTimeSeg2      = cfg->time_seg2  ? cfg->time_seg2  : 3;

    hfdcan.Init.DataPrescaler     = 1;
    hfdcan.Init.DataSyncJumpWidth = 1;
    hfdcan.Init.DataTimeSeg1      = 1;
    hfdcan.Init.DataTimeSeg2      = 1;

    hfdcan.Init.StdFiltersNbr       = 0;
    hfdcan.Init.ExtFiltersNbr       = 1;
    hfdcan.Init.RxFifo0ElmtsNbr     = 16;
    hfdcan.Init.RxFifo0ElmtSize     = FDCAN_DATA_BYTES_8;
    hfdcan.Init.RxFifo1ElmtsNbr     = 0;
    hfdcan.Init.RxFifo1ElmtSize     = FDCAN_DATA_BYTES_8;
    hfdcan.Init.RxBuffersNbr        = 0;
    hfdcan.Init.RxBufferSize        = FDCAN_DATA_BYTES_8;
    hfdcan.Init.TxEventsNbr         = 0;
    hfdcan.Init.TxBuffersNbr        = 0;
    hfdcan.Init.TxFifoQueueElmtsNbr = 32;
    hfdcan.Init.TxFifoQueueMode     = FDCAN_TX_FIFO_OPERATION;
    hfdcan.Init.TxElmtSize          = FDCAN_DATA_BYTES_8;
    hfdcan.Init.MessageRAMOffset    = 0;

    if (HAL_FDCAN_Init(&hfdcan) != HAL_OK)
        return false;

    // 接受所有扩展帧到 FIFO0
    FDCAN_FilterTypeDef filter;
    filter.IdType        = FDCAN_EXTENDED_ID;
    filter.FilterIndex   = 0;
    filter.FilterType    = FDCAN_FILTER_MASK;
    filter.FilterConfig  = FDCAN_FILTER_TO_RXFIFO0;
    filter.FilterID1     = 0;
    filter.FilterID2     = 0;
    filter.RxBufferIndex = 0;
    filter.IsCalibrationMsg = 0;
    if (HAL_FDCAN_ConfigFilter(&hfdcan, &filter) != HAL_OK)
        return false;

    // 未匹配到 Filter 的消息也接收至 FIFO0
    HAL_FDCAN_ConfigGlobalFilter(&hfdcan,
        FDCAN_ACCEPT_IN_RX_FIFO0,
        FDCAN_ACCEPT_IN_RX_FIFO0,
        FDCAN_REJECT_REMOTE,
        FDCAN_REJECT_REMOTE);

    // 注册回调
    HAL_FDCAN_RegisterCallback(&hfdcan, HAL_FDCAN_TX_FIFO_EMPTY_CB_ID, (pFDCAN_CallbackTypeDef)0);
    HAL_FDCAN_RegisterCallback(&hfdcan, HAL_FDCAN_RX_BUFFER_NEW_MSG_CB_ID, (pFDCAN_CallbackTypeDef)0);
    HAL_FDCAN_RegisterCallback(&hfdcan, HAL_FDCAN_ERROR_CALLBACK_CB_ID, (pFDCAN_CallbackTypeDef)0);

#define CAN_SETUP_CB(N)                                                                                                                                                                                 \
    do {                                                                                                                                                                                                \
        HAL_FDCAN_RegisterCallback(&hfdcan, HAL_FDCAN_TX_FIFO_EMPTY_CB_ID, (pFDCAN_CallbackTypeDef)canDrvTxFifoEmptyCb<N>);                                                                             \
        HAL_FDCAN_RegisterCallback(&hfdcan, HAL_FDCAN_RX_BUFFER_NEW_MSG_CB_ID, (pFDCAN_CallbackTypeDef)canDrvRxNewMsgCb<N>);                                                                            \
        HAL_FDCAN_RegisterCallback(&hfdcan, HAL_FDCAN_ERROR_CALLBACK_CB_ID, (pFDCAN_CallbackTypeDef)canDrvErrorCb<N>);                                                                                  \
    } while (0)

    switch (num)
    {
        case 1:  CAN_SETUP_CB(1); break;
        case 2:  CAN_SETUP_CB(2); break;
        default: break;
    }
#undef CAN_SETUP_CB

    if (HAL_FDCAN_Start(&hfdcan) != HAL_OK)
        return false;

    // 激活 RX FIFO0 新消息中断
    if (HAL_FDCAN_ActivateNotification(&hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK)
        return false;

    mutex  = osMutexNew(&s_recursiveMutexAttr);
    txSem  = osSemaphoreNew(1, 0, NULL);
    rxSem  = osSemaphoreNew(CAN_RX_FIFO_SIZE, 0, NULL);
    rxHead = 0;
    rxTail = 0;

    HAL_NVIC_SetPriority(fdcanIt0Irqn[num], cfg->irq_priority, 0);
    HAL_NVIC_SetPriority(fdcanIt1Irqn[num], cfg->irq_priority, 0);

    can_inited[num] = true;

    HAL_NVIC_EnableIRQ(fdcanIt0Irqn[num]);
    HAL_NVIC_EnableIRQ(fdcanIt1Irqn[num]);

    return true;
}

void CAN_CLASS::deinit()
{
    HAL_NVIC_DisableIRQ(fdcanIt0Irqn[canNum]);
    HAL_NVIC_DisableIRQ(fdcanIt1Irqn[canNum]);
    HAL_FDCAN_DeInit(&hfdcan);
}

bool CAN_CLASS::lock(double timeout)
{
    uint32_t ticks = (timeout < 0) ? osWaitForever : (uint32_t)(timeout * (double)osKernelGetTickFreq());
    return osMutexAcquire(mutex, ticks) == osOK;
}

void CAN_CLASS::unlock()
{
    osMutexRelease(mutex);
}

CAN_DrvResult CAN_CLASS::send(const CAN_Msg *msg, double timeout)
{
    if (!msg)
        return CAN_Drv_BadParam;

    if (!lock(timeout))
        return CAN_Drv_Timeout;

    FDCAN_TxHeaderTypeDef txHeader;
    txHeader.Identifier       = msg->id;
    txHeader.IdType           = msg->is_ext ? FDCAN_EXTENDED_ID : FDCAN_STANDARD_ID;
    txHeader.TxFrameType      = msg->is_remote ? FDCAN_REMOTE_FRAME : FDCAN_DATA_FRAME;
    txHeader.DataLength       = ((uint32_t)msg->dlc) << 16U;
    txHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    txHeader.BitRateSwitch    = FDCAN_BRS_OFF;
    txHeader.FDFormat         = FDCAN_CLASSIC_CAN;
    txHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    txHeader.MessageMarker    = 0;

    osSemaphoreAcquire(txSem, 0);
    txResult = CAN_Drv_Error;

    HAL_StatusTypeDef st = HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan, &txHeader, (uint8_t *)msg->data);
    CAN_DrvResult r;
    if (st != HAL_OK)
    {
        r = (st == HAL_TIMEOUT) ? CAN_Drv_Timeout : CAN_Drv_Error;
        unlock();
        return r;
    }

    uint32_t ticks = (timeout < 0) ? osWaitForever : (uint32_t)(timeout * (double)osKernelGetTickFreq());
    if (osSemaphoreAcquire(txSem, ticks) != osOK)
    {
        unlock();
        return CAN_Drv_Timeout;
    }

    r = txResult;
    unlock();
    return r;
}

uint16_t CAN_CLASS::read(CAN_Msg *msg, uint16_t max_msgs, double timeout)
{
    if (!msg || max_msgs == 0)
        return 0;

    if (!lock(timeout))
        return 0;

    uint32_t ticks = (timeout < 0) ? osWaitForever : (uint32_t)(timeout * (double)osKernelGetTickFreq());
    uint16_t got = 0;
    while (got < max_msgs)
    {
        uint32_t t = (got == 0) ? ticks : 0;
        if (osSemaphoreAcquire(rxSem, t) != osOK)
            break;
        msg[got] = rxRing[rxTail];
        if (++rxTail >= CAN_RX_FIFO_SIZE)
            rxTail = 0;
        got++;
    }

    unlock();
    return got;
}

extern "C" void FDCAN1_IT0_IRQHandler(void) { canDrvIrq0<1>(); }
extern "C" void FDCAN1_IT1_IRQHandler(void) { canDrvIrq1<1>(); }
extern "C" void FDCAN2_IT0_IRQHandler(void) { canDrvIrq0<2>(); }
extern "C" void FDCAN2_IT1_IRQHandler(void) { canDrvIrq1<2>(); }

bool register_can(uint8_t can_num, const CAN_Config *cfg)
{
    if (!HW_CAN_INDEX_VALID(can_num) || !cfg)
        return false;
    if (can_inited[can_num])
        return false;
    if (!can_instances[can_num].init(can_num, cfg))
        return false;
    can_inited[can_num] = true;
    return true;
}

CAN_CLASS *get_can_instance(uint8_t can_num)
{
    if (HW_CAN_INDEX_VALID(can_num) && can_inited[can_num])
        return &can_instances[can_num];
    return 0;
}

void init_drv_can(void)
{
    for (uint8_t i = 0; i < BOARD_CAN_COUNT; ++i)
    {
        if (board_can_ports[i].hw_num == 0 || board_can_ports[i].config == 0)
            continue;
        register_can(board_can_ports[i].hw_num, board_can_ports[i].config);
    }
}
