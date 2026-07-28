#pragma once

#include "drv_common.hpp"
#include "stm32h7xx_hal.h"
#include "cmsis_os2.h"
#include <stdint.h>

#define I2C_DRV_BUF_SIZE 256u

typedef enum
{
    I2C_Drv_Ok = 0,
    I2C_Drv_BadIndex,
    I2C_Drv_BadParam,
    I2C_Drv_Timeout,
    I2C_Drv_Busy,
    I2C_Drv_NAK,
    I2C_Drv_BusError,
    I2C_Drv_ArbLost,
    I2C_Drv_Error,
} I2C_DrvResult;

typedef enum
{
    I2C_SPEED_50k = 0,
    I2C_SPEED_100k,
    I2C_SPEED_400k,
} I2C_Speed;

struct I2C_Config
{
    HwPin scl;
    HwPin sda;

    uint32_t timing;
    I2C_Speed speed;

    uint32_t own_address;
    uint32_t pull;
    uint32_t speed_gpio;

    uint8_t irq_priority;
};

class I2C_CLASS
{
  public:
    bool init(uint8_t i2c_num, const I2C_Config *cfg);
    void deinit();

    I2C_DrvResult write(uint8_t dev_addr, const uint8_t *tx_data, uint16_t size, double timeout = -1.0);
    I2C_DrvResult read(uint8_t dev_addr, uint8_t *rx_data, uint16_t size, double timeout = -1.0);
    I2C_DrvResult writeRead(uint8_t dev_addr, const uint8_t *tx_data, uint16_t tx_size, uint8_t *rx_data, uint16_t rx_size, double timeout = -1.0);

    bool lock(double timeout = -1.0);
    void unlock();

    uint8_t number() const { return i2cNum; }
    I2C_HandleTypeDef *handle() { return &hi2c; }

  private:
    I2C_HandleTypeDef hi2c;
    osMutexId_t mutex;
    osSemaphoreId_t xferSem;

    uint8_t i2cNum;
    volatile I2C_DrvResult lastResult;

    void notify(I2C_DrvResult r);
    I2C_DrvResult waitComplete(double timeout);

    void onMasterTxCplt();
    void onMasterRxCplt();
    void onError();
    void onAbort();

    template <int N> friend void i2cDrvMemTxCallbackT(I2C_HandleTypeDef *);
    template <int N> friend void i2cDrvMemRxCallbackT(I2C_HandleTypeDef *);
    template <int N> friend void i2cDrvMasterTxCb(I2C_HandleTypeDef *);
    template <int N> friend void i2cDrvMasterRxCb(I2C_HandleTypeDef *);
    template <int N> friend void i2cDrvErrorCb(I2C_HandleTypeDef *);
    template <int N> friend void i2cDrvAbortCb(I2C_HandleTypeDef *);
    template <int N> friend void i2cDrvEvIrq();
    template <int N> friend void i2cDrvErIrq();
};

bool register_i2c(uint8_t i2c_num, const I2C_Config *cfg);
I2C_CLASS *get_i2c_instance(uint8_t i2c_num);
void init_drv_i2c(void);
