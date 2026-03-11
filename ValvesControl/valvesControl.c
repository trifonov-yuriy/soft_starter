#include "ValvesControl/valvesControl.h"

extern xQueueHandle 				SensorParameters_Queue;            //Данные с датчика давления

extern xQueueHandle            		ValvesImpulseParameters_Queue;     //Данные от пользователя по настройке параметров продувки
extern xSemaphoreHandle        		StartSeries_Semaphore;             //Сигнал на принудительный пуск серии клапанов
//extern xSemaphoreHandle        		FinishStartingEngine_Semaphore; //Сигнал о завершении запуска

extern xSemaphoreHandle         	StartEngine_Semaphore;             //Сигнал СТОП
//extern xSemaphoreHandle           StopEngine_Semaphore;              //Сигнал о начале запуска двигателя
extern xSemaphoreHandle				ValvesTimer_Semaphore;             //Таймер, работающий с клапанами
extern xTaskHandle            		IWatchDog_Handler;

void Valves_Pins_init()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = VALVE_5 | VALVE_6 | VALVE_7 | VALVE_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; //Обязательно в режим low-спид
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	//AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_1; //Устраняем проблему с пинами PA15 PB3 PB4

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = VALVE_1 | VALVE_2 | VALVE_3 | VALVE_4;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; //Обязательно в режим low-спид
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIOA->ODR &= ~(VALVE_5 | VALVE_6 | VALVE_7 | VALVE_8);
	GPIOB->ODR &= ~(VALVE_1 | VALVE_2 | VALVE_3 | VALVE_4);
}

void ValvesOFF()
{
	GPIOA->ODR &= ~(VALVE_5 | VALVE_6 | VALVE_7 | VALVE_8);
	GPIOB->ODR &= ~(VALVE_1 | VALVE_2 | VALVE_3 | VALVE_4);
}

void Valves_Timer_init(uint16_t TIM_Period)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	TIM_TimeBaseInitTypeDef TIM_InitStructure;
	TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up; //Счет вверх
	TIM_InitStructure.TIM_Prescaler = TIM2_PRESCALLER;//36;//256;//TIM_PRESCALLER;//36000;//50000;//TIM_PRESCALLER; //Частота таймера 72/18 = 4МГц
	TIM_InitStructure.TIM_Period = TIM_Period;
	TIM_TimeBaseInit(TIM2, &TIM_InitStructure);


	//Настройка прерываний
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 9;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	TIM_ClearITPendingBit(TIM2, TIM_IT_Update); //Сразу очищаем флаг прерывания, чтобы сразу не уйти в прерывание
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE); //

	TIM_Cmd(TIM2, ENABLE);

	uint8_t counter = 0;
}

void ValvesImpulseParametersInit(ValvesImpulseParametersTypeDef *valvesImpulseParametersTypeDef)
{
	if(Flash_Read(29, 0) == FLASH_WRITED) //Если во флэш-память уже что-то записывали, то считываем эти данные
	{
		valvesImpulseParametersTypeDef->ImpulseActive =       (Flash_Read(31, 1) | Flash_Read(31, 2) << 8); //Длительность импульса, мс
		valvesImpulseParametersTypeDef->ImpulsePause =        (Flash_Read(31, 3) | Flash_Read(31, 4) << 8) * 1000; //Длительность паузы, с
		valvesImpulseParametersTypeDef->SeriesPeriod =        (Flash_Read(31, 5) | Flash_Read(31, 6) << 8) * 60; //Период между сериями
		valvesImpulseParametersTypeDef->PurgeParameters =     (Flash_Read(31, 7)); 							//Продувка по умолчанию чисто по нажатию кнопки
		valvesImpulseParametersTypeDef->PressureMax =         (Flash_Read(31, 8) | Flash_Read(31, 9));      //Уставка предельного давления
		valvesImpulseParametersTypeDef->PauseBetweenCycles =  DELAY_BETWEEN_CYCLES;
		valvesImpulseParametersTypeDef->BetweenCyclesCount =  CYCLES_COUNT;
	}
	else
	{
		valvesImpulseParametersTypeDef->ImpulseActive = 	300; //Длительность импульса, мс
		valvesImpulseParametersTypeDef->ImpulsePause = 		3; //Длительность паузы, с
		valvesImpulseParametersTypeDef->SeriesPeriod = 		15 * 60;//2500; //Период между сериями, с
		valvesImpulseParametersTypeDef->PurgeParameters = 	3; //Продувка чисто по нажатию кнопки
		valvesImpulseParametersTypeDef->PressureMax = 		0;
		valvesImpulseParametersTypeDef->PauseBetweenCycles = DELAY_BETWEEN_CYCLES;
		valvesImpulseParametersTypeDef->BetweenCyclesCount = CYCLES_COUNT;
	}
}



void ValvesSetMicrocontrollerPort(ValvesControlTypeDef *valveControlArray) 
{
	valveControlArray[0].ValvePort = GPIOB_Port;
	valveControlArray[1].ValvePort = GPIOB_Port;
	valveControlArray[2].ValvePort = GPIOB_Port;
	valveControlArray[3].ValvePort = GPIOB_Port;
	valveControlArray[4].ValvePort = GPIOA_Port;
	valveControlArray[5].ValvePort = GPIOA_Port;
	valveControlArray[6].ValvePort = GPIOA_Port;
	valveControlArray[7].ValvePort = GPIOA_Port;
}

void ValvesControlArrayInit(ValvesControlTypeDef *valveControlArray) 
{
	static uint8_t i = 0;
	ValvesSetMicrocontrollerPort(valveControlArray);
	
	valveControlArray[0].ValveNumber = VALVE_1;
	valveControlArray[1].ValveNumber = VALVE_2;
	valveControlArray[2].ValveNumber = VALVE_3;
	valveControlArray[3].ValveNumber = VALVE_4;
	valveControlArray[4].ValveNumber = VALVE_5;
	valveControlArray[5].ValveNumber = VALVE_6;
	valveControlArray[6].ValveNumber = VALVE_7;
	valveControlArray[7].ValveNumber = VALVE_8;
}

void Flash_WriteValvesSettings(ValvesImpulseParametersTypeDef *valvesImpulseParametersTypeDef, uint8_t command)
{
	uint32_t dataArray[10];
	dataArray[0] = command;  //Запись во флэш память данных об успешном запуске или неуспешном запуске
	dataArray[1] = valvesImpulseParametersTypeDef->ImpulseActive & 0xff;
	dataArray[2] = valvesImpulseParametersTypeDef->ImpulseActive >> 8;
	dataArray[3] = valvesImpulseParametersTypeDef->ImpulsePause & 0xff;
	dataArray[4] = valvesImpulseParametersTypeDef->ImpulsePause >> 8;
	dataArray[5] = valvesImpulseParametersTypeDef->SeriesPeriod & 0xff;
	dataArray[6] = valvesImpulseParametersTypeDef->SeriesPeriod >> 8;
	dataArray[7] = valvesImpulseParametersTypeDef->PurgeParameters;
	dataArray[8] = valvesImpulseParametersTypeDef->PressureMax & 0xff;
	dataArray[9] = valvesImpulseParametersTypeDef->PressureMax >> 8;

	Flash_WriteParameters(31, dataArray, 10);
}


void ValveSetState(ValvesControlTypeDef *valvesControlTypeDef, ValvesStateEnum valvesStateEnum) 
{
	switch(valvesControlTypeDef->ValvePort)
	{
		case GPIOA_Port: 
		{
			if(valvesStateEnum == ImpulseActive) 
			{
				GPIO_SetBits(GPIOA, valvesControlTypeDef->ValveNumber);
			}
			else if(valvesStateEnum == ImpulsePause) 
			{
				GPIO_ResetBits(GPIOA, valvesControlTypeDef->ValveNumber);
			}
			else if(valvesStateEnum == PauseBetweenCycles) 
			{
				GPIO_ResetBits(GPIOA, valvesControlTypeDef->ValveNumber);
			}
		} break;
				
		case GPIOB_Port: 
		{
			if(valvesStateEnum == ImpulseActive) 
			{
				GPIO_SetBits(GPIOB, valvesControlTypeDef->ValveNumber);
			}
			else if(valvesStateEnum == ImpulsePause) 
			{
				GPIO_ResetBits(GPIOB, valvesControlTypeDef->ValveNumber);
			}
			else if (valvesStateEnum == PauseBetweenCycles)
			{
				GPIO_ResetBits(GPIOA, valvesControlTypeDef->ValveNumber);
			}
		} break;
	}
}



void ValvesControl_Task(void *pvParameters)
{
	ValvesStateEnum valvesStateEnum = SeriesPeriod;
	uint8_t valvesCounter = 0;
	uint16_t secondsCounter = 0; //Счетчик секунд.
	uint8_t betweenCyclesCounter = 0; //Счетчик количества циклов продувок
	uint16_t timeCounter = 0;
	uint8_t startSeriesButton = 0; //Состояние кнопки ПРИНУДИТЕЛЬНАЯ ПРОДУВКА. Переменная для борьбы с дребезгом
	uint8_t startSeriesButtonTimeCounter = 0; //Счетчик для замера времени после нажатия кнопки. Для борьбы с дребезгом

	
	ValvesImpulseParametersTypeDef valvesImpulseParametersTypeDef; //Параметры задержек
	ValvesImpulseParametersInit(&valvesImpulseParametersTypeDef);  //Первичная инициализация параметров задержек

	ValvesControlTypeDef valveControlArray[VALVES_COUNT]; 
	ValvesControlArrayInit(valveControlArray);

	SensorParametersTypeDef SensorParameters;

	Valves_Pins_init();

	//vTaskDelay(portMAX_DELAY);

	//xQueueSemaphoreTake(FinishStartingEngine_Semaphore, portMAX_DELAY); //Ждем пока запустится двигатель
	

	//else if(Flash_Read(31, 0) != FINISH_START_COMMAND) //Иначе, если двигатель ранее не был запущен, то также записываем данные о параметрах работы клапанов во флэш-память
	//{
	//	Flash_WriteValvesSettings(&valvesImpulseParametersTypeDef);
	//}

	EXTI->IMR |= EXTI_IMR_MR3; //Разрешаем прерывания от кнопки ПРИНУДИТЕЛЬНАЯ ПРОДУВКА
	//EXTI->IMR |= EXTI_IMR_MR0; //Разрешаем прерывания от кнопки ПУСК/СТОП

	Valves_Timer_init(TIM2_PERIOD);

	while(1)
	{
		xQueueSemaphoreTake(ValvesTimer_Semaphore, portMAX_DELAY); //Ждем пока таймер отсчитает
		timeCounter++; //Таймер сделал отсчет
		if (startSeriesButton == 1)
		{
			startSeriesButtonTimeCounter++;
			if (startSeriesButtonTimeCounter > START_SERIES_BUTTON_DELAY)
			{
				EXTI->IMR |= EXTI_IMR_MR3; //Разрешаем прерывания от кнопки ПРИНУДИТЕЛЬНАЯ ПРОДУВКА
				EXTI->IMR |= EXTI_IMR_MR0; //Разрешаем прерывания от кнопки ПУСК. Выключали для предупреждения одновременного срабатывания
				startSeriesButton = 0;
				startSeriesButtonTimeCounter = 0;
			}
		}

		if (xQueueReceive(ValvesImpulseParameters_Queue, &valvesImpulseParametersTypeDef, 0) == pdTRUE) //Пришли данные от HMI об изменении параметров работы клапанов
		{
			Flash_WriteValvesSettings(&valvesImpulseParametersTypeDef, CHANGE_VALVES_SETTINGS); //Записываем эти данные во флэш-память
			//valvesImpulseParametersTypeDef.ImpulseActive = valvesImpulseParametersTypeDef.ImpulseActive;   //Время импульса, мс
			valvesImpulseParametersTypeDef.ImpulsePause = valvesImpulseParametersTypeDef.ImpulsePause * 1000; //Время паузы, мс - с
			valvesImpulseParametersTypeDef.SeriesPeriod = valvesImpulseParametersTypeDef.SeriesPeriod * 60;   //Период очистки, мин - с
		}

		xQueueReceive(SensorParameters_Queue, &SensorParameters, 0);
		if(valvesImpulseParametersTypeDef.PurgeParameters == PURGE_PARAMETERS_1 && SensorParameters.Pressure > valvesImpulseParametersTypeDef.PressureMax)
		{
			if(valvesStateEnum == SeriesPeriod) 
			{
				secondsCounter = valvesImpulseParametersTypeDef.SeriesPeriod;
			}
		}

		if(valvesStateEnum == SeriesPeriod)
		{
			if (valvesImpulseParametersTypeDef.PurgeParameters == PURGE_PARAMETERS_2 && timeCounter >= TIM2_ONE_SECOND_COUNTER)
			{
				secondsCounter++;
				timeCounter = 0;
			}
			if(secondsCounter >= valvesImpulseParametersTypeDef.SeriesPeriod)
			{
				valvesStateEnum = ImpulseActive;
				ValveSetState(&valveControlArray[valvesCounter], ImpulseActive);
				secondsCounter = 0;
				timeCounter = 0;
			}
		}
		else if(valvesStateEnum == PauseBetweenCycles) 
		{
			if (timeCounter >= TIM2_ONE_SECOND_COUNTER)
			{
				secondsCounter++;
				timeCounter = 0;
			}
			if (secondsCounter >= valvesImpulseParametersTypeDef.PauseBetweenCycles)
			{
				valvesStateEnum = ImpulseActive;
				ValveSetState(&valveControlArray[valvesCounter], ImpulseActive);
				secondsCounter = 0;
				timeCounter = 0;
			}
		}
		else if(valvesStateEnum == ImpulseActive)
		{
			if (timeCounter >= (valvesImpulseParametersTypeDef.ImpulseActive / TIM2_TIME))
			{
				valvesStateEnum = ImpulsePause;
				ValveSetState(&valveControlArray[valvesCounter], ImpulsePause);
				timeCounter = 0;
			}
		}
		else if(valvesStateEnum == ImpulsePause)
		{
			if (timeCounter >= TIM2_ONE_SECOND_COUNTER)
			{
				secondsCounter++;
				timeCounter = 0;
			}
			if (secondsCounter >= valvesImpulseParametersTypeDef.ImpulsePause)
			{
				valvesCounter++;
				if (valvesCounter >= VALVES_COUNT)
				{
					valvesCounter = 0;
					betweenCyclesCounter++;
					if(betweenCyclesCounter < valvesImpulseParametersTypeDef.BetweenCyclesCount) 
					{
						valvesStateEnum = PauseBetweenCycles;
						ValveSetState(&valveControlArray[valvesCounter], PauseBetweenCycles);
					}
					else 
					{
						betweenCyclesCounter = 0;
						valvesStateEnum = SeriesPeriod;
						ValveSetState(&valveControlArray[valvesCounter], SeriesPeriod);
					}
				}
				else
				{
					valvesStateEnum = ImpulseActive;
					ValveSetState(&valveControlArray[valvesCounter], ImpulseActive);
				}
				secondsCounter = 0;
			}
		}
		
		if(xQueueSemaphoreTake(StartSeries_Semaphore, 0) == pdTRUE && 
			(valvesImpulseParametersTypeDef.PurgeParameters == PURGE_PARAMETERS_2 || valvesImpulseParametersTypeDef.PurgeParameters == PURGE_PARAMETERS_3))
		{
			startSeriesButton = 1; //Устанавливаем состояние кнопки. Для борьбы с дребезгом
			valvesStateEnum = SeriesPeriod;
			secondsCounter = valvesImpulseParametersTypeDef.SeriesPeriod;
			valvesCounter = 0;
			ValvesOFF();
		}
		//taskYIELD();

		//taskYIELD();
	}
}

void TIM2_IRQHandler()
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(ValvesTimer_Semaphore, &xHigherPriorityTaskWoken);
	if (xHigherPriorityTaskWoken == pdTRUE)
	{
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
}
