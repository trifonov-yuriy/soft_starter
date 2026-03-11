#define DEBUG

#include "stm32f10x.h"
#include <FreeRTOS.h>
#include <FreeRTOSConfig.h>
#include <semphr.h>
#include <task.h>
#include <timers.h>
#include <stm32f10x_usart.h>
#include "EngineControl/engineControl.h"
#include "HMI_Interaction/hmi_interaction.h"
#include "HMI_Interaction/I2C_Slave/i2c_slave.h"
#include "stm32f10x_exti.h"
#include "Clocking/clocking.h"
#include "GlobalValues/structures.h"
#include "AlarmActions/alarmActions.h"
#include "ParametersControl/parametersControl.h"
#include "ValvesControl/valvesControl.h"
#include "HMI_Interaction/SensorWork/sensorWork.h"
#include <timers.h>
#include "IWatchDog/iwatchdog.h"

//xQueueHandle    	    USART_Data_Queue; 	    	       //Принимаемые от UART данные
//xQueueHandle            SPI_Data_Queue;  	    	       //Принимаемые от SPI данные
//xSemaphoreHandle        TIM1_UP_IRQ_Semaphore;   	       //Прошло 100 мкс от таймера
xQueueHandle            ValvesImpulseParameters_Queue;      //Данные по параметрам продувки
xQueueHandle            EngineParameters_Queue;            //Данные по запуску и остановке
xQueueHandle            SensorParameters_Queue;            //Данные с датчика давления
xSemaphoreHandle        StartSeries_Semaphore;               //Сигнал на принудительный пуск серии клапанов

xSemaphoreHandle        ValvesTimer_Semaphore;               //Таймер, работающий с клапанами
//xSemaphoreHandle        FinishStartingEngine_Semaphore;     //Сигнал о завершении запуска
xSemaphoreHandle        StartEngine_Semaphore;              //Сигнал о начале запуска двигателя
//xSemaphoreHandle        StopEngine_Semaphore;               //Сигнал о начале торможения двигателя
xSemaphoreHandle        USART1_DataReceived_Semaphore;      //Получены данные от модуля USART1
xTimerHandle            RTOS_tim_1;
xTaskHandle             IWatchDog_Handler;
xTaskHandle             SensorWork_Handler;

volatile ValvesControlTypeDef *valvesControl;
volatile EngineControlTypeDef *engineControl;

void SolveFrequency_Task(void *pvParameters)
{

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; //Вход с поддяжкой к земле
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; //!!!ПРОВЕРИТЬ на 2 MHz!!!
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;

	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;

	GPIO_Init(GPIOA, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);



    /*
	  Настройка EXTI
    */
	AFIO->EXTICR[0] &= ~(AFIO_EXTICR1_EXTI0); //Нулевой канал EXTI подключен к порту PA0
	EXTI->FTSR |= EXTI_FTSR_TR0; //Прерывание по спаду импульса
	EXTI->PR = EXTI_PR_PR0;      //Сбрасываем флаг прерывания
	                               //перед включением самого прерывания

	EXTI->IMR |= EXTI_IMR_MR0;   //Включаем прерывание 0-го канала EXTI

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

	NVIC_Init(&NVIC_InitStructure);


	//Настройка TIM2 Для замера количества импульсов
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	TIM_TimeBaseInitTypeDef TIM_InitStructure;

	TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_InitStructure.TIM_Prescaler = 36000;
	TIM_InitStructure.TIM_Period = 2000;
	TIM_TimeBaseInit(TIM2, &TIM_InitStructure);

	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 8;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM2, ENABLE);






	//Настройка USART
	/*
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE); //usart1 function remapping
	// TX PB6
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);

	// RX PB7
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;

	GPIO_Init(GPIOB,&GPIO_InitStructure);



	//Настройка USART

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE); //Разрешаем тактирование

	USART_InitTypeDef USART_InitStructure;
	USART_StructInit(&USART_InitStructure);
	USART_InitStructure.USART_BaudRate = 19200;
	USART_ITConfig(USART1, USART_IT_TC, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 8;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USART_Init(USART1, &USART_InitStructure);
	USART_Cmd(USART1, ENABLE);
	 */

	while(1)
	{
		uint8_t a = 50;
		a++;
	}
}

uint32_t frequency;
uint32_t dataToUART;
uint16_t counter;

void JTAG_off() 
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	//GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);
	//AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_1; //Устраняем проблему с пинами PA15 PB3 PB4, которые по умолчанию сконфигурированы под интерфейс JTAG
}

int main(void)
{

	//RCC_DeInit();
	//RCC_PCLK2Config(RCC_HCLK_Div16); //8 МГц / 16 = 500кГц
	//NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	//RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
	//RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    //RCC->APB2ENR |=RCC_APB2Periph_AFIO;
    //(void) AFIO->MAPR;
    //AFIO->MAPR   |= AFIO_MAPR_SWJ_CFG_DISABLE; // 25 бит все как по даташиту JTAG-DP Disabled and SW-DP Enabled
    //AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_1; //Устраняем проблему с пинами PA15 PB3 PB4, которые по умолчанию сконфигурированы под интерфейс JTAG

	frequency = 0;
	counter = 0;
	//xTaskCreate(SolveFrequency_Task, (const char *) "SolveFrequency_Task", 256, NULL, 8, (xTaskHandle *) NULL);



	SetClockSettings(); //Настройки тактирования
	JTAG_off();         //Отключить JTAG
	//AlarmActionsInit(); //Действия при отказах


	//TIM1_UP_IRQ_Semaphore           =   xSemaphoreCreateCounting(10, 0);
	StartSeries_Semaphore           =   xSemaphoreCreateCounting(10, 0);
	StartEngine_Semaphore           =   xSemaphoreCreateCounting(10, 0);
	ValvesTimer_Semaphore			=	xSemaphoreCreateCounting(10, 0);
	//StopEngine_Semaphore            =   xSemaphoreCreateCounting(10, 0);
	//FinishStartingEngine_Semaphore  =   xSemaphoreCreateCounting(10, 0);
	USART1_DataReceived_Semaphore   =   xSemaphoreCreateCounting(10, 0);
	ValvesImpulseParameters_Queue   =   xQueueCreate(10, sizeof(ValvesImpulseParametersTypeDef));
	SensorParameters_Queue          =   xQueueCreate(10, sizeof(SensorParametersTypeDef));
	EngineParameters_Queue          =   xQueueCreate(10, sizeof(EngineStartingAndStoppingParametersTypeDef));

	//RTOS_tim_1 = xTimerCreate("TIM1", 1000, pdFALSE, 0, DataReadyTimer_Function);
	xTaskCreate(EngineControl_Task, (const char *) "EngineControl_Task", 256, NULL, 8, (xTaskHandle *) NULL);
	xTaskCreate(HMI_Interaction_Task, (const char *) "HMI_Interaction_Task", 256, NULL, 7, (xTaskHandle *) NULL);
	//xTaskCreate(ParametersControl_Task, (const char *) "ParametersControl_Task", 128, NULL, 8, (xTaskHandle *) NULL);
	xTaskCreate(ValvesControl_Task, (const char *) "ValvesControlTask", 256, NULL, 8, (xTaskHandle *) NULL);
	xTaskCreate(IWatchDog_Task, (const char *) "IWatchDog_Task", 128, NULL, 8, &IWatchDog_Handler);
	//xTaskCreate(SensorWork_Task, (const char *) "SensorWork_Task", 128, NULL, 8, &SensorWork_Handler);


#ifdef DEBUG
	//RTOS_tim_1 = xTimerCreate("TIM1", 10, pdFALSE, 0, DataReadyTimer_Function);
	//xTaskCreate(I2C_Slave_Task, (const char *) "I2C_Slave_Task", 256, NULL, 8, (xTaskHandle *) NULL);
#endif
	vTaskStartScheduler();



	/*
	EngineGPIO_init();
	PWM_GPIO_Init();
	TimerInit();
	PWM_Init();
	Valves_Pins_init();

	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);


	GPIOB->ODR |= 1 << 12;

	uint8_t xgdf = 0;
*/

	uint8_t xgdf = 0;

    while(1)
    {

    }
}
/*
void TIM2_IRQHandler()
{
	GPIOB->ODR ^= 1 << 12;
	//TIM2->CNT = 0;
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
}

void TIM3_IRQHandler()
{
	//GPIOB->ODR ^= 1 << 12;
	//TIM1_counter = TIM1->CNT;
	//engineControl->TimeCounter_1 = engineControl->TimeCounter_1 + 1;
	//GPIOA->ODR ^= 1 << 15;
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
}

void TIM4_IRQHandler()
{
	//TIM4->CNT = 0;
	GPIOB->ODR ^= 1 << 12;
	//TIM1_counter = TIM1->CNT;
	//engineControl->TimeCounter_1 = engineControl->TimeCounter_1 + 1;
	//GPIOA->ODR ^= 1 << 15;
	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
}

void TIM1_UP_IRQHandler()
{
	//GPIOB->ODR ^= 1 << 12;
	//GPIOB->ODR ^= 1 << 12;
	//TIM1_counter = TIM1->CNT;
	//engineControl->TimeCounter_1 = engineControl->TimeCounter_1 + 1;
	//GPIOA->ODR ^= 1 << 15;
	TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
}
*/
/*
void USART1_IRQHandler()
{
	switch(counter)
	{
		case 1:
		{
			USART_SendData(USART1, dataToUART >> 8);
			counter++;

		} break;

		case 2:
		{
			USART_SendData(USART1, dataToUART >> 16);
			counter++;
		} break;

	}

	USART_ClearFlag(USART1,USART_FLAG_TC);

}
*/
/*
void TIM2_IRQHandler()
{
	dataToUART = frequency;
	frequency = 0;

	//USART_SendData(USART1, dataToUART & 0xFF);
	counter = 1;


	TIM2->CNT = 0;
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
}
*/
/*
void EXTI0_IRQHandler()
{

	//if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) != RESET)
	//GPIOA->ODR ^= 1 << 2;
	if((GPIOA->IDR & (1 << 1)) != 0)
	{
		GPIO_SetBits(GPIOA, GPIO_Pin_2);
	}
	else
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_2);
	}
	frequency++;
	EXTI_ClearFlag(EXTI_Line0);
}
*/



void vApplicationIdleHook(){}
void vApplicationMallocFailedHook()
{
for ( ;; );
}
void vApplicationStackOverflowHook(xTaskHandle pxTask, signed char *pcTaskName)
{
( void ) pcTaskName;
( void ) pxTask;
for ( ;; );
}
void vApplicationTickHook(){}
