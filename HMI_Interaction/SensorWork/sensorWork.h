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
#include "HMI_Interaction/I2C_Slave/i2c_slave.h"
#include <timers.h>

#define I2C_PROTOCOL_SIZE                           								   9
#define SLAVE_ADDRESS                              									   0x25
#define START_CONTINUES_MEASUREMENT_COMMAND_MASS_FLOW_AVERAGE_TILL_READ                0x3603
#define START_CONTINUES_MEASUREMENT_COMMAND_MASS_FLOW_AVARAGE_NONE                     0x3608
#define START_CONTINUES_MEASUREMENT_COMMAND_DIFFERENTITAL_PRESSURE_AVARAGE_TILL_READ   0x3615
#define START_CONTINUES_MEASUREMENT_COMMAND_DIFFERENTITAL_PRESSURE_AVARAGE_NONE        0x361E
#define STOP_CONTINUES_MEASUREMENT_COMMAND                                             0x3FF9
#define TRIGGERED_MEASUREMENT_MASS_FLOW_CLOCK_STRETCHING_NO                            0x3624
#define TRIGGERED_MEASUREMENT_MASS_FLOW_CLOCK_STRETCHING_YES                           0x3726
#define TRIGGERED_MEASUREMENT_DIFFERENTIAL_PRESSURE_CLOCK_STRETCHING_NO                0x362F
#define TRIGGERED_MEASUREMENT_DIFFERENTIAL_PRESSURE_CLOCK_STRETCHING_YES               0x372D
#define GENERAL_CALL_RESET_COMMAND                                                     0x0006
#define ENTER_SLEEP_MODE_COMMAND                                                       0x3677
#define POLINOM                                                                        0x31
#define SLAVE_ADDRESS																   0x25

void SensorWork_Task(void *pvParameters);
void DataReadyTimer_Function(xTimerHandle RTOS_tim_1);
