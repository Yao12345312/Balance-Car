#pragma once

#include "board.hpp"
#include "cmsis_os2.h"

#define ICM20948_SPI_BUF_SIZE  64

class DrvICM20948
{
public:
    DrvICM20948(BoardSpiPort spi_port, BoardCsPin cs);

    int8_t init();

    int8_t getAccelData(float &accel_x, float &accel_y, float &accel_z);
    int8_t getGyroData(float &gyro_x, float &gyro_y, float &gyro_z);

    int8_t getAccelDataCalibrated(float &accel_x, float &accel_y, float &accel_z);
    int8_t getGyroDataCalibrated(float &gyro_x, float &gyro_y, float &gyro_z);

    int8_t getAllMotionData(float &ax, float &ay, float &az,
                            float &gx, float &gy, float &gz);
    int8_t getAllMotionDataCalibrated(float &ax, float &ay, float &az,
                                      float &gx, float &gy, float &gz);

    int8_t calibrateAccelStatic();
    int8_t calibrateGyroStatic();
    int8_t calibrateAllStatic();

    bool isAccelCalibrated() const { return m_accel_calibrated; }
    bool isGyroCalibrated()  const { return m_gyro_calibrated; }

    void setGyroLpfAlpha(float alpha) { m_gyro_lpf_alpha = alpha; }

    void resetSPI();

private:
    BoardSpiPort m_spi_port;
    BoardCsPin   m_cs;

    float m_accel_offset[3];
    float m_gyro_offset[3];
    bool  m_accel_calibrated;
    bool  m_gyro_calibrated;

    float m_gyro_lpf_alpha;
    float m_gyro_lpf_state[3];
    bool  m_gyro_lpf_init;

    uint32_t m_spi_error_count;
    static constexpr uint32_t SPI_ERROR_THRESHOLD = 10;

    osMutexId_t m_mutex = NULL;

    struct RawData {
        int16_t accel_x;
        int16_t accel_y;
        int16_t accel_z;
        int16_t gyro_x;
        int16_t gyro_y;
        int16_t gyro_z;
    };

    void csLow()  { board_cs_clear(m_cs); }
    void csHigh() { board_cs_set(m_cs); }

    int8_t readRegister(uint8_t reg, uint8_t *data, uint16_t len);
    int8_t writeRegister(uint8_t reg, uint8_t data);
    int8_t selectBank(uint8_t bank);
    bool readRaw(RawData &raw);
};

void init_drv_icm20948();
DrvICM20948 *drv_icm20948();
