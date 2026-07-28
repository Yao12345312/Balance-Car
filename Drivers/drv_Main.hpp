#pragma once

#include <stdint.h>

struct SPI_Config;
struct I2C_Config;
struct UART_Config;
struct CAN_Config;
struct PWM_Config;

typedef struct
{
    uint8_t hw_num;
    const SPI_Config *config;
} BoardSpiEntry;

typedef struct
{
    uint8_t hw_num;
    const I2C_Config *config;
} BoardI2cEntry;

typedef struct
{
    uint8_t hw_num;
    const UART_Config *config;
} BoardUartEntry;

typedef struct
{
    uint8_t hw_num;
    const CAN_Config *config;
} BoardCanEntry;

typedef struct
{
    uint8_t hw_num;
    const PWM_Config *config;
} BoardPwmEntry;

//驱动初始化函数
void init_drv_Main();

//应用任务初始化函数
void create_application_tasks(void);