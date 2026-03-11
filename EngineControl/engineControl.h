#ifndef ENGINE_CONTROL_H
#define ENGINE_CONTROL_H

#include "stm32f10x.h"
#include "stm32f10x_adc.h"
#include <FreeRTOSConfig.h>
#include <FreeRTOS.h>
#include <task.h>
#include "Timer/timer.h"
#include "GlobalValues/structures.h"
#include <misc.h>
#include <stm32f10x_exti.h>
#include <timers.h>
#include "Flash/flash.h"

#define MIN_DUTY_CYCLE                     				19 //Минимальный коэффициент заполнения
#define MAX_DUTY_CYCLE                     				79 //Максимальный коэффициент заполнения
#define MIN_OPERATING_TIME				   				14 //Минимально необходимое время работы транзистора
#define ZERO_DETECTOR_1					   				GPIO_Pin_1 //PA1
#define ZERO_DETECTOR_2                   				GPIO_Pin_2 //PA2
//#define ZERO_DETECTOR_3                       		GPIO_Pin_3 //PA3
#define ZERO_CONTROL_PORT                  				GPIOA
#define PULSE_BEAM_COUNT                   				20 //Количество импульсов по 100 мкс
#define STARTING_OPENING_ANGLE             				180//90//180//20000//40000//200//10//180 //9 мс, 50 мкс (2 МГц) * 180 = 9000 мкс = 9 мс
#define VALUE_CHANGED_OPENING_ANGLE        				2//0//1000//10
#define FINISH_OPENGING_ANGLE              				0//178//0
#define VALUE_CHANGED_OPENING_ANGLE_COUNTER 			20 //Запуск по умолчанию за 5 секунд.
#define VALUE_CHANGED_OPENING_ANGLE_COUNTER_STOPPING	20 //Торможение по умолчанию за 20 секунд.
#define VALUE_SAMPLES_COUNTER							100 //Количество отсчетов по 10 мс, за которые должен стартануть двигатель при времени запуска 1 секунда. 10 мс * 100 = 1000 мс = 1с
#define VALUE_FINISH_COUNTER 			   				200
#define START_LED						   				GPIO_Pin_3 //PB3//GPIO_Pin_15 //PA15
#define ALARM_LED                          				GPIO_Pin_4 //PB4
#define FINISH_STARTING_BUTTON             				GPIO_Pin_15//PA15 // GPIO_Pin_3  //PB3
#define PHASE_CROSSING1_TIME			   				34 //Момент пересечения фаз. Межфазное напряжение между 1-й и 3-й фазой = 0
#define PHASE_CROSSING2_TIME			   				100 //Момент пересечения фаз. Межфазное напряжение между 2-й и 3-й фазой = 0
#define PHASE_CORRECTING_TIME              				130//60//130 //Для контроля правильности чередования фаз
#define PHASE_CORRECTING_MAX                            200 //Период 0,01 с. 

#define ADC_GPIO_Pin_1					   				GPIO_Pin_4 //PA4
#define ADC_GPIO_Pin_2					   				GPIO_Pin_5 //PA5
#define ADC_GPIO_Pin_3                     				GPIO_Pin_7 //PA7

#define ADC_VOLTAGE_1                      				1667
#define ADC_VOLTAGE_2                      				2986
#define ADC_CURRENT_1                      				0
#define ADC_CURRENT_2                      				30

#define ADC_U_REF                          				330
#define ADC_BIT_DEPTH                      				4095
#define ADC_U_REF_1                        				450

#define CURRENT_MAX                        				12

#define FINISH_START_COMMAND               				0x5 //Команда о завершении запуска
#define NOT_START_COMMAND                  				0xC //Команда о том, что запуска не было


void EngineControl_Task(void *pvParameters);
void EngineGPIO_init();
void TimerFunction_one_shot(xTimerHandle RTOS_tim_1);
void ADC_GPIO_init();
void ADC_init();
void DMA_init();

void EXTI1_IRQHandler();
void EXTI2_IRQHandler();
//void EXTI3_IRQHandler();

#endif
