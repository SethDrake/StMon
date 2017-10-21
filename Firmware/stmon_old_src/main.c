#include <CoOS.h>
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_gpio.h"
#include "misc.h"
#include <stdbool.h>
#include <sim900.h>
#include "CoOS/kernel/CoOS.h"

enum LED_STATUS {
	LS_OK = 0, LS_GSM_INIT_ERROR = 1, LS_NET_ERROR = 2, LS_OTHER_ERROR = 3
};

#define TASK_STK_SIZE 128

OS_STK task1_stk[TASK_STK_SIZE];
OS_STK task2_stk[TASK_STK_SIZE];

OS_MutexID usart2_mutex; /*!< USART2 mutex id */

vu16 AD_Value;

bool initGSMError = false;
uint8_t ledStatus;

void NVIC_Configuration(void) {
	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void GPIO_Configuration(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

//	LED ports
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
//	USART2, PWRKEY
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

//	PB_0, PB_1
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

//	USART2_TX
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

//	USART2_RX
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

//	PA_5 - PWRKEY for SIM900
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void USART2_Configuration(void) {
	USART_InitTypeDef USART_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl =
			USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(USART2, &USART_InitStructure);

	USART_Cmd(USART2, ENABLE);
}

void switchStatusLed(bool state) {
	if (state) {
		GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_SET);
	} else {
		GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_RESET);
	}
}

void switchSim900PowerState( state) {
	if (state) {
		switchActivityLed(true);
		GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_RESET);
		CoTickDelay(1500);
		GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_SET);
	} else {
		GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_RESET);
		CoTickDelay(1800);
		GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_SET);
		switchActivityLed(false);
	}
	//dummy
}

bool disableSim900SleepMode() {
	sendCommandWithRepeat("AT", 1);
	currentGSMState.isSleeping = false;
	return sendCommandWithWaitReceive("AT+CSCLK=0", "OK", 200) == RS_OK;
}

bool activateSim900SleepMode() {
	currentGSMState.isSleeping = true;
	return sendCommandWithWaitReceive("AT+CSCLK=2", "OK", 200) == RS_OK;
}

void prepareModule() {
	sendCommand("ATE0");
	CoTickDelay(20);
	sendCommand("AT+GSMBUSY=1");
	CoTickDelay(20);
}

bool initSim900() {
	CoTickDelay(15000); //wait for power-up module
	disableSim900SleepMode();
	if (sendCommandWithWaitReceive("AT", "OK", 200) != RS_OK) {
		log_message("SIM900 doesn't respond at powerup!");
		return false;
	}
	return true;
}

void readSim900Info() {
	identifyModule();
	log_message(currentGSMState.imei);
}

void readSignalLevel() {
	readCSQ();

}

void taskGSM(void* pdata) {
	usart2_mutex = CoCreateMutex();
	if (usart2_mutex == E_CREATE_FAIL) {
		log_message("Error on create USART2 mutex!");
		ledStatus = LS_OTHER_ERROR;
		return;
	}
	CoEnterMutexSection(usart2_mutex);
	switchSim900PowerState(true);
	if (initSim900()) {
		currentGSMState.isOnline = true;
		prepareModule();
		readSim900Info();
		activateSim900SleepMode();
	} else { //if returned false - switch module power off and break execution
		initGSMError = true;
	}
	CoLeaveMutexSection(usart2_mutex);
	if (initGSMError) {
		log_message("Error on init SIM900!");
		switchSim900PowerState(false);
		ledStatus = LS_GSM_INIT_ERROR;
	}
	for (;;) {
		if (!initGSMError) {
			CoEnterMutexSection(usart2_mutex);
			disableSim900SleepMode();
			readSignalLevel();
			activateSim900SleepMode();
			CoLeaveMutexSection(usart2_mutex);
		}
		CoTimeDelay(0, 0, 30, 0); //delay for 30sec
	}
}

void taskLEDStatusControl(void* pdata) {
	for (;;) {
		if (ledStatus == LS_OK) {
			switchStatusLed(true);
			CoTickDelay(500);
			switchStatusLed(false);
			CoTickDelay(5000);
		} else if (ledStatus == LS_GSM_INIT_ERROR) {
			switchStatusLed(true);
			CoTickDelay(500);
			switchStatusLed(false);
			CoTickDelay(500);
		} else if (ledStatus == LS_NET_ERROR) {
			switchStatusLed(true);
			CoTickDelay(500);
			switchStatusLed(false);
			CoTickDelay(1000);
		} else if (ledStatus == LS_OTHER_ERROR) {
			switchStatusLed(true);
			CoTickDelay(1000);
			switchStatusLed(false);
			CoTickDelay(500);
		} else {
			CoTickDelay(1000);
		}
	}
}

int main(void) {
	NVIC_Configuration();
	GPIO_Configuration();
	USART2_Configuration();

	setAirLedPortAndPin(GPIOB, GPIO_Pin_1); //set AIR led port & pin

	ledStatus = LS_OK;
	switchStatusLed(true);

	CoInitOS();

	CoCreateTask(taskGSM, 0, 0, &task1_stk[TASK_STK_SIZE-1], TASK_STK_SIZE);
	CoCreateTask(taskLEDStatusControl, 0, 1, &task2_stk[TASK_STK_SIZE-1],
			TASK_STK_SIZE);

	CoStartOS();
}
