#include "HMI_Interaction/hmi_interaction.h"

//extern xQueueHandle    	                UART_Data_Queue;
extern xQueueHandle          			  ValvesImpulseParameters_Queue;
extern xQueueHandle     		          EngineParameters_Queue;
extern xSemaphoreHandle                   USART1_DataReceived_Semaphore;
extern xSemaphoreHandle                   StartEngine_Semaphore;  //Сигнал о начале запуска двигателя
extern xSemaphoreHandle                   StartSeries_Semaphore;
extern ValvesControlTypeDef 			  *valvesControl;
void UART_GPIO_init()
{

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE); //usart1 function remapping
	// TX PB6
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);

	// RX PB7
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;

	GPIO_Init(GPIOB,&GPIO_InitStructure);
}

void UART_init()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE); //Разрешаем тактирование

	USART_InitTypeDef USART_InitStructure;
	USART_StructInit(&USART_InitStructure);
	USART_InitStructure.USART_BaudRate = UART_BAUD_RATE;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	//USART_ITConfig(USART1, USART_IT_PE, ENABLE);

	//NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	//NVIC_InitTypeDef NVIC_InitStructure;
	//NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	//NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 8;
	//NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	//NVIC_Init(&NVIC_InitStructure);

	USART_Init(USART1, &USART_InitStructure);
	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
	USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
	USART_Cmd(USART1, ENABLE);
}


void DMA_init(uint8_t *dataFromUART, uint8_t *dataToUART)
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	DMA_InitTypeDef DMA_InitStructure;
	DMA_InitStructure.DMA_BufferSize = DMA_BUFFER_COMMAND_SIZE;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)dataFromUART;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 8;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);


	DMA_Init(DMA1_Channel5, &DMA_InitStructure);
	DMA_ClearITPendingBit(DMA_IT_TC);
	DMA_ClearITPendingBit(DMA1_IT_TC5);
	DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE);
	DMA_Cmd(DMA1_Channel5, ENABLE);



	DMA_InitStructure.DMA_BufferSize = DMA_ACKNOWLEDGE_BYTE_SIZE;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)dataToUART;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 8;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	DMA_Init(DMA1_Channel4, &DMA_InitStructure);
	DMA_ClearITPendingBit(DMA_IT_TC);
	DMA_ClearITPendingBit(DMA1_IT_TC4);
	DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);

	DMA_Cmd(DMA1_Channel4, DISABLE);

}

void DMA1_Channel5_changeBufferSize(uint8_t *dataFromUART, uint8_t bufferSize)
{
	DMA_DeInit(DMA1_Channel5);
	DMA_InitTypeDef DMA_InitStructure;
	DMA_InitStructure.DMA_BufferSize = bufferSize;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)dataFromUART;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;


	DMA_Init(DMA1_Channel5, &DMA_InitStructure);
	DMA_ClearITPendingBit(DMA_IT_TC);
	DMA_ClearITPendingBit(DMA1_IT_TC5);
	DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE);

	DMA_Cmd(DMA1_Channel5, ENABLE);





}

//void USART1_IRQHandler()
//{
//	USART_ClearFlag(USART1,USART_FLAG_PE);
//}


uint8_t CalculateCRC8(uint8_t *data, size_t len)
{
    uint8_t crc = 0xff;
    size_t i, j;
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if ((crc & 0x80) != 0)
                crc = (uint8_t)((crc << 1) ^ POLYNOMIAL);
            else
                crc <<= 1;
        }
    }
    return crc;
}

void StartSeries()
{
	xSemaphoreGive(StartSeries_Semaphore);
}

void StartSeriesFromISR()
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(StartSeries_Semaphore, &xHigherPriorityTaskWoken);
	if(xHigherPriorityTaskWoken == pdTRUE)
	{
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}
}

void HMI_Interaction_Task(void *pvParameters)
{
	ValvesImpulseParametersTypeDef valvesImpulseParametersTypeDef; //Параметры продувки
	EngineStartingAndStoppingParametersTypeDef  engineStartingAndStoppingParametersTypeDef;        //Параметры по запуску и торможению

	uint8_t dataFromUART[DMA_BUFFER_SETTINGS_SIZE];
	uint8_t dataToUART[DMA_ACKNOWLEDGE_BYTE_SIZE];

	Buttons_init();

	//Настройка DMA
	DMA_init(&dataFromUART[0], &dataToUART[0]);

	//Настройка USART
	UART_GPIO_init();
	UART_init();

	I2C_GPIO_init();
	I2C_init();

	//vTaskDelay(5000);
	//xSemaphoreGive(StartEngine_Semaphore);


	while(1)
	{

		//vTaskDelay(portMAX_DELAY);

		xSemaphoreTake(USART1_DataReceived_Semaphore, portMAX_DELAY);
		if(dataFromUART[1] == CalculateCRC8(dataFromUART, 1))
		{
			dataToUART[0] = ACKNOWLEDGE_BYTE;
			dataToUART[1] = CalculateCRC8(dataToUART, 1);
			vTaskDelay(1);
			DMA_Cmd(DMA1_Channel4, ENABLE);    //Отправка данных в модуль UART с помощью DMA

			switch(dataFromUART[0])
			{
				case START_ENGINE_COMMAND: 					  //Получена команда на пуск двигателя
				{
					xSemaphoreGive(StartEngine_Semaphore);
				} break;

				case STOP_ENGINE_COMMAND:					//Получена команда на стоп двигателя
				{

				} break;

				case START_SERIES_COMMAND:                 //Получена команда на запуск серии по работе с клапанами
				{
					StartSeries();
				} break;

				case SET_VALVES_SETTINGS_COMMAND:         //Получена команда на задание настроек управления клапанами
				{
					DMA1_Channel5_changeBufferSize(dataFromUART, DMA_BUFFER_SETTINGS_SIZE);
					if (xSemaphoreTake(USART1_DataReceived_Semaphore, 1000) == pdTRUE)
					{
						DMA_Cmd(DMA1_Channel4, ENABLE);
						vTaskDelay(10);
						SetValvesAndEngineSettings(&valvesImpulseParametersTypeDef, &engineStartingAndStoppingParametersTypeDef, dataFromUART);
						xQueueSend(ValvesImpulseParameters_Queue, &valvesImpulseParametersTypeDef, 0);
						xQueueSend(EngineParameters_Queue, &engineStartingAndStoppingParametersTypeDef, 0);
					}
					DMA1_Channel5_changeBufferSize(dataFromUART, COMMAND_SIZE);
				} break;

			}
		}
		else
		{
			dataToUART[0] = NO_ACKNOWLEDGE_BYTE;
			dataToUART[1] = CalculateCRC8(dataToUART, 1);
			DMA_Cmd(DMA1_Channel4, ENABLE);
		}
	}
}

void SetValvesAndEngineSettings(ValvesImpulseParametersTypeDef *valvesImpulseParameters, EngineStartingAndStoppingParametersTypeDef *engineStartingAndStoppingParametersTypeDef, uint8_t *dataFromUART)
{
	valvesImpulseParameters->SeriesPeriod = (dataFromUART[0] | (dataFromUART[1] << 8));
	valvesImpulseParameters->ImpulseActive = (dataFromUART[3] | (dataFromUART[4] << 8));
	valvesImpulseParameters->ImpulsePause = (dataFromUART[6] | (dataFromUART[7] << 8));
	valvesImpulseParameters->PurgeParameters = dataFromUART[9];
	valvesImpulseParameters->PressureMax = (dataFromUART[11] | (dataFromUART[12] << 8));
	
	engineStartingAndStoppingParametersTypeDef->StartingTime = (dataFromUART[14] | (dataFromUART[15] << 8)) ; //Время в секундах
	engineStartingAndStoppingParametersTypeDef->StoppingTime =  (dataFromUART[17] | (dataFromUART[18] << 8));
}


void DMA1_Channel5_IRQHandler()
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(USART1_DataReceived_Semaphore, &xHigherPriorityTaskWoken);
	if(xHigherPriorityTaskWoken == pdTRUE)
	{
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}

	DMA_ClearITPendingBit(DMA_IT_TC);
	DMA_ClearITPendingBit(DMA1_IT_TC5);
	//DMA_ClearFlag(DMA1_FLAG_TC5);
}

void DMA1_Channel4_IRQHandler()
{
	DMA_Cmd(DMA1_Channel4, DISABLE);
	//DMA_ClearFlag(DMA1_FLAG_TC4);
	DMA_ClearITPendingBit(DMA_IT_TC);
	DMA_ClearITPendingBit(DMA1_IT_TC4);
}

void Buttons_init()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //Вход с поддяжкой к питанию
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; //!!!ПРОВЕРИТЬ на 2 MHz!!!
	GPIO_InitStructure.GPIO_Pin = START_STOP_BUTTON | VALVES_START_BUTTON; 

	GPIO_Init(GPIOA, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	vTaskDelay(500);

	 /*
	  Настройка EXTI
    */

	AFIO->EXTICR[0] &= ~((AFIO_EXTICR1_EXTI0) | AFIO_EXTICR1_EXTI3); //Нулевой канал EXTI подключен к порту PA0 и PA3
	EXTI->FTSR |= EXTI_FTSR_TR0 | EXTI_FTSR_TR3; //Прерывание по спаду импульса
	EXTI->PR = EXTI_PR_PR0 | EXTI_PR_PR3;      //Сбрасываем флаг прерывания
	                               //перед включением самого прерывания

	EXTI->IMR |= EXTI_IMR_MR0 | EXTI_IMR_MR3;   //Включаем прерывание 0-го и 3-го каналов EXTI

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 9;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

	NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 9;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

	NVIC_Init(&NVIC_InitStructure);
}

void EXTI0_IRQHandler()
{
	if (GPIO_ReadInputDataBit(GPIOA, START_STOP_BUTTON) == 0) //Отслеживаем нажатие кнопки, может быть просто наводка
	{
		EXTI->IMR &= ~EXTI_IMR_MR0; //Сразу вырубаем прерывания - боремся с дребезгом
		EXTI->IMR &= ~EXTI_IMR_MR3; //Сразу вырубаем прерывания на второй кнопке. По непонятной причине при нажатии на одну кнопку, вторая тоже отрабатывает

		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(StartEngine_Semaphore, &xHigherPriorityTaskWoken);
		if (xHigherPriorityTaskWoken == pdTRUE)
		{
			portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
		}
	}
	
	EXTI_ClearFlag(EXTI_Line0);
	EXTI_ClearFlag(EXTI_Line3); //Для предупреждения одновременного срабатывания
}

void EXTI3_IRQHandler()
{
	if (GPIO_ReadInputDataBit(GPIOA, VALVES_START_BUTTON) == 0)  //Отслеживаем нажатие кнопки, может быть просто наводка
	{
		EXTI->IMR &= ~EXTI_IMR_MR3; //Сразу вырубаем прерывания - боремся с дребезгом
		EXTI->IMR &= ~EXTI_IMR_MR0; //Сразу вырубаем прерывания на второй кнопке. По непонятной причине при нажатии на одну кнопку, вторая тоже отрабатывает

		StartSeriesFromISR();
	}
	
	EXTI_ClearFlag(EXTI_Line3);
	EXTI_ClearFlag(EXTI_Line0); //Для предупреждения одновременного срабатывания
}



