#include "task_Communication.hpp"
#include "ble/drv_KT6368A.hpp"
#include "drv_buzzer.hpp"
#include "mavlink.hpp"
#include "board.hpp"
#include "task_Control.hpp"

void StartCommunicationTask(void *argument)
{
    auto *ble = bluetooth_driver();
	
	auto *buzzer = drv_buzzer();
	
    // 蓝牙扫频初始化函数
    if (!ble->autoBaudScan()) {
        while (1);
    }

    // 初始化 MAVLink 解析器
    MAVLink::Init();
	
    uint32_t next_wake = osKernelGetTickCount();

	buzzer->beep(500 ,500);
	
    while (1)
    {	
		
        // 接收并解析上位机数据 
        uint8_t buf[128];
        uint16_t n = board_uart_read(BOARD_UART_BT, buf, sizeof(buf), 0.01, 0.00);
        if (n > 0)
            MAVLink::ParseData(buf, n);

        // 从控制任务队列取传感数据, 有则上报姿态 + 电池
        if (g_mavSensorQueue != NULL)
        {
            MavSensorData_t data;
            if (osMessageQueueGet(g_mavSensorQueue, &data, NULL, 0) == osOK)
            {
                MAVLink::SendAttitude(
                    data.roll, data.pitch, data.yaw,
                    data.rollspeed, data.pitchspeed, data.yawspeed);

                MAVLink::SendBatteryStatus(
                    data.voltage, data.current, data.battery_remaining);
            }
        }

        MAVLink::SendHeartbeat();

        // 推进参数流式广播 (PARAM_REQUEST_LIST 触发, 每周期发 2 帧)
        MAVLink::ParamStreamTick();

        next_wake += 50U;
        osDelayUntil(next_wake);
    }
}
