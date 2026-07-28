#pragma once

#include "stm32h7xx_hal.h"
#include "cmsis_os2.h"
#include "canard.h"

#include <stdint.h>

// 电调索引
#define ESC1_Index   0
#define ESC2_Index   1
#define ESC3_Index   2
#define Max_ESC_Num  4

// 温度保护阈值
#define ESC_MAX_Temperature  80
// 转速保护阈值
#define ESC_MAX_SpeedRPM     1000

class CAN_CLASS;

class CubeFOC
{
  public:
    struct ESCStatusCache
    {
        int32_t rpm;
        int32_t target_rpm;
        float   voltage;
        float   current;
        float   temperature;
        volatile bool calib_flag;
    };

    CubeFOC(CAN_CLASS *can);
    ~CubeFOC() = default;

    void init();

    // 每周期调用, 处理 CAN 收发
    void spin_once();

    // 节点状态广播
    void send_node_status();

    // 电调指令
    void set_esc_index_command(uint8_t target_esc_index);
    void calib_esc_command(uint8_t target_esc_index);
    void send_esc_current_commands(const int32_t *cmd_array, uint8_t len);
    void send_esc_rpm_commands(const int32_t *cmd_array, uint8_t len);

    // 获取电调状态
    bool get_esc_status(uint8_t esc_index, ESCStatusCache &out);

    // 时间戳 (us)
    uint64_t micros64();

  private:
    // canard 回调
    static void onTransferReceived(CanardInstance *ins, CanardRxTransfer *transfer);
    static bool shouldAcceptTransfer(const CanardInstance *ins,
                                     uint64_t *out_data_type_signature,
                                     uint16_t data_type_id,
                                     CanardTransferType transfer_type,
                                     uint8_t source_node_id);

    // 消息处理
    void handle_GetNodeInfo(CanardInstance *ins, CanardRxTransfer *transfer);
    void handle_esc_status(CanardInstance *ins, CanardRxTransfer *transfer);

    // 底层 CAN 收发
    void process_tx(uint8_t max_frames);
    void process_rx(uint8_t max_frames);

    CAN_CLASS *can_;

    CanardInstance canard_;
    uint8_t memory_pool_[2048];

    ESCStatusCache esc_status_[Max_ESC_Num];

    osMutexId_t m_send_mutex;
    osMutexId_t m_esc_status_mutex;

    uint8_t node_status_transfer_id_;
    uint8_t esc_index_transfer_id_;
    uint8_t calib_esc_transfer_id_;
    uint8_t esc_rpm_command_transfer_id_;

     volatile bool esc_arm_flag;
};

void init_drv_cubefoc();
CubeFOC *drv_cubefoc();
