#include "IWatchDog/iwatchdog.h"

void IWatchDog_init()
{
	 IWDG_Enable();
	 while(IWDG_GetFlagStatus(IWDG_FLAG_PVU) == SET || IWDG_GetFlagStatus(IWDG_FLAG_RVU) == SET); //Æהול סבנמסא פכאדמג
	 IWDG_SetPrescaler(IWDG_Prescaler_32); //1250 Ãצ
	 IWDG_SetReload(125); //100 לס

}

void IWatchDog_Task(void *pvParameters)
{
	IWatchDog_init();

	while(1)
	{
		vTaskDelay(50);
		IWDG_ReloadCounter();
	}
}
