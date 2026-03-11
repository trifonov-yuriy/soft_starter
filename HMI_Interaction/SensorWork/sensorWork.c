#include "HMI_Interaction/SensorWork/sensorWork.h"
static I2CWorkerStructTypeDef *I2CWorker;
extern xTimerHandle RTOS_tim_1;
extern xQueueHandle SensorParameters_Queue;            //Данные с датчика давления

void DMA_SensorWork_init(uint8_t *dataFromI2C)
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	DMA_InitTypeDef DMA_InitStructure;
	DMA_InitStructure.DMA_BufferSize = (I2C_PROTOCOL_SIZE - 1);
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) dataFromI2C;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_PeripheralBaseAddr = &I2C1->DR;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel7_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 8;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	DMA_Init(DMA1_Channel7, &DMA_InitStructure);
	DMA_ITConfig(DMA1_Channel7, DMA_IT_TC, ENABLE);
	DMA_Cmd(DMA1_Channel7, DISABLE);

}


void I2C_GPIO_init()
{

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_I2C1, ENABLE); //i2c1 function remapping
	// SCL PB8 // SDA PB9
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);

}

void I2C_init()
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
	I2C_InitTypeDef I2C_InitStructure;
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitStructure.I2C_ClockSpeed = 400000;
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStructure.I2C_OwnAddress1 = 0x68;


	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = I2C1_EV_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 8;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);


	I2C_ITConfig(I2C1, I2C_IT_EVT, ENABLE);

	//I2C_ITConfig(I2C1, I2C_IT_ERR, ENABLE);
	//I2C_ITConfig(I2C1, I2C_IT_EVT, ENABLE);
	//I2C_ITConfig(I2C1, I2C_IT_EVT, ENABLE);
	I2C_Init(I2C1, &I2C_InitStructure);


	I2C_Cmd(I2C1, ENABLE);


	I2C_DMACmd(I2C1, ENABLE);
	//I2C_DMALastTransferCmd(I2C1, ENABLE);


}

int CalculateCRC(uint8_t *data, int len)
{
	uint8_t crc = 0xff;
	uint8_t i, j;
	for (i = 0; i < len; i++)
	{
		crc ^= data[i];
		for (j = 0; j < 8; j++)
		{
			if ((crc & 0x80) != 0)
			{
				crc = (uint8_t)((crc << 1) ^ POLINOM);
			}
			else
			{
				crc <<= 1;
			}
		}
	}
	return crc;
}

void SensorWork_Task(void *pvParameters)
{
	uint8_t dataFromI2C[I2C_PROTOCOL_SIZE];
	uint8_t *dataForCRC;
	I2CWorkerStructTypeDef I2CWorkerStruct;
	I2CWorkerStruct.I2CWorkerState = IsTransmit; //Изначально передаем данные на шину I2C
	I2CWorkerStruct.JobIsDone = 1; //Работа изначально завершена
	I2CWorkerStruct.dataFromI2C = dataFromI2C;
	I2CWorkerStruct.IsWaiting = 1;
	I2CWorker = &I2CWorkerStruct;

	SensorParametersTypeDef SensorParameters;


	DMA_SensorWork_init(&dataFromI2C[0]);

	I2C_GPIO_init();
	I2C_init();
	//xTimerStart(RTOS_tim_1, 0);

	//I2C2_GPIO_init();
	//I2C2_init();

/*
	DMA_Cmd(DMA1_Channel7, DISABLE);
	I2C_ITConfig(I2C1, I2C_IT_EVT, DISABLE);
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET);
	I2C_GenerateSTART(I2C1, ENABLE);
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_SB) == RESET);
	I2C_Send7bitAddress(I2C1, SLAVE_ADDRESS << 1, I2C_Direction_Transmitter);
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE) == RESET);

	if(I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR) == SET)
	{
		I2C_ClearFlag(I2C1, I2C_FLAG_ADDR);
		(void)I2C1->SR1;
		(void)I2C1->SR2;
		I2C_SendData(I2C1, START_CONTINUES_MEASUREMENT_COMMAND_MASS_FLOW_AVARAGE_NONE >> 8);
		while(I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE) == RESET);
		I2C_SendData(I2C1, START_CONTINUES_MEASUREMENT_COMMAND_MASS_FLOW_AVARAGE_NONE & 0xff);
		while(I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE) == RESET);


	}
	I2C_GenerateSTOP(I2C1, ENABLE);
*/
	while(1)
	{
		vTaskDelay(50);
		if(I2CWorker->JobIsDone == 1)
		{
			dataForCRC = &dataFromI2C[0];
			if(CalculateCRC(dataForCRC, 2) == dataFromI2C[2])
			{
				//SensorParameters.Pressure =
			}

			while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET);
			I2CWorker->IsWaiting = 1;
			I2CWorker->JobIsDone = 0;
			I2C_AcknowledgeConfig(I2C1, ENABLE);
			I2C_GenerateSTART(I2C1, ENABLE);
		}


/*
		vTaskDelay(300);
		I2C_AcknowledgeConfig(I2C1, ENABLE);
		while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET);
		I2C_GenerateSTART(I2C1, ENABLE);
		while(I2C_GetFlagStatus(I2C1, I2C_FLAG_SB) == RESET);
		I2C_Send7bitAddress(I2C1, SLAVE_ADDRESS << 1, I2C_Direction_Receiver);
		I2C_ClearFlag(I2C1, I2C_FLAG_ADDR);
		(void)I2C1->SR1;
		(void)I2C1->SR2;
		while(I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET);
		dataFromI2C[0] = I2C_ReceiveData(I2C1);
		while(I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET);
		dataFromI2C[1] = I2C_ReceiveData(I2C1);
		while(I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET);
		dataFromI2C[2] = I2C_ReceiveData(I2C1);
		while(I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET);
		dataFromI2C[3] = I2C_ReceiveData(I2C1);
		while(I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET);
		dataFromI2C[4] = I2C_ReceiveData(I2C1);
		while(I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET);
		dataFromI2C[5] = I2C_ReceiveData(I2C1);
		while(I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET);
		dataFromI2C[6] = I2C_ReceiveData(I2C1);
		while(I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET);
		dataFromI2C[7] = I2C_ReceiveData(I2C1);
		while(I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET);
		dataFromI2C[8] = I2C_ReceiveData(I2C1);
		I2C_AcknowledgeConfig(I2C1, DISABLE);
		while(I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET);
		dataFromI2C[9] = I2C_ReceiveData(I2C1);
		I2C_GenerateSTOP(I2C1, ENABLE);
		//DMA_Cmd(DMA1_Channel7, ENABLE);
*/
	}
}

void I2C1_EV_IRQHandler()
{
	// если шина не занята (start условие сработало)
	//(void) I2C1->SR1;
	//(void) I2C1->SR2;
	//(void) I2C2->SR1;
	//(void) I2C2->SR2;

	if(I2C_GetITStatus(I2C1, I2C_IT_SB))
	{
		if(I2CWorker->I2CWorkerState == IsReceive)
		{
			I2C_Send7bitAddress(I2C1, SLAVE_ADDRESS << 1, I2C_Direction_Receiver);
			//I2C_ITConfig(I2C1, I2C_IT_BUF, ENABLE);
			DMA_Cmd(DMA1_Channel7, ENABLE);
		}

		else if(I2CWorker->I2CWorkerState == IsTransmit)
		{
			I2C_Send7bitAddress(I2C1, SLAVE_ADDRESS << 1, I2C_Direction_Transmitter);
			//I2C_ITConfig(I2C1, I2C_IT_BUF , ENABLE);
		}

		I2C_ClearITPendingBit(I2C1, I2C_IT_SB);
	}

	else if(I2C_GetITStatus(I2C1, I2C_IT_RXNE))
	{
		I2CWorker->dataFromI2C[8] = I2C_ReceiveData(I2C1);
		I2C_ClearITPendingBit(I2C1, I2C_IT_RXNE);
	}

	else if(I2C_GetITStatus(I2C1, I2C_IT_ADDR))
	{
		(void) I2C1->SR1;
		(void) I2C1->SR2;
		I2C_ClearITPendingBit(I2C1, I2C_IT_ADDR);
		if(I2CWorker->I2CWorkerState == IsTransmit)
		{
			I2C_SendData(I2C1, START_CONTINUES_MEASUREMENT_COMMAND_MASS_FLOW_AVARAGE_NONE >> 8);
		}
	}

	else if(I2C_GetITStatus(I2C1, I2C_IT_TXE))
	{
		if(I2CWorker->I2CWorkerState == IsTransmit)
		{
			if(I2C_GetITStatus(I2C1, I2C_IT_AF))
			{
				(void)I2C1->SR1;
				(void)I2C1->SR2;
				I2C_ITConfig(I2C1, I2C_IT_BUF, DISABLE);
				I2CWorker->I2CWorkerState = IsTransmit;
				I2CWorker->JobIsDone = 1;
				I2C_GenerateSTOP(I2C1, ENABLE);
				I2C_ClearITPendingBit(I2C1, I2C_IT_AF);
			}
			else
			{
				I2CWorker->I2CWorkerState = IsReceive;
				I2C_SendData(I2C1, START_CONTINUES_MEASUREMENT_COMMAND_MASS_FLOW_AVARAGE_NONE & 0xff);
			}
		}
		else
		{
			(void) I2C1->SR1;
			(void) I2C1->SR2;
			//(void) I2C2->SR1;
			//(void) I2C2->SR2;
			I2C_GenerateSTOP(I2C1, ENABLE);
			I2CWorker->JobIsDone = 1;
			I2C_ITConfig(I2C1, I2C_IT_BUF, DISABLE);
		}
	}
/*
	switch(I2C_GetLastEvent(I2C1))
	{
		case I2C_EVENT_MASTER_BYTE_RECEIVED:
		{
			I2CWorker->dataFromI2C[8] = I2C_ReceiveData(I2C1);
		} break;

		case I2C_EVENT_MASTER_MODE_SELECT:
		{
			if(I2CWorker->I2CWorkerState == IsReceive)
			{
				I2C_Send7bitAddress(I2C1, SLAVE_ADDRESS << 1, I2C_Direction_Receiver);
				DMA_Cmd(DMA1_Channel7, ENABLE);
				I2C_ClearITPendingBit(I2C1, I2C_IT_SB);
			}

			else if(I2CWorker->I2CWorkerState == IsTransmit)
			{
				I2C_Send7bitAddress(I2C1, SLAVE_ADDRESS << 1, I2C_Direction_Transmitter);
				//I2C_ITConfig(I2C1, I2C_IT_BUF , ENABLE);
				I2C_ClearITPendingBit(I2C1, I2C_IT_SB);
			}
		} break;

		case I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED:
		{
			(void) I2C1->SR1;
			(void) I2C1->SR2;
			I2C_ClearITPendingBit(I2C1, I2C_IT_ADDR);

		} break;

		case I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED:
		{
			(void) I2C1->SR1;
			(void) I2C1->SR2;
			I2C_ClearITPendingBit(I2C1, I2C_IT_ADDR);
			I2C_SendData(I2C1, START_CONTINUES_MEASUREMENT_COMMAND_MASS_FLOW_AVARAGE_NONE >> 8);
		} break;

		case I2C_EVENT_MASTER_BYTE_TRANSMITTING:
		case I2C_EVENT_MASTER_BYTE_TRANSMITTED:
		{
			if(I2CWorker->I2CWorkerState == IsTransmit)
			{
				if(I2C_GetITStatus(I2C1, I2C_IT_AF))
				{
					(void)I2C1->SR1;
					(void)I2C1->SR2;
					I2C_ITConfig(I2C1, I2C_IT_BUF, DISABLE);
					I2CWorker->I2CWorkerState = IsTransmit;
					I2CWorker->JobIsDone = 1;
					I2C_GenerateSTOP(I2C1, ENABLE);
					I2C_ClearITPendingBit(I2C1, I2C_IT_AF);
				}
				else
				{
					I2CWorker->I2CWorkerState = IsReceive;
					I2C_SendData(I2C1, START_CONTINUES_MEASUREMENT_COMMAND_MASS_FLOW_AVARAGE_NONE & 0xff);
				}
			}
			else
			{
				(void) I2C1->SR1;
				(void) I2C1->SR2;
				//(void) I2C2->SR1;
				//(void) I2C2->SR2;
				I2C_GenerateSTOP(I2C1, ENABLE);
				I2CWorker->JobIsDone = 1;
				I2C_ITConfig(I2C1, I2C_IT_BUF, DISABLE);
			}
		} break;

	}
*/
	 I2C_ClearITPendingBit(I2C1, I2C_IT_EVT);

}

void DMA1_Channel7_IRQHandler()
{
	(void)I2C1->SR1;
	(void)I2C1->SR2;
    //I2C1->CR1 |= I2C_CR1_PE;
	I2C_AcknowledgeConfig(I2C1, DISABLE);
	I2C_GenerateSTOP(I2C1, ENABLE);
	I2CWorker->JobIsDone = 1;
	I2C_ITConfig(I2C1, I2C_IT_BUF, DISABLE);
	DMA_Cmd(DMA1_Channel7, DISABLE);



	DMA_ClearITPendingBit(DMA_IT_TC);
	DMA_ClearITPendingBit(DMA1_IT_TC7);
}

void DataReadyTimer_Function(xTimerHandle RTOS_tim_1)
{
	I2CWorker->IsWaiting = 0;
}
