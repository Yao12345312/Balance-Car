#include "task_Display.hpp"
#include "drv_oled.hpp"

void StartDisplayTask(void *argument)
{
	auto *oled = drv_oled();

	oled->clear();
	
	osDelay(100);
	
	oled->displayLogo();
	
	oled->update();
	
	osDelay(1000);
	
	uint32_t next_wake = osKernelGetTickCount();
	
	while (1)
	{
		

		next_wake += 30U;
        osDelayUntil(next_wake);
		
	}
}
