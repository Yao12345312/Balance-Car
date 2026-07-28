#pragma once

#include "board.hpp"
#include "cmsis_os2.h"

extern "C"
{
#include "bmi08x.h"
}

#define BMI088_READ_WRITE_LEN  64
#define BMI088_SPI_BUF_SIZE    128

class DrvBMI088
{
public:
    DrvBMI088(BoardSpiPort spi_port, BoardCsPin acc_cs, BoardCsPin gyro_cs);

    int8_t init();

    bmi08_dev *getDev() { return &m_dev; }

    int8_t getAccelData(float &ax, float &ay, float &az);
    int8_t getGyroData(float &gx, float &gy, float &gz);

    int8_t getAccelDataCalibrated(float &ax, float &ay, float &az);
    int8_t getGyroDataCalibrated(float &gx, float &gy, float &gz);

    int8_t calibrateAccelStatic();
    int8_t calibrateGyroStatic();
    int8_t calibrateAllStatic();

    bool isAccelCalibrated() const { return m_accel_calibrated; }
    bool isGyroCalibrated() const { return m_gyro_calibrated; }

    void setGyroIirAlpha(float alpha) { m_gyro_iir_alpha = alpha; }

private:
    BoardSpiPort m_spi_port;
    BoardCsPin   m_acc_cs;
    BoardCsPin   m_gyro_cs;
    uint8_t      m_currentDev;

    bmi08_dev m_dev;

    float m_accel_offset[3];
    float m_gyro_offset[3];
    bool  m_accel_calibrated;
    bool  m_gyro_calibrated;

    float m_gyro_iir_alpha;
    float m_gyro_iir_state[3];
    bool  m_gyro_iir_init;

    osMutexId_t m_mutex = NULL;

    void csLow()  { board_cs_clear(m_currentDev == 0 ? m_acc_cs : m_gyro_cs); }
    void csHigh() { board_cs_set  (m_currentDev == 0 ? m_acc_cs : m_gyro_cs); }

    static BMI08_INTF_RET_TYPE spiRead(uint8_t reg_addr, uint8_t *reg_data,
                                       uint32_t len, void *intf_ptr);
    static BMI08_INTF_RET_TYPE spiWrite(uint8_t reg_addr, const uint8_t *reg_data,
                                        uint32_t len, void *intf_ptr);
    static void delayUs(uint32_t period, void *intf_ptr);

    float getAccelSensitivity();
    float getGyroSensitivity();
};

void init_drv_bmi088();
DrvBMI088 *drv_bmi088();
