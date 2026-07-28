#pragma once

#include "cmsis_os2.h"

// 通信任务回调函数
void StartCommunicationTask(void *argument);

// 通信任务句柄（由 taskManager 创建后赋值）
extern osThreadId_t communicationTaskHandle;
