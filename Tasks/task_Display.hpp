#pragma once

#include "cmsis_os2.h"

// 显示任务回调函数
void StartDisplayTask(void *argument);

// 显示任务句柄（由 taskManager 创建后赋值）
extern osThreadId_t displayTaskHandle;
