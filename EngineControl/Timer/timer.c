#include "Timer/timer.h"

//extern xSemaphoreHandle               TIM1_UP_IRQ_Semaphore;
extern EngineControlTypeDef           *engineControl;

void PWM_Init()
{
	TIM_OCInitTypeDef PWM_InitStructure;
	PWM_InitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	PWM_InitStructure.TIM_Pulse = PWM_IMPULSE_OFF;//PWM_IMPULSE_OFF; //PWM_IMPULSE_ON - Длительность импульса 4 мкс (1, 2, 3, 4)
	PWM_InitStructure.TIM_OutputState = TIM_OutputState_Enable;
	PWM_InitStructure.TIM_OutputNState = TIM_OutputNState_Disable;
	PWM_InitStructure.TIM_OCPolarity = TIM_OCNPolarity_High;
	PWM_InitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;

	TIM_OC2Init(TIM1, &PWM_InitStructure); //2 канал
	TIM_OC1Init(TIM3, &PWM_InitStructure); //1 канал

	//TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable); //Автоматическая перезарядка таймера

}

void TimerInit()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	//RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	TIM_TimeBaseInitTypeDef TIM_InitStructure;
	TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_CenterAligned1;
	TIM_InitStructure.TIM_Prescaler = TIM_PRESCALLER;
	TIM_InitStructure.TIM_Period = PWM_COUNTER;
	TIM_InitStructure.TIM_RepetitionCounter = 0; //Генерация прерывания по каждому переполнению. Досчитал вверх (прерывание), досчитал вниз (прерывание)
	TIM_TimeBaseInit(TIM1, &TIM_InitStructure);
	TIM_TimeBaseInit(TIM3, &TIM_InitStructure);
	//TIM_TimeBaseInit(TIM4, &TIM_InitStructure);

	//Настройка прерываний
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);



	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 8;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 8;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	//NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	//NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 8;
	//NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	//NVIC_Init(&NVIC_InitStructure);

	TIM_ClearITPendingBit(TIM1, TIM_IT_Update); //Сразу сбрасываем флаг прерывания, чтобы не уйти в прерывание после инициализации таймера
	TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);

	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	TIM_ITConfig(TIM3, TIM_IT_Update, DISABLE);

	//TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
	//TIM_ITConfig(TIM4, TIM_IT_Update, DISABLE);
	TIM_Cmd(TIM1, ENABLE);
	TIM_Cmd(TIM3, ENABLE);
	//TIM_Cmd(TIM4, ENABLE);

	TIM_CtrlPWMOutputs(TIM1, ENABLE); //Даем сигналы на выводы ШИМ
	TIM_CtrlPWMOutputs(TIM3, ENABLE);



}

void PWM_GPIO_Init()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//GPIO_Mode_Out_PP;//GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = PWM_GPIO_OUTPUT_PINS_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; //Обязательно в режим low-спид
	GPIO_Init(PWM_GPIO_OUTPUT_PORT_1, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//GPIO_Mode_Out_PP;//GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = PWM_GPIO_OUTPUT_PINS_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; //Обязательно в режим low-спид
	GPIO_Init(PWM_GPIO_OUTPUT_PORT_2, &GPIO_InitStructure);

}

//uint16_t TIM1_counter = 0;
void TIM1_UP_IRQHandler()
{
	//GPIOB->ODR ^= 1 << 12;
	//TIM1_counter = TIM1->CNT;
	//engineControl->TimeCounter_1 = engineControl->TimeCounter_1 + 1;
// 
	//GPIOA->ODR ^= 1 << 15;
	engineControl->TimeCounterPhaseCorrecting++;
	engineControl->TimeCounterCrossingPhase++;
	engineControl->TimeCounter_1++;
	engineControl->TimeCounter_2++;
	TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
}

//void TIM3_IRQHandler()
//{
//	//GPIOB->ODR ^= (1 << 12);
//	engineControl->TimeCounter_2 = engineControl->TimeCounter_2 + 1;
//	//GPIOA->ODR ^= 1 << 9;
//	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
//}

//void TIM4_IRQHandler()
//{
//	//GPIOB->ODR ^= (1 << 12);
//	engineControl->TimeCounterCrossingPhase = engineControl->TimeCounterCrossingPhase + 1;
//	//GPIOA->ODR ^= 1 << 9;
//	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
//}






