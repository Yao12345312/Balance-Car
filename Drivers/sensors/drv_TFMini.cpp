#include "drv_TFMini.hpp"

// 触发测距命令 (cm): 5A 05 00 01 60
static const uint8_t s_trigger_cmd[TFMINI_CMD_SIZE] = {0x5A, 0x05, 0x00, 0x01, 0x60};

DrvTFMini::DrvTFMini(BoardI2cPort port, uint8_t dev_addr)
{
    m_port    = port;
    m_dev_addr = dev_addr;
    distance  = -1;
    strength  = -1;

    oa_enter_dist       = 20.0f;
    oa_exit_dist        = 30.0f;
    oa_exit_safe_need   = 25U;
    oa_invalid_hold_max = 250U;
    last_valid_distance = 1000.0f;
    invalid_read_cnt    = 0;
    safe_cnt            = 0;
    oa_active           = false;
}

void DrvTFMini::init(void)
{
    state    = IDLE;
    distance = -1;
    strength = -1;

    last_valid_distance = 1000.0f;
    invalid_read_cnt    = 0;
    safe_cnt            = 0;
    oa_active           = false;
}

bool DrvTFMini::sendTrigger(void)
{
    // board_i2c_write: dev_addr 为7位, 内部左移; timeout 单位秒
    return board_i2c_write(m_port, m_dev_addr, s_trigger_cmd, TFMINI_CMD_SIZE, 0.1) == BOARD_OK;
}

bool DrvTFMini::readFrame(void)
{
    if (state == IDLE)
    {
        if (!sendTrigger())
            return false;
        triggerTick = osKernelGetTickCount();
        state = WAITING;
        return false;
    }

    // state == WAITING
    if ((osKernelGetTickCount() - triggerTick) < TFMINI_WAIT_TICKS)
        return false;

    // 等待时间到, 读取数据
    state = IDLE;

    if (board_i2c_read(m_port, m_dev_addr, frame, TFMINI_FRAME_SIZE, 0.1) != BOARD_OK)
        return false;

    // 校验帧头
    if (frame[0] != TFMINI_HEADER1 || frame[1] != TFMINI_HEADER2)
        return false;

    // 校验和: 前8字节累加取低8位
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < TFMINI_FRAME_SIZE - 1; i++)
        checksum += frame[i];

    if (checksum != frame[8])
        return false;

    // 解析距离和信号强度字段 (小端序)
    distance = (int16_t)(frame[2] | (frame[3] << 8));
    strength = (int16_t)(frame[4] | (frame[5] << 8));

    return true;
}

int16_t DrvTFMini::getDistance(void)
{
    if (!readFrame())
        return -1;
    return distance;
}

int16_t DrvTFMini::getStrength(void)
{
    if (!readFrame())
        return -1;
    return strength;
}

bool DrvTFMini::getData(int16_t &dist, int16_t &str)
{
    if (!readFrame())
        return false;
    dist = distance;
    str  = strength;
    return true;
}

void DrvTFMini::configObstacleAvoidance(float enter_cm, float exit_cm,
                                        uint32_t exit_safe_need,
                                        uint32_t invalid_hold_max)
{
    oa_enter_dist       = enter_cm;
    oa_exit_dist        = exit_cm;
    oa_exit_safe_need   = exit_safe_need;
    oa_invalid_hold_max = invalid_hold_max;
}

bool DrvTFMini::updateObstacleAvoidance(void)
{
    int16_t raw = getDistance();

    // 无效数据(-1): 沿用上次有效距离; 连续无效过久则判为安全放开
    if (raw < 0)
    {
        if (++invalid_read_cnt > oa_invalid_hold_max)
        {
            last_valid_distance = 1000.0f;
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

static DrvTFMini *g_drv_tfmini = nullptr;

void init_drv_tfmini()
{
    if (g_drv_tfmini)
        return;

    g_drv_tfmini = new DrvTFMini(BOARD_I2C_TFMINI);
    if (g_drv_tfmini)
        g_drv_tfmini->init();
}

DrvTFMini *drv_tfmini()
{
    return g_drv_tfmini;
}
