#include "stm32f10x.h"
#include "misc.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_gpio.h"
#include <FreeRTOS.h>
#include <semphr.h>
#include <portmacro.h>
#include "GlobalValues/structures.h"

#define PWM_COUNTER      			99//500//2000//99//1200//99//1200//99
#define TIM_PRESCALLER  	 	  	36//1000//36000//36//60000//36//60000//72 //PWM 10 кГц, f_pwm = 72000000 / (2 /*выравнивание по центру*/ * 36 * (99 + 1))

#define PWM_GPIO_OUTPUT_PORT_1        GPIOA
#define PWM_GPIO_OUTPUT_PINS_1	      GPIO_Pin_9//GPIO_Pin_8// | GPIO_Pin_9

#define PWM_GPIO_OUTPUT_PORT_2        GPIOA
#define PWM_GPIO_OUTPUT_PINS_2        GPIO_Pin_6

#define PWM_IMPULSE_TIME                  3//600//3
#define PWM_IMPULSE_ON                    PWM_IMPULSE_TIME
#define PWM_IMPULSE_OFF                   0
#define PWM_PULSE_BEAM_COUNT              8//20000//10//20000//20000//10000//5//10000//5 //Количество импульсов по 100 мкс. 8 = 4 импульса по 100 мкс
#define PWM_PULSE_BEAM_COUNT_MAX          120 //Максимальное количество импульсов. 2 = 1 импульсу. 120 = 60 импульсов по 100 мкс
#define PWM_PULSE_BEAM_DEFFERENCE         112 //120-8. Разница - значение для расчетов

void PWM_Init();
void TimerInit();
void PWM_GPIO_Init();
void TIM1_UP_IRQHandler();
//void TIM1_UP_TIM10_IRQHandler();
//void TIM3_IRQHandler();
//void TIM4_IRQHandler();
//void TIM3_IRQHandler();
