/**************************************************************************//**
 * @file     sim900.c
 * @brief    SimCom SIM900 driver File
 * @version  V1.1
 * @date     20.04.2014
 *
 * @note
 * Copyright (C) 2014 Serg Sizov
 *
 ******************************************************************************/

#include <sim900.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <CoOS.h>
#include "stm32f10x.h"
#include "stm32f10x_usart.h"

uint16_t bufCntr = 0;

void setAirLedPortAndPin(GPIO_TypeDef *ledPort, uint16_t ledPin) {
	activityLedPort = ledPort;
	activityLedPin = ledPin;
}

void switchActivityLed(uint8_t state) {
	GPIO_WriteBit(activityLedPort, activityLedPin, state);
}

void sendChar(uint8_t c)
{
	while (USART_GetFlagStatus(sim900USART, USART_FLAG_TC) == RESET);
	USART_SendData(sim900USART, (uint8_t) c); /* Loop until the end of transmission */
}

void sendString(const char *str)
{
	uint16_t i = 0;
	while(str[i]) {
		sendChar(str[i++]);
	}
}

bool isBufferNotEmpty() {
	return bufCntr > 0;
}

uint8_t sendCommand(const char *command) {
	switchActivityLed(true);
	sendString(command);
	sendString("\r");
	switchActivityLed(false);
	return 0;
}

uint8_t sendCommandWithRepeat(const char *command, uint16_t count) {
	uint16_t i;
	for(i = 0; i < count; i++) {
		sendCommand(command);
		CoTickDelay(10);
	}
}

uint8_t sendCommandWithWaitReceive(const char *command, const char *respStr, uint32_t timeout) {
	sendCommand(command);
	return waitReceive(respStr, timeout);
}

void USART2_IRQHandler(void) {
	CoEnterISR();
	if (USART_GetITStatus(sim900USART, USART_IT_RXNE)) {
		if (bufCntr >= RX_BUFF_SIZE) {
			bufCntr = 0;
		}
		sim900_rxbuff[bufCntr++] = USART_ReceiveData(sim900USART) & 0x7F;
  	}
	CoExitISR();
}


void preReceive() {
	switchActivityLed(true);
	memset(sim900_rxbuff, '\0', RX_BUFF_SIZE);
	bufCntr = 0;
	USART_ITConfig(sim900USART, USART_IT_RXNE, ENABLE);
}

void postReceive() {
	USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
	switchActivityLed(false);
}

uint8_t waitReceive(const char *respStr, uint32_t timeout) {
	char *p = 0;
	char ch;
	uint16_t i;
	U64 startTick, currentTick, elapsedTicks, timeoutTick;

	timeoutTick = (timeout * CFG_SYSTICK_FREQ) / 1000;
	i = 0;
	startTick = CoGetOSTime();

	preReceive();
	do {
		if (isBufferNotEmpty()){
	        if (respStr) {
	        	p = strstr(sim900_rxbuff, ERROR_STRING);
	        	if(p > 0) {
	        		postReceive();
	        		return RS_ERROR; // error received
	        	}
	        	p = strstr(sim900_rxbuff, respStr);
	        	if(p > 0)  {
	        		postReceive();
	        		return RS_OK; // string to be checked was found
	        	}
	        }
	    }
		CoTickDelay(1);
		currentTick = CoGetOSTime();
		elapsedTicks = currentTick - startTick;
	} while (elapsedTicks < timeoutTick);

	postReceive();
	if (i < (RX_BUFF_SIZE - 1)) {
		return RS_TIMEOUT; // timeout
	}
	return RS_BUFFER_OVERFLOW; // rx buffer full
}

char * getReceivedData() {
	return sim900_rxbuff;
}

void identifyModule() {
	currentGSMState.imei = NULL;
	if (sendCommandWithWaitReceive("AT+GSN", "OK", 200) == RS_OK) {
		currentGSMState.imei = strtok(getReceivedData(), "\r\n");
	}
}

void readCSQ() {
	currentGSMState.csq = 0;
	if (sendCommandWithWaitReceive("AT+CSQ", "OK", 200) == RS_OK) {
		char *csqStr = strtok(getReceivedData(), "\r\n");
		char *csq = strrchr(' ', csqStr);
		currentGSMState.csq = atoi(strtok(csq, ","));
	}
	return (NULL);
}
