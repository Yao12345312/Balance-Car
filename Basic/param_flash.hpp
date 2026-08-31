#pragma once

#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// 参数 Flash 固化 (Basic/param_flash.cpp)
//
// 存储区: BANK2 末扇区 (扇区7, 0x081E0000, 128KB), 主固件镜像仅占 BANK2 扇区 0-2
// 结构:   512 条 x 256B 记录顺序追加, 空槽为 0xFF; 写满 512 条才擦除一次
// 停顿:   追加写 ~ms 级 (运行中可接受); 128KB 扇区擦除 ~1s (仅停机模式执行)
// 掉电:   写一半的记录 CRC 校验不过, 自动回退上一条有效记录
// 版本:   Params 结构增删字段/改变语义时必须递增 PARAM_FLASH_VERSION, 旧记录作废
// =============================================================================

#define PARAM_FLASH_BASE        0x081E0000UL   // BANK2 扇区7 起始地址
#define PARAM_FLASH_BANK        2U             // Flash BANK2
#define PARAM_FLASH_SECTOR_NUM  7U             // BANK2 内扇区号
#define PARAM_RECORD_SIZE       256U           // 单条记录字节数 = 8 个 256-bit flashword
#define PARAM_RECORD_COUNT      512U           // 记录条数 (128KB / 256B)

// 参数布局版本 (与 Params 结构一一对应, 变更布局时递增)
#define PARAM_FLASH_VERSION     1U

// 上电加载 (main 早期, 内核启动前调用): 取 CRC 有效且最新的记录覆盖 g_params,
// 无有效记录/版本不匹配时保持编译期默认值
void param_load_from_flash(void);

// 追加固化当前 g_params (通信任务上下文调用):
// 成功返回 true 并清除 g_params_dirty; 扇区满/写失败返回 false 并置挂起擦除标志
bool param_save_to_flash(void);

// 查询参数扇区是否已写满, 等待停机后擦除
bool param_flash_needs_erase(void);

// 擦除参数扇区并立即固化当前 g_params (~1s CPU 停顿, 仅停机模式调用)
bool param_flash_flush_erase(void);

// 恢复出厂默认: g_params = k_param_defaults 并擦扇区固化 (~1s 停顿, 仅停机模式调用)
void param_reset_to_default(void);
