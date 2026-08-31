#include "board.hpp"
#include "drv_Main.hpp"
#include "param_flash.hpp"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"

//初始化任务对象句柄
osThreadId_t initTaskHandle;
//线程描述结构体
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 6 * 1024,
  .priority = (osPriority_t) osPriorityNormal,
};

//驱动初始化任务
void DriverInitTask(void * argument)
{	
	//等待硬件状态稳定
	osDelay(2000);
	
	//初始化硬件驱动
	init_drv_Main();
	
	//初始化应用程序
	create_application_tasks();
	
	//删除初始化任务
	vTaskDelete(NULL);
}

int main(void)
{
	//板级初始化
	board_init();

	//从 Flash 参数区加载上次固化的参数 (无有效记录保持编译期默认值)
	param_load_from_flash();
	
	//OS内核初始化
	osKernelInitialize();
	
	//创建初始化任务
	initTaskHandle = osThreadNew(DriverInitTask, NULL, &defaultTask_attributes);
	
	if (initTaskHandle == NULL)
	{
		while(1);
	}   
     
	//启动任务调度器
    osKernelStart();

	while(1);
}


