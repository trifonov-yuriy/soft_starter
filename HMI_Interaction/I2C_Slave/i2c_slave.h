#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <misc.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_dma.h>
#include <FreeRTOS.h>
#include <FreeRTOSConfig.h>
#include <semphr.h>
#include <task.h>
#include "GlobalValues/structures.h"
#include <stm32f10x_i2c.h>
#include <timers.h>
#include <stm32f10x_i2c.h>


void I2C2_GPIO_init();
void I2C2_init();

void I2C_Slave_Task(void *pvParameters);
