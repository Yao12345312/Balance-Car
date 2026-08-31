#include "basic.hpp"
#include "stm32h7xx_hal.h"
#include "FreeRTOS.h"

// =============================================================================
// STM32H743 内存区域 MPU 配置
//
//   0x08000000  BANK1 Flash 1MB  (Bootloader 领地)     → 只读 Write-Back
//   0x08100000  BANK2 Flash 512KB(App 代码+常量)       → 只读 Write-Back
//   0x081E0000  参数扇区 128KB (参数固化区)            → 读写 Write-Through
//   0x20000000  DTCM     128KB  (CPU 专用, DMA 不可达) → Write-Through 可缓存
//   0x24000000  AXI SRAM 512KB  (DMA 缓冲区所在)       → Non-cacheable
//   0x40000000+ 外设              (PRIVDEFENA 背景区域) → Device 默认属性
//
// 冷启动时 I-Cache/D-Cache/MPU 均处于 DISABLED, 无需先 disable.
// AXI SRAM 设为不可缓存, 消除 DMA 一致性问题.
// 参数扇区必须可写: H7 的 256-bit Flash 编程机制是 CPU 向 Flash 地址直接
// store 8 个字, 只读属性会在 HAL_FLASH_Program 内触发 MemManage/HardFault;
// 且必须 Write-Through (非 Write-Back), 否则 store 驻留 D-cache 脏行,
// 到不了 Flash 控制器导致编程失效.
// =============================================================================

static void config_mpu_region(uint8_t number, uint32_t addr, uint8_t size,
                              uint8_t tex, uint8_t access, uint8_t exec,
                              uint8_t shareable, uint8_t cacheable, uint8_t bufferable)
{
    MPU_Region_InitTypeDef mpu = {0};
    mpu.Enable           = MPU_REGION_ENABLE;
    mpu.Number           = number;
    mpu.BaseAddress      = addr;
    mpu.Size             = size;
    mpu.SubRegionDisable = 0x00;
    mpu.TypeExtField     = tex;
    mpu.AccessPermission = access;
    mpu.DisableExec      = exec;
    mpu.IsShareable      = shareable;
    mpu.IsCacheable      = cacheable;
    mpu.IsBufferable     = bufferable;
    HAL_MPU_ConfigRegion(&mpu);
}

void mpu_and_cache_init(void)
{
    // Region 0: BANK1 Flash (Bootloader 区) — Write-Back, 只读, 可执行
    config_mpu_region(
        MPU_REGION_NUMBER0,
        0x08000000,
        MPU_REGION_SIZE_1MB,
        MPU_TEX_LEVEL1,
        MPU_REGION_PRIV_RO,
        MPU_INSTRUCTION_ACCESS_ENABLE,
        MPU_ACCESS_NOT_SHAREABLE,
        MPU_ACCESS_CACHEABLE,
        MPU_ACCESS_BUFFERABLE);

    // Region 1: DTCM — Write-Through, 可读写, 可执行
    config_mpu_region(
        MPU_REGION_NUMBER1,
        0x20000000,
        MPU_REGION_SIZE_128KB,
        MPU_TEX_LEVEL1,
        MPU_REGION_PRIV_RW,
        MPU_INSTRUCTION_ACCESS_ENABLE,
        MPU_ACCESS_NOT_SHAREABLE,
        MPU_ACCESS_CACHEABLE,
        MPU_ACCESS_NOT_BUFFERABLE);

    // Region 2: AXI SRAM — Non-cacheable, 可读写
    config_mpu_region(
        MPU_REGION_NUMBER2,
        0x24000000,
        MPU_REGION_SIZE_512KB,
        MPU_TEX_LEVEL1,
        MPU_REGION_PRIV_RW,
        MPU_INSTRUCTION_ACCESS_ENABLE,
        MPU_ACCESS_NOT_SHAREABLE,
        MPU_ACCESS_NOT_CACHEABLE,
        MPU_ACCESS_NOT_BUFFERABLE);

    // Region 3: BANK2 Flash App 代码区 — Write-Back, 只读, 可执行
    // (覆盖至 512KB; 超出部分走 PRIVDEFENA 背景默认映射)
    config_mpu_region(
        MPU_REGION_NUMBER3,
        0x08100000,
        MPU_REGION_SIZE_512KB,
        MPU_TEX_LEVEL1,
        MPU_REGION_PRIV_RO,
        MPU_INSTRUCTION_ACCESS_ENABLE,
        MPU_ACCESS_NOT_SHAREABLE,
        MPU_ACCESS_CACHEABLE,
        MPU_ACCESS_BUFFERABLE);

    // Region 4: 参数扇区 (0x081E0000, 128KB) — Write-Through, 可读写
    // 供 Basic/param_flash.cpp 固化参数, 仅此区域放开 Flash 写权限
    config_mpu_region(
        MPU_REGION_NUMBER4,
        0x081E0000,
        MPU_REGION_SIZE_128KB,
        MPU_TEX_LEVEL1,
        MPU_REGION_PRIV_RW,
        MPU_INSTRUCTION_ACCESS_ENABLE,
        MPU_ACCESS_NOT_SHAREABLE,
        MPU_ACCESS_CACHEABLE,
        MPU_ACCESS_NOT_BUFFERABLE);

    // 启用 MPU + PRIVDEFENA (特权模式可访问未配置区域, 如外设)
    MPU->CTRL = MPU_CTRL_ENABLE_Msk | MPU_PRIVILEGED_DEFAULT;
    __DSB();
    __ISB();

    // 启用 I-Cache / D-Cache
    SCB_EnableICache();
    SCB_EnableDCache();
}

// =============================================================================
// 全局 operator new / delete 重写
// 将 C++ 动态内存分配重定向到 FreeRTOS 的 pvPortMalloc / vPortFree
// 统一使用 FreeRTOS heap_4 (configTOTAL_HEAP_SIZE = 64KB)
// =============================================================================
void *operator new(size_t size)
{
    return pvPortMalloc(size);
}

void *operator new[](size_t size)
{
    return pvPortMalloc(size);
}

void operator delete(void *p) noexcept
{
    vPortFree(p);
}

void operator delete[](void *p) noexcept
{
    vPortFree(p);
}
