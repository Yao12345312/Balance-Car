#include "mavlink.hpp"
#include "board.hpp"
#include "param.hpp"
#include "param_flash.hpp"
#include "task_Control.hpp"
#include "led/drv_led.hpp"

#include <cmsis_os2.h>
#include <string.h>
#include <stdio.h>

// MAVLink 外部缓冲区 
extern "C" {
mavlink_status_t m_mavlink_status[MAVLINK_COMM_NUM_BUFFERS];
mavlink_message_t m_mavlink_buffer[MAVLINK_COMM_NUM_BUFFERS];
}


namespace {

static const uint8_t mav_sysid = 1;
static const uint8_t mav_compid = 1;

static uint8_t       mav_tx_buf[MAVLINK_TX_BUF_LEN];
static volatile bool mavlink_connected = false;

static mavlink_status_t  mav_status;
static mavlink_message_t mav_msg;

static struct {
    uint32_t last_hb_time;
    uint8_t  link_active;
} mav_status_info = {0, 0};

static osMutexId_t mav_status_mutex;

// 参数流式广播状态机 (PARAM_REQUEST_LIST 触发)
static volatile bool    param_stream_active = false;
static volatile uint16_t param_stream_idx   = 0;

// 每周期最多发送的 PARAM_VALUE 帧数 (避免堵塞蓝牙, ~25B/帧)
#define PARAM_STREAM_CHUNK 2u

// 心跳超时阈值: 超过该时间未收到心跳即判定蓝牙断连 (上位机心跳约 1Hz)
#define MAVLINK_HEARTBEAT_TIMEOUT_MS 3000U

// LED 灯语状态 (仅状态变化时才切换闪烁, 避免反复重启闪烁任务)
enum LedLinkState {
    LED_LINK_UNKNOWN = 0,   // 未初始化
    LED_LINK_CONNECTED,     // 已连接 -> 闪绿灯
    LED_LINK_DISCONNECTED,  // 已断连 -> 闪蓝灯
};
static LedLinkState s_led_link_state = LED_LINK_UNKNOWN;

// 发送单条 PARAM_VALUE (按索引), 失败/越界返回 false
static bool sendParamValue(uint16_t index)
{
    if (index >= param_count())
        return false;

    mavlink_message_t msg;
    mavlink_msg_param_value_pack(mav_sysid, mav_compid, &msg,
                                 g_param_table[index].name,   // param_id (<=16)
                                 param_get(index),            // param_value
                                 g_param_table[index].type,   // param_type
                                 param_count(),               // param_count
                                 index);                      // param_index
    uint16_t len = mavlink_msg_to_send_buffer(mav_tx_buf, &msg);
    board_uart_write(BOARD_UART_BT, mav_tx_buf, len, 0.02, 0.02);
    return true;
}

}

namespace MAVLink {

void Init(void)
{
    mav_status_mutex = osMutexNew(NULL);

    memset(&mav_status, 0, sizeof(mav_status));
    memset(&mav_msg, 0, sizeof(mav_msg));

    mav_status_info.last_hb_time = osKernelGetTickCount();
    mav_status_info.link_active  = 0;
    mavlink_connected            = false;
}

void set_mavlink_connect_status(bool status)
{
    mavlink_connected = status;
}

bool get_mavlink_connect_status(void)
{
    return mavlink_connected;
}

void ParseData(const uint8_t *data, uint16_t len)
{
    if (!data || len == 0)
        return;

    for (uint16_t i = 0; i < len; i++) {
        if (mavlink_parse_char(MAVLINK_COMM_0, data[i], &mav_msg, &mav_status) != MAVLINK_FRAMING_OK)
            continue;

        switch (mav_msg.msgid) {

        case MAVLINK_MSG_ID_HEARTBEAT: {
            mavlink_heartbeat_t hb;
            mavlink_msg_heartbeat_decode(&mav_msg, &hb);

            if (mav_status_mutex)
                osMutexAcquire(mav_status_mutex, osWaitForever);

            mav_status_info.last_hb_time = osKernelGetTickCount();
            if (mav_status_info.link_active != 1) {
                mav_status_info.link_active = 1;
                set_mavlink_connect_status(true);

            }

            if (mav_status_mutex)
                osMutexRelease(mav_status_mutex);
            break;
        }

        case MAVLINK_MSG_ID_COMMAND_LONG: {
            mavlink_command_long_t cmd;
            mavlink_msg_command_long_decode(&mav_msg, &cmd);

            // 控制模式切换 (写权威模式字 g_control_mode, 最后写入者生效)
            if (cmd.command == 0x4101)       // 平衡模式
            {
                g_control_mode = 0;
                if (g_controlModeSem != NULL)
                    osSemaphoreRelease(g_controlModeSem);
            }
            else if (cmd.command == 0x4102)  // 单点保持
            {
                g_control_mode = 1;
                if (g_controlModeSem != NULL)
                    osSemaphoreRelease(g_controlModeSem);
            }
            else if (cmd.command == 0x4103)  // 断电停机
            {
                g_control_mode = 0xFF;
                if (g_controlModeSem != NULL)
                    osSemaphoreRelease(g_controlModeSem);
            }
            else if (cmd.command == 0x4104)  // 轮速测试模式
            {
                g_control_mode = 4;
                if (g_controlModeSem != NULL)
                    osSemaphoreRelease(g_controlModeSem);
            }
            else if (cmd.command == 0x4106)  // 恢复出厂参数
            {
                // 仅停机状态允许: 擦除参数扇区 (~1s) 会冻结控制环
                if (g_control_mode == 0xFF)
                    param_reset_to_default();
            }
            break;
        }

        case MAVLINK_MSG_ID_MANUAL_CONTROL: {
            mavlink_manual_control_t manual;
            mavlink_msg_manual_control_decode(&mav_msg, &manual);

            // Z 通道 -> 速度环目标 (rad/s), 限幅 ±20
            float x_val = (float)manual.z / 50.0f;
            if (x_val >  20.0f) x_val =  20.0f;
            if (x_val < -20.0f) x_val = -20.0f;
            g_rc_speed_target = x_val;

            // Y 通道 -> 转向 (偏航角速度目标), 限幅 ±4
            float y_val = -(float)manual.y / 250.0f;
            if (y_val >  4.0f) y_val =  4.0f;
            if (y_val < -4.0f) y_val = -4.0f;
            g_rc_manual_y = y_val;
            break;
        }

        // 上位机请求全表: 启动流式广播, 下个 ParamStreamTick() 开始回 PARAM_VALUE
        case MAVLINK_MSG_ID_PARAM_REQUEST_LIST: {
            param_stream_idx   = 0;
            param_stream_active = true;
            break;
        }

        // 上位机请求单个参数 (按 index 或 param_id 名)
        case MAVLINK_MSG_ID_PARAM_REQUEST_READ: {
            mavlink_param_request_read_t req;
            mavlink_msg_param_request_read_decode(&mav_msg, &req);

            int16_t idx = -1;
            if (req.param_index >= 0 && req.param_index < param_count())
                idx = (int16_t)req.param_index;
            else
                idx = param_find(req.param_id);   // 名字查找 (param_id[16] 已 '\0' 填充)

            if (idx >= 0)
                sendParamValue((uint16_t)idx);
            break;
        }

        // 上位机在线设参 -> 写 g_params -> 回 PARAM_VALUE 确认
        case MAVLINK_MSG_ID_PARAM_SET: {
            mavlink_param_set_t set;
            mavlink_msg_param_set_decode(&mav_msg, &set);

            int16_t idx = param_find(set.param_id);
            if (idx >= 0)
            {
                param_set((uint16_t)idx, set.param_value);   // 写入并触发派生标志
                sendParamValue((uint16_t)idx);               // 回当前实际值
            }
            break;
        }

        default:
            break;
        }
    }
}

void SendHeartbeat(void)
{
    static uint32_t last_send_tick = 0;
    uint32_t now = osKernelGetTickCount();
    if ((now - last_send_tick) < 1000U)
        return;
    last_send_tick = now;

    mavlink_message_t msg;
    uint8_t  type          = MAV_TYPE_QUADROTOR;
    uint8_t  autopilot     = MAV_AUTOPILOT_ARDUPILOTMEGA;
    uint8_t  base_mode     = MAV_MODE_FLAG_SAFETY_ARMED | MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
    uint32_t custom_mode   = 0;
    uint8_t  system_status = MAV_STATE_ACTIVE;

    mavlink_msg_heartbeat_pack(mav_sysid, mav_compid, &msg,
                               type, autopilot, base_mode, custom_mode, system_status);

    uint16_t len = mavlink_msg_to_send_buffer(mav_tx_buf, &msg);
    board_uart_write(BOARD_UART_BT, mav_tx_buf, len, 0.02, 0.02);
}

void SendBatteryStatus(float voltage_v, float current_a, int8_t battery_remaining)
{
    static uint32_t last_send_tick = 0;
    uint32_t now = osKernelGetTickCount();
    if ((now - last_send_tick) < 1000U)
        return;
    last_send_tick = now;

    mavlink_message_t msg;

    uint16_t voltages[10] = {0};
    voltages[0] = (uint16_t)(voltage_v * 1000.0f);

    int16_t current_battery = (int16_t)(current_a * 100.0f);

    uint16_t voltages_ext[4] = {0};

    mavlink_msg_battery_status_pack(
        mav_sysid, mav_compid, &msg,
        0,                                       // id
        MAV_BATTERY_FUNCTION_ALL,                // battery_function
        MAV_BATTERY_TYPE_LIPO,                   // type
        INT16_MAX,                               // temperature
        voltages,                                // voltages[10]
        current_battery,                         // current_battery
        -1,                                      // current_consumed
        -1,                                      // energy_consumed
        battery_remaining,                       // battery_remaining
        0,                                       // time_remaining
        MAV_BATTERY_CHARGE_STATE_UNDEFINED,      // charge_state
        voltages_ext,                            // voltages_ext[4]
        MAV_BATTERY_MODE_UNKNOWN,                // mode
        0                                        // fault_bitmask
    );

    uint16_t len = mavlink_msg_to_send_buffer(mav_tx_buf, &msg);
    board_uart_write(BOARD_UART_BT, mav_tx_buf, len, 0.02, 0.02);
}

void SendAttitude(float roll, float pitch, float yaw,
                  float rollspeed, float pitchspeed, float yawspeed)
{
    static uint32_t last_send_tick = 0;
    uint32_t now = osKernelGetTickCount();
    if ((now - last_send_tick) < 50U)
        return;
    last_send_tick = now;

    mavlink_message_t msg;
    uint32_t time_boot_ms = osKernelGetTickCount();

    mavlink_msg_attitude_pack(mav_sysid, mav_compid, &msg,
                              time_boot_ms,
                              roll, pitch, yaw,
                              rollspeed, pitchspeed, yawspeed);

    uint16_t len = mavlink_msg_to_send_buffer(mav_tx_buf, &msg);
    board_uart_write(BOARD_UART_BT, mav_tx_buf, len, 0.02, 0.02);
}

void ParamStreamTick(void)
{
    if (!param_stream_active)
        return;

    // 每周期发 PARAM_STREAM_CHUNK 帧, 全部参数约 ceil(N/2) 周期发完,
    for (uint16_t i = 0; i < PARAM_STREAM_CHUNK; i++)
    {
        if (param_stream_idx >= param_count())
        {
            param_stream_active = false;
            break;
        }
        sendParamValue(param_stream_idx);
        param_stream_idx++;
    }
}

void LedTick(void)
{
    uint32_t now = osKernelGetTickCount();
    bool connected;

    // 超时检测: 收到心跳后更新 last_hb_time, 超出阈值未收到则判为断连
    if (mav_status_mutex)
        osMutexAcquire(mav_status_mutex, osWaitForever);

    if (mav_status_info.link_active &&
        (now - mav_status_info.last_hb_time) > MAVLINK_HEARTBEAT_TIMEOUT_MS)
    {
        mav_status_info.link_active = 0;
        set_mavlink_connect_status(false);
    }
    connected = (mav_status_info.link_active != 0);

    if (mav_status_mutex)
        osMutexRelease(mav_status_mutex);

    // 根据连接状态切换 LED 灯语, 状态变化时才调用
    DrvLed *led = drv_led();
    if (!led)
        return;

    if (connected)
    {
        if (s_led_link_state != LED_LINK_CONNECTED)
        {
            led->setRGBBlink(0, 100, 0, 1);   // 绿灯 1Hz 闪烁
            s_led_link_state = LED_LINK_CONNECTED;
        }
    }
    else
    {
        if (s_led_link_state != LED_LINK_DISCONNECTED)
        {
            led->setRGBBlink(0, 0, 100, 1);   // 蓝灯 1Hz 闪烁
            s_led_link_state = LED_LINK_DISCONNECTED;
        }
    }
}

}
