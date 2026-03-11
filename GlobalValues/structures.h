#include "stm32f10x.h"

#define VALVES_COUNT         8
//#define VALVE_1              GPIO_Pin_11 //PA11
//#define VALVE_2              GPIO_Pin_15 //PA15
//#define VALVE_3              GPIO_Pin_12 //PB12
//#define VALVE_4				 GPIO_Pin_3 //PB3
//#define VALVE_5              GPIO_Pin_4 //PB4
//#define VALVE_6              GPIO_Pin_5 //PB5
//#define VALVE_7              GPIO_Pin_0 //PB10
//#define VALVE_8              GPIO_Pin_1 //PB11
#define VALVE_1              GPIO_Pin_12 //PB12
#define VALVE_2              GPIO_Pin_13 //PB13
#define VALVE_3              GPIO_Pin_14 //PB14
#define VALVE_4				 GPIO_Pin_15 //PB15
#define VALVE_5              GPIO_Pin_8 //PA8
#define VALVE_6              GPIO_Pin_10 //PA10
#define VALVE_7              GPIO_Pin_11 //PA11
#define VALVE_8              GPIO_Pin_12 //PA12

#ifndef STRUCTURES_H
#define STRUCTURES_H

typedef enum
{
	GPIOA_Port,
	GPIOB_Port
} ValvePortEnum;

typedef enum
{
	PulseBeamOff = 0,
	PulseBeamOn,
} PulseBeamState;

typedef enum
{
	ZeroDetectorSleep = 0,
	ZeroDetectorHappened
} ZeroDetectorState;

typedef enum
{
	EngineStarting = 0, //Состояние двигателя ЗАПУСК
	EngineStopping,     //Состояние двигателя ТОРМОЖЕНИЕ
	EngineStarted,       //Состояние двигателя ЗАПУЩЕН
	EngineStopped,       //Состояние двигателя ОСТАНОВЛЕН
	EngineStoppingNow,   //Состояние двигателя НЕМЕДЛЕННОЕ ТОРМОЖЕНИЕ. Когда нажали СТОП, но двигатель еще не запущен до конца
	EngineAlarm         //Состояние двигателя АВАРИЯ
} EngineState;

typedef enum
{
	IsTransmit = 0,
	IsReceive
} I2CWorkerStateEnum;

typedef enum 
{
	SeriesPeriod = 0,   //Пауза между серией
	PauseBetweenCycles, //Пауза между циклами
	ImpulseActive,      //Клапан открыт
	ImpulsePause        //Клапан закрыт, но идет серия
} ValvesStateEnum;   //Состояние клапанов

typedef struct
{
	uint8_t TIM1_UP_Handler_wait_flag; //флаг ожидания прерывания от первого таймер-счетчика
	uint8_t UART1_RXNE_Handler_flag;  //флаг по приему данных от UART
} FlagsTypeDef;


typedef struct
{
	uint32_t SeriesPeriod; 			//Период очистки, мин
	uint16_t PauseBetweenCycles;    //Пауза между циклами продувок
	uint8_t BetweenCyclesCount;     //Количество циклов продувки
	uint16_t ImpulseActive; 		//Длительность импульса, мс
	uint16_t ImpulsePause; 			//Длительность паузы, с
	uint8_t  PurgeParameters; 		//Параметры продувки
	uint16_t PressureMax;   		//Уставка предельного давления
} ValvesImpulseParametersTypeDef; 	//Настройка периодов режимов клапанов


typedef struct 
{
	uint16_t StartingTime; 			 //Время запуска
	uint16_t StoppingTime;  			//Время торможения
} EngineStartingAndStoppingParametersTypeDef;

typedef struct
{
	ValvePortEnum ValvePort;   									//Порт
	uint16_t ValveNumber;                                       //Номер клапана
} ValvesControlTypeDef;        									//Управление клапанами

typedef struct
{
	ZeroDetectorState  	ZeroDetector_1;
	ZeroDetectorState  ZeroDetector_2;
	uint8_t            IsPhaseChecked; //Пройдена проверка чередования фаз
	uint16_t           TimeCounterPhaseCorrecting; //Счетчик для проверки правильности чередования фаз
	uint16_t           TimeCounterCrossingPhase; //Счетчик времени. Отсчитывает время с момента пересечения первой фазой нуля
	uint16_t           TimeCounter_1; //Счетчик времени
	uint16_t           TimeCounter_2; //Счетчик времени
	uint16_t           OpeningAngle;  //Угол открытия тиристора. Величина для обеспечения фазного регулирования
	uint16_t           PwmPulseBeamCount; //Количество импульсов по 100 мкс. !!!Причем в одном импульсе 2 таких счетчика!!!
	PulseBeamState     PulseBeamState_1;
	PulseBeamState     PulseBeamState_2;
	EngineState        Engine_State;
	uint16_t           StartingTime; //Время запуска
	uint16_t           StoppingTime; //Время торможения
	uint8_t            IsPhaseCrossing_13; //Было/НЕбыло пересечения фаз 1 и 3
	uint8_t            IsPhaseCrossing_23; //Было/НЕбыло пересечения фаз 2 и 3
} EngineControlTypeDef;

typedef struct
{
	I2CWorkerStateEnum I2CWorkerState;
	uint8_t JobIsDone;
	uint8_t *dataFromI2C;
	uint8_t IsWaiting;
	uint8_t IsDataReceived;
} I2CWorkerStructTypeDef;

typedef struct
{
	uint16_t Pressure;
	uint16_t Temperature;
	uint16_t ScalePressureFactor;
} SensorParametersTypeDef;

#endif


