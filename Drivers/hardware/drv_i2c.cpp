#include "drv_i2c.hpp"
#include "drv_common.hpp"

#include "cmsis_os2.h"
#include "board.hpp"

static const osMutexAttr_t s_recursiveMutexAttr = {.attr_bits = osMutexRecursive};

#include "stm32h743xx.h"
#include <string.h>

static I2C_CLASS i2c_instances[HW_DRV_MAX_I2C];
static bool i2c_inited[HW_DRV_MAX_I2C] = {};

static I2C_TypeDef *const i2cTable[HW_DRV_MAX_I2C] = {0, I2C1, I2C2, I2C3, I2C4};
static const IRQn_Type i2cEvIrqn[HW_DRV_MAX_I2C] = {(IRQn_Type)0, I2C1_EV_IRQn, I2C2_EV_IRQn, I2C3_EV_IRQn, I2C4_EV_IRQn};
static const IRQn_Type i2cErIrqn[HW_DRV_MAX_I2C] = {(IRQn_Type)0, I2C1_ER_IRQn, I2C2_ER_IRQn, I2C3_ER_IRQn, I2C4_ER_IRQn};

static void i2c_clk_enable(uint8_t num)
{
    switch (num)
    {
        case 1: __HAL_RCC_I2C1_CLK_ENABLE(); break;
        case 2: __HAL_RCC_I2C2_CLK_ENABLE(); break;
        case 3: __HAL_RCC_I2C3_CLK_ENABLE(); break;
        case 4: __HAL_RCC_I2C4_CLK_ENABLE(); break;
        default: break;
    }
}

static uint32_t i2c_kernel_clock_hz(uint8_t num)
{
    if (num == 4)
        return HAL_RCCEx_GetD3PCLK1Freq();
    return HAL_RCC_GetPCLK1Freq();
}

static uint32_t i2c_calc_timing(uint32_t f_i2cclk, I2C_Speed speed)
{
    if (f_i2cclk == 0)
        return 0x10C0ECFFu;

    uint32_t target_freq, scll_min_ns, sclh_min_ns, sdadel_min_ns, scldel_min_ns;

    switch (speed)
    {
        case I2C_SPEED_400k:
            target_freq = 400000;
            scll_min_ns = 650;
            sclh_min_ns = 340;
            sdadel_min_ns = 100;
            scldel_min_ns = 200;
            break;
        case I2C_SPEED_50k:
            target_freq = 50000;
            scll_min_ns = 5200;
            sclh_min_ns = 4300;
            sdadel_min_ns = 350;
            scldel_min_ns = 350;
            break;
        default:
            target_freq = 100000;
            scll_min_ns = 4900;
            sclh_min_ns = 4200;
            sdadel_min_ns = 280;
            scldel_min_ns = 320;
            break;
    }

    for (uint32_t presc = 0; presc <= 15; ++presc)
    {
        uint32_t f_presc = f_i2cclk / (presc + 1);
        uint32_t scll_min = ((uint64_t)scll_min_ns * f_presc + 999999999ULL) / 1000000000ULL;
        uint32_t sclh_min = ((uint64_t)sclh_min_ns * f_presc + 999999999ULL) / 1000000000ULL;
        uint32_t sdadel = ((uint64_t)sdadel_min_ns * f_presc + 999999999ULL) / 1000000000ULL;
        uint32_t scldel = ((uint64_t)scldel_min_ns * f_presc + 999999999ULL) / 1000000000ULL;

        if (scll_min < 2) scll_min = 2;
        if (sclh_min < 2) sclh_min = 2;
        if (sdadel > 15) sdadel = 15;
        if (scldel > 15) scldel = 15;

        uint32_t period = f_presc / target_freq;
        if (period < scll_min + sclh_min + 8)
            continue;

        uint32_t sclh = sclh_min;
        uint32_t scll = period - sclh - 6;
        if (scll < scll_min || scll > 255 || sclh > 255)
            continue;

        return ((presc << 28) | (scldel << 20) | (sdadel << 16) | (sclh << 8) | scll);
    }
    return 0x10C0ECFFu;
}

template <int N> void i2cDrvMasterTxCb(I2C_HandleTypeDef *) { i2c_instances[N].onMasterTxCplt(); }
template <int N> void i2cDrvMasterRxCb(I2C_HandleTypeDef *) { i2c_instances[N].onMasterRxCplt(); }
template <int N> void i2cDrvErrorCb(I2C_HandleTypeDef *) { i2c_instances[N].onError(); }
template <int N> void i2cDrvAbortCb(I2C_HandleTypeDef *) { i2c_instances[N].onAbort(); }
template <int N> void i2cDrvMemTxCallbackT(I2C_HandleTypeDef *) { i2c_instances[N].onMasterTxCplt(); }
template <int N> void i2cDrvMemRxCallbackT(I2C_HandleTypeDef *) { i2c_instances[N].onMasterRxCplt(); }

template <int N> void i2cDrvEvIrq()
{
    if (i2c_inited[N])
        HAL_I2C_EV_IRQHandler(&i2c_instances[N].hi2c);
}
template <int N> void i2cDrvErIrq()
{
    if (i2c_inited[N])
        HAL_I2C_ER_IRQHandler(&i2c_instances[N].hi2c);
}

void I2C_CLASS::notify(I2C_DrvResult r)
{
    lastResult = r;
    osSemaphoreRelease(xferSem);
}

void I2C_CLASS::onMasterTxCplt() { notify(I2C_Drv_Ok); }
void I2C_CLASS::onMasterRxCplt() { notify(I2C_Drv_Ok); }
void I2C_CLASS::onError()
{
    uint32_t err = HAL_I2C_GetError(&hi2c);
    if (err & HAL_I2C_ERROR_ARLO)
        notify(I2C_Drv_ArbLost);
    else if (err & HAL_I2C_ERROR_BERR)
        notify(I2C_Drv_BusError);
    else if (err & HAL_I2C_ERROR_AF)
        notify(I2C_Drv_NAK);
    else
        notify(I2C_Drv_Error);
}
void I2C_CLASS::onAbort() { notify(I2C_Drv_Error); }

I2C_DrvResult I2C_CLASS::waitComplete(double timeout)
{
    uint32_t ticks = (timeout < 0) ? osWaitForever : (uint32_t)(timeout * (double)osKernelGetTickFreq());
    if (osSemaphoreAcquire(xferSem, ticks) != osOK)
        return I2C_Drv_Timeout;
    return lastResult;
}

bool I2C_CLASS::init(uint8_t num, const I2C_Config *cfg)
{
    if (!HW_I2C_INDEX_VALID(num) || !cfg)
        return false;
    if (!cfg->scl.port && !cfg->scl.pin)
        return false;

    i2cNum = num;
    lastResult = I2C_Drv_Ok;

    HW_ConfigurePinAF(&cfg->scl, GPIO_MODE_AF_OD, cfg->pull ? cfg->pull : GPIO_NOPULL, cfg->speed_gpio ? cfg->speed_gpio : GPIO_SPEED_FREQ_VERY_HIGH);
    HW_ConfigurePinAF(&cfg->sda, GPIO_MODE_AF_OD, cfg->pull ? cfg->pull : GPIO_NOPULL, cfg->speed_gpio ? cfg->speed_gpio : GPIO_SPEED_FREQ_VERY_HIGH);

    i2c_clk_enable(num);
    HW_Delay(0.001);

    hi2c.Instance = i2cTable[num];
    hi2c.Init.Timing = cfg->timing ? cfg->timing : i2c_calc_timing(i2c_kernel_clock_hz(num), cfg->speed ? cfg->speed : I2C_SPEED_100k);
    hi2c.Init.OwnAddress1 = cfg->own_address;
    hi2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c.Init.OwnAddress2 = 0;
    hi2c.Init.OwnAddress2Masks = 0;
    hi2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c) != HAL_OK)
        return false;

    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
        return false;
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c, 0) != HAL_OK)
        return false;

    mutex = osMutexNew(&s_recursiveMutexAttr);
    xferSem = osSemaphoreNew(1, 0, NULL);

#define I2C_SETUP_CB(N)                                                                                                                                                                                 \
    do {                                                                                                                                                                                                \
        HAL_I2C_RegisterCallback(&hi2c, HAL_I2C_MASTER_TX_COMPLETE_CB_ID, i2cDrvMasterTxCb<N>);                                                                                                         \
        HAL_I2C_RegisterCallback(&hi2c, HAL_I2C_MASTER_RX_COMPLETE_CB_ID, i2cDrvMasterRxCb<N>);                                                                                                         \
        HAL_I2C_RegisterCallback(&hi2c, HAL_I2C_MEM_TX_COMPLETE_CB_ID, i2cDrvMemTxCallbackT<N>);                                                                                                        \
        HAL_I2C_RegisterCallback(&hi2c, HAL_I2C_MEM_RX_COMPLETE_CB_ID, i2cDrvMemRxCallbackT<N>);                                                                                                        \
        HAL_I2C_RegisterCallback(&hi2c, HAL_I2C_ERROR_CB_ID, i2cDrvErrorCb<N>);                                                                                                                         \
        HAL_I2C_RegisterCallback(&hi2c, HAL_I2C_ABORT_CB_ID, i2cDrvAbortCb<N>);                                                                                                                         \
    } while (0)

    switch (num)
    {
        I2C_SETUP_CB(1);
        I2C_SETUP_CB(2);
        I2C_SETUP_CB(3);
        I2C_SETUP_CB(4);
        default: break;
    }
#undef I2C_SETUP_CB

    HAL_NVIC_SetPriority(i2cEvIrqn[num], cfg->irq_priority, 0);
    HAL_NVIC_SetPriority(i2cErIrqn[num], cfg->irq_priority, 0);

    i2c_inited[num] = true;

    HAL_NVIC_EnableIRQ(i2cEvIrqn[num]);
    HAL_NVIC_EnableIRQ(i2cErIrqn[num]);

    return true;
}

void I2C_CLASS::deinit()
{
    HAL_NVIC_DisableIRQ(i2cEvIrqn[i2cNum]);
    HAL_NVIC_DisableIRQ(i2cErIrqn[i2cNum]);
    HAL_I2C_DeInit(&hi2c);
}

bool I2C_CLASS::lock(double timeout)
{
    uint32_t ticks = (timeout < 0) ? osWaitForever : (uint32_t)(timeout * (double)osKernelGetTickFreq());
    return osMutexAcquire(mutex, ticks) == osOK;
}

void I2C_CLASS::unlock()
{
    osMutexRelease(mutex);
}

//阻塞模式下 HAL 错误码 -> 驱动结果 (与参考工程一致, 不依赖中断回调)
static I2C_DrvResult i2c_map_hal(I2C_HandleTypeDef *h, HAL_StatusTypeDef st)
{
    if (st == HAL_OK)      return I2C_Drv_Ok;
    if (st == HAL_TIMEOUT) return I2C_Drv_Timeout;
    if (st == HAL_BUSY)    return I2C_Drv_Busy;
    uint32_t err = HAL_I2C_GetError(h);
    if (err & HAL_I2C_ERROR_AF)   return I2C_Drv_NAK;
    if (err & HAL_I2C_ERROR_ARLO) return I2C_Drv_ArbLost;
    if (err & HAL_I2C_ERROR_BERR) return I2C_Drv_BusError;
    return I2C_Drv_Error;
}

I2C_DrvResult I2C_CLASS::write(uint8_t dev_addr, const uint8_t *tx_data, uint16_t size, double timeout)
{
    if (size == 0 || tx_data == 0)
        return I2C_Drv_BadParam;

    if (!lock(timeout))
        return I2C_Drv_Timeout;

    //阻塞模式, timeout(秒) -> ms
    uint32_t ms = (timeout < 0) ? HAL_MAX_DELAY : (uint32_t)(timeout * 1000.0) + 1u;
    HAL_StatusTypeDef st = HAL_I2C_Master_Transmit(&hi2c, (uint16_t)(dev_addr << 1), (uint8_t *)tx_data, size, ms);

    unlock();
    return i2c_map_hal(&hi2c, st);
}

I2C_DrvResult I2C_CLASS::read(uint8_t dev_addr, uint8_t *rx_data, uint16_t size, double timeout)
{
    if (size == 0 || rx_data == 0)
        return I2C_Drv_BadParam;

    if (!lock(timeout))
        return I2C_Drv_Timeout;

    uint32_t ms = (timeout < 0) ? HAL_MAX_DELAY : (uint32_t)(timeout * 1000.0) + 1u;
    HAL_StatusTypeDef st = HAL_I2C_Master_Receive(&hi2c, (uint16_t)(dev_addr << 1), rx_data, size, ms);

    unlock();
    return i2c_map_hal(&hi2c, st);
}

I2C_DrvResult I2C_CLASS::writeRead(uint8_t dev_addr, const uint8_t *tx_data, uint16_t tx_size, uint8_t *rx_data, uint16_t rx_size, double timeout)
{
    if (tx_size == 0 || tx_data == 0 || rx_size == 0 || rx_data == 0)
        return I2C_Drv_BadParam;

    if (!lock(timeout))
        return I2C_Drv_Timeout;

    uint32_t ms = (timeout < 0) ? HAL_MAX_DELAY : (uint32_t)(timeout * 1000.0) + 1u;
    uint16_t addr8 = (uint16_t)(dev_addr << 1);

    //阻塞模式: 先发 (带 STOP), 再读 (带 STOP)。非 repeated-start, 但兼容绝大多数从机
    HAL_StatusTypeDef st = HAL_I2C_Master_Transmit(&hi2c, addr8, (uint8_t *)tx_data, tx_size, ms);
    if (st == HAL_OK)
        st = HAL_I2C_Master_Receive(&hi2c, addr8, rx_data, rx_size, ms);

    unlock();
    return i2c_map_hal(&hi2c, st);
}

extern "C" void I2C1_EV_IRQHandler(void) { i2cDrvEvIrq<1>(); }
extern "C" void I2C1_ER_IRQHandler(void) { i2cDrvErIrq<1>(); }
extern "C" void I2C2_EV_IRQHandler(void) { i2cDrvEvIrq<2>(); }
extern "C" void I2C2_ER_IRQHandler(void) { i2cDrvErIrq<2>(); }
extern "C" void I2C3_EV_IRQHandler(void) { i2cDrvEvIrq<3>(); }
extern "C" void I2C3_ER_IRQHandler(void) { i2cDrvErIrq<3>(); }
extern "C" void I2C4_EV_IRQHandler(void) { i2cDrvEvIrq<4>(); }
extern "C" void I2C4_ER_IRQHandler(void) { i2cDrvErIrq<4>(); }

bool register_i2c(uint8_t i2c_num, const I2C_Config *cfg)
{
    if (!HW_I2C_INDEX_VALID(i2c_num) || !cfg)
        return false;
    if (i2c_inited[i2c_num])
        return false;
    if (!i2c_instances[i2c_num].init(i2c_num, cfg))
        return false;
    i2c_inited[i2c_num] = true;
    return true;
}

I2C_CLASS *get_i2c_instance(uint8_t i2c_num)
{
    if (HW_I2C_INDEX_VALID(i2c_num) && i2c_inited[i2c_num])
        return &i2c_instances[i2c_num];
    return 0;
}

void init_drv_i2c(void)
{
    for (uint8_t i = 0; i < BOARD_I2C_COUNT; ++i)
    {
        if (board_i2c_ports[i].hw_num == 0 || board_i2c_ports[i].config == 0)
            continue;
        register_i2c(board_i2c_ports[i].hw_num, board_i2c_ports[i].config);
    }
}
