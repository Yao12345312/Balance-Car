#pragma once

#include "drv_common.hpp"
#include "stm32h7xx_hal.h"
#include "cmsis_os2.h"
#include <stdint.h>

#define CAN_RX_FIFO_SIZE 32

typedef enum
{
    CAN_Drv_Ok = 0,
    CAN_Drv_BadIndex,
    CAN_Drv_BadParam,
    CAN_Drv_Timeout,
    CAN_Drv_Busy,
    CAN_Drv_Error,
} CAN_DrvResult;

struct CAN_Msg
{
    uint32_t id;
    bool is_ext;
    bool is_remote;
    uint8_t dlc;
    uint8_t data[8];
};

struct CAN_Config
{
    HwPin tx;
    HwPin rx;

    uint32_t prescaler;
    uint32_t time_seg1;
    uint32_t time_seg2;
    uint32_t sjw;

    uint8_t irq_priority;
};

class CAN_CLASS
{
  public:
    bool init(uint8_t can_num, const CAN_Config *cfg);
    void deinit();

    CAN_DrvResult send(const CAN_Msg *msg, double timeout = -1.0);
    uint16_t read(CAN_Msg *msg, uint16_t max_msgs, double timeout = -1.0);

    bool lock(double timeout = -1.0);
    void unlock();

    uint8_t number() const { return canNum; }
    FDCAN_HandleTypeDef *handle() { return &hfdcan; }

  private:
    FDCAN_HandleTypeDef hfdcan;
    osMutexId_t mutex;
    osSemaphoreId_t txSem;
    osSemaphoreId_t rxSem;

    uint8_t canNum;
    volatile CAN_DrvResult txResult;

    CAN_Msg rxRing[CAN_RX_FIFO_SIZE];
    volatile uint8_t rxHead;
    volatile uint8_t rxTail;

    void feedRx(const CAN_Msg *msg);
    void onTxFifoEmpty();
    void onRxNewMsg();
    void onError();

    template <int N> friend void canDrvTxFifoEmptyCb(FDCAN_HandleTypeDef *);
    template <int N> friend void canDrvRxNewMsgCb(FDCAN_HandleTypeDef *);
    template <int N> friend void canDrvErrorCb(FDCAN_HandleTypeDef *);
    template <int N> friend void canDrvIrq0();
    template <int N> friend void canDrvIrq1();
};

bool register_can(uint8_t can_num, const CAN_Config *cfg);
CAN_CLASS *get_can_instance(uint8_t can_num);
void init_drv_can(void);
