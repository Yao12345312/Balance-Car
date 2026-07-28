#pragma once

#include "board.hpp"
#include "cmsis_os2.h"
#include <stdint.h>

// US-100 触发测距命令 (返回2字节距离, 单位mm)
#define US100_TRIGGER_DIST_CMD  0x55
// US-100 触发温度命令 (返回1字节温度, 摄氏度 = 值 - 45)
#define US100_TRIGGER_TEMP_CMD  0x50

// 距离回帧字节数 (高字节 + 低字节)
#define US100_DIST_FRAME_SIZE   2
// 温度回帧字节数
#define US100_TEMP_FRAME_SIZE   1

// 触发后等待回波的 tick 数 (覆盖最大量程回波时间 ~26ms + 串口传输余量, 1kHz tick)
#define US100_DIST_WAIT_TICKS   30U
// 温度回帧较快
#define US100_TEMP_WAIT_TICKS   5U
// 整体超时 (tick): 超过则复位状态机
#define US100_RX_TIMEOUT_TICKS  100U

// 有效距离量程 (mm), 超出视为无效
#define US100_DIST_MIN_MM       2
#define US100_DIST_MAX_MM       4500

// US-100 超声波测距 (UART 接口)
class DrvUS100
{
public:
    DrvUS100(BoardUartPort port);

    // 复位内部状态机 (模块上电后自动就绪, 无需额外配置)
    void init(void);

    // 获取距离值 (发送0x55触发后读取), 失败/未就绪/超量程返回 -1
    // 非阻塞: 触发当次返回 -1, 回帧接收完成后下次返回有效值
    int16_t getDistance(void);

    // 获取温度 (发送0x50触发后读取), 失败返回 -999
    int16_t getTemperature(void);

    /* ===== 避障状态机(迟滞/去抖/无效值保持) ===== */

    // 配置避障参数 (进入/退出阈值需 exit>enter 形成迟滞带)
    void configObstacleAvoidance(float enter_mm, float exit_mm,
                                 uint32_t exit_safe_need,
                                 uint32_t invalid_hold_max);

    // 周期调用: 内部读取测距并更新避障状态机, 返回 true 表示当前处于避障状态
    bool updateObstacleAvoidance(void);

    bool isObstacleAvoiding(void) const { return oa_active; }
    float getLastValidDistance(void) const { return last_valid_distance; }

private:
    enum Mode {
        MODE_IDLE,       // 空闲
        MODE_DIST,       // 测距流程中
        MODE_TEMP,       // 测温流程中
    };

    bool sendDistTrigger(void);   // 发送测距触发命令
    bool sendTempTrigger(void);   // 发送温度触发命令
    bool readDistFrame(void);     // 非阻塞读取距离帧
    bool readTempFrame(void);     // 非阻塞读取温度帧

    BoardUartPort m_port;
    uint8_t frame[US100_DIST_FRAME_SIZE];

    Mode     mode;
    uint32_t triggerTick;

    int16_t distance;
    int16_t temperature;

    /* ---- 避障状态机私有成员 ---- */
    float    oa_enter_dist;       // 进入阈值(mm)
    float    oa_exit_dist;        // 退出阈值(mm, 迟滞)
    uint32_t oa_exit_safe_need;   // 退出去抖次数
    uint32_t oa_invalid_hold_max; // 无效值保持上限
    float    last_valid_distance; // 最近有效距离(mm)
    uint32_t invalid_read_cnt;    // 连续无效计数
    uint32_t safe_cnt;            // 连续安全计数
    bool     oa_active;           // 避障激活标志
};

void init_drv_us100();
DrvUS100 *drv_us100();
