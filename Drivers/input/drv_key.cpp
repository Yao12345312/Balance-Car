#include "drv_key.hpp"

DrvKey::DrvKey(const HwPin *pin,
               ActiveLevel activeLevel,
               uint32_t longPressTime,
               uint32_t debounceTime,
               uint32_t pull)
    : m_hwpin({0, 0, 0})
    , m_port(NULL)
    , m_pin(0)
    , m_activeLevel(activeLevel)
    , m_longPressTime(longPressTime)
    , m_debounceTime(debounceTime)
    , m_pull(pull)
    , m_lastPhyState(GPIO_PIN_RESET)
    , m_stableState(GPIO_PIN_RESET)
    , m_lastDebounceTick(0)
    , m_pressStartTick(0)
    , m_longPressTriggered(false)
    , m_event(Event::None)
    , m_mutex(NULL)
{
    if (pin && pin->port <= 10)
    {
        m_hwpin = *pin;
        m_port = HW_GpioTable[pin->port];
        m_pin = (uint16_t)(1u << pin->pin);
    }

    m_mutex = osMutexNew(NULL);
}

DrvKey::~DrvKey()
{
    if (m_mutex != NULL)
    {
        osMutexDelete(m_mutex);
        m_mutex = NULL;
    }
}

uint32_t DrvKey::ticksDiff(uint32_t tick1, uint32_t tick2) const
{
    if (tick1 >= tick2)
        return tick1 - tick2;
    // 处理 32 位 tick 回绕
    return (UINT32_MAX - tick2 + 1u) + tick1;
}

uint32_t DrvKey::msToTicks(uint32_t ms) const
{
    uint32_t freq = osKernelGetTickFreq();      // tick/秒
    if (freq == 0)
        return ms;
    return (ms * freq + 999u) / 1000u;
}

bool DrvKey::isActive(GPIO_PinState state) const
{
    return (m_activeLevel == ActiveLevel::High) ? (state == GPIO_PIN_SET) : (state == GPIO_PIN_RESET);
}

bool DrvKey::init()
{
    if (m_port == NULL || m_pin == 0)
        return false;

    // 配置 GPIO 为输入 (时钟/模式/上下拉由 HW_ConfigurePinInput 完成)
    HW_ConfigurePinInput(&m_hwpin, m_pull);

    // 读取初始电平
    GPIO_PinState initState = HAL_GPIO_ReadPin(m_port, m_pin);
    m_lastPhyState = initState;
    m_stableState = initState;
    m_lastDebounceTick = osKernelGetTickCount();

    return true;
}

void DrvKey::update()
{
    if (m_mutex != NULL)
        osMutexAcquire(m_mutex, osWaitForever);

    GPIO_PinState currentState = HAL_GPIO_ReadPin(m_port, m_pin);
    uint32_t now = osKernelGetTickCount();

    // 消抖处理
    if (currentState != m_lastPhyState)
    {
        m_lastDebounceTick = now;
        m_lastPhyState = currentState;
    }

    if (ticksDiff(now, m_lastDebounceTick) >= msToTicks(m_debounceTime))
    {
        // 状态已稳定
        if (m_stableState != currentState)
        {
            m_stableState = currentState;

            if (isActive(m_stableState))
            {
                // 按下
                m_pressStartTick = now;
                m_longPressTriggered = false;
            }
            else
            {
                // 释放
                if (!m_longPressTriggered)
                {
                    // 未触发过长按 -> 短按
                    m_event = Event::ShortPress;
                }
            }
        }
    }

    // 长按检测 (仅在稳定按下状态检测)
    if (isActive(m_stableState) && !m_longPressTriggered)
    {
        if (ticksDiff(now, m_pressStartTick) >= msToTicks(m_longPressTime))
        {
            m_longPressTriggered = true;
            m_event = Event::LongPress;
        }
    }

    if (m_mutex != NULL)
        osMutexRelease(m_mutex);
}

DrvKey::Event DrvKey::getEvent()
{
    Event e = Event::None;

    if (m_mutex != NULL)
        osMutexAcquire(m_mutex, osWaitForever);

    e = m_event;
    m_event = Event::None;      // 清事件

    if (m_mutex != NULL)
        osMutexRelease(m_mutex);

    return e;
}

bool DrvKey::isPressed() const
{
    return isActive(m_stableState);
}

// =============================================================================
// 全局访问 (板级配置表 board_key_configs 由 board.cpp 提供)
// =============================================================================

static DrvKey *g_drv_keys[BOARD_KEY_COUNT] = { NULL };

void init_drv_key()
{
    for (uint8_t i = (uint8_t)BOARD_KEY_NONE + 1; i < (uint8_t)BOARD_KEY_COUNT; i++)
    {
        if (g_drv_keys[i] != NULL)
            continue;

        const KeyConfig *cfg = &board_key_configs[i];
        if (cfg->pin.port == 0 && cfg->pin.pin == 0)
            continue;

        g_drv_keys[i] = new DrvKey(&cfg->pin, cfg->activeLevel,
                                   cfg->longPressTime, cfg->debounceTime, cfg->pull);
        if (g_drv_keys[i] != NULL)
            g_drv_keys[i]->init();
    }
}

DrvKey *drv_key(BoardKey key)
{
    if ((uint8_t)key <= (uint8_t)BOARD_KEY_NONE || (uint8_t)key >= (uint8_t)BOARD_KEY_COUNT)
        return NULL;
    return g_drv_keys[(uint8_t)key];
}
