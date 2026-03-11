#include "HMI_Interaction/I2C_Slave/i2c_slave.h"


static uint8_t dataReady = 0;
static uint8_t dataToMaster[9];
static uint8_t dataCounter = 0;
static uint16_t command;

void I2C2_GPIO_init()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
	GPIO_InitTypeDef GPIO_InitStruct;
	//PB10 SCL, PB11 SDA
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_OD;
	//GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStruct);

}
void I2C2_init()
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
	I2C_InitTypeDef I2C_InitStructure;

	I2C_InitStructure.I2C_ClockSpeed = 100000;
	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
	I2C_InitStructure.I2C_OwnAddress1 = (0x25 << 1);
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;




	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = I2C2_EV_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 8;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);



    I2C_ITConfig(I2C2, I2C_IT_EVT, ENABLE);
   // I2C_ITConfig(I2C2, I2C_IT_STOPF, ENABLE);

    I2C_Init(I2C2, &I2C_InitStructure);
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
	dataReady = 0;

	I2C2_GPIO_init();
	I2C2_init();


	while(1)
	{
	}
}

void I2C2_EV_IRQHandler()
{
	switch (I2C_GetLastEvent(I2C2))
	{

	  /* Slave Transmitter ---------------------------------------------------*/
	 	 case I2C_EVENT_SLAVE_BYTE_TRANSMITTED:
	 	 case I2C_EVENT_SLAVE_BYTE_TRANSMITTING:             /* EV3 */
	 	 {
	 		 /* This and the one below are both sent from the send mode. I don’t really understand the specific difference between the two. It feels like the difference between empty and non-empty shift register. Get ready to send data */
	 		I2C_SendData(I2C2, dataToMaster[dataCounter]);
	 		dataCounter++;
	 	 } break;

	 	 case I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED:
	 	 {
	 		(void)I2C2->SR1;
	 		(void)I2C2->SR2;
	 		I2C_SendData(I2C2, dataToMaster[dataCounter]);
	 		dataCounter++;
	 		I2C_ITConfig(I2C2, I2C_IT_BUF, ENABLE);
	 		I2C_ClearFlag(I2C2, I2C_IT_ADDR);
	 		I2C_ClearITPendingBit(I2C2, I2C_IT_ADDR);
	 	 } break;

	  /* Transmit I2C2 data */

	 /* Slave Receiver ------------------------------------------------------*/

	 	 case I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED:     /* EV1 */
	 	 {
		 	(void)I2C2->SR1;
		 	(void)I2C2->SR2;
		 	 I2C_ITConfig(I2C2, I2C_IT_BUF, ENABLE);
	 		 I2C_ClearFlag(I2C2, I2C_IT_ADDR);
	 		 I2C_ClearITPendingBit(I2C2, I2C_IT_ADDR);
	 	 } break;
	     /* Address matching interrupt, no matter from sending or receiving, the address must be matched, as shown in Figure 244, 243 will respond to EV1 after sending the address */

	  /* Store I2C1 received data */

	     /* This interrupt is in response to the EV2 interrupt, as shown in Figure 244, every time the host sends a data, an EV2 interrupt will be generated */
	     /* Fill the received interrupts into the array */
	     /* Note: The address will not be filled in */

	 	 case I2C_EVENT_SLAVE_STOP_DETECTED:                /* EV4 */
	 	 {
	 		 (void)I2C2->SR1;
	 		 (void)I2C2->SR2;
	 		  I2C_ITConfig(I2C2, I2C_IT_BUF, DISABLE);
	 		  I2C_Cmd(I2C2, ENABLE);
	 		I2C_ClearFlag(I2C2, I2C_FLAG_STOPF);
	 		 I2C_ClearITPendingBit(I2C2, I2C_IT_STOPF);
	 	 } break;
	 	 /* Clear I2C1 STOPF flag */
	     /* This is a stop signal generated during a normal stop */
	     /* I don’t know why this is the case. If you don’t respond to the host after receiving a bunch of data, you can turn off i2c, and then reconfigure i2c after processing the data, remember to reconfigure */

	 	 default:
	 	 {
	 		I2C_ITConfig(I2C2, I2C_IT_EVT | I2C_IT_BUF, DISABLE);
	 	 }  break;
	}
	I2C_ClearITPendingBit(I2C2, I2C_IT_EVT);
}


