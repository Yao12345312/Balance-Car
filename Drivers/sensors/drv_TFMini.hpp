#pragma once

#include "board.hpp"
#include "cmsis_os2.h"
#include <stdint.h>

// TFmini-S 默认 I2C 7位地址 (board 层内部左移1位)
#define TFMINI_DEFAULT_ADDR  0x10

// 标准数据帧长度
#define TFMINI_FRAME_SIZE    9

// 触发测距命令长度
#define TFMINI_CMD_SIZE      5

// 帧头标识
#define TFMINI_HEADER1       0x59
#define TFMINI_HEADER2       0x59

// 触发后等待测距完成的 tick 数 (3ms @ 1kHz FreeRTOS tick)
#define TFMINI_WAIT_TICKS    3

// TFmini-S 激光测距 (I2C 接口)
class DrvTFMini
{
public:
    // port: 板级逻辑 I2C 端口, dev_addr: 7位设备地址 (board 层内部左移)
    DrvTFMini(BoardI2cPort port, uint8_t dev_addr = TFMINI_DEFAULT_ADDR);

    // 上电后传感器自动初始化, 此处仅复位内部状态
    void init(void);

    // 获取距离值 (发送触发命令后读取), 失败返回 -1
    int16_t getDistance(void);

    // 获取信号强度, 失败返回 -1
    int16_t getStrength(void);

    // 一次 I2C 读取获取距离与信号强度, 失败返回 false
    bool getData(int16_t &distance, int16_t &strength);

    /* ===== 简易避障状态机(迟滞/去抖/无效值保持) ===== */

    // 配置避障参数 (进入/退出阈值需 exit>enter 形成迟滞带)
    void configObstacleAvoidance(float enter_cm, float exit_cm,
                                 uint32_t exit_safe_need,
                                 uint32_t invalid_hold_max);

    // 周期调用: 内部读取测距并更新避障状态机, 返回 true 表示当前处于避障状态
    bool updateObstacleAvoidance(void);

    bool isObstacleAvoiding(void) const { return oa_active; }
    float getLastValidDistance(void) const { return last_valid_distance; }

private:
    enum State {
        IDLE,       // 空闲, 下次调用时发送触发命令
        WAITING,    // 已发送触发命令, 等待测距完成
    };

    bool sendTrigger(void);   // 发送触发测距命令
    bool readFrame(void);     // 读取一帧数据并校验 (非阻塞)

    BoardI2cPort m_port;
    uint8_t      m_dev_addr;
    uint8_t      frame[TFMINI_FRAME_SIZE];

    State    state;
    uint32_t triggerTick;

    int16_t distance;
    int16_t strength;

    /* ---- 避障状态机私有成员 ---- */
    float    oa_enter_dist;
    float    oa_exit_dist;
    uint32_t oa_exit_safe_need;
    uint32_t oa_invalid_hold_max;
    float    last_valid_distance;
    uint32_t invalid_read_cnt;
    uint32_t safe_cnt;
    bool     oa_active;
};

void init_drv_tfmini();
DrvTFMini *drv_tfmini();
