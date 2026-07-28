#include "drv_led.hpp"

/* ================= 构造函数 ================= */

DrvLed::DrvLed(BoardPwmPort port)
    : m_port(port)
    , m_blinkTaskHandle(NULL)
    , m_blinkRed(0)
    , m_blinkGreen(0)
    , m_blinkBlue(0)
    , m_blinkHalfPeriodMs(0)
    , m_blinkRunning(false)
    , m_isInitialized(false)
{
}

/* ================= 百分比转占空比 ================= */

float DrvLed::percentToDuty(uint8_t percent)
{
    if (percent > 100) percent = 100;
    return (float)percent / 100.0f;
}

/* ================= 初始化 ================= */

bool DrvLed::init()
{
    if (m_isInitialized)
        return true;

    if (m_port == BOARD_PWM_NONE)
        return false;

    // 启动三路 PWM 通道 (位或一次传入)
    uint32_t channels = RED_CHANNEL | GREEN_CHANNEL | BLUE_CHANNEL;
    if (board_pwm_start(m_port, channels) != BOARD_OK)
        return false;

    turnOff();

    m_isInitialized = true;
    return true;
}

/* ================= 设置 RGB ================= */

void DrvLed::setRGB(uint8_t red, uint8_t green, uint8_t blue)
{
    if (!m_isInitialized) return;

    setRed(red);
    setGreen(green);
    setBlue(blue);
}

/* ================= 闪烁任务入口 (静态) ================= */

void DrvLed::blinkTaskFunc(void *parameter)
{
    DrvLed *pThis = static_cast<DrvLed *>(parameter);
    if (pThis != nullptr)
        pThis->blinkTask();
}

/* ================= 闪烁任务 (成员) ================= */

void DrvLed::blinkTask()
{
    while (1)
    {
        while (m_blinkRunning)
        {
            // 点亮 LED
            setRGB(m_blinkRed, m_blinkGreen, m_blinkBlue);
            osDelay(m_blinkHalfPeriodMs);

            if (!m_blinkRunning) break;

            // 熄灭 LED
            setRGB(0, 0, 0);
            osDelay(m_blinkHalfPeriodMs);
        }

        // 闪烁停止后熄灭, 然后休眠等待下次启动
        turnOff();
        osDelay(100);
    }
}

/* ================= 启动 RGB 闪烁 ================= */

void DrvLed::setRGBBlink(uint8_t red, uint8_t green, uint8_t blue, uint8_t frequency_hz)
{
    if (!m_isInitialized) return;

    // 停止当前闪烁
    stopBlink();

    // 参数校验
    if (frequency_hz == 0)
    {
        // 频率为 0 直接常亮
        setRGB(red, green, blue);
        return;
    }

    // 计算周期 (毫秒)
    uint16_t period_ms = 1000 / frequency_hz;
    m_blinkHalfPeriodMs = period_ms / 2;

    // 设置闪烁参数
    m_blinkRed   = red;
    m_blinkGreen = green;
    m_blinkBlue  = blue;
    m_blinkRunning = true;

    // 创建闪烁任务 (若尚未创建)
    if (m_blinkTaskHandle == NULL)
    {
        osThreadAttr_t taskAttr = {
            .name = "LedBlink",
            .stack_size = 1024,
            .priority = osPriorityNormal,
        };
        m_blinkTaskHandle = osThreadNew(blinkTaskFunc, this, &taskAttr);
        if (m_blinkTaskHandle == NULL)
            return;
    }

    // 唤醒闪烁任务
    osThreadFlagsSet(m_blinkTaskHandle, 0x01);
}

/* ================= 停止闪烁 ================= */

void DrvLed::stopBlink(void)
{
    m_blinkRunning = false;
    turnOff();
}

/* ================= 查询是否正在闪烁 ================= */

bool DrvLed::isBlinking(void)
{
    return m_blinkRunning;
}

/* ================= 单通道设置 ================= */

void DrvLed::setRed(uint8_t value)
{
    if (!m_isInitialized) return;
    board_pwm_set_duty(m_port, RED_CHANNEL, percentToDuty(value));
}

void DrvLed::setGreen(uint8_t value)
{
    if (!m_isInitialized) return;
    board_pwm_set_duty(m_port, GREEN_CHANNEL, percentToDuty(value));
}

void DrvLed::setBlue(uint8_t value)
{
    if (!m_isInitialized) return;
    board_pwm_set_duty(m_port, BLUE_CHANNEL, percentToDuty(value));
}

/* ================= 熄灭全部 LED ================= */

void DrvLed::turnOff()
{
    setRGB(0, 0, 0);
}

// =============================================================================
// 全局访问函数
// =============================================================================

static DrvLed *g_drv_led = nullptr;

void init_drv_led(BoardPwmPort port)
{
    if (g_drv_led)
        return;

    g_drv_led = new DrvLed(port);
    if (g_drv_led)
        g_drv_led->init();
}

DrvLed *drv_led()
{
    return g_drv_led;
}
