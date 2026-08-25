#include "stm32h7xx_hal.h"

extern PCD_HandleTypeDef hpcd_USB_OTG_FS;

/**
 * @brief USB OTG full-speed global interrupt.
 */
void OTG_FS_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
}
