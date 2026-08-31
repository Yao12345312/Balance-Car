#include "task_Communication.hpp"
#include "ble/drv_KT6368A.hpp"
#include "drv_buzzer.hpp"
#include "mavlink.hpp"
#include "board.hpp"
#include "task_Control.hpp"
#include "param.hpp"
#include "param_flash.hpp"

void StartCommunicationTask(void *argument)
{
    auto *ble = bluetooth_driver();
	
	auto *buzzer = drv_buzzer();

    // 蓝牙扫频初始化
    if (!ble->autoBaudScan()) {
		while(1);
    }

    // 初始化 MAVLink 解析器
    MAVLink::Init();

    uint32_t next_wake = osKernelGetTickCount();
	
    while (1)
    {	
		
        // 接收并解析上位机数据 
        uint8_t buf[128];
        uint16_t n = board_uart_read(BOARD_UART_BT, buf, sizeof(buf), 0.01, 0.00);
        if (n > 0)
            MAVLink::ParseData(buf, n);

        // LED 心跳监视 (收到心跳 1Hz 闪绿灯, 超时断连 1Hz 闪蓝灯)
        MAVLink::LedTick();

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

        // ===== 参数固化调度 =====
        // 修改后立即追加落盘 (追加写 ~ms 级停顿, 运行中安全);
        // 扇区写满时挂起, 等待停机状态 (g_control_mode == 0xFF) 再擦除 (~1s 停顿)
        if (g_params_dirty)
        {
            if (param_flash_needs_erase())
            {
                // 连续 2 个周期确认停机, 给控制任务时间发出电机零电流指令
                static uint8_t erase_idle_count = 0;
                if (g_control_mode == 0xFF)
                {
                    if (++erase_idle_count >= 2U)
                    {
                        erase_idle_count = 0;
                        param_flash_flush_erase();
                    }
                }
                else
                {
                    erase_idle_count = 0;
                }
            }
            else
            {
                param_save_to_flash();
            }
        }

        next_wake += 50U;
        osDelayUntil(next_wake);
    }
}
