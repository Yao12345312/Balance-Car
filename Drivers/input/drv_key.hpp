#pragma once

#include "board.hpp"
#include "hardware/drv_common.hpp"
#include "cmsis_os2.h"
#include <stdint.h>

// 按键驱动 (GPIO 输入 + 软件消抖 + 短按/长按事件检测)
// 需在固定周期任务中调用 update() 进行状态扫描 
class DrvKey
{
public:
    enum class Event
    {
        None,
        ShortPress,
        LongPress
    };

    // 按下时的有效电平
    enum class ActiveLevel
    {
        High,   // 按下为高电平 
        Low     // 按下为低电平 
    };
	
	//构造函数
    DrvKey(const HwPin *pin,
           ActiveLevel activeLevel = ActiveLevel::High,
           uint32_t longPressTime = 1000,
           uint32_t debounceTime = 20,
           uint32_t pull = GPIO_PULLDOWN);

	//析构函数
    ~DrvKey();

    // 配置 GPIO 输入并读取初始电平, 返回 false 表示引脚无效
    bool init();

    // 周期扫描按键状态, 在固定周期任务中调用
    void update();

    // 获取按键事件, 读取后自动清除
    Event getEvent();

    // 当前稳定状态是否处于按下
    bool isPressed() const;

    // 当前物理电平 (实时)
    GPIO_PinState getPhyState() const { return m_lastPhyState; }

private:
    // 判断指定电平是否为有效 (按下) 状态
    bool isActive(GPIO_PinState state) const;

    // 计算 tick 差值 (处理溢出回绕)
    uint32_t ticksDiff(uint32_t tick1, uint32_t tick2) const;

    // 毫秒转换为系统 tick
    uint32_t msToTicks(uint32_t ms) const;

    HwPin         m_hwpin;
    GPIO_TypeDef *m_port;
    uint16_t      m_pin;
    ActiveLevel   m_activeLevel;
    uint32_t      m_longPressTime;   // 长按阈值(ms)
    uint32_t      m_debounceTime;    // 消抖时间(ms)
    uint32_t      m_pull;            // GPIO 上/下拉

    GPIO_PinState m_lastPhyState;    // 上一次物理电平
    GPIO_PinState m_stableState;     // 消抖后稳定电平
    uint32_t      m_lastDebounceTick;// 上次电平变化时刻
    uint32_t      m_pressStartTick;  // 按下起始时刻

    bool          m_longPressTriggered; // 是否已触发长按
    Event         m_event;              // 当前事件

    osMutexId_t   m_mutex;              // 互斥锁
};

// =============================================================================
// 板级按键配置 (BoardKey 枚举定义在 board.hpp, 配置表定义在 board.cpp)
// =============================================================================

// 单个按键配置
typedef struct
{
    HwPin              pin;
    DrvKey::ActiveLevel activeLevel;
    uint32_t           longPressTime;   // ms
    uint32_t           debounceTime;    // ms
    uint32_t           pull;            // GPIO 上/下拉
} KeyConfig;

// 板级按键配置表 (由 board.cpp 提供, 按 BoardKey 枚举索引)
extern const KeyConfig board_key_configs[BOARD_KEY_COUNT];

// 初始化全部板级按键 
void init_drv_key();

// 按逻辑端口获取按键实例 (未初始化或越界返回 NULL)
DrvKey *drv_key(BoardKey key);
