#include "taskManager.hpp"
#include <cstring>
#include "stm32h7xx.h"

/* ===== 内部任务注册表 ===== */
typedef struct {
    osThreadId_t handle;
    const char  *name;
} TaskEntry_t;

static TaskEntry_t s_registry[TASK_MANAGER_MAX_TASKS];
static uint8_t     s_registry_count = 0;

/* ===== 故障记录 ===== */
static FaultRecord_t s_last_fault;
static volatile bool s_fault_flag = false;

static void record_fault(SystemFault_t type, const char *task_name)
{
    if (s_fault_flag) return;

    s_fault_flag       = true;
    s_last_fault.type  = type;
    s_last_fault.tick  = xTaskGetTickCount();

    if (task_name) {
        strncpy(s_last_fault.task_name, task_name, configMAX_TASK_NAME_LEN - 1);
        s_last_fault.task_name[configMAX_TASK_NAME_LEN - 1] = '\0';
    } else {
        s_last_fault.task_name[0] = '\0';
    }
}

/* ===== 统一任务创建 ===== */
osThreadId_t taskManager_createTask(const char *name,
                                    uint32_t stack_size,
                                    osPriority_t priority,
                                    osThreadFunc_t entry,
                                    void *argument)
{
    osThreadAttr_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.name       = name;
    attr.stack_size = stack_size;
    attr.priority   = priority;

    osThreadId_t handle = osThreadNew(entry, argument, &attr);
    if (handle == NULL) {
        record_fault(FAULT_TASK_CREATE_FAILED, name);
        return NULL;
    }

    if (s_registry_count < TASK_MANAGER_MAX_TASKS) {
        s_registry[s_registry_count].handle = handle;
        s_registry[s_registry_count].name   = name;
        s_registry_count++;
    }

    return handle;
}

const FaultRecord_t *taskManager_getLastFault(void)
{
    return s_fault_flag ? &s_last_fault : NULL;
}

bool taskManager_hasFault(void)
{
    return s_fault_flag;
}


/* 栈溢出：致命错误，状态已不可信，关中断后死循环便于调试器捕获 */
extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    record_fault(FAULT_STACK_OVERFLOW, pcTaskName);
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}

/* 堆分配失败：通常可恢复（如释放后重试），此处仅记录不陷死 */
extern "C" void vApplicationMallocFailedHook(void)
{
    record_fault(FAULT_MALLOC_FAILED, NULL);
}

/* 空闲钩子：周期性巡检各任务栈剩余量 + 进入低功耗等待中断 */
extern "C" void vApplicationIdleHook(void)
{
    static uint32_t last_check = 0;
    uint32_t now = xTaskGetTickCount();

    if ((now - last_check) >= 1000) {
        last_check = now;
        for (uint8_t i = 0; i < s_registry_count; i++) {
            if (s_registry[i].handle == NULL) continue;
			uint32_t free_bytes = osThreadGetStackSpace(s_registry[i].handle);
				if (free_bytes < TASK_MANAGER_STACK_WARN_BYTES) {
                record_fault(FAULT_STACK_OVERFLOW, s_registry[i].name);
            }
        }
    }
	//进入低功耗模式会导致USB设备描述符请求失败，这里注释掉
    //__WFI();
}
