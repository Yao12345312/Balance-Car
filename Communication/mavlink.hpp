#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "mavlink.h"

#ifdef __cplusplus
}
#endif

// MAVLink 发送缓冲区大小
#define MAVLINK_TX_BUF_LEN  128

namespace MAVLink {

void Init(void);

// 发送函数 
void SendHeartbeat(void);
void SendBatteryStatus(float voltage_v, float current_a, int8_t battery_remaining);
void SendAttitude(float roll, float pitch, float yaw,
                  float rollspeed, float pitchspeed, float yawspeed);

// 参数协议: 推进流式广播 (PARAM_REQUEST_LIST 触发, 每周期发若干帧 PARAM_VALUE)
void ParamStreamTick(void);

// 连接状态
bool get_mavlink_connect_status(void);
void set_mavlink_connect_status(bool status);

// 解析原始数据 (由接收任务调用)
void ParseData(const uint8_t* data, uint16_t len);

}
