#pragma once

#include "drv_common.hpp"
#include "stm32h7xx_hal.h"
#include "cmsis_os2.h"
#include <stdint.h>

#define SPI_MODE_0  0
#define SPI_MODE_1  1
#define SPI_MODE_2  2
#define SPI_MODE_3  3

#define SPI_DMA_BUF_SIZE 256u

typedef enum
{
    SPI_Drv_Ok = 0,
    SPI_Drv_BadIndex,
    SPI_Drv_BadParam,
    SPI_Drv_Timeout,
    SPI_Drv_Busy,
    SPI_Drv_Error,
} SPI_DrvResult;

struct SPI_Config
{
    HwPin sck;
    HwPin miso;
    HwPin mosi;

    uint32_t mode;
    uint32_t data_size;
    uint32_t firstbit;
    uint32_t baudrate_prescaler;
    uint32_t nss;

    uint32_t pull;
    uint32_t speed;

    HwDmaCh tx_dma;
    HwDmaCh rx_dma;

    uint8_t irq_priority;
};

class SPI_CLASS
{
  public:
    bool init(uint8_t spi_num, const SPI_Config *cfg);
    void deinit();

    SPI_DrvResult transmitReceive(const uint8_t *tx_data, uint8_t *rx_data, uint16_t size, double timeout = -1.0);
    SPI_DrvResult write(const uint8_t *tx_data, uint16_t size, double timeout = -1.0);
    SPI_DrvResult read(uint8_t *rx_data, uint16_t size, double timeout = -1.0);

    bool lock(double timeout = -1.0);
    void unlock();

    uint8_t number() const { return spiNum; }
    bool usesDma() const { return useDma; }
    SPI_HandleTypeDef *handle() { return &hspi; }

  private:
    SPI_HandleTypeDef hspi;
    DMA_HandleTypeDef hdmaTx;
    DMA_HandleTypeDef hdmaRx;
    osMutexId_t mutex;
    osSemaphoreId_t xferSem;

    uint8_t spiNum;
    bool useDma;
    volatile SPI_DrvResult lastResult;

    alignas(32) uint8_t txBounce[SPI_DMA_BUF_SIZE];
    alignas(32) uint8_t rxBounce[SPI_DMA_BUF_SIZE];

    void notify(SPI_DrvResult r);
    void onTransferCplt();
    void onTransferErr();

    template <int N> friend void spiDrvTxCpltCb(SPI_HandleTypeDef *);
    template <int N> friend void spiDrvRxCpltCb(SPI_HandleTypeDef *);
    template <int N> friend void spiDrvTxRxCpltCb(SPI_HandleTypeDef *);
    template <int N> friend void spiDrvAbortCpltCb(SPI_HandleTypeDef *);
    template <int N> friend void spiDrvErrorCb(SPI_HandleTypeDef *);
};

bool register_spi(uint8_t spi_num, const SPI_Config *cfg);
SPI_CLASS *get_spi_instance(uint8_t spi_num);
void init_drv_spi(void);
