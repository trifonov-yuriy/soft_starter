#include "i2c.h"

extern SensorParametersTypeDef *sensorParameters;
extern xTimerHandle RTOS_tim_1;
static uint8_t dataReady = 0;
uint8_t dataToMaster[9];
static uint8_t dataCounter = 0;

void I2C2_GPIO_init() 
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &GPIO_InitStruct);
}
void I2C2_init() 
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
	
	I2C_InitTypeDef I2C_InitStructure;
	
	I2C_InitStructure.I2C_ClockSpeed = 400000;
	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_16_9;
	I2C_InitStructure.I2C_OwnAddress1 = 0x25;
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	
	I2C_Init(I2C2, &I2C_InitStructure);
	
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = I2C2_EVT_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 8;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
	
	
	I2C_ITConfig(I2C2, I2C_IT_EVT, ENABLE);
	I2C_ITConfig(I2C2, I2C_IT_SB, ENABLE);
	I2C_ITConfig(I2C2, I2C_IT_ADDR, ENABLE);
	I2C_ITConfig(I2C2, I2C_IT_STOPF, ENABLE);
	I2C_ITConfig(I2C2, I2C_IT_RXNE, ENABLE);
	I2C_ITConfig(I2C2, I2C_IT_BTF, ENABLE);
	
	I2C_Cmd(I2C2, ENABLE);
	
	//I2C_ITConfig(I2C1, I2C_IT_TXE, ENABLE);
	//I2C_ITConfig(I2C1, I2C_IT_RXNE, ENABLE);
	
	
}

void I2C_Slave_Task(void *pvParameters) 
{
	dataToMaster[0] = 123;
	dataToMaster[1] = 232;
	dataToMaster[2] = 54;
	dataToMaster[3] = 234;
	dataToMaster[4] = 65;
	dataToMaster[5] = 73;
	dataToMaster[6] = 5;
	dataToMaster[7] = 194;
	dataToMaster[8] = 56;
	dataCounter = 0;
	
	I2C2_GPIO_init();
	I2C2_init();
	
	
	while(1) 
	{
		
	}
}

void I2C2_EVT_IRQHandler() 
{
	(void) I2C2->SR1;
	(void) I2C2->SR2;
	if(I2C_GetITStatus(I2C2, I2C_IT_SB)) 
	{
		I2C_ClearITPendingBit(I2C2, I2C_IT_SB);
	}
	else if(I2C_GetITStatus(I2C2, I2C_IT_ADDR)) 
	{
		I2C_SendData(I2C2, dataToMaster[dataCounter]);
		dataCounter++;
		I2C_ClearITPendingBit(I2C2, I2C_IT_ADDR);
	}
	else if(I2C_GetITStatus(I2C2, I2C_IT_STOPF)) 
	{
		dataCounter = 0;
		dataReady = 0;
		xTimerStart(RTOS_tim_1, 0);
		I2C_ClearITPendingBit(I2C2, I2C_IT_STOPF);
	}
	else if(I2C_GetITStatus(I2C2, I2C_IT_BTF) && I2C_GetITStatus(I2C2, I2C_IT_AF)) 
	{
		dataCounter = 0;
		dataReady = 0;
		xTimerStart(RTOS_tim_1, 0);
		I2C_ClearITPendingBit(I2C2, I2C_IT_AF);
	}
	else if(I2C_GetITStatus(I2C2, I2C_IT_BTF)) 
	{
		I2C_SendData(I2C2, dataToMaster[dataCounter]);
		dataCounter++;
	}
	
	I2C_ClearITPendingBit(I2C2, I2C_IT_EVT);
}

void DataReadyTimer_Function(xTimerHandle RTOS_tim_1) 
{
	dataReady = 1;
}