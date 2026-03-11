#include "EngineControl/engineControl.h"

//extern xQueueHandle    	               UART_Data_Queue;
//extern xSemaphoreHandle                TIM1_UP_IRQ_Semaphore;
volatile extern EngineControlTypeDef           *engineControl;
extern xSemaphoreHandle                StartEngine_Semaphore;                //Сигнал о начале запуска двигателя
//extern xSemaphoreHandle                StopEngine_Semaphore;                //Сигнал о начале запуска двигателя
//extern xSemaphoreHandle                FinishStartingEngine_Semaphore;       //Сигнал о завершении запуска
extern xTaskHandle                     IWatchDog_Handler;
extern xQueueHandle     		       EngineParameters_Queue;

void EngineControlTypeDefInit(EngineControlTypeDef *engineControlTypeDef)
{
	engineControlTypeDef->OpeningAngle = 			STARTING_OPENING_ANGLE;
	engineControlTypeDef->PulseBeamState_1 = 		PulseBeamOff;
	engineControlTypeDef->PulseBeamState_2 = 		PulseBeamOff;
	engineControlTypeDef->ZeroDetector_1 = 			ZeroDetectorSleep;
	engineControlTypeDef->ZeroDetector_2 = 			ZeroDetectorSleep;
	engineControlTypeDef->TimeCounter_1 = 			0;
	engineControlTypeDef->TimeCounter_2 = 			0;
	engineControlTypeDef->TimeCounterCrossingPhase = 0;
	engineControlTypeDef->Engine_State = 			EngineStarting;
	engineControlTypeDef->PwmPulseBeamCount = 		PWM_PULSE_BEAM_COUNT;
	engineControlTypeDef->IsPhaseCrossing_13 = 0;
	engineControlTypeDef->IsPhaseCrossing_23 = 0;
	
	if(Flash_Read(29, 0) == FLASH_WRITED) 
	{
		engineControlTypeDef->StartingTime =       Flash_Read(30, 0) | (Flash_Read(30, 1) << 8);
		engineControlTypeDef->StoppingTime =       Flash_Read(30, 2) | (Flash_Read(30, 3) << 8);
	}
	else 
	{
		engineControlTypeDef->StartingTime =       VALUE_CHANGED_OPENING_ANGLE_COUNTER;
		engineControlTypeDef->StoppingTime  = 	   VALUE_CHANGED_OPENING_ANGLE_COUNTER_STOPPING;
	}
}

void EngineControl_ChangePhaseRegulatorValue(EngineControlTypeDef *engineControlTypeDef, uint16_t openingAngle)
{
	engineControlTypeDef->OpeningAngle = openingAngle;
}

void GPIO_StartAlarmFinishLeds_init()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = ALARM_LED | START_LED;//| FINISH_STARTING_BUTTON;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = FINISH_STARTING_BUTTON;//START_LED | ALARM_LED;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//GPIO_SetBits(GPIOB, START_LED);
	GPIO_ResetBits(GPIOB, START_LED); //Изначально этот бит в 1
	GPIO_ResetBits(GPIOB, ALARM_LED);
	GPIO_ResetBits(GPIOA, FINISH_STARTING_BUTTON); //Изначально этот бит в 1
}

void GPIO_FinishStartingLed_init()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = FINISH_STARTING_BUTTON;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void ADC_GPIO_init()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = ADC_GPIO_Pin_1 | ADC_GPIO_Pin_2 | ADC_GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void ADC_init()
{
	ADC_GPIO_init();

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_ADCCLKConfig(RCC_CFGR_ADCPRE_DIV6); //12 МГц тактирование АЦП

	ADC_InitTypeDef ADC_InitStructure;

	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent; // Режим работы АЦП: АЦП1 и АЦП2 работают в независимом режиме
	ADC_InitStructure.ADC_NbrOfChannel = 3; // Количество каналов АЦП для штатного преобразования в последовательности
	ADC_InitStructure.ADC_ScanConvMode = ENABLE; // Аналого-цифровое преобразование работает в многоканальном режиме

	ADC_Init(ADC1, &ADC_InitStructure);
	ADC_InjectedSequencerLengthConfig(ADC1, 3);
	//ADC_InjectedDiscModeCmd(ADC1, ENABLE); //Прерывистый режим
	ADC_AutoInjectedConvCmd(ADC1, ENABLE);


	ADC_Cmd (ADC1, ENABLE); // Включить указанный ADC1
	ADC_ResetCalibration (ADC1); // Разрешить сброс калибровки
	while (ADC_GetResetCalibrationStatus (ADC1)); // Ожидание окончания сброса калибровки
	ADC_StartCalibration (ADC1); // Открываем калибровку AD
	while (ADC_GetCalibrationStatus (ADC1)); // ждем окончания калибровки


	ADC_InjectedChannelConfig(ADC1, ADC_Channel_4, 1, ADC_SampleTime_28Cycles5);
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_5, 2, ADC_SampleTime_28Cycles5);
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_7, 3, ADC_SampleTime_28Cycles5);


}

//int16_t SolveCurrent(uint16_t ADC_value)
//{
//	return (((ADC_CURRENT_2 - ADC_CURRENT_1) * ((ADC_value * ADC_U_REF / ADC_BIT_DEPTH) - ADC_VOLTAGE_1)) /
//			(ADC_VOLTAGE_2 - ADC_VOLTAGE_1)) + ADC_CURRENT_1;
//}

int16_t SolveCurrent(uint16_t ADC_value)
{
	static int32_t U_ADC = 0; //Напряжение на АЦП
	static int32_t U_ADC_current = 0;  //Напряжение на выходном выводе датчика тока ACS712. Находим, используя делитель напряжения
	static int32_t result = 0;

	U_ADC = (ADC_value * ADC_U_REF) / ADC_BIT_DEPTH;
	U_ADC_current = ((400 * U_ADC - 13200) / 267) + 50;
	result = ((60 * U_ADC_current - 3000) / 400) - 30;
	return result;
}

void ZeroDetectorFirst_Happened(EngineControlTypeDef *engineControlTypeDef) //Напряжение фаза-нейтраль = 0
{
	engineControlTypeDef->TimeCounterCrossingPhase = 0;
	engineControlTypeDef->ZeroDetector_1 = ZeroDetectorSleep;
	//TIM4->CNT = 0;
	TIM_ClearITPendingBit(TIM4, TIM_IT_Update); //Сразу сбрасываем флаг прерывания, чтобы не уйти в прерывание после инициализации таймера
	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
}

void PhaseCrossing1_Happened(EngineControlTypeDef *engineControlTypeDef) //Момент пересечения фаз. Межфазное напряжение = 0
{
	static uint16_t openingAngleValue = 0;
	static uint16_t CountDownCounter = VALUE_SAMPLES_COUNTER; //Считаем вниз 
	
	switch(engineControlTypeDef->Engine_State)
	{
		case EngineStarting: 
		{
			if(engineControlTypeDef->OpeningAngle > FINISH_OPENGING_ANGLE)
			{
				openingAngleValue++;
				if(openingAngleValue >= engineControlTypeDef->StartingTime)
				{
					engineControlTypeDef->OpeningAngle = engineControlTypeDef->OpeningAngle - (engineControlTypeDef->OpeningAngle / CountDownCounter);
					engineControlTypeDef->PwmPulseBeamCount = (((STARTING_OPENING_ANGLE * PWM_PULSE_BEAM_DEFFERENCE - engineControlTypeDef->OpeningAngle * PWM_PULSE_BEAM_DEFFERENCE) / STARTING_OPENING_ANGLE)) + PWM_PULSE_BEAM_COUNT;
					openingAngleValue = 0;
					if(CountDownCounter > 0)
					{
						CountDownCounter--;
					}
					else
					{
						CountDownCounter = 1;
					}
				}
			}
			else
			{
				engineControlTypeDef->Engine_State = EngineStarted;
			}
		} break;
		
		case EngineStopping: 
		{
			if(engineControlTypeDef->OpeningAngle < STARTING_OPENING_ANGLE)
			{
				openingAngleValue++;
				if(openingAngleValue >= engineControlTypeDef->StoppingTime)
				{
					engineControlTypeDef->OpeningAngle = engineControlTypeDef->OpeningAngle + ((180 - engineControlTypeDef->OpeningAngle) / CountDownCounter);
					engineControlTypeDef->PwmPulseBeamCount = (((STARTING_OPENING_ANGLE * PWM_PULSE_BEAM_DEFFERENCE - engineControlTypeDef->OpeningAngle * PWM_PULSE_BEAM_DEFFERENCE) / STARTING_OPENING_ANGLE)) + PWM_PULSE_BEAM_COUNT;
					openingAngleValue = 0;
					if(CountDownCounter > 0)
					{
						CountDownCounter--;
					}
					else
					{
						CountDownCounter = 1;
					}
				}
			}
			else 
			{
				openingAngleValue = 0;
				CountDownCounter = VALUE_SAMPLES_COUNTER;
				engineControlTypeDef->Engine_State = EngineStopped;
			}
		} break;
		
	}
}

void AlarmFlashing(uint8_t count)
{
	uint8_t i = 0;
	for(i = 0; i < count; i++)
	{
		GPIO_SetBits(GPIOB, ALARM_LED);
		vTaskDelay(500);
		GPIO_ResetBits(GPIOB, ALARM_LED);
		vTaskDelay(500);
	}
	GPIO_SetBits(GPIOB, ALARM_LED);
}

void AlarmState(EngineControlTypeDef *engineControlTypeDef)
{
	engineControlTypeDef->Engine_State = EngineAlarm;
	TIM1->CCR2 = PWM_IMPULSE_OFF;
	TIM3->CCR1 = PWM_IMPULSE_OFF;
	GPIO_ResetBits(GPIOB, START_LED);
}

void WaitZeroDetector(EngineControlTypeDef *engineControlTypeDef, uint8_t alarmFlashingCount)
{
	if(engineControlTypeDef->TimeCounterPhaseCorrecting > PHASE_CORRECTING_MAX)
	{
		AlarmState(engineControlTypeDef);
		AlarmFlashing(alarmFlashingCount);
		vTaskDelete(NULL);
	}
}

void CheckPhaseAlgoritm(EngineControlTypeDef *engineControlTypeDef)  //Контроль правильности чередования фаз
{
	EXTI->IMR |= EXTI_IMR_MR1; 					//Разрешаем прерывания от первого детектора нуля для проверки правильности чередования фаз
	EXTI->IMR &= ~EXTI_IMR_MR2; 					//Запрещаем прерывания от второго детектора нуля. Ждем сигнала от первого
	engineControlTypeDef->TimeCounterPhaseCorrecting = 0; //Обнуляем счетчик
	while (engineControlTypeDef->ZeroDetector_1 != ZeroDetectorHappened)
	{
		WaitZeroDetector(engineControlTypeDef, 6);
	}
	engineControlTypeDef->TimeCounterPhaseCorrecting = 0; //Обнуляем счетчик
	EXTI->IMR |= EXTI_IMR_MR2; //Разрешаем прерывания от второго детектора нуля.

	while (engineControlTypeDef->ZeroDetector_2 != ZeroDetectorHappened)
	{
		WaitZeroDetector(engineControlTypeDef, 4);
	}

	 //Произошло пересечение нуля второй фазой
	if (engineControlTypeDef->TimeCounterPhaseCorrecting < PHASE_CORRECTING_TIME) //Смотрим, сколько прошло времени между срабатываниями двух детекторов
	{
		//Прошло слишком мало времени
		AlarmState(engineControlTypeDef);
		AlarmFlashing(2);
		vTaskDelete(NULL);
	}
	else if (engineControlTypeDef->TimeCounterPhaseCorrecting > PHASE_CORRECTING_MAX)
	{
		//Прошло слишком много времени
		AlarmState(engineControlTypeDef);
		AlarmFlashing(1);
		vTaskDelete(NULL);
	}
	engineControlTypeDef->ZeroDetector_1 = ZeroDetectorSleep;
	engineControlTypeDef->ZeroDetector_2 = ZeroDetectorSleep;
	engineControlTypeDef->TimeCounterPhaseCorrecting = 0; //Обнуляем счетчик
}


void Flash_WriteEngineParameters(EngineStartingAndStoppingParametersTypeDef *engineStartingAndStoppingParametersTypeDef)
{
	uint32_t dataArray[4];
	dataArray[0] = engineStartingAndStoppingParametersTypeDef->StartingTime & 0xff;
	dataArray[1] = engineStartingAndStoppingParametersTypeDef->StartingTime >> 8;
	dataArray[2] = engineStartingAndStoppingParametersTypeDef->StoppingTime & 0xff;
	dataArray[3] = engineStartingAndStoppingParametersTypeDef->StoppingTime >> 8;
	Flash_WriteParameters(30, dataArray, 4);
}

void Timer1_Start(EngineControlTypeDef* engineControlTypeDef) 
{
	engineControlTypeDef->TimeCounter_1 = 0;
}

void Timer2_Start(EngineControlTypeDef* engineControlTypeDef)
{
	engineControlTypeDef->TimeCounter_2 = 0;
}

void Timer1_Clear(EngineControlTypeDef* engineControlTypeDef)
{
	engineControlTypeDef->IsPhaseCrossing_13 = 0;
	engineControlTypeDef->TimeCounter_1 = 0;
}

void Timer2_Clear(EngineControlTypeDef* engineControlTypeDef)
{
	engineControlTypeDef->IsPhaseCrossing_23 = 0;
	engineControlTypeDef->TimeCounter_2 = 0;
}

void MaxCurrent_Alarm(EngineControlTypeDef* engineControlTypeDef) //Действия при превышении допустимого тока
{
	engineControlTypeDef->Engine_State = EngineAlarm;
	TIM1->CCR2 = PWM_IMPULSE_OFF;
	TIM3->CCR1 = PWM_IMPULSE_OFF;
	GPIO_ResetBits(GPIOB, START_LED);
	GPIO_SetBits(GPIOB, ALARM_LED);
	vTaskDelete(NULL);
}

int32_t abs_for_current(int32_t current)
{
	if(current < 0)
	{
		return current * (-1);
	}
	else
	{
		return current;
	}
}

void ReadyParametersToStart(EngineControlTypeDef* engineControlTypeDef, EngineStartingAndStoppingParametersTypeDef *engineStartingAndStoppingParametersTypeDef) //Подготовка параметров к запуску
{
	if (xQueueReceive(EngineParameters_Queue, engineStartingAndStoppingParametersTypeDef, 0) == pdTRUE) //Пришли данные от HMI об изменении параметров запуска двигателя
	{
		if (Flash_Read(29, 0) != FLASH_WRITED)
		{
			Flash_Write(29, 0, FLASH_WRITED);                      //Делаем пометку о том, что данные записаны во флэш-память
		}
		Flash_WriteEngineParameters(engineStartingAndStoppingParametersTypeDef); //Записываем эти данные во флэш-память
		engineControlTypeDef->StartingTime = engineStartingAndStoppingParametersTypeDef->StartingTime; //Время запуска
		engineControlTypeDef->StoppingTime = engineStartingAndStoppingParametersTypeDef->StoppingTime; //Время остановки
	}

	EXTI->IMR |= EXTI_IMR_MR0;							  //Разрешаем прерывания от кнопки ПУСК. Отключали ранее в прерывании для борьбы с дребезгом
	EXTI->IMR |= EXTI_IMR_MR3;							  //Разрешаем прерывания от кнопки ПРИНУДИТЕЛЬНАЯ ПРОДУВКА. Отключали ранее в прерывании по кнопке ПУСК для предупреждения одновременного срабатывания
	EXTI->IMR |= EXTI_IMR_MR1;							  //Разрешаем прерывания от первого детектора нуля
	EXTI->IMR &= ~EXTI_IMR_MR2; 						  //Отключаем второй детектор нуля
	engineControlTypeDef->ZeroDetector_1 = ZeroDetectorSleep;
	engineControlTypeDef->ZeroDetector_2 = ZeroDetectorSleep;
	while (engineControlTypeDef->ZeroDetector_1 != ZeroDetectorHappened); //Ждем срабатывания первого детектора и начинаем запуск
	engineControlTypeDef->TimeCounterCrossingPhase = 0;
	engineControlTypeDef->TimeCounterPhaseCorrecting = 0;
	engineControlTypeDef->TimeCounter_1 = 0;
	engineControlTypeDef->TimeCounter_2 = 0;
	engineControlTypeDef->IsPhaseCrossing_13 = 0;
	engineControlTypeDef->IsPhaseCrossing_23 = 0;
}

void TimeCheck(EngineControlTypeDef* engineControlTypeDef) //Функция для проверки частоты сети
{
	EXTI->IMR |= EXTI_IMR_MR1; 					//Разрешаем прерывания от первого детектора нуля для проверки правильности чередования фаз
	EXTI->IMR &= ~EXTI_IMR_MR2; 					//Запрещаем прерывания от второго детектора нуля. Ждем сигнала от первого
	engineControlTypeDef->TimeCounterPhaseCorrecting = 0; //Обнуляем счетчик
	engineControlTypeDef->ZeroDetector_1 = ZeroDetectorSleep;
	while (engineControlTypeDef->ZeroDetector_1 != ZeroDetectorHappened);
	
	engineControlTypeDef->ZeroDetector_1 = ZeroDetectorSleep;
	engineControlTypeDef->TimeCounterPhaseCorrecting = 0; //Обнуляем счетчик
	EXTI->IMR |= EXTI_IMR_MR1; 					//Разрешаем прерывания от первого детектора нуля для проверки правильности чередования фаз
	while (engineControlTypeDef->ZeroDetector_1 != ZeroDetectorHappened);
	uint16_t timeCount = engineControlTypeDef->TimeCounterPhaseCorrecting;
}

void TimeCheckWithCounter(EngineControlTypeDef* engineControlTypeDef)
{
	EXTI->IMR |= EXTI_IMR_MR1; 					//Разрешаем прерывания от первого детектора нуля для проверки правильности чередования фаз
	EXTI->IMR &= ~EXTI_IMR_MR2; 					//Запрещаем прерывания от второго детектора нуля. Ждем сигнала от первого
	engineControlTypeDef->TimeCounterPhaseCorrecting = 0; //Обнуляем счетчик
	engineControlTypeDef->ZeroDetector_1 = ZeroDetectorSleep;
	while (engineControlTypeDef->ZeroDetector_1 != ZeroDetectorHappened);

	engineControlTypeDef->ZeroDetector_1 = ZeroDetectorSleep;
	engineControlTypeDef->TimeCounterPhaseCorrecting = 0; //Обнуляем счетчик
	EXTI->IMR |= EXTI_IMR_MR1; 					//Разрешаем прерывания от первого детектора нуля для проверки правильности чередования фаз
	while (engineControlTypeDef->ZeroDetector_1 != ZeroDetectorHappened)
	{
		if(engineControlTypeDef->IsPhaseCrossing_13 == 1)
		{
			engineControlTypeDef->TimeCounterPhaseCorrecting++;
			engineControlTypeDef->IsPhaseCrossing_13 = 0;
		}
	}
	uint16_t timeCount = engineControlTypeDef->TimeCounterPhaseCorrecting;
}

void Engine_ParametersClear(EngineControlTypeDef* engineControlTypeDef) 
{
	TIM1->CCR2 = PWM_IMPULSE_OFF;
	TIM3->CCR1 = PWM_IMPULSE_OFF;
	EXTI->IMR &= ~EXTI_IMR_MR1;							//Запрещаем прерывания от первого детектора нуля
	EXTI->IMR &= ~EXTI_IMR_MR2;							//Запрещаем прерывания от второго детектора нуля
	EXTI->IMR |= EXTI_IMR_MR0;							//Разрешаем прерывания от кнопки ПУСК
	EXTI->IMR |= EXTI_IMR_MR3;							//Разрешаем прерывания от кнопки ПРИНУДИТЕЛЬНАЯ ПРОДУВКА
	engineControlTypeDef->TimeCounter_1 = 0;
	engineControlTypeDef->PulseBeamState_1 = PulseBeamOff;
	engineControlTypeDef->TimeCounter_2 = 0;
	engineControlTypeDef->PulseBeamState_2 = PulseBeamOff;
	engineControlTypeDef->TimeCounterCrossingPhase = 0;
	engineControlTypeDef->TimeCounterPhaseCorrecting = 0;
	engineControlTypeDef->IsPhaseCrossing_13 = 0;
	engineControlTypeDef->IsPhaseCrossing_23 = 0;
	engineControlTypeDef->ZeroDetector_1 = ZeroDetectorSleep;
	engineControlTypeDef->ZeroDetector_2 = ZeroDetectorSleep;
}

void WaitStartOrStopCommand() //Ждем сигнала ПУСК или СТОП
{
	xSemaphoreTake(StartEngine_Semaphore, portMAX_DELAY); //Ждем пока придет сигнал о запуске или начале торможения
	vTaskDelay(100);
	while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0); //Ждем пока отпустят кнопку ПУСК
	vTaskDelay(100);
	Flash_Write(31, 0, NOT_START_COMMAND);                //Запись о незапущенном двигателе
}

void EngineControl_Task(void *pvParameters)
{
	int32_t current_1 = 0; //Сила тока на фазе 1
	int32_t current_2 = 0; //Сила тока на фазе 2
	int32_t current_3 = 0; //Сила тока на фазе 3

	EngineControlTypeDef engineControlTypeDef;         //Параметры контроля работы двигателя
	EngineControlTypeDefInit(&engineControlTypeDef);

	engineControl = &engineControlTypeDef;
	
	EngineStartingAndStoppingParametersTypeDef engineStartingAndStoppingParametersTypeDef; //Параметры запуска/остановки двигателя

	GPIO_StartAlarmFinishLeds_init();
	//GPIO_FinishStartingLed_init();
	ADC_GPIO_init();
	ADC_init();
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	ADC_SoftwareStartInjectedConvCmd(ADC1, ENABLE);

	EngineGPIO_init();
	PWM_GPIO_Init();
	TimerInit();
	PWM_Init();

	xSemaphoreTake(StartEngine_Semaphore, portMAX_DELAY); //Ждем пока придет сигнал о начале запуска
	vTaskDelay(100);
	while(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0); //Ждем пока отпустят кнопку ПУСК
	vTaskDelay(100);
	CheckPhaseAlgoritm(&engineControlTypeDef);  //Контроль правильности чередования фаз
	
	GPIO_SetBits(GPIOB, START_LED);             //Замыкаем реле СТАРТ

	ReadyParametersToStart(&engineControlTypeDef, &engineStartingAndStoppingParametersTypeDef); //Подготавливаем параметры структуры engineControlTypeDef к пуску.

	while(1)
	{
		//Не забыть сделать первоначальную инициализацию параметров структуры engineControlTypeDef

		// Вычисление значений тока
		current_1 = SolveCurrent(ADC1->JDR1); 
		current_2 = SolveCurrent(ADC1->JDR2);
		current_3 = SolveCurrent(ADC1->JDR3);

		if(abs_for_current(current_1) > CURRENT_MAX || abs_for_current(current_2) > CURRENT_MAX || abs_for_current(current_3) > CURRENT_MAX)
		{
			MaxCurrent_Alarm(&engineControlTypeDef);
		}

		if(engineControlTypeDef.IsPhaseCrossing_13 == 0 && engineControlTypeDef.TimeCounterCrossingPhase >= PHASE_CROSSING1_TIME)
		{
			engineControlTypeDef.IsPhaseCrossing_13 = 1;
			Timer1_Start(&engineControlTypeDef);
			PhaseCrossing1_Happened(&engineControlTypeDef);
		}

		else if (engineControlTypeDef.IsPhaseCrossing_23 == 0 && engineControlTypeDef.TimeCounterCrossingPhase >= PHASE_CROSSING2_TIME)
		{
			engineControlTypeDef.IsPhaseCrossing_23 = 1;
			Timer2_Start(&engineControlTypeDef);
			EXTI->IMR |= EXTI_IMR_MR1;   //Включаем прерывание от первого детектора нуля. Ранее был отключен в прерывании для борьбы с повторным срабатыванием
		}

		if (engineControlTypeDef.IsPhaseCrossing_13 == 1) 
		{
			switch (engineControlTypeDef.PulseBeamState_1)
			{
				case PulseBeamOff:
				{
					if (engineControlTypeDef.TimeCounter_1 >= engineControlTypeDef.OpeningAngle)
					{
						TIM1->CCR2 = PWM_IMPULSE_ON;
						engineControlTypeDef.PulseBeamState_1 = PulseBeamOn;
						Timer1_Start(&engineControlTypeDef);
					}
				} break;

				case PulseBeamOn:
				{
					if (engineControlTypeDef.TimeCounter_1 >= engineControlTypeDef.PwmPulseBeamCount)
					{
						engineControlTypeDef.TimeCounter_1 = 0;
						engineControlTypeDef.PulseBeamState_1 = PulseBeamOff;
						TIM1->CCR2 = PWM_IMPULSE_OFF;
						Timer1_Clear(&engineControlTypeDef);
					}
				} break;
			}
		}

		if (engineControlTypeDef.IsPhaseCrossing_23 == 1) 
		{
			switch (engineControlTypeDef.PulseBeamState_2)
			{
				case PulseBeamOff:
				{
					if (engineControlTypeDef.TimeCounter_2 >= engineControlTypeDef.OpeningAngle)
					{
						TIM3->CCR1 = PWM_IMPULSE_ON;
						engineControlTypeDef.PulseBeamState_2 = PulseBeamOn;
						Timer2_Start(&engineControlTypeDef);
					}
				} break;

				case PulseBeamOn:
				{
					if (engineControlTypeDef.TimeCounter_2 >= engineControlTypeDef.PwmPulseBeamCount)
					{
						engineControlTypeDef.TimeCounter_2 = 0;
						engineControlTypeDef.PulseBeamState_2 = PulseBeamOff;
						TIM3->CCR1 = PWM_IMPULSE_OFF;
						Timer2_Clear(&engineControlTypeDef);
					}
				} break;
			}
		}

		switch(engineControlTypeDef.Engine_State)
		{
			case EngineStarted:
			{
				TIM1->CCR2 = PWM_IMPULSE_ON;
				TIM3->CCR1 = PWM_IMPULSE_ON;
				GPIO_SetBits(GPIOA, FINISH_STARTING_BUTTON);
				vTaskDelay(3000);
				Engine_ParametersClear(&engineControlTypeDef);
				WaitStartOrStopCommand();
				ReadyParametersToStart(&engineControlTypeDef, &engineStartingAndStoppingParametersTypeDef); //Подготавливаем параметры структуры engineControlTypeDef
				GPIO_ResetBits(GPIOA, FINISH_STARTING_BUTTON);        //Размыкаем шунтирующее реле и начинаем останов
				engineControlTypeDef.Engine_State = EngineStopping;
			} break;
			
			case EngineStopped: 
			{
				Engine_ParametersClear(&engineControlTypeDef);
				vTaskDelay(3000);									 //Ждем 3 секунды до полной остановки
				GPIO_ResetBits(GPIOB, START_LED);					  //Размыкаем реле СТАРТ
				WaitStartOrStopCommand();
				CheckPhaseAlgoritm(&engineControlTypeDef);			 //Контроль правильности чередования фаз
				ReadyParametersToStart(&engineControlTypeDef, &engineStartingAndStoppingParametersTypeDef); //Подготавливаем параметры структуры engineControlTypeDef к пуску.
				GPIO_SetBits(GPIOB, START_LED);						  //Замыкаем реле СТАРТ
				engineControlTypeDef.Engine_State = EngineStarting;
			} break;
		}

		if(xSemaphoreTake(StartEngine_Semaphore, 0) == pdTRUE)
		{
			//Сигнал о прекращении запуска/СТОП
			GPIO_ResetBits(GPIOB, START_LED);						//Размыкаем реле СТАРТ
			Engine_ParametersClear(&engineControlTypeDef);
			WaitStartOrStopCommand();
			CheckPhaseAlgoritm(&engineControlTypeDef);				//Контроль правильности чередования фаз
			ReadyParametersToStart(&engineControlTypeDef, &engineStartingAndStoppingParametersTypeDef); //Подготавливаем параметры структуры engineControlTypeDef к пуску.
			GPIO_SetBits(GPIOB, START_LED);						   //Замыкаем реле СТАРТ
			engineControlTypeDef.Engine_State = EngineStarting;
		}
	}
}

void EngineGPIO_init()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //Вход с поддяжкой к питанию
	GPIO_InitStructure.GPIO_Pin = ZERO_DETECTOR_1 | ZERO_DETECTOR_2; //| ZERO_DETECTOR_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; //Обязательно в режим low-спид
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	AFIO->EXTICR[0] &= ~(AFIO_EXTICR1_EXTI1); //Первый канал EXTI подключен к порту PA1
	AFIO->EXTICR[0] &= ~(AFIO_EXTICR1_EXTI2); //Второй канал EXTI подключен к порту PA2
	//AFIO->EXTICR[0] &= ~(AFIO_EXTICR1_EXTI3); //Первый канал EXTI подключен к порту PA3
	//EXTI->RTSR |= EXTI_RTSR_TR1 | EXTI_RTSR_TR2; //| EXTI_FTSR_TR3; //Прерывание по нарастанию сигнала
	EXTI->FTSR |= EXTI_FTSR_TR1 | EXTI_FTSR_TR2; //Прерывание по спаду сигнала
	EXTI->PR = EXTI_PR_PR1 | EXTI_PR_PR2;// | EXTI_PR_PR3;      //Сбрасываем флаг прерывания
	                               //перед включением самого прерывания

	EXTI->IMR &= ~EXTI_IMR_MR1; //Пока не включаем прерывания от 1-го канала EXTI //EXTI->IMR |= EXTI_IMR_MR1;   //Включаем прерывание 1-го канала EXTI
	EXTI->IMR &= ~EXTI_IMR_MR2; //Пока не включаем прерывания от 2-го канала EXTI//EXTI->IMR |= EXTI_IMR_MR2;   //Включаем прерывание 2-го канала EXTI
	//EXTI->IMR |= EXTI_IMR_MR3;   //Включаем прерывание 3-го канала EXTI

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	//NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
	//NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
	//NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	//NVIC_Init(&NVIC_InitStructure);
}


void EXTI1_IRQHandler()
{
	if(GPIO_ReadInputDataBit(GPIOA, ZERO_DETECTOR_1) == 0)
	{
		engineControl->ZeroDetector_1 = ZeroDetectorHappened;
		engineControl->TimeCounterCrossingPhase = 0;
		engineControl->TimeCounterPhaseCorrecting = 0;
		EXTI->IMR &= ~EXTI_IMR_MR1;   //Боремся с повторным срабатыванием
	}
	EXTI_ClearFlag(EXTI_Line1);
}

void EXTI2_IRQHandler()
{
	if(GPIO_ReadInputDataBit(GPIOA, ZERO_DETECTOR_2) == 0)
	{
		engineControl->ZeroDetector_2 = ZeroDetectorHappened;
		EXTI->IMR &= ~EXTI_IMR_MR2;   //Боремся с повторным срабатыванием
	}
	EXTI_ClearFlag(EXTI_Line2);
}

//void EXTI3_IRQHandler()
//{
//	EXTI_ClearFlag(EXTI_Line3);
//}
