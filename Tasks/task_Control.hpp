#pragma once

#include "cmsis_os2.h"

// 控制任务回调函数
void StartControlTask(void *argument);

// 控制任务句柄（由 taskManager 创建后赋值）
extern osThreadId_t controlTaskHandle;

// =============================================================================
// 跨任务共享数据 (移植自 balance-car-7_14 Math/attitute.hpp)
//
// 控制任务每 10 个周期 (20ms@500Hz) 打包一帧 MavSensorData 入队,
// 通信任务从队列取出后调 MAVLink::SendAttitude / SendBatteryStatus 上报。
// =============================================================================

// MAVLink 传感数据帧 (控制任务 -> 通信任务)
typedef struct
{
    float roll;
    float pitch;
    float yaw;
    float rollspeed;       // X 角速度 rad/s
    float pitchspeed;      // Y 角速度 rad/s
    float yawspeed;        // Z 角速度 rad/s
    float voltage;         // 电池电压 V (取 ESC 总线电压)
    float current;         // 电池电流 A, -1 = 未知
    int8_t battery_remaining; // 剩余百分比, 0 = 未知
} MavSensorData_t;

// 控制任务 -> 通信任务 的传感数据队列
extern osMessageQueueId_t g_mavSensorQueue;

// 控制模式切换信号量 (通信任务 release, 控制任务 acquire 0 即清)
extern osSemaphoreId_t g_controlModeSem;

// 遥控指令 (通信任务 ParseData 写, 控制任务读) - volatile 跨任务可见
extern volatile uint8_t g_rc_control_mode;   // 0=平衡 1=单点 4=轮速测试 0xFF=停机
extern volatile float   g_rc_speed_target;   // 速度环目标 (rad/s)
extern volatile float   g_rc_manual_y;       // 转向 (偏航角速度目标)
