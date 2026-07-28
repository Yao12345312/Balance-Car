#pragma once

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"

/* 任务注册表最大容量（用于空闲钩子周期性检查栈剩余量） */
#ifndef TASK_MANAGER_MAX_TASKS
#define TASK_MANAGER_MAX_TASKS 8
#endif

/* 栈剩余预警阈值（单位：字节，由 osThreadGetStackSpace 返回）。
 * 低于该值说明栈即将耗尽，记为故障 */
#ifndef TASK_MANAGER_STACK_WARN_BYTES
#define TASK_MANAGER_STACK_WARN_BYTES 256
#endif

/* 系统故障类型 */
typedef enum {
    FAULT_NONE = 0,
    FAULT_TASK_CREATE_FAILED,   /* 任务创建失败（堆不足/参数非法） */
    FAULT_STACK_OVERFLOW,       /* 栈溢出或接近溢出 */
    FAULT_MALLOC_FAILED,        /* FreeRTOS 堆分配失败 */
} SystemFault_t;

/* 故障记录（仅保留首次故障，供事后读取） */
typedef struct {
    SystemFault_t type;
    char          task_name[configMAX_TASK_NAME_LEN];
    uint32_t      tick;
} FaultRecord_t;

/**
 * 统一的任务创建入口：自动填充 osThreadAttr_t、创建任务、登记到注册表。
 * @param name       任务名（<= configMAX_TASK_NAME_LEN）
 * @param stack_size 栈大小（字节）
 * @param priority   优先级
 * @param entry      任务回调函数（签名 void(*)(void*)）
 * @param argument   传给回调的参数，可为 nullptr
 * @return 任务句柄；失败返回 NULL，并记录 FAULT_TASK_CREATE_FAILED
 */
osThreadId_t taskManager_createTask(const char *name,
                                    uint32_t stack_size,
                                    osPriority_t priority,
                                    osThreadFunc_t entry,
                                    void *argument = nullptr);

/* 读取最近一次故障记录；无故障返回 NULL */
const FaultRecord_t *taskManager_getLastFault(void);

/* 是否已发生故障 */
bool taskManager_hasFault(void);
