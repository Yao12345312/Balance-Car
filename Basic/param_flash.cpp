#include "param_flash.hpp"
#include "param.hpp"

#include "stm32h7xx_hal.h"

#include <string.h>

// =============================================================================
// 记录格式: 32B 头 (恰好 1 个 256-bit flashword) + 224B payload
// 头部字段自然对齐无填充, sizeof 必须为 32
// =============================================================================
struct ParamRecordHeader
{
    uint32_t magic;       // 0x50415241 'PARA'
    uint16_t version;     // 参数布局版本 (PARAM_FLASH_VERSION)
    uint16_t count;       // 写入时参数表条数 (信息用)
    uint32_t seq;         // 递增序号
    uint32_t crc;         // payload CRC32
    uint32_t reserved[4]; // 保留 (0xFF)
};

static constexpr uint32_t kPayloadSize = PARAM_RECORD_SIZE - sizeof(ParamRecordHeader);

struct ParamRecord
{
    ParamRecordHeader hdr;
    uint8_t payload[kPayloadSize];
};

static_assert(sizeof(ParamRecordHeader) == 32, "header must be exactly 1 flashword");
static_assert(sizeof(ParamRecord) == PARAM_RECORD_SIZE, "record size mismatch");
static_assert(PARAM_RECORD_SIZE * PARAM_RECORD_COUNT == 128U * 1024U, "sector not fully used");
static_assert(sizeof(Params) <= kPayloadSize, "Params must fit in record payload");

namespace {

const uint32_t kMagic = 0x50415241UL;

// 运行状态 (param_load_from_flash 初始化)
uint32_t s_write_cursor  = 0;      // 下一个待写记录索引
uint32_t s_seq           = 0;      // 最后一条记录序号
bool     s_erase_pending = false;  // 扇区满, 等待停机擦除
uint8_t  s_fail_count    = 0;      // 连续擦/写失败计数 (Flash 损坏保护)

// 编程数据缓冲: H7 的 256-bit 编程要求数据源可被 Flash 接口访问,
// static 数组落 .bss (AXI SRAM), 不可使用 DTCM 上的临时数据
uint32_t s_prog_buf[PARAM_RECORD_SIZE / 4U];

const ParamRecord *record_at(uint32_t idx)
{
    return reinterpret_cast<const ParamRecord *>(PARAM_FLASH_BASE + idx * PARAM_RECORD_SIZE);
}

uint32_t crc32_calc(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFUL;
    for (uint32_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint32_t j = 0; j < 8U; j++)
        {
            uint32_t mask = 0UL - (crc & 1UL);
            crc = (crc >> 1) ^ (0xEDB88320UL & mask);
        }
    }
    return crc;
}

bool record_valid(const ParamRecord *r)
{
    return r->hdr.magic   == kMagic &&
           r->hdr.version == PARAM_FLASH_VERSION &&
           r->hdr.crc     == crc32_calc(r->payload, kPayloadSize);
}

bool record_slot_empty(const ParamRecord *r)
{
    const uint32_t *w = reinterpret_cast<const uint32_t *>(r);
    for (uint32_t i = 0; i < PARAM_RECORD_SIZE / 4U; i++)
        if (w[i] != 0xFFFFFFFFUL)
            return false;
    return true;
}

// 扫描参数扇区: 返回最后一条有效记录索引 (-1 = 无), free_slot 输出首个空槽
// 记录严格顺序追加, 首个空槽之后不会再有有效记录
// magic 存在但 CRC 校验不过 ， 掉电时写了一半的记录, 跳过
int32_t scan_records(uint32_t &free_slot)
{
    int32_t  last_valid = -1;
    uint32_t i = 0;
    for (; i < PARAM_RECORD_COUNT; i++)
    {
        const ParamRecord *r = record_at(i);
        if (record_slot_empty(r))
            break;
        if (record_valid(r))
            last_valid = (int32_t)i;
    }
    free_slot = i;
    return last_valid;
}

// Flash 区域为 Write-Back Cacheable, 擦/写后必须失效 D-Cache 再读
void invalidate_param_region(void)
{
    SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t *>(PARAM_FLASH_BASE),
                                 PARAM_RECORD_COUNT * PARAM_RECORD_SIZE);
}

bool flash_program_record(const ParamRecord *slot)
{
    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG_BANK2(FLASH_FLAG_ALL_ERRORS_BANK2);

    HAL_StatusTypeDef st = HAL_OK;
    for (uint32_t w = 0; w < PARAM_RECORD_SIZE / 32U && st == HAL_OK; w++)
    {
        st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                               reinterpret_cast<uint32_t>(slot) + w * 32U,
                               reinterpret_cast<uint32_t>(&s_prog_buf[w * 8U]));
    }

    HAL_FLASH_Lock();
    invalidate_param_region();
    return st == HAL_OK;
}

bool flash_erase_param_sector(void)
{
    FLASH_EraseInitTypeDef erase;
    uint32_t sector_error = 0;

    erase.TypeErase  = FLASH_TYPEERASE_SECTORS;
    erase.Banks      = FLASH_BANK_2;
    erase.Sector     = FLASH_SECTOR_7;
    erase.NbSectors  = 1;

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG_BANK2(FLASH_FLAG_ALL_ERRORS_BANK2);

    HAL_StatusTypeDef st = HAL_FLASHEx_Erase(&erase, &sector_error);

    HAL_FLASH_Lock();
    invalidate_param_region();

    return st == HAL_OK && sector_error == 0xFFFFFFFFUL;
}

// 填充编程缓冲并写入 slot, 回读校验
bool write_record(const ParamRecord *slot)
{
    memset(s_prog_buf, 0xFF, sizeof(s_prog_buf));

    ParamRecord *rec = reinterpret_cast<ParamRecord *>(s_prog_buf);
    rec->hdr.magic   = kMagic;
    rec->hdr.version = PARAM_FLASH_VERSION;
    rec->hdr.count   = param_count();
    rec->hdr.seq     = ++s_seq;
    memcpy(rec->payload, &g_params, sizeof(Params));
    rec->hdr.crc     = crc32_calc(rec->payload, kPayloadSize);

    if (!flash_program_record(slot))
        return false;

    return memcmp(slot, s_prog_buf, PARAM_RECORD_SIZE) == 0;
}

} // namespace

// =============================================================================
// 公开接口
// =============================================================================

void param_load_from_flash(void)
{
    invalidate_param_region();   // 上电首次访问, 清除可能的 cache 残留

    uint32_t free_slot;
    int32_t last = scan_records(free_slot);

    s_write_cursor  = free_slot;
    s_erase_pending = (free_slot >= PARAM_RECORD_COUNT);

    if (last < 0)
        return;   // 无有效记录/版本不匹配 → 保持编译期默认值

    const ParamRecord *r = record_at((uint32_t)last);
    s_seq = r->hdr.seq;
    memcpy(&g_params, r->payload, sizeof(Params));

    // g_eso_dirty 上电默认 true, LESO 首次 update 会重算 beta, 无需额外处理
}

bool param_save_to_flash(void)
{
    if (s_fail_count >= 3U)
        return false;   // Flash 疑似损坏, 停止重试 (参数保留在 RAM)

    if (s_erase_pending || s_write_cursor >= PARAM_RECORD_COUNT)
    {
        s_erase_pending = true;
        return false;
    }

    const ParamRecord *slot = record_at(s_write_cursor);
    if (!record_slot_empty(slot))
    {
        // 游标处非空 (异常残留数据) → 转擦除流程
        s_erase_pending = true;
        return false;
    }

    if (!write_record(slot))
    {
        s_erase_pending = true;
        s_fail_count++;
        return false;
    }

    s_fail_count = 0;
    s_write_cursor++;
    if (s_write_cursor >= PARAM_RECORD_COUNT)
        s_erase_pending = true;   // 本条已固化, 下一次修改前需停机擦除
    g_params_dirty = false;
    return true;
}

bool param_flash_needs_erase(void)
{
    return s_erase_pending;
}

bool param_flash_flush_erase(void)
{
    if (s_fail_count >= 3U)
        return false;

    if (!flash_erase_param_sector())
    {
        s_fail_count++;
        return false;
    }

    s_write_cursor  = 0;
    s_erase_pending = false;

    if (!write_record(record_at(0)))   // 立即固化当前参数为首条记录
    {
        s_fail_count++;
        s_erase_pending = true;
        return false;
    }

    s_fail_count   = 0;
    g_params_dirty = false;
    return true;
}

void param_reset_to_default(void)
{
    memcpy(&g_params, &k_param_defaults, sizeof(Params));
    g_eso_dirty = true;   // 默认 ESO 带宽可能与当前不同, 强制重算 beta

    if (flash_erase_param_sector())
    {
        s_write_cursor  = 0;
        s_erase_pending = false;
        if (write_record(record_at(0)))
            g_params_dirty = false;
        // 写失败: g_params_dirty 保持, 通信任务调度自动重试
    }
    else
    {
        s_fail_count++;
        s_erase_pending = true;
    }
}
