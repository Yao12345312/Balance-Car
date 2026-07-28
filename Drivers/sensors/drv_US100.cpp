#include "drv_US100.hpp"

// board_uart_read 的 rc_wait / sync_wait 单位为秒
static const double US100_UART_TX_WAIT = 0.02;   // 触发命令单字节发送等待
static const double US100_UART_RX_WAIT = 0.005;  // 从环形缓冲区拉取字节的等待

DrvUS100::DrvUS100(BoardUartPort port)
{
    m_port   = port;
    distance = -1;
    temperature = -999;

    mode         = MODE_IDLE;
    triggerTick  = 0;

    oa_enter_dist       = 200.0f;   // 200mm = 20cm
    oa_exit_dist        = 300.0f;   // 300mm = 30cm
    oa_exit_safe_need   = 25U;
    oa_invalid_hold_max = 250U;
    last_valid_distance = 10000.0f;
    invalid_read_cnt    = 0;
    safe_cnt            = 0;
    oa_active           = false;
}

void DrvUS100::init(void)
{
    mode        = MODE_IDLE;
    distance    = -1;
    temperature = -999;

    last_valid_distance = 10000.0f;
    invalid_read_cnt    = 0;
    safe_cnt            = 0;
    oa_active           = false;

    // 清空接收缓冲残留
    board_uart_reset_rx(m_port);
}

bool DrvUS100::sendDistTrigger(void)
{
    board_uart_reset_rx(m_port);

    uint8_t cmd = US100_TRIGGER_DIST_CMD;
    if (board_uart_write(m_port, &cmd, 1, US100_UART_TX_WAIT, US100_UART_TX_WAIT) != BOARD_OK)
        return false;

    triggerTick = osKernelGetTickCount();
    mode        = MODE_DIST;
    return true;
}

bool DrvUS100::sendTempTrigger(void)
{
    board_uart_reset_rx(m_port);

    uint8_t cmd = US100_TRIGGER_TEMP_CMD;
    if (board_uart_write(m_port, &cmd, 1, US100_UART_TX_WAIT, US100_UART_TX_WAIT) != BOARD_OK)
        return false;

    triggerTick = osKernelGetTickCount();
    mode        = MODE_TEMP;
    return true;
}

bool DrvUS100::readDistFrame(void)
{
    if (mode != MODE_DIST)
    {
        // 启动一轮新的测距
        sendDistTrigger();
        return false;
    }

    // 等待回波时间到再读 (覆盖最大量程声波往返时间)
    uint32_t elapsed = osKernelGetTickCount() - triggerTick;
    if (elapsed < US100_DIST_WAIT_TICKS)
        return false;

    mode = MODE_IDLE;

    if (board_uart_read(m_port, frame, US100_DIST_FRAME_SIZE, US100_UART_RX_WAIT, US100_UART_RX_WAIT)
        != US100_DIST_FRAME_SIZE)
        return false;

    // 高字节在前
    int16_t dist = (int16_t)(((uint16_t)frame[0] << 8) | (uint16_t)frame[1]);

    // 量程校验
    if (dist < US100_DIST_MIN_MM || dist > US100_DIST_MAX_MM)
        return false;

    distance = dist;
    return true;
}

bool DrvUS100::readTempFrame(void)
{
    if (mode != MODE_TEMP)
    {
        sendTempTrigger();
        return false;
    }

    uint32_t elapsed = osKernelGetTickCount() - triggerTick;
    if (elapsed < US100_TEMP_WAIT_TICKS)
        return false;

    mode = MODE_IDLE;

    if (board_uart_read(m_port, frame, US100_TEMP_FRAME_SIZE, US100_UART_RX_WAIT, US100_UART_RX_WAIT)
        != US100_TEMP_FRAME_SIZE)
        return false;

    // 摄氏度 = 原始值 - 45
    temperature = (int16_t)frame[0] - 45;
    return true;
}

int16_t DrvUS100::getDistance(void)
{
    // 整体超时保护: 超时则复位状态机, 等待下轮重新触发
    if (mode == MODE_DIST && (osKernelGetTickCount() - triggerTick) >= US100_RX_TIMEOUT_TICKS)
        mode = MODE_IDLE;

    if (!readDistFrame())
        return -1;
    return distance;
}

int16_t DrvUS100::getTemperature(void)
{
    if (mode == MODE_TEMP && (osKernelGetTickCount() - triggerTick) >= US100_RX_TIMEOUT_TICKS)
        mode = MODE_IDLE;

    if (!readTempFrame())
        return -999;
    return temperature;
}

void DrvUS100::configObstacleAvoidance(float enter_mm, float exit_mm,
                                       uint32_t exit_safe_need,
                                       uint32_t invalid_hold_max)
{
    oa_enter_dist       = enter_mm;
    oa_exit_dist        = exit_mm;
    oa_exit_safe_need   = exit_safe_need;
    oa_invalid_hold_max = invalid_hold_max;
}

bool DrvUS100::updateObstacleAvoidance(void)
{
    int16_t raw = getDistance();

    // 无效数据(-1): 沿用上次有效距离; 连续无效过久则判为安全放开
    if (raw < 0)
    {
        if (++invalid_read_cnt > oa_invalid_hold_max)
        {
            last_valid_distance = 10000.0f;
            invalid_read_cnt    = 0;
        }
    }
    else
    {
        last_valid_distance = (float)raw;
        invalid_read_cnt    = 0;
    }
    float eff_dist = last_valid_distance;

    if (!oa_active)
    {
        // 进入: 有效距离小于阈值
        if (eff_dist < oa_enter_dist)
        {
            oa_active = true;
            safe_cnt  = 0;
        }
    }
    else
    {
        // 退出: 迟滞阈值 + 连续安全去抖
        if (eff_dist > oa_exit_dist)
        {
            if (++safe_cnt >= oa_exit_safe_need)
            {
                oa_active = false;
                safe_cnt  = 0;
            }
        }
        else
        {
            safe_cnt = 0;
        }
    }

    return oa_active;
}

// =============================================================================
// 全局访问函数
// =============================================================================

static DrvUS100 *g_drv_us100 = nullptr;

void init_drv_us100()
{
    if (g_drv_us100)
        return;

    g_drv_us100 = new DrvUS100(BOARD_UART_US100);
    if (g_drv_us100)
        g_drv_us100->init();
}

DrvUS100 *drv_us100()
{
    return g_drv_us100;
}
