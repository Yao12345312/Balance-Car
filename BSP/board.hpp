#pragma once

#include <stdint.h>
#include "drv_Main.hpp"
#include "hardware/drv_can.hpp"
#include "hardware/drv_pwm.hpp"

typedef enum
{
    BOARD_OK = 0,
    BOARD_ERR_INVALID_PORT,
    BOARD_ERR_NOT_CONFIGURED,
    BOARD_ERR_PARAM,
    BOARD_ERR_TIMEOUT,
    BOARD_ERR_BUSY,
    BOARD_ERR_NAK,
    BOARD_ERR_BUS,
    BOARD_ERR_IO,
} BoardResult;

//为本板指定逻辑SPI端口名
typedef enum
{
    BOARD_SPI_NONE = 0,

    BOARD_SPI_OLED,         
    BOARD_SPI_ICM20948,

    BOARD_SPI_COUNT
} BoardSpiPort;

//为本板指定逻辑GPIO端口名
typedef enum
{
    BOARD_CS_NONE = 0,

    BOARD_CS_BMI088_ACC,
    BOARD_CS_BMI088_GYRO,
    BOARD_CS_ICM20948,

    BOARD_CS_COUNT
} BoardCsPin;

//为本板指定逻辑 I2C 端口名
typedef enum
{
    BOARD_I2C_NONE = 0,

    BOARD_I2C_TFMINI,       // TFmini-S 激光测距 (I2C4, PD12/PD13)

    BOARD_I2C_COUNT
} BoardI2cPort;

//为本板指定逻辑 UART 端口名
typedef enum
{
    BOARD_UART_NONE = 0,
	
    BOARD_UART_US100,        // US-100 超声波测距 (UART4, PA0/PA1)
    BOARD_UART_BT,           // 蓝牙模块 (USART1, PA9/PA10)

    BOARD_UART_COUNT
} BoardUartPort;

//为本板指定逻辑 CAN 端口名
typedef enum
{
    BOARD_CAN_NONE = 0,

    BOARD_CAN_ESC,          // 电调 CAN 总线 (FDCAN1)

    BOARD_CAN_COUNT
} BoardCanPort;

//为本板指定逻辑 PWM 端口名
typedef enum
{
    BOARD_PWM_NONE = 0,

    BOARD_PWM_BUZZER,       // 蜂鸣器 (TIM4, PD14)

    BOARD_PWM_COUNT
} BoardPwmPort;

//获取外设注册表
extern const BoardSpiEntry  board_spi_ports[BOARD_SPI_COUNT];
extern const BoardI2cEntry  board_i2c_ports[BOARD_I2C_COUNT];
extern const BoardUartEntry board_uart_ports[BOARD_UART_COUNT];
extern const BoardCanEntry  board_can_ports[BOARD_CAN_COUNT];
extern const BoardPwmEntry  board_pwm_ports[BOARD_PWM_COUNT];

void board_init(void);
void board_init_hardware(void);

//板级外设数据读写接口
BoardResult board_spi_write(BoardSpiPort port, const uint8_t *data, uint16_t size, double timeout = -1.0);
BoardResult board_spi_read(BoardSpiPort port, uint8_t *data, uint16_t size, double timeout = -1.0);
BoardResult board_spi_write_read(BoardSpiPort port, const uint8_t *tx, uint8_t *rx, uint16_t size, double timeout = -1.0);
bool        board_spi_lock(BoardSpiPort port, double timeout = -1.0);
void        board_spi_unlock(BoardSpiPort port);

void        board_cs_init(BoardCsPin cs);
void        board_cs_set(BoardCsPin cs);
void        board_cs_clear(BoardCsPin cs);

BoardResult board_i2c_write(BoardI2cPort port, uint8_t dev_addr, const uint8_t *data, uint16_t size, double timeout = -1.0);
BoardResult board_i2c_read(BoardI2cPort port, uint8_t dev_addr, uint8_t *data, uint16_t size, double timeout = -1.0);
BoardResult board_i2c_write_read(BoardI2cPort port, uint8_t dev_addr, const uint8_t *tx, uint16_t tx_size, uint8_t *rx, uint16_t rx_size, double timeout = -1.0);

BoardResult board_uart_write(BoardUartPort port, const uint8_t *data, uint16_t size, double send_wait = -1.0, double sync_wait = -1.0);
uint16_t    board_uart_read(BoardUartPort port, uint8_t *data, uint16_t size, double rc_wait = -1.0, double sync_wait = -1.0);
bool        board_uart_set_baudrate(BoardUartPort port, uint32_t baudrate);
bool        board_uart_reset_rx(BoardUartPort port);

// CAN 数据收发接口
CAN_DrvResult board_can_send(BoardCanPort port, const CAN_Msg *msg, double timeout = -1.0);
uint16_t      board_can_read(BoardCanPort port, CAN_Msg *msg, uint16_t max_msgs, double timeout = -1.0);

// PWM 输出接口 (channels 参数使用 drv_pwm.hpp 中的 PWM_CH_x 位掩码)
BoardResult board_pwm_start(BoardPwmPort port, uint32_t channels);
BoardResult board_pwm_stop(BoardPwmPort port, uint32_t channels);
BoardResult board_pwm_set_pulse(BoardPwmPort port, uint32_t channel, uint32_t pulse);
BoardResult board_pwm_set_duty(BoardPwmPort port, uint32_t channel, float duty);
BoardResult board_pwm_set_frequency(BoardPwmPort port, uint32_t prescaler, uint32_t period);

