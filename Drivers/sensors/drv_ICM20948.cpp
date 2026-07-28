#include "drv_ICM20948.hpp"

#include "hardware/drv_spi.hpp"
#include "cmsis_os2.h"
#include <cmath>
#include <cstdio>

#define ICM20948_WHO_AM_I        0x00
#define ICM20948_USER_CTRL       0x03
#define ICM20948_PWR_MGMT_1      0x06
#define ICM20948_ACCEL_XOUT_H    0x2D
#define ICM20948_REG_BANK_SEL    0x7F

#define ICM20948_BANK2_GYRO_SMPLRT_DIV  0x00
#define ICM20948_BANK2_GYRO_CONFIG_1    0x01
#define ICM20948_BANK2_ACCEL_CONFIG     0x14

#define ACCEL_LSB_PER_G   16384.0f
#define GYRO_LSB_PER_DPS    131.0f
#define DEG2RAD            0.017453292519943295f
#define G_STD              9.80665f

DrvICM20948::DrvICM20948(BoardSpiPort spi_port, BoardCsPin cs)
{
    m_spi_port = spi_port;
    m_cs       = cs;

    m_accel_offset[0] = m_accel_offset[1] = m_accel_offset[2] = 0.0f;
    m_gyro_offset[0]  = m_gyro_offset[1]  = m_gyro_offset[2]  = 0.0f;
    m_accel_calibrated = false;
    m_gyro_calibrated  = false;

    m_gyro_lpf_alpha = 0.9f;
    m_gyro_lpf_state[0] = m_gyro_lpf_state[1] = m_gyro_lpf_state[2] = 0.0f;
    m_gyro_lpf_init = false;

    m_spi_error_count = 0;
}

int8_t DrvICM20948::readRegister(uint8_t reg, uint8_t *data, uint16_t len)
{
    if (len + 1 > ICM20948_SPI_BUF_SIZE)
        return -1;

    uint8_t tx_buf[ICM20948_SPI_BUF_SIZE];
    uint8_t rx_buf[ICM20948_SPI_BUF_SIZE];

    tx_buf[0] = reg | 0x80;
    for (uint16_t i = 1; i <= len; i++)
        tx_buf[i] = 0x00;

    if (!board_spi_lock(m_spi_port, 0.1))
        return -1;

    csLow();
    BoardResult r = board_spi_write_read(m_spi_port, tx_buf, rx_buf, (uint16_t)(len + 1), 0.1);
    csHigh();

    board_spi_unlock(m_spi_port);

    if (r != BOARD_OK)
    {
        m_spi_error_count++;
        return -1;
    }

    for (uint16_t i = 0; i < len; i++)
        data[i] = rx_buf[i + 1];

    m_spi_error_count = 0;
    return 0;
}

int8_t DrvICM20948::writeRegister(uint8_t reg, uint8_t data)
{
    uint8_t tx_buf[2];
    uint8_t rx_buf[2];

    tx_buf[0] = reg & 0x7F;
    tx_buf[1] = data;

    if (!board_spi_lock(m_spi_port, 0.1))
        return -1;

    csLow();
    BoardResult r = board_spi_write_read(m_spi_port, tx_buf, rx_buf, 2, 0.1);
    csHigh();

    board_spi_unlock(m_spi_port);

    if (r != BOARD_OK)
    {
        m_spi_error_count++;
        return -1;
    }

    m_spi_error_count = 0;
    return 0;
}

int8_t DrvICM20948::selectBank(uint8_t bank)
{
    return writeRegister(ICM20948_REG_BANK_SEL, bank & 0x03);
}

bool DrvICM20948::readRaw(RawData &raw)
{
    uint8_t buf[12] = {0};

    if (readRegister(ICM20948_ACCEL_XOUT_H, buf, 12) != 0)
        return false;

    raw.accel_x = (int16_t)((buf[0] << 8) | buf[1]);
    raw.accel_y = (int16_t)((buf[2] << 8) | buf[3]);
    raw.accel_z = (int16_t)((buf[4] << 8) | buf[5]);
    raw.gyro_x  = (int16_t)((buf[6] << 8) | buf[7]);
    raw.gyro_y  = (int16_t)((buf[8] << 8) | buf[9]);
    raw.gyro_z  = (int16_t)((buf[10] << 8) | buf[11]);

    return true;
}

int8_t DrvICM20948::getAccelData(float &accel_x, float &accel_y, float &accel_z)
{
    if (!m_mutex || osMutexAcquire(m_mutex, 0) != osOK)
        return -1;

    int8_t ret = -1;
    RawData raw;
    if (readRaw(raw))
    {
        accel_x = (float)raw.accel_x / ACCEL_LSB_PER_G * G_STD;
        accel_y = (float)raw.accel_y / ACCEL_LSB_PER_G * G_STD;
        accel_z = (float)raw.accel_z / ACCEL_LSB_PER_G * G_STD;
        ret = 0;
    }

    osMutexRelease(m_mutex);
    return ret;
}

int8_t DrvICM20948::getGyroData(float &gyro_x, float &gyro_y, float &gyro_z)
{
    if (!m_mutex || osMutexAcquire(m_mutex, 0) != osOK)
        return -1;

    int8_t ret = -1;
    RawData raw;
    if (readRaw(raw))
    {
        float gx_raw = (float)raw.gyro_x / GYRO_LSB_PER_DPS * DEG2RAD;
        float gy_raw = (float)raw.gyro_y / GYRO_LSB_PER_DPS * DEG2RAD;
        float gz_raw = (float)raw.gyro_z / GYRO_LSB_PER_DPS * DEG2RAD;

        if (!m_gyro_lpf_init)
        {
            m_gyro_lpf_state[0] = gx_raw;
            m_gyro_lpf_state[1] = gy_raw;
            m_gyro_lpf_state[2] = gz_raw;
            m_gyro_lpf_init = true;
        }
        else
        {
            m_gyro_lpf_state[0] = m_gyro_lpf_alpha * gx_raw + (1.0f - m_gyro_lpf_alpha) * m_gyro_lpf_state[0];
            m_gyro_lpf_state[1] = m_gyro_lpf_alpha * gy_raw + (1.0f - m_gyro_lpf_alpha) * m_gyro_lpf_state[1];
            m_gyro_lpf_state[2] = m_gyro_lpf_alpha * gz_raw + (1.0f - m_gyro_lpf_alpha) * m_gyro_lpf_state[2];
        }

        gyro_x = m_gyro_lpf_state[0];
        gyro_y = m_gyro_lpf_state[1];
        gyro_z = m_gyro_lpf_state[2];
        ret = 0;
    }

    osMutexRelease(m_mutex);
    return ret;
}

int8_t DrvICM20948::getAllMotionData(float &ax, float &ay, float &az,
                                     float &gx, float &gy, float &gz)
{
    if (!m_mutex || osMutexAcquire(m_mutex, 0) != osOK)
        return -1;

    int8_t ret = -1;
    RawData raw;
    if (readRaw(raw))
    {
        ax = (float)raw.accel_x / ACCEL_LSB_PER_G * G_STD;
        ay = (float)raw.accel_y / ACCEL_LSB_PER_G * G_STD;
        az = (float)raw.accel_z / ACCEL_LSB_PER_G * G_STD;

        float gx_raw = (float)raw.gyro_x / GYRO_LSB_PER_DPS * DEG2RAD;
        float gy_raw = (float)raw.gyro_y / GYRO_LSB_PER_DPS * DEG2RAD;
        float gz_raw = (float)raw.gyro_z / GYRO_LSB_PER_DPS * DEG2RAD;

        if (!m_gyro_lpf_init)
        {
            m_gyro_lpf_state[0] = gx_raw;
            m_gyro_lpf_state[1] = gy_raw;
            m_gyro_lpf_state[2] = gz_raw;
            m_gyro_lpf_init = true;
        }
        else
        {
            m_gyro_lpf_state[0] = m_gyro_lpf_alpha * gx_raw + (1.0f - m_gyro_lpf_alpha) * m_gyro_lpf_state[0];
            m_gyro_lpf_state[1] = m_gyro_lpf_alpha * gy_raw + (1.0f - m_gyro_lpf_alpha) * m_gyro_lpf_state[1];
            m_gyro_lpf_state[2] = m_gyro_lpf_alpha * gz_raw + (1.0f - m_gyro_lpf_alpha) * m_gyro_lpf_state[2];
        }

        gx = m_gyro_lpf_state[0];
        gy = m_gyro_lpf_state[1];
        gz = m_gyro_lpf_state[2];
        ret = 0;
    }

    osMutexRelease(m_mutex);
    return ret;
}

int8_t DrvICM20948::getAllMotionDataCalibrated(float &ax, float &ay, float &az,
                                               float &gx, float &gy, float &gz)
{
    if (getAllMotionData(ax, ay, az, gx, gy, gz) != 0)
        return -1;

    if (m_accel_calibrated)
    {
        ax -= m_accel_offset[0];
        ay -= m_accel_offset[1];
        az -= m_accel_offset[2];
    }

    if (m_gyro_calibrated)
    {
        gx -= m_gyro_offset[0];
        gy -= m_gyro_offset[1];
        gz -= m_gyro_offset[2];
    }

    return 0;
}

void DrvICM20948::resetSPI()
{
    uint8_t hw = board_spi_ports[m_spi_port].hw_num;
    SPI_CLASS *s = get_spi_instance(hw);
    if (!s)
        return;

    s->deinit();
    s->init(hw, board_spi_ports[m_spi_port].config);
}

int8_t DrvICM20948::calibrateAccelStatic()
{
    if (!m_mutex || osMutexAcquire(m_mutex, 0) != osOK)
        return -1;

    const float ACC_NORM_THRESHOLD = 0.3f;
    const uint16_t REQUIRED_COUNT = 1000;
    const uint32_t TIMEOUT = 10000;

    float ax, ay, az;
    float sum_x = 0, sum_y = 0, sum_z = 0;

    uint16_t count = 0;
    uint32_t time_ms = 0;

    printf("Accel calibration start...\n");

    while (1)
    {
        if (getAccelData(ax, ay, az) == 0)
        {
            float norm = sqrtf(ax * ax + ay * ay + az * az);

            if (fabsf(norm - G_STD) < ACC_NORM_THRESHOLD)
            {
                count++;

                sum_x += ax;
                sum_y += ay;
                sum_z += az;

                if (count >= REQUIRED_COUNT)
                {
                    float mean_x = sum_x / count;
                    float mean_y = sum_y / count;
                    float mean_z = sum_z / count;

                    float gx = 0.0f;
                    float gy = 0.0f;
                    float gz = -G_STD;

                    m_accel_offset[0] = mean_x - gx;
                    m_accel_offset[1] = mean_y - gy;
                    m_accel_offset[2] = mean_z - gz;

                    m_accel_calibrated = true;

                    printf("Accel cal success:\n");
                    printf("bias x: %.6f\n", m_accel_offset[0]);
                    printf("bias y: %.6f\n", m_accel_offset[1]);
                    printf("bias z: %.6f\n", m_accel_offset[2]);

                    osMutexRelease(m_mutex);
                    return 0;
                }
            }
            else
            {
                count = 0;
                sum_x = sum_y = sum_z = 0;

                if (time_ms % 200 == 0)
                    printf("Keep IMU static!\n");
            }
        }

        osDelay(1);
        time_ms++;

        if (time_ms > TIMEOUT)
        {
            printf("Accel calibration timeout!\n");
            osMutexRelease(m_mutex);
            return -1;
        }
    }
}

int8_t DrvICM20948::getAccelDataCalibrated(float &accel_x, float &accel_y, float &accel_z)
{
    int8_t rslt = getAccelData(accel_x, accel_y, accel_z);

    if (rslt == 0 && m_accel_calibrated)
    {
        accel_x -= m_accel_offset[0];
        accel_y -= m_accel_offset[1];
        accel_z -= m_accel_offset[2];
    }

    return rslt;
}

int8_t DrvICM20948::calibrateGyroStatic()
{
    if (!m_mutex || osMutexAcquire(m_mutex, 0) != osOK)
        return -1;

    const float GYRO_THRESHOLD = 0.05f;
    const uint16_t REQUIRED_COUNT = 1500;
    const uint32_t TIMEOUT = 100000;

    float gx, gy, gz;
    float sum_x = 0, sum_y = 0, sum_z = 0;

    uint16_t count = 0;
    uint32_t time_ms = 0;

    printf("Gyro calibration start...\n");

    while (1)
    {
        if (getGyroData(gx, gy, gz) == 0)
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

                    printf("Gyro cal success:\n");
                    printf("gx: %.6f rad/s\n", m_gyro_offset[0]);
                    printf("gy: %.6f rad/s\n", m_gyro_offset[1]);
                    printf("gz: %.6f rad/s\n", m_gyro_offset[2]);

                    osMutexRelease(m_mutex);
                    return 0;
                }
            }
            else
            {
                count = 0;
                sum_x = sum_y = sum_z = 0;

                if (time_ms % 200 == 0)
                    printf("Keep IMU static!\n");
            }
        }

        osDelay(1);
        time_ms++;

        if (time_ms > TIMEOUT)
        {
            printf("Gyro calibration timeout!\n");
            osMutexRelease(m_mutex);
            return -1;
        }
    }
}

int8_t DrvICM20948::getGyroDataCalibrated(float &gyro_x, float &gyro_y, float &gyro_z)
{
    int8_t rslt = getGyroData(gyro_x, gyro_y, gyro_z);

    if (rslt == 0 && m_gyro_calibrated)
    {
        gyro_x -= m_gyro_offset[0];
        gyro_y -= m_gyro_offset[1];
        gyro_z -= m_gyro_offset[2];
    }

    return rslt;
}

int8_t DrvICM20948::calibrateAllStatic()
{
    if (!m_mutex || osMutexAcquire(m_mutex, 0) != osOK)
        return -1;

    int8_t ret = 0;

    printf("\nStatic calibration start\n");

    if (calibrateGyroStatic() != 0)
    {
        printf("Gyro calibration failed\n");
        ret = -1;
    }

    osDelay(500);

    printf("Calibration done\n\n");

    osMutexRelease(m_mutex);
    return ret;
}

int8_t DrvICM20948::init()
{
    if (!m_mutex)
    {
        const osMutexAttr_t mtx_attr = {
            .name      = "icm20948_mtx",
            .attr_bits = osMutexPrioInherit | osMutexRecursive
        };
        m_mutex = osMutexNew(&mtx_attr);
    }

    board_cs_set(m_cs);

    osDelay(10);

    uint8_t whoami = 0;

    if (readRegister(ICM20948_WHO_AM_I, &whoami, 1) != 0)
    {
        printf("ICM20948: SPI read failed!\n");
        return -1;
    }

    if (whoami != 0xEA)
    {
        printf("ICM20948: WHO_AM_I mismatch (0x%02X)\n", whoami);
        return -1;
    }

    writeRegister(ICM20948_PWR_MGMT_1, 0x80);
    osDelay(10);
    writeRegister(ICM20948_PWR_MGMT_1, 0x01);
    osDelay(10);

    selectBank(2);

    writeRegister(ICM20948_BANK2_GYRO_SMPLRT_DIV, 0x00);
    //陀螺仪使能DLPF, 截止频率约50Hz
    writeRegister(ICM20948_BANK2_GYRO_CONFIG_1, 0x03);
    //加速度计使能DLPF, 截止频率约50Hz
    writeRegister(ICM20948_BANK2_ACCEL_CONFIG, 0x03);

    selectBank(0);

    printf("ICM20948 init OK\n");
    return 0;
}

static DrvICM20948 *g_drv_icm20948 = 0;

void init_drv_icm20948()
{
    if (g_drv_icm20948)
        return;

    g_drv_icm20948 = new DrvICM20948(BOARD_SPI_ICM20948, BOARD_CS_ICM20948);
    if (!g_drv_icm20948)
        return;

    g_drv_icm20948->init();
}

DrvICM20948 *drv_icm20948()
{
    return g_drv_icm20948;
}
