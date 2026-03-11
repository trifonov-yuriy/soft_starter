#include "AlarmActions/alarmActions.h"
void AlarmActionsInit()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //Вход с поддяжкой к плюсу
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; //!!!ПРОВЕРИТЬ на 2 MHz!!!
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;

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
}
/*
void EXTI0_IRQHandler()
{
	TIM_CtrlPWMOutputs(TIM1, DISABLE);
	TIM_Cmd(TIM1, DISABLE);



	EXTI_ClearFlag(EXTI_Line0);
}*/
