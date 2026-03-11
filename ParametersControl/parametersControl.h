#include <stm32f10x.h>
#include <stm32f10x_adc.h>
#include <FreeRTOS.h>
#include <FreeRTOSConfig.h>
#include <semphr.h>
#include <task.h>

void ParametersControl_Task(void *pvParameters);
void ADC1_init();
void ADC2_init();
