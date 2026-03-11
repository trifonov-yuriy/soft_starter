#include "stm32f10x.h"
#include "stm32f10x_i2c.h"
#include "timers.h"

void I2C2_GPIO_init();
void I2C2_init();

void I2C_Slave_Task(void *pvParameters);
void DataReadyTimer_Function(xTimerHandle RTOS_tim_1);