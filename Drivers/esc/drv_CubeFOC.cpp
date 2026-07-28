#include "drv_CubeFOC.hpp"
#include "hardware/drv_can.hpp"
#include "dronecan_msgs.h"

#include <string.h>

static struct uavcan_protocol_NodeStatus node_status;

CubeFOC::CubeFOC(CAN_CLASS *can)
    : can_(can)
    , node_status_transfer_id_(0)
    , esc_index_transfer_id_(0)
    , calib_esc_transfer_id_(0)
    , esc_rpm_command_transfer_id_(0)
    , esc_arm_flag(false)
{
    memset(esc_status_, 0, sizeof(esc_status_));
}

void CubeFOC::init()
{
    m_send_mutex       = osMutexNew(NULL);
    m_esc_status_mutex = osMutexNew(NULL);

    canardInit(&canard_,
               memory_pool_,
               sizeof(memory_pool_),
               onTransferReceived,
               shouldAcceptTransfer,
               this);

    canardSetLocalNodeID(&canard_, 10);

    // 启用 DWT 周期计数器作为时间戳
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL   |= DWT_CTRL_CYCCNTENA_Msk;
}

uint64_t CubeFOC::micros64()
{
    static uint32_t last = 0;
    static uint64_t high = 0;

    uint32_t now = DWT->CYCCNT;
	//DWT计数器溢出处理
    if (now < last)
        high += (1ULL << 32);

    last = now;
    uint64_t full = high | now;

    return full / (SystemCoreClock / 1000000);
}

void CubeFOC::process_tx(uint8_t max_frames)
{
    FDCAN_HandleTypeDef *hfdcan = can_->handle();
    uint8_t count = 0;

    const CanardCANFrame *txf;
    while ((txf = canardPeekTxQueue(&canard_)) != NULL)
    {
        if (count++ >= max_frames)
            break;

        FDCAN_TxHeaderTypeDef txHeader = {0};

        txHeader.Identifier = txf->id & CANARD_CAN_EXT_ID_MASK;
        txHeader.IdType     = FDCAN_EXTENDED_ID;

        uint32_t dlc;
        switch (txf->data_len)
        {
            case 0:  dlc = FDCAN_DLC_BYTES_0; break;
            case 1:  dlc = FDCAN_DLC_BYTES_1; break;
            case 2:  dlc = FDCAN_DLC_BYTES_2; break;
            case 3:  dlc = FDCAN_DLC_BYTES_3; break;
            case 4:  dlc = FDCAN_DLC_BYTES_4; break;
            case 5:  dlc = FDCAN_DLC_BYTES_5; break;
            case 6:  dlc = FDCAN_DLC_BYTES_6; break;
            case 7:  dlc = FDCAN_DLC_BYTES_7; break;
            case 8:  dlc = FDCAN_DLC_BYTES_8; break;
            default: dlc = FDCAN_DLC_BYTES_8; break;
        }

        txHeader.DataLength           = dlc;
        txHeader.TxFrameType          = FDCAN_DATA_FRAME;
        txHeader.ErrorStateIndicator  = FDCAN_ESI_ACTIVE;
        txHeader.BitRateSwitch        = FDCAN_BRS_OFF;
        txHeader.FDFormat             = FDCAN_CLASSIC_CAN;
        txHeader.TxEventFifoControl   = FDCAN_NO_TX_EVENTS;
        txHeader.MessageMarker        = 0;

        if (HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &txHeader, txf->data) == HAL_OK)
            canardPopTxQueue(&canard_);
        else
            break;
    }
}

void CubeFOC::process_rx(uint8_t max_frames)
{
    FDCAN_HandleTypeDef *hfdcan = can_->handle();
    FDCAN_RxHeaderTypeDef rxHeader;
    uint8_t data[8];
    uint8_t count = 0;

    while (HAL_FDCAN_GetRxFifoFillLevel(hfdcan, FDCAN_RX_FIFO0) > 0)
    {
        if (count++ >= max_frames)
            break;

        HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rxHeader, data);

        CanardCANFrame rx_frame;
        rx_frame.id = rxHeader.Identifier | CANARD_CAN_FRAME_EFF;

        uint8_t len;
        switch (rxHeader.DataLength)
        {
            case FDCAN_DLC_BYTES_0: len = 0; break;
            case FDCAN_DLC_BYTES_1: len = 1; break;
            case FDCAN_DLC_BYTES_2: len = 2; break;
            case FDCAN_DLC_BYTES_3: len = 3; break;
            case FDCAN_DLC_BYTES_4: len = 4; break;
            case FDCAN_DLC_BYTES_5: len = 5; break;
            case FDCAN_DLC_BYTES_6: len = 6; break;
            case FDCAN_DLC_BYTES_7: len = 7; break;
            case FDCAN_DLC_BYTES_8: len = 8; break;
            default: len = 8; break;
        }

        rx_frame.data_len = len;
        memcpy(rx_frame.data, data, len);

        canardHandleRxFrame(&canard_, &rx_frame, micros64());
    }
}

void CubeFOC::spin_once()
{
    process_tx(5);
    process_rx(64);
}

// =============================================================================
// canard 回调
// =============================================================================

void CubeFOC::onTransferReceived(CanardInstance *ins, CanardRxTransfer *transfer)
{
    CubeFOC *self = (CubeFOC *)ins->user_reference;

    if (transfer->transfer_type == CanardTransferTypeRequest)
    {
        switch (transfer->data_type_id)
        {
            case UAVCAN_PROTOCOL_GETNODEINFO_ID:
                self->handle_GetNodeInfo(ins, transfer);
                break;
        }
    }

    if (transfer->transfer_type == CanardTransferTypeBroadcast)
    {
        switch (transfer->data_type_id)
        {
            case UAVCAN_EQUIPMENT_ESC_CUBESTATUS_ID:
                self->handle_esc_status(ins, transfer);
                break;
        }
    }
}

bool CubeFOC::shouldAcceptTransfer(const CanardInstance *ins,
                                   uint64_t *out_data_type_signature,
                                   uint16_t data_type_id,
                                   CanardTransferType transfer_type,
                                   uint8_t source_node_id)
{
    (void)source_node_id;

    if (transfer_type == CanardTransferTypeRequest)
    {
        switch (data_type_id)
        {
            case UAVCAN_PROTOCOL_GETNODEINFO_ID:
                *out_data_type_signature = UAVCAN_PROTOCOL_GETNODEINFO_REQUEST_SIGNATURE;
                return true;
        }
    }

    if (transfer_type == CanardTransferTypeBroadcast)
    {
        switch (data_type_id)
        {
            case UAVCAN_EQUIPMENT_ESC_CUBESTATUS_ID:
                *out_data_type_signature = UAVCAN_EQUIPMENT_ESC_CUBESTATUS_SIGNATURE;
                return true;
        }
    }

    return false;
}

// =============================================================================
// 消息处理
// =============================================================================

void CubeFOC::handle_GetNodeInfo(CanardInstance *ins, CanardRxTransfer *transfer)
{
    uint8_t buffer[UAVCAN_PROTOCOL_GETNODEINFO_RESPONSE_MAX_SIZE];
    struct uavcan_protocol_GetNodeInfoResponse pkt;
    memset(&pkt, 0, sizeof(pkt));

    node_status.uptime_sec = micros64() / 1000000;
    pkt.status = node_status;
    pkt.software_version.major = 1;
    pkt.software_version.minor = 1;
    pkt.software_version.optional_field_flags = 1;
    pkt.software_version.vcs_commit = 1;

    uint32_t total_size = uavcan_protocol_GetNodeInfoResponse_encode(&pkt, buffer);

    canardRequestOrRespond(ins,
                           transfer->source_node_id,
                           UAVCAN_PROTOCOL_GETNODEINFO_SIGNATURE,
                           UAVCAN_PROTOCOL_GETNODEINFO_ID,
                           &transfer->transfer_id,
                           transfer->priority,
                           CanardResponse,
                           buffer,
                           total_size);
}

void CubeFOC::handle_esc_status(CanardInstance *ins, CanardRxTransfer *transfer)
{
    (void)ins;

    struct uavcan_equipment_esc_CubeStatus msg;
    if (uavcan_equipment_esc_CubeStatus_decode(transfer, &msg))
        return;

    uint8_t esc_id = msg.esc_index - 1;
    if (esc_id >= Max_ESC_Num)
        return;

    float voltage     = msg.voltage;
    float current     = msg.current;
    float temperature = msg.temperature;
    int32_t rpm       = msg.rpm;
    int32_t target_rpm= msg.target_rpm;
    bool calib_flag   = msg.calib_done;

    if (osMutexAcquire(m_esc_status_mutex, osWaitForever) == osOK)
    {
        esc_status_[esc_id].rpm         = rpm;
        esc_status_[esc_id].target_rpm  = target_rpm;
        esc_status_[esc_id].voltage     = voltage;
        esc_status_[esc_id].current     = current;
        esc_status_[esc_id].temperature = temperature - 273.15f;
        esc_status_[esc_id].calib_flag  = calib_flag;

        osMutexRelease(m_esc_status_mutex);
    }
}

// =============================================================================
// 电调指令
// =============================================================================

void CubeFOC::send_node_status()
{
    struct uavcan_protocol_NodeStatus msg;
    msg.uptime_sec = micros64() / 1000000;
    msg.health     = UAVCAN_PROTOCOL_NODESTATUS_HEALTH_OK;
    msg.mode       = UAVCAN_PROTOCOL_NODESTATUS_MODE_OPERATIONAL;
    msg.sub_mode   = 0;
    msg.vendor_specific_status_code = 0;

    uint8_t buffer[32];
    uint32_t size = uavcan_protocol_NodeStatus_encode(&msg, buffer);

    osMutexAcquire(m_send_mutex, osWaitForever);
    canardBroadcast(&canard_,
                    UAVCAN_PROTOCOL_NODESTATUS_SIGNATURE,
                    UAVCAN_PROTOCOL_NODESTATUS_ID,
                    &node_status_transfer_id_,
                    CANARD_TRANSFER_PRIORITY_LOW,
                    buffer,
                    size);
    node_status_transfer_id_++;
    osMutexRelease(m_send_mutex);
}

void CubeFOC::set_esc_index_command(uint8_t target_esc_index)
{
    struct uavcan_equipment_esc_CubeSetID msg;
    memset(&msg, 0, sizeof(msg));
    msg.esc_index = target_esc_index;

    uint8_t buffer[32];
    uint32_t size = uavcan_equipment_esc_CubeSetID_encode(&msg, buffer);
    if (size == 0) return;

    osMutexAcquire(m_send_mutex, osWaitForever);
    canardBroadcast(&canard_,
                    UAVCAN_EQUIPMENT_ESC_CUBESETID_SIGNATURE,
                    UAVCAN_EQUIPMENT_ESC_CUBESETID_ID,
                    &esc_index_transfer_id_,
                    CANARD_TRANSFER_PRIORITY_MEDIUM,
                    buffer,
                    size);
    esc_index_transfer_id_++;
    osMutexRelease(m_send_mutex);
}

void CubeFOC::calib_esc_command(uint8_t target_esc_index)
{
    struct uavcan_equipment_esc_CubeCalibCommand msg;
    memset(&msg, 0, sizeof(msg));
    msg.esc_index = target_esc_index;

    uint8_t buffer[32];
    uint32_t size = uavcan_equipment_esc_CubeCalibCommand_encode(&msg, buffer);
    if (size == 0) return;

    osMutexAcquire(m_send_mutex, osWaitForever);
    canardBroadcast(&canard_,
                    UAVCAN_EQUIPMENT_ESC_CUBECALIBCOMMAND_SIGNATURE,
                    UAVCAN_EQUIPMENT_ESC_CUBECALIBCOMMAND_ID,
                    &calib_esc_transfer_id_,
                    CANARD_TRANSFER_PRIORITY_MEDIUM,
                    buffer,
                    size);
    calib_esc_transfer_id_++;
    osMutexRelease(m_send_mutex);
}

void CubeFOC::send_esc_current_commands(const int32_t *cmd_array, uint8_t len)
{
    struct uavcan_equipment_esc_CubeIqCommand msg;
    memset(&msg, 0, sizeof(msg));

    if (len > Max_ESC_Num) len = Max_ESC_Num;

    msg.Iq.len = len + 1;
    msg.arm = 1;

    for (uint8_t i = 0; i < len; i++)
        msg.Iq.data[i] = cmd_array[i];

    uint8_t buffer[64];
    uint32_t size = uavcan_equipment_esc_CubeIqCommand_encode(&msg, buffer);
    if (size == 0) return;

    osMutexAcquire(m_send_mutex, osWaitForever);
    canardBroadcast(&canard_,
                    UAVCAN_EQUIPMENT_ESC_CUBEIQCOMMAND_SIGNATURE,
                    UAVCAN_EQUIPMENT_ESC_CUBEIQCOMMAND_ID,
                    &esc_rpm_command_transfer_id_,
                    CANARD_TRANSFER_PRIORITY_HIGH,
                    buffer,
                    size);
    esc_rpm_command_transfer_id_++;
    osMutexRelease(m_send_mutex);
}

void CubeFOC::send_esc_rpm_commands(const int32_t *cmd_array, uint8_t len)
{
    struct uavcan_equipment_esc_CubeRPMCommand msg;
    memset(&msg, 0, sizeof(msg));

    if (len > Max_ESC_Num) len = Max_ESC_Num;

    msg.rpm.len = len + 1;
    msg.arm = 1;

    for (uint8_t i = 0; i < len; i++)
    {
        int32_t rpm = cmd_array[i];
        if (rpm >  8000) rpm =  8000;
        if (rpm < -8000) rpm = -8000;
        msg.rpm.data[i] = rpm;
    }

    uint8_t buffer[64];
    uint32_t size = uavcan_equipment_esc_CubeRPMCommand_encode(&msg, buffer);
    if (size == 0) return;

    osMutexAcquire(m_send_mutex, osWaitForever);
    canardBroadcast(&canard_,
                    UAVCAN_EQUIPMENT_ESC_CUBERPMCOMMAND_SIGNATURE,
                    UAVCAN_EQUIPMENT_ESC_CUBERPMCOMMAND_ID,
                    &esc_rpm_command_transfer_id_,
                    CANARD_TRANSFER_PRIORITY_HIGH,
                    buffer,
                    size);
    esc_rpm_command_transfer_id_++;
    osMutexRelease(m_send_mutex);
}

// =============================================================================
// 状态查询
// =============================================================================

bool CubeFOC::get_esc_status(uint8_t esc_index, ESCStatusCache &out)
{
    if (esc_index >= Max_ESC_Num)
        return false;

    if (osMutexAcquire(m_esc_status_mutex, 0) == osOK)
    {
        out = esc_status_[esc_index];
        osMutexRelease(m_esc_status_mutex);
    }

    return true;
}

// =============================================================================
// 全局初始化 / 访问函数
// =============================================================================

static CubeFOC *g_drv_cubefoc = 0;

void init_drv_cubefoc()
{
    if (g_drv_cubefoc)
        return;

    CAN_CLASS *can = get_can_instance(1);
    if (!can)
        return;

    g_drv_cubefoc = new CubeFOC(can);
    if (!g_drv_cubefoc)
        return;

    g_drv_cubefoc->init();
}

CubeFOC *drv_cubefoc()
{
    return g_drv_cubefoc;
}
