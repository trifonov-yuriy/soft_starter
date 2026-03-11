#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include <stm32f10x_exti.h>
#include "misc.h"
#include <FreeRTOSConfig.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>
#include <semphr.h>
#include "GlobalValues/structures.h"
#include "Flash/flash.h"
#include "EngineControl/engineControl.h"

#define VALVE_NUMBER_PORT_CHANGED 4 //С 4 клапана смена порта

#define TIM2_PRESCALLER           36000
#define TIM2_PERIOD               20 // 10 мс период таймера
#define TIM2_TIME                 10 //10 мс. Время одного отсчета, мс
#define TIM2_ONE_SECOND_COUNTER   100//100 отсчетов соотвествуют одной секунде //2000 //2000 отсчетов соответствует одной секунде. (72 МГц / 36000) / 2000 = 1 с
#define DELAY_BETWEEN_CYCLES      2   //Задержка между циклами продувки, с
#define CYCLES_COUNT              2   //Количество циклов продувки
#define START_SERIES_BUTTON_DELAY 30  //Задержка для борьбы с дребезгом кнопки ПРИНУДИТЕЛЬНАЯ ПРОДУВКА

#define PURGE_PARAMETERS_1        1 //Параметр очиски 1. Запуск серии продувки по давлению или по нажатию кнопки
#define PURGE_PARAMETERS_2        2 //Параметр очистки 2. Запуск серии продувки через заданный интервал времени или по нажатию кнопки
#define PURGE_PARAMETERS_3        3 //Параметр очистки 3. Запуск серии продувки только по нажатию кнопки

#define FINISH_START_COMMAND  	  0x5 //Команда о завершении запуска
#define NOT_START_COMMAND         0xB //Команда во флэш-память о том, что запуска не было
#define CHANGE_VALVES_SETTINGS 	  0xC //Команда об изменении параметров продувки

void ValvesControl_Task(void *pvParameters);

void Valves_Pins_init();

void Valves_Timer_init(uint16_t TIM_Period);

void Valves_PressureSensor_init();

void TIM2_IRQHandler();

void StartSeries_IRQHandler();


