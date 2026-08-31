#include "board.hpp"

#include "hardware/drv_spi.hpp"
#include "hardware/drv_i2c.hpp"
#include "hardware/drv_uart.hpp"
#include "hardware/drv_can.hpp"
#include "hardware/drv_pwm.hpp"
#include "hardware/drv_common.hpp"

#include "input/drv_key.hpp"
#include "drv_Main.hpp"
#include "TimeBase.h"
#include "basic.hpp"


// =============================================================================
// 板级初始化 (时钟/DMA 等)
// =============================================================================

void board_init_hardware(void)
{
    HW_InitCommon();
}

void board_init(void)
{
	//初始化时钟
    init_TimeBase();

	//允许在 Sleep/Stop/Standby 模式下保持 SWD 调试连接
	//(空闲钩子 vApplicationIdleHook 调用 __WFI, 若不设置则会 "Invalid ROM Table")
	HAL_DBGMCU_EnableDBGSleepMode();
	HAL_DBGMCU_EnableDBGStopMode();
	HAL_DBGMCU_EnableDBGStandbyMode();
	
	//配置 MPU 并启用 I-Cache / D-Cache
	//AXI SRAM (DMA 缓冲区所在) 设为不可缓存, 消除 DMA 一致性问题
	mpu_and_cache_init();
	
	//初始化基础硬件 (DMA 时钟等)
    board_init_hardware();
}

// =============================================================================
// 端口定义表 
//
// 填写规则:
//   .hw_num  = 物理外设号 (SPI:1-6  I2C:1-4  UART:1-8),  0 = 该逻辑端口未使用
//   .config  = 指向该外设的完整配置 (引脚/AF/DMA/波特率...),  NULL = 未使用
//
// 每个物理外设号 (例如 SPI1) 在三张表里最多出现一次; 同一逻辑端口名只对应一个物理外设。
// 应用层只认 BOARD_SPI_xxx / BOARD_I2C_xxx / BOARD_UART_xxx 这些逻辑名, 看不到物理号。
//
// =============================================================================


// ===============================SPI=====================================

// OLED 显示屏 (SPI1)
// SCK = PA5 AF5, MOSI = PA7 AF5 (OLED 只写不读, MISO 不接)

static const SPI_Config cfg_spi1 = {
    .sck  = HW_PIN(0, 5, 5),        // PA5  AF5  (SPI1_SCK)
    .miso = {0, 0, 0},              // OLED 无 MISO, 不配置
    .mosi = HW_PIN(0, 7, 5),        // PA7  AF5  (SPI1_MOSI)
    .mode = SPI_MODE_0,
    .data_size  = SPI_DATASIZE_8BIT,
    .firstbit   = SPI_FIRSTBIT_MSB,
    .baudrate_prescaler = SPI_BAUDRATEPRESCALER_32,
    .nss        = SPI_NSS_SOFT,
    .pull       = GPIO_NOPULL,
    .speed      = GPIO_SPEED_FREQ_HIGH,
    .tx_dma     = HW_DMA_NONE,
    .rx_dma     = HW_DMA_NONE,
    .irq_priority = 5,
};

// [已停用] 原 BMI088 IMU 配置 (SPI1 引脚现划归 OLED)
// static const SPI_Config cfg_spi1_bmi088 = {
//     .sck  = HW_PIN(0, 5, 5),  .miso = HW_PIN(0, 6, 5),  .mosi = HW_PIN(0, 7, 5),
//     ...
// };

static const SPI_Config cfg_spi2 = {
    .sck  = HW_PIN(1, 13, 5),       // PB13 AF5 (SPI2_SCK)
    .miso = HW_PIN(1, 14, 5),        // PB14 AF5 (SPI2_MISO)
    .mosi = HW_PIN(1, 15, 5),       // PB15 AF5 (SPI2_MOSI)
    .mode = SPI_MODE_0,
    .data_size  = SPI_DATASIZE_8BIT,
    .firstbit   = SPI_FIRSTBIT_MSB,
    .baudrate_prescaler = SPI_BAUDRATEPRESCALER_32,
    .nss        = SPI_NSS_SOFT,
    .pull       = GPIO_NOPULL,
    .speed      = GPIO_SPEED_FREQ_HIGH,
    .tx_dma     = HW_DMA_NONE,
    .rx_dma     = HW_DMA_NONE,
    .irq_priority = 5,
};


const BoardSpiEntry board_spi_ports[BOARD_SPI_COUNT] = {
    {0, NULL},              // BOARD_SPI_NONE
    {1, &cfg_spi1},         // BOARD_SPI_OLED
    {2, &cfg_spi2},         // BOARD_SPI_ICM20948
};


// ===============================SPI=====================================

// ===============================IIC=====================================

// TFmini-S 激光测距 (I2C4)
// PD12 AF4 (I2C4_SCL), PD13 AF4 (I2C4_SDA)
static const I2C_Config cfg_i2c4 = {
    .scl      = HW_PIN(3, 12, 4),   // PD12 AF4 (I2C4_SCL)
    .sda      = HW_PIN(3, 13, 4),   // PD13 AF4 (I2C4_SDA)
    .timing   = 0,         //默认100khz
    .speed    = I2C_SPEED_100k,
    .own_address = 0,
    .pull      = GPIO_PULLUP,
    .speed_gpio= GPIO_SPEED_FREQ_LOW,
    .irq_priority = 5,
};


const BoardI2cEntry board_i2c_ports[BOARD_I2C_COUNT] = {
    {0, NULL},               // BOARD_I2C_NONE      
    {4, &cfg_i2c4},          // BOARD_I2C_TFMINI    
};

// ===============================IIC=====================================

// ===============================UART=====================================

static const UART_Config cfg_uart1 = {
    .tx        = HW_PIN(0, 9, 7),   // PA9  AF7 (USART1_TX)
    .rx        = HW_PIN(0, 10, 7),  // PA10 AF7 (USART1_RX)
    .baudrate  = 115200,            // 初始波特率, BluetoothDriver::autoBaudScan 会切换
    .word_length = UART_WORDLENGTH_8B,
    .stop_bits   = UART_STOPBITS_1,
    .parity      = UART_PARITY_NONE,
    .hw_flow_ctl = UART_HWCONTROL_NONE,
    .pull      = GPIO_PULLUP,
    .speed     = GPIO_SPEED_FREQ_HIGH,
    .swap      = false,
    .irq_priority = 5,
    .tx_dma    = HW_DMA(1, 5),      // DMA1_Stream5 (USART1_TX)
    .rx_dma    = HW_DMA(1, 4),      // DMA1_Stream4 (USART1_RX)
};

static const UART_Config cfg_uart4 = {
    .tx        = HW_PIN(0, 0, 8),   // PA0 AF8 (UART4_TX)
    .rx        = HW_PIN(0, 1, 8),   // PA1 AF8 (UART4_RX)
    .baudrate  = 9600,              // US-100 超声波模块默认波特率
    .word_length = UART_WORDLENGTH_8B,
    .stop_bits   = UART_STOPBITS_1,
    .parity      = UART_PARITY_NONE,
    .hw_flow_ctl = UART_HWCONTROL_NONE,
    .pull      = GPIO_PULLUP,
    .speed     = GPIO_SPEED_FREQ_HIGH,
    .swap      = false,
    .irq_priority = 5,
    .tx_dma    = HW_DMA_NONE,		//不使用DMA
    .rx_dma    = HW_DMA_NONE,		//不使用DMA
};

const BoardUartEntry board_uart_ports[BOARD_UART_COUNT] = {
    {0, NULL},               // BOARD_UART_NONE
    {4, &cfg_uart4},         // BOARD_UART_US100
    {1, &cfg_uart1},         // BOARD_UART_BT
};

// ===============================UART=====================================


// ===============================CAN=====================================

// 波特率计算: FDCAN_CLK / (Prescaler × (1 + TimeSeg1 + TimeSeg2)) = 16MHz / (2 × 16) = 500kHz
static const CAN_Config cfg_can1 = {
    .tx          = HW_PIN(3, 1, 9),    // PD1 AF9 (FDCAN1_TX)
    .rx          = HW_PIN(3, 0, 9),    // PD0 AF9 (FDCAN1_RX)
    .prescaler   = 2,
    .time_seg1   = 12,
    .time_seg2   = 3,
    .sjw         = 1,
    .irq_priority = 5,
};

const BoardCanEntry board_can_ports[BOARD_CAN_COUNT] = {
    {0, NULL},               // BOARD_CAN_NONE
    {1, &cfg_can1},          // BOARD_CAN_ESC
};

// ===============================CAN=====================================


// ===============================PWM=====================================



// TIM4 时钟 = 200MHz (APB1 定时器)
// 蜂鸣器使用 CH3 -> PD14 AF2 (TIM4_CH3)
// prescaler/period 由 DrvBuzzer 在运行时动态修改以产生不同音调
static const PWM_Config cfg_pwm4 = {
    .ch1          = {0, 0, 0},           //CH1 未用
    .ch2          = {0, 0, 0},           //CH2 未用
    .ch3          = HW_PIN(3, 14, 2),    //CH3 -> PD14 AF2 (TIM4_CH3)
    .ch4          = {0, 0, 0},           //CH4 未用
    .pulse1       = 0,
    .pulse2       = 0,
    .pulse3       = 0,
    .pulse4       = 0,
    .prescaler    = 199,                 //初始 1MHz 计数频率 (与 DrvBuzzer 计算一致)
    .period       = 999,                 //初始 1kHz (会被 setFrequency 覆盖)
    .counter_mode = TIM_COUNTERMODE_UP,
    .clockdivision= TIM_CLOCKDIVISION_DIV1,
    .pull         = GPIO_NOPULL,
    .speed        = GPIO_SPEED_FREQ_LOW,
    .irq_priority = 5,
};

const BoardPwmEntry board_pwm_ports[BOARD_PWM_COUNT] = {
    {0, NULL},               // BOARD_PWM_NONE
    {4, &cfg_pwm4},          // BOARD_PWM_BUZZER
};

// ===============================PWM=====================================


// =============================================================================
// 板级片选 (CS) 引脚定义
// =============================================================================

static const HwPin board_cs_pins[BOARD_CS_COUNT] = {
    {0, 0, 0},               // BOARD_CS_NONE
	
    HW_PIN(2, 4, 0),         // BOARD_CS_BMI088_ACC  -> PC4
    HW_PIN(2, 5, 0),         // BOARD_CS_BMI088_GYRO -> PC5
    HW_PIN(1, 12, 0),        // BOARD_CS_ICM20948    -> PB12
};


void board_cs_init(BoardCsPin cs)
{
    if (cs <= BOARD_CS_NONE || cs >= BOARD_CS_COUNT)
        return;
    const HwPin *p = &board_cs_pins[cs];
    if (!p->port && !p->pin)
        return;
    HW_ConfigurePinOutput(p, HW_OTYPE_PUSH_PULL, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);
    HAL_GPIO_WritePin(HW_GpioTable[p->port], (uint16_t)(1u << p->pin), GPIO_PIN_SET);
}

void board_cs_set(BoardCsPin cs)
{
    if (cs <= BOARD_CS_NONE || cs >= BOARD_CS_COUNT)
        return;
    const HwPin *p = &board_cs_pins[cs];
    HAL_GPIO_WritePin(HW_GpioTable[p->port], (uint16_t)(1u << p->pin), GPIO_PIN_SET);
}

void board_cs_clear(BoardCsPin cs)
{
    if (cs <= BOARD_CS_NONE || cs >= BOARD_CS_COUNT)
        return;
    const HwPin *p = &board_cs_pins[cs];
    HAL_GPIO_WritePin(HW_GpioTable[p->port], (uint16_t)(1u << p->pin), GPIO_PIN_RESET);
}

// =============================================================================
// 板级三色 LED 配置 (R=PD5 G=PD4 B=PD3)
// =============================================================================

// 三色 LED 引脚 (无对应 TIM 复用通道, 直接 GPIO 高低电平控制, 高电平点亮)
static const HwPin board_led_pins[BOARD_LED_COUNT] = {
    {0, 0, 0},               // BOARD_LED_NONE

    HW_PIN(3, 3, 0),         // BOARD_LED_R -> PD3
    HW_PIN(3, 5, 0),         // BOARD_LED_G -> PD5
    HW_PIN(3, 4, 0),         // BOARD_LED_B -> PD4
};

void board_led_init(BoardLedPin led)
{
    if (led <= BOARD_LED_NONE || led >= BOARD_LED_COUNT)
        return;
    const HwPin *p = &board_led_pins[led];
    if (!p->port && !p->pin)
        return;
    HW_ConfigurePinOutput(p, HW_OTYPE_PUSH_PULL, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    HAL_GPIO_WritePin(HW_GpioTable[p->port], (uint16_t)(1u << p->pin), GPIO_PIN_RESET);
}

void board_led_set(BoardLedPin led, bool on)
{
    if (led <= BOARD_LED_NONE || led >= BOARD_LED_COUNT)
        return;
    const HwPin *p = &board_led_pins[led];
    HAL_GPIO_WritePin(HW_GpioTable[p->port], (uint16_t)(1u << p->pin),
                      on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

// =============================================================================
// 板级按键配置表
// =============================================================================

// 按键引脚 ( KEY1=PB4 KEY2=PD7 KEY3=PD6)
// 接 VCC, 内部下拉, 高电平有效
const KeyConfig board_key_configs[BOARD_KEY_COUNT] = {
    { {0, 0, 0},                 DrvKey::ActiveLevel::High, 0,    0,  GPIO_NOPULL    },  // BOARD_KEY_NONE

    { HW_PIN(1, 4, 0),           DrvKey::ActiveLevel::High, 1000, 20, GPIO_PULLDOWN  },  // BOARD_KEY_1 -> PB4
    { HW_PIN(3, 7, 0),           DrvKey::ActiveLevel::High, 1000, 20, GPIO_PULLDOWN  },  // BOARD_KEY_2 -> PD7
    { HW_PIN(3, 6, 0),           DrvKey::ActiveLevel::High, 1000, 20, GPIO_PULLDOWN  },  // BOARD_KEY_3 -> PD6
};



// =============================================================================
// 应用层统一 SPI 读写接口
// =============================================================================

static SPI_CLASS *spi_of(BoardSpiPort port)
{
    if ((uint8_t)port >= (uint8_t)BOARD_SPI_COUNT)
        return 0;
    uint8_t hw = board_spi_ports[port].hw_num;
    if (hw == 0)
        return 0;
    return get_spi_instance(hw);
}

static BoardResult spi_map_result(SPI_DrvResult r)
{
    switch (r)
    {
        case SPI_Drv_Ok:       return BOARD_OK;
        case SPI_Drv_Timeout:  return BOARD_ERR_TIMEOUT;
        case SPI_Drv_Busy:     return BOARD_ERR_BUSY;
        case SPI_Drv_BadParam: return BOARD_ERR_PARAM;
        default:               return BOARD_ERR_IO;
    }
}

BoardResult board_spi_write(BoardSpiPort port, const uint8_t *data, uint16_t size, double timeout)
{
    SPI_CLASS *s = spi_of(port);
    if (!s) return BOARD_ERR_NOT_CONFIGURED;
    return spi_map_result(s->write(data, size, timeout));
}

BoardResult board_spi_read(BoardSpiPort port, uint8_t *data, uint16_t size, double timeout)
{
    SPI_CLASS *s = spi_of(port);
    if (!s) return BOARD_ERR_NOT_CONFIGURED;
    return spi_map_result(s->read(data, size, timeout));
}

BoardResult board_spi_write_read(BoardSpiPort port, const uint8_t *tx, uint8_t *rx, uint16_t size, double timeout)
{
    SPI_CLASS *s = spi_of(port);
    if (!s) return BOARD_ERR_NOT_CONFIGURED;
    return spi_map_result(s->transmitReceive(tx, rx, size, timeout));
}

bool board_spi_lock(BoardSpiPort port, double timeout)
{
    SPI_CLASS *s = spi_of(port);
    if (!s) return false;
    return s->lock(timeout);
}

void board_spi_unlock(BoardSpiPort port)
{
    SPI_CLASS *s = spi_of(port);
    if (s) s->unlock();
}

// =============================================================================
// 应用层统一 I2C 读写接口
// =============================================================================

static I2C_CLASS *i2c_of(BoardI2cPort port)
{
    if ((uint8_t)port >= (uint8_t)BOARD_I2C_COUNT)
        return 0;
    uint8_t hw = board_i2c_ports[port].hw_num;
    if (hw == 0)
        return 0;
    return get_i2c_instance(hw);
}

static BoardResult i2c_map_result(I2C_DrvResult r)
{
    switch (r)
    {
        case I2C_Drv_Ok:       return BOARD_OK;
        case I2C_Drv_Timeout:  return BOARD_ERR_TIMEOUT;
        case I2C_Drv_NAK:      return BOARD_ERR_NAK;
        case I2C_Drv_BusError: return BOARD_ERR_BUS;
        case I2C_Drv_ArbLost:  return BOARD_ERR_BUS;
        case I2C_Drv_BadParam: return BOARD_ERR_PARAM;
        default:               return BOARD_ERR_IO;
    }
}

BoardResult board_i2c_write(BoardI2cPort port, uint8_t dev_addr, const uint8_t *data, uint16_t size, double timeout)
{
    I2C_CLASS *b = i2c_of(port);
    if (!b) return BOARD_ERR_NOT_CONFIGURED;
    return i2c_map_result(b->write(dev_addr, data, size, timeout));
}

BoardResult board_i2c_read(BoardI2cPort port, uint8_t dev_addr, uint8_t *data, uint16_t size, double timeout)
{
    I2C_CLASS *b = i2c_of(port);
    if (!b) return BOARD_ERR_NOT_CONFIGURED;
    return i2c_map_result(b->read(dev_addr, data, size, timeout));
}

BoardResult board_i2c_write_read(BoardI2cPort port, uint8_t dev_addr, const uint8_t *tx, uint16_t tx_size, uint8_t *rx, uint16_t rx_size, double timeout)
{
    I2C_CLASS *b = i2c_of(port);
    if (!b) return BOARD_ERR_NOT_CONFIGURED;
    return i2c_map_result(b->writeRead(dev_addr, tx, tx_size, rx, rx_size, timeout));
}

// =============================================================================
// 应用层统一 UART 读写接口
// =============================================================================

static UART_CLASS *uart_of(BoardUartPort port)
{
    if ((uint8_t)port >= (uint8_t)BOARD_UART_COUNT)
        return 0;
    uint8_t hw = board_uart_ports[port].hw_num;
    if (hw == 0)
        return 0;
    return get_uart_instance(hw);
}

BoardResult board_uart_write(BoardUartPort port, const uint8_t *data, uint16_t size, double send_wait, double sync_wait)
{
    UART_CLASS *u = uart_of(port);
    if (!u) return BOARD_ERR_NOT_CONFIGURED;
    UART_DrvResult r = u->write(data, size, send_wait, sync_wait);
    switch (r)
    {
        case UART_Drv_Ok:       return BOARD_OK;
        case UART_Drv_Timeout:  return BOARD_ERR_TIMEOUT;
        case UART_Drv_Busy:     return BOARD_ERR_BUSY;
        case UART_Drv_BadParam: return BOARD_ERR_PARAM;
        default:                return BOARD_ERR_IO;
    }
}

uint16_t board_uart_read(BoardUartPort port, uint8_t *data, uint16_t size, double rc_wait, double sync_wait)
{
    UART_CLASS *u = uart_of(port);
    if (!u) 
		return 0;
    return u->read(data, size, rc_wait, sync_wait);
}

bool board_uart_set_baudrate(BoardUartPort port, uint32_t baudrate)
{
    UART_CLASS *u = uart_of(port);
    if (!u)
		return false;
    return u->setBaudRate(baudrate);
}

bool board_uart_reset_rx(BoardUartPort port)
{
    UART_CLASS *u = uart_of(port);
    if (!u) 
		return false;
    return u->resetRx();
}

// =============================================================================
// 应用层统一 CAN 收发接口
// =============================================================================

static CAN_CLASS *can_of(BoardCanPort port)
{
    if ((uint8_t)port >= (uint8_t)BOARD_CAN_COUNT)
        return 0;
    uint8_t hw = board_can_ports[port].hw_num;
    if (hw == 0)
        return 0;
    return get_can_instance(hw);
}

CAN_DrvResult board_can_send(BoardCanPort port, const CAN_Msg *msg, double timeout)
{
    CAN_CLASS *c = can_of(port);
    if (!c) return CAN_Drv_BadIndex;
    return c->send(msg, timeout);
}

uint16_t board_can_read(BoardCanPort port, CAN_Msg *msg, uint16_t max_msgs, double timeout)
{
    CAN_CLASS *c = can_of(port);
    if (!c) return 0;
    return c->read(msg, max_msgs, timeout);
}

// =============================================================================
// 应用层统一 PWM 输出接口
// =============================================================================

static PWM_CLASS *pwm_of(BoardPwmPort port)
{
    if ((uint8_t)port >= (uint8_t)BOARD_PWM_COUNT)
        return 0;
    uint8_t hw = board_pwm_ports[port].hw_num;
    if (hw == 0)
        return 0;
    return get_pwm_instance(hw);
}

static BoardResult pwm_map_result(PWM_DrvResult r)
{
    switch (r)
    {
        case PWM_Drv_Ok:       return BOARD_OK;
        case PWM_Drv_Timeout:  return BOARD_ERR_TIMEOUT;
        case PWM_Drv_Busy:     return BOARD_ERR_BUSY;
        case PWM_Drv_BadParam: return BOARD_ERR_PARAM;
        default:               return BOARD_ERR_IO;
    }
}

BoardResult board_pwm_start(BoardPwmPort port, uint32_t channels)
{
    PWM_CLASS *p = pwm_of(port);
    if (!p) return BOARD_ERR_NOT_CONFIGURED;
    return pwm_map_result(p->start(channels));
}

BoardResult board_pwm_stop(BoardPwmPort port, uint32_t channels)
{
    PWM_CLASS *p = pwm_of(port);
    if (!p) return BOARD_ERR_NOT_CONFIGURED;
    return pwm_map_result(p->stop(channels));
}

BoardResult board_pwm_set_pulse(BoardPwmPort port, uint32_t channel, uint32_t pulse)
{
    PWM_CLASS *p = pwm_of(port);
    if (!p) return BOARD_ERR_NOT_CONFIGURED;
    return pwm_map_result(p->setPulse(channel, pulse));
}

BoardResult board_pwm_set_duty(BoardPwmPort port, uint32_t channel, float duty)
{
    PWM_CLASS *p = pwm_of(port);
    if (!p) return BOARD_ERR_NOT_CONFIGURED;
    return pwm_map_result(p->setDuty(channel, duty));
}

BoardResult board_pwm_set_frequency(BoardPwmPort port, uint32_t prescaler, uint32_t period)
{
    PWM_CLASS *p = pwm_of(port);
    if (!p) return BOARD_ERR_NOT_CONFIGURED;
    return pwm_map_result(p->setFrequency(prescaler, period));
}
