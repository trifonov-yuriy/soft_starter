#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_usart.h>
#include <misc.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_dma.h>
#include <FreeRTOS.h>
#include <FreeRTOSConfig.h>
#include <semphr.h>
#include <task.h>
#include "GlobalValues/structures.h"
#include <stm32f10x_exti.h>
#include "Flash/flash.h"


#define DMA_BUFFER_SIZE       						3 // 9 байт данных + 5 контрольных сумм
#define DMA_BUFFER_COMMAND_SIZE 					2 //1 байт - команда + 1 контрольная сумма
#define DMA_ACKNOWLEDGE_BYTE_SIZE 					2 //1 байт - команда + 1 контрольная сумма
#define DMA_BUFFER_SETTINGS_SIZE					20
#define DATA_FROM_UART_SIZE   						DMA_BUFFER_SIZE
#define POLYNOMIAL            						0x31
#define ACKNOWLEDGE_BYTE        					0xB0 //Байт подтверждения
#define NO_ACKNOWLEDGE_BYTE     					0xD0 //Байт НЕ подтверждения
#define START_SERIES_COMMAND                        0x03 //команда на принудительный пуск серии импульсов
#define SET_VALVES_SETTINGS_COMMAND                 0x05 //Команда для задания настроек управления клапанами
#define START_ENGINE_COMMAND                        0x06 //команда на запуск двигателя
#define STOP_ENGINE_COMMAND                         0x08 //команда на принудительный останов двигателя
#define COMMAND_SIZE                                2
#define UART_BAUD_RATE                              9600

#define START_STOP_BUTTON                          GPIO_Pin_0 //PA0 //Запуск/Останов
#define VALVES_START_BUTTON                        GPIO_Pin_3 //PA3 //Принудительная продувка


void HMI_Interaction_Task(void *pvParameters);
//void USART1_IRQHandler();
void DMA1_Channel5_IRQHandler();
void DMA1_Channel4_IRQHandler();


void Buttons_init();
void EXTI0_IRQHandler();
void EXTI3_IRQHandler();

