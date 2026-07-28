#pragma once

#include "drv_common.hpp"
#include "stm32h7xx_hal.h"
#include "cmsis_os2.h"
#include <stdint.h>

#define UART_DRV_RX_DMA_BUF 256u
#define UART_DRV_RX_RING    256u
#define UART_DRV_TX_DMA_BUF 128u

typedef enum
{
    UART_Drv_Ok = 0,
    UART_Drv_BadIndex,
    UART_Drv_BadParam,
    UART_Drv_Timeout,
    UART_Drv_Busy,
    UART_Drv_Error,
} UART_DrvResult;

struct UART_Config
{
    HwPin tx;
    HwPin rx;

    uint32_t baudrate;
    uint32_t word_length;
    uint32_t stop_bits;
    uint32_t parity;
    uint32_t hw_flow_ctl;

    uint32_t pull;
    uint32_t speed;
    bool swap;

    uint8_t irq_priority;

    HwDmaCh tx_dma;
    HwDmaCh rx_dma;
};

class UART_CLASS
{
  public:
    bool init(uint8_t uart_num, const UART_Config *cfg);
    void deinit();

    UART_DrvResult write(const uint8_t *data, uint16_t size, double send_wait = -1.0, double sync_wait = -1.0);
    uint16_t read(uint8_t *data, uint16_t size, double rc_wait = -1.0, double sync_wait = -1.0);

    bool lock(double sync_wait = -1.0);
    void unlock();
    bool resetRx(double sync_wait = -1.0);
    bool waitTxSent(double wait = -1.0);
    bool setBaudRate(uint32_t baudrate, double sync_wait = -1.0);

    uint8_t number() const { return uartNum; }
    UART_HandleTypeDef *handle() { return &huart; }

  private:
    UART_HandleTypeDef huart;
    DMA_HandleTypeDef hdmaTx;
    DMA_HandleTypeDef hdmaRx;

    osMutexId_t txMutex;
    osMutexId_t rxMutex;
    osSemaphoreId_t txCpltSem;
    osSemaphoreId_t rxSem;

    uint8_t rxRing[UART_DRV_RX_RING];
    volatile uint16_t rxHead;
    volatile uint16_t rxTail;

    uint8_t uartNum;
    bool useTxDma;
    bool useRxDma;

    alignas(32) uint8_t rxDmaBuf[UART_DRV_RX_DMA_BUF];
    alignas(32) uint8_t txDmaBuf[UART_DRV_TX_DMA_BUF];
    volatile uint16_t rxLastConsumed;
    uint8_t rxItByte;

    void startRx();
    void registerCallbacks();
    void onTxCplt();
    void onRxEvent(uint16_t pos);
    void onRxCplt();
    void onRxItCplt();
    void onError();
    void feedRx(const uint8_t *data, uint16_t len);
    uint16_t copyToRing(const uint8_t *data, uint16_t len);

    template <int N> friend void uartDrvTxCpltCb(UART_HandleTypeDef *);
    template <int N> friend void uartDrvRxCpltCb(UART_HandleTypeDef *);
    template <int N> friend void uartDrvErrorCb(UART_HandleTypeDef *);
    template <int N> friend void uartDrvAbortCb(UART_HandleTypeDef *);
    template <int N> friend void uartDrvRxEventCb(UART_HandleTypeDef *, uint16_t);
    template <int N> friend void uartDrvIrq();
};

bool register_uart(uint8_t uart_num, const UART_Config *cfg);
UART_CLASS *get_uart_instance(uint8_t uart_num);
void init_drv_uart(void);
