#include "drv_BMI088.hpp"

#include "stm32h7xx_hal.h"
#include "cmsis_os2.h"
#include <cstring>
#include <cmath>

#define BMI088_G_TO_MS2   9.7986f

DrvBMI088::DrvBMI088(BoardSpiPort spi_port, BoardCsPin acc_cs, BoardCsPin gyro_cs)
{
    m_spi_port   = spi_port;
    m_acc_cs     = acc_cs;
    m_gyro_cs    = gyro_cs;
    m_currentDev = 0;

    m_accel_offset[0] = m_accel_offset[1] = m_accel_offset[2] = 0.0f;
    m_gyro_offset[0]  = m_gyro_offset[1]  = m_gyro_offset[2]  = 0.0f;
    m_accel_calibrated = false;
    m_gyro_calibrated  = false;

    m_gyro_iir_alpha = 0.3f;
    m_gyro_iir_state[0] = 0.0f;
    m_gyro_iir_state[1] = 0.0f;
    m_gyro_iir_state[2] = 0.0f;
    m_gyro_iir_init = false;

    memset(&m_dev, 0, sizeof(m_dev));
}

BMI08_INTF_RET_TYPE DrvBMI088::spiRead(uint8_t reg_addr, uint8_t *reg_data,
                                       uint32_t len, void *intf_ptr)
{
    DrvBMI088 *self = static_cast<DrvBMI088 *>(intf_ptr);

    if (len + 1 > BMI088_SPI_BUF_SIZE)
        return -1;

    uint8_t tx_buf[BMI088_SPI_BUF_SIZE];
    uint8_t rx_buf[BMI088_SPI_BUF_SIZE];

    tx_buf[0] = reg_addr | 0x80;
    memset(&tx_buf[1], 0xFF, len);

    if (!board_spi_lock(self->m_spi_port, 0.1))
        return -1;

    self->csLow();
    BoardResult r = board_spi_write_read(self->m_spi_port, tx_buf, rx_buf, (uint16_t)(len + 1), 0.1);
    self->csHigh();

    board_spi_unlock(self->m_spi_port);

    if (r != BOARD_OK)
        return -1;

    memcpy(reg_data, &rx_buf[1], len);
    return BMI08_INTF_RET_SUCCESS;
}

BMI08_INTF_RET_TYPE DrvBMI088::spiWrite(uint8_t reg_addr, const uint8_t *reg_data,
                                        uint32_t len, void *intf_ptr)
{
    DrvBMI088 *self = static_cast<DrvBMI088 *>(intf_ptr);

    if (len + 1 > BMI088_SPI_BUF_SIZE)
        return -1;

    uint8_t tx_buf[BMI088_SPI_BUF_SIZE];
    uint8_t rx_buf[BMI088_SPI_BUF_SIZE];

    tx_buf[0] = reg_addr & 0x7F;
    memcpy(&tx_buf[1], reg_data, len);

    if (!board_spi_lock(self->m_spi_port, 0.1))
        return -1;

    self->csLow();
    BoardResult r = board_spi_write_read(self->m_spi_port, tx_buf, rx_buf, (uint16_t)(len + 1), 0.1);
    self->csHigh();

    board_spi_unlock(self->m_spi_port);

    return (r == BOARD_OK) ? BMI08_INTF_RET_SUCCESS : -1;
}

void DrvBMI088::delayUs(uint32_t period, void *intf_ptr)
{
    (void)intf_ptr;
    if (period == 0)
        return;

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    if ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0)
    {
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }

    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = period * (SystemCoreClock / 1000000u);
    while ((DWT->CYCCNT - start) < ticks) { }
}

float DrvBMI088::getAccelSensitivity()
{
    switch (m_dev.accel_cfg.range)
    {
        case BMI088_ACCEL_RANGE_24G: return 1365.0f;
        case BMI088_ACCEL_RANGE_12G: return 2730.0f;
        case BMI088_ACCEL_RANGE_6G:  return 5460.0f;
        case BMI088_ACCEL_RANGE_3G:  return 10920.0f;
        default:                     return 1365.0f;
    }
}

float DrvBMI088::getGyroSensitivity()
{
    switch (m_dev.gyro_cfg.range)
    {
        case BMI08_GYRO_RANGE_2000_DPS: return 16.384f;
        case BMI08_GYRO_RANGE_1000_DPS: return 32.768f;
        case BMI08_GYRO_RANGE_500_DPS:  return 65.536f;
        case BMI08_GYRO_RANGE_250_DPS:  return 131.072f;
        case BMI08_GYRO_RANGE_125_DPS:  return 262.144f;
        default:                        return 16.384f;
    }
}

int8_t DrvBMI088::getAccelData(float &ax, float &ay, float &az)
{
    if (!m_mutex || osMutexAcquire(m_mutex, 0) != osOK)
        return -1;

    struct bmi08_sensor_data accel;
    m_currentDev = 0;

    int8_t rslt = bmi08a_get_data(&accel, &m_dev);
    if (rslt == BMI08_OK)
    {
        float sensitivity = getAccelSensitivity();
        ax = (float)accel.x / sensitivity * BMI088_G_TO_MS2;
        ay = (float)accel.y / sensitivity * BMI088_G_TO_MS2;
        az = (float)accel.z / sensitivity * BMI088_G_TO_MS2;
    }
    osMutexRelease(m_mutex);
    return rslt;
}

int8_t DrvBMI088::getGyroData(float &gx, float &gy, float &gz)
{
    if (!m_mutex || osMutexAcquire(m_mutex, 0) != osOK)
        return -1;

    struct bmi08_sensor_data gyro;
    m_currentDev = 1;

    int8_t rslt = bmi08g_get_data(&gyro, &m_dev);
    if (rslt == BMI08_OK)
    {
        float sensitivity = getGyroSensitivity();

        float rx = (float)gyro.x / sensitivity;
        float ry = (float)gyro.y / sensitivity;
        float rz = (float)gyro.z / sensitivity;

        if (!m_gyro_iir_init)
        {
            m_gyro_iir_state[0] = rx;
            m_gyro_iir_state[1] = ry;
            m_gyro_iir_state[2] = rz;
            m_gyro_iir_init = true;
        }
        else
        {
            m_gyro_iir_state[0] = m_gyro_iir_alpha * rx + (1.0f - m_gyro_iir_alpha) * m_gyro_iir_state[0];
            m_gyro_iir_state[1] = m_gyro_iir_alpha * ry + (1.0f - m_gyro_iir_alpha) * m_gyro_iir_state[1];
            m_gyro_iir_state[2] = m_gyro_iir_alpha * rz + (1.0f - m_gyro_iir_alpha) * m_gyro_iir_state[2];
        }

        gx = m_gyro_iir_state[0];
        gy = m_gyro_iir_state[1];
        gz = m_gyro_iir_state[2];
    }
    osMutexRelease(m_mutex);
    return rslt;
}

int8_t DrvBMI088::calibrateAccelStatic()
{
    if (!m_mutex || osMutexAcquire(m_mutex, 0) != osOK)
        return -1;

    const float ACC_NORM_THRESHOLD = 0.3f;
    const uint16_t REQUIRED_COUNT  = 1000;
    const uint32_t TIMEOUT         = 10000;
    const float g = BMI088_G_TO_MS2;

    float ax, ay, az;
    float sum_x = 0, sum_y = 0, sum_z = 0;
    uint16_t count = 0;
    uint32_t time_ms = 0;

    while (1)
    {
        if (getAccelData(ax, ay, az) == BMI08_OK)
        {
            float norm = sqrtf(ax * ax + ay * ay + az * az);

            if (fabsf(norm - g) < ACC_NORM_THRESHOLD)
            {
                count++;
                sum_x += ax;
                sum_y += ay;
                sum_z += az;

                if (count >= REQUIRED_COUNT)
                {
                    m_accel_offset[0] = sum_x / count - 0.0f;
                    m_accel_offset[1] = sum_y / count - 0.0f;
                    m_accel_offset[2] = sum_z / count - (-g);
                    m_accel_calibrated = true;
                    osMutexRelease(m_mutex);
                    return 0;
                }
            }
            else
            {
                count = 0;
                sum_x = sum_y = sum_z = 0.0f;
            }
        }

        osDelay(1);
        if (++time_ms > TIMEOUT)
        {
            osMutexRelease(m_mutex);
            return -1;
        }
    }
}

int8_t DrvBMI088::calibrateGyroStatic()
{
    if (!m_mutex || osMutexAcquire(m_mutex, 0) != osOK)
        return -1;

    const float GYRO_THRESHOLD = 3.0f;
    const uint16_t REQUIRED_COUNT = 1500;
    const uint32_t TIMEOUT = 100000;

    float gx, gy, gz;
    float sum_x = 0, sum_y = 0, sum_z = 0;
    uint16_t count = 0;
    uint32_t time_ms = 0;

    while (1)
    {
        if (getGyroData(gx, gy, gz) == BMI08_OK)
        {
            if (fabsf(gx) < GYRO_THRESHOLD &&
                fabsf(gy) < GYRO_THRESHOLD &&
                fabsf(gz) < GYRO_THRESHOLD)
            {
                count++;
                sum_x += gx;
                sum_y += gy;
                sum_z += gz;

                if (count >= REQUIRED_COUNT)
                {
                    m_gyro_offset[0] = sum_x / count;
                    m_gyro_offset[1] = sum_y / count;
                    m_gyro_offset[2] = sum_z / count;
                    m_gyro_calibrated = true;
                    osMutexRelease(m_mutex);
                    return 0;
                }
            }
            else
            {
                count = 0;
                sum_x = sum_y = sum_z = 0.0f;
            }
        }

        osDelay(1);
        if (++time_ms > TIMEOUT)
        {
            osMutexRelease(m_mutex);
            return -1;
        }
    }
}

//IMU静态校准函数
int8_t DrvBMI088::calibrateAllStatic()
{
    if (!m_mutex || osMutexAcquire(m_mutex, 0) != osOK)
        return -1;

    int8_t ret = 0;
    if (calibrateGyroStatic() != 0)
        ret = -1;
    osDelay(500);
    osMutexRelease(m_mutex);
    return ret;
}

//仅在调用calibrateAccelStatic()后使用
int8_t DrvBMI088::getAccelDataCalibrated(float &ax, float &ay, float &az)
{
    if (!m_mutex || osMutexAcquire(m_mutex, 0) != osOK)
        return -1;

    int8_t rslt = getAccelData(ax, ay, az);
    if (rslt == BMI08_OK && m_accel_calibrated)
    {
        ax -= m_accel_offset[0];
        ay -= m_accel_offset[1];
        az -= m_accel_offset[2];
    }
    osMutexRelease(m_mutex);
    return rslt;
}

//仅在调用calibrateGyroStatic()后使用
int8_t DrvBMI088::getGyroDataCalibrated(float &gx, float &gy, float &gz)
{
    if (!m_mutex || osMutexAcquire(m_mutex, 0) != osOK)
        return -1;

    int8_t rslt = getGyroData(gx, gy, gz);
    if (rslt == BMI08_OK && m_gyro_calibrated)
    {
        gx -= m_gyro_offset[0];
        gy -= m_gyro_offset[1];
        gz -= m_gyro_offset[2];
    }
    osMutexRelease(m_mutex);
    return rslt;
}

int8_t DrvBMI088::init()
{
    if (!m_mutex)
    {
        const osMutexAttr_t mtx_attr = {
            .name      = "bmi088_mtx",
            .attr_bits = osMutexPrioInherit | osMutexRecursive
        };
        m_mutex = osMutexNew(&mtx_attr);
    }

    int8_t rslt;

    m_dev.intf       = BMI08_SPI_INTF;
    m_dev.read       = spiRead;
    m_dev.write      = spiWrite;
    m_dev.delay_us   = delayUs;
    m_dev.read_write_len = BMI088_READ_WRITE_LEN;
    m_dev.dummy_byte = 1;
    m_dev.variant    = BMI088_VARIANT;

    m_dev.intf_ptr_accel = this;
    m_dev.intf_ptr_gyro  = this;

    board_cs_set(m_acc_cs);
    board_cs_set(m_gyro_cs);

    // 上电默认是 I2C 模式, 需要 CS 下降沿 + SPI 时钟切换到 SPI 模式;
    // 切换瞬间首次读到的数据无效, 因此先发 dummy 脉冲 + soft reset 再读 chip_id
    osDelay(50);

    for (uint8_t attempt = 0; attempt < 5; ++attempt)
    {
        // 强制 BMI088 进入 SPI 模式: CS 拉低 -> 发若干 0xFF -> CS 拉高
        m_currentDev = 0;
        csLow();
        uint8_t dummy = 0xFF;
        board_spi_write_read(m_spi_port, &dummy, &dummy, 1, 0.01);
        csHigh();

        osDelay(2);

        // 读 chip_id
        rslt = bmi08a_init(&m_dev);
        if (rslt == BMI08_OK)
            break;

        // 失败则软复位后重试
        bmi08a_soft_reset(&m_dev);
        osDelay(50);
    }

    if (rslt != BMI08_OK) return rslt;

    m_dev.accel_cfg.range  = BMI088_ACCEL_RANGE_24G;
    m_dev.accel_cfg.odr    = BMI08_ACCEL_ODR_1600_HZ;
    m_dev.accel_cfg.bw     = BMI08_ACCEL_BW_NORMAL;
    m_dev.accel_cfg.power  = BMI08_ACCEL_PM_ACTIVE;

    rslt = bmi08a_set_power_mode(&m_dev);
    if (rslt != BMI08_OK) return rslt;

    rslt = bmi08a_set_meas_conf(&m_dev);
    if (rslt != BMI08_OK) return rslt;

    osDelay(50);

    m_currentDev = 1;
    rslt = bmi08g_init(&m_dev);
    if (rslt != BMI08_OK) return rslt;

    m_dev.gyro_cfg.range = BMI08_GYRO_RANGE_2000_DPS;
    m_dev.gyro_cfg.odr   = BMI08_GYRO_BW_230_ODR_2000_HZ;
    m_dev.gyro_cfg.power = BMI08_GYRO_PM_NORMAL;

    rslt = bmi08g_set_power_mode(&m_dev);
    if (rslt != BMI08_OK) return rslt;

    rslt = bmi08g_set_meas_conf(&m_dev);
    return rslt;
}

static DrvBMI088 *g_drv_bmi088 = 0;

void init_drv_bmi088()
{
    if (g_drv_bmi088)
        return;

    //BMI088 已停用, SPI1 现划归 OLED; 此处保留接口避免编译错误
    g_drv_bmi088 = new DrvBMI088(BOARD_SPI_OLED,
                                 BOARD_CS_BMI088_ACC,
                                 BOARD_CS_BMI088_GYRO);
    if (!g_drv_bmi088)
        return;

    g_drv_bmi088->init();
}

DrvBMI088 *drv_bmi088()
{
    return g_drv_bmi088;
}
