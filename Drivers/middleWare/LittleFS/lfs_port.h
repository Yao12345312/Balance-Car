#pragma once

#include "FreeRTOS.h"

//禁用默认malloc，改用FreeRTOS的pvPortMalloc/vPortFree（heap_4会合并相邻块，防止产生内存碎片）
#define LFS_MALLOC(sz) pvPortMalloc(sz)
#define LFS_FREE(p)    vPortFree(p)
