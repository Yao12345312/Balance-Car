#include "drv_Main.hpp"

#include "hardware/drv_spi.hpp"
#include "hardware/drv_i2c.hpp"
#include "hardware/drv_uart.hpp"
#include "hardware/drv_can.hpp"
#include "hardware/drv_pwm.hpp"
#include "hardware/drv_common.hpp"
#include "sensors/drv_BMI088.hpp"
#include "sensors/drv_ICM20948.hpp"
#include "sensors/drv_TFMini.hpp"
#include "sensors/drv_US100.hpp"
#include "ble/drv_KT6368A.hpp"
#include "buzzer/drv_buzzer.hpp"
#include "esc/drv_CubeFOC.hpp"
#include "oled/drv_oled.hpp"

#include "taskManager.hpp"
#include "task_Control.hpp"
#include "task_Communication.hpp"
#include "task_Display.hpp"

#include "board.hpp"

//任务句柄定义
osThreadId_t controlTaskHandle       = NULL;
osThreadId_t communicationTaskHandle = NULL;
osThreadId_t displayTaskHandle       = NULL;

void init_drv_Main()
{	
	osDelay(2000);
	
	//初始化DMA
    HW_InitCommon();
	
	//外设初始化
    init_drv_spi();
    init_drv_i2c();
    init_drv_uart();
    init_drv_can();
    init_drv_pwm();
	
	//CS GPIO初始化
    board_cs_init(BOARD_CS_ICM20948);
	
	//IMU初始化
    init_drv_icm20948();

    //激光测距初始化
    init_drv_tfmini();

    //超声波测距初始化
    init_drv_us100();

	//蓝牙初始化
    init_drv_bluetooth();

	//电调初始化
    init_drv_cubefoc();

	//蜂鸣器初始化
    init_drv_buzzer();
	
	//OLED屏幕初始化
	init_drv_oled();
}

void create_application_tasks(void)
{
	//控制任务（10ms周期）
    controlTaskHandle = taskManager_createTask(
        "ControlTask", 6 * 1024, (osPriority_t)osPriorityHigh, StartControlTask);

	//通信任务（50ms周期）
    communicationTaskHandle = taskManager_createTask(
        "CommunicationTask", 12 * 1024, (osPriority_t)osPriorityNormal, StartCommunicationTask);

	//显示任务（50ms周期）
    displayTaskHandle = taskManager_createTask(
        "DisplayTask", 8 * 1024, (osPriority_t)osPriorityNormal, StartDisplayTask);
}
