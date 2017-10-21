/**************************************************************************//**
 * @file     sim900.h
 * @brief    SimCom SIM900 driver Header File
 * @version  V1.1
 * @date     20.04.2014
 *
 * @note
 * Copyright (C) 2014 Serg Sizov
 *
 ******************************************************************************/

#include <stdint.h>
#include "stm32f10x.h"
#include "stm32f10x_usart.h"

#ifndef __SIM900_H__
#define __SIM900_H__

#ifdef __cplusplus
 extern "C" {
#endif

#define sim900USART USART2

#define RX_BUFF_SIZE 512
char sim900_rxbuff[RX_BUFF_SIZE];

GPIO_TypeDef *activityLedPort;
uint16_t activityLedPin;


#define ERROR_STRING "ERROR"

enum RCV_STATUS {
	RS_OK = 0,
	RS_TIMEOUT = 1,
	RS_ERROR = 2,
	RS_BUFFER_OVERFLOW = 3
};

struct GSMState
{
	uint8_t isOnline;
	uint8_t isSleeping;
	int csq;
    char *operator;
    char *imei;
} currentGSMState;

extern void setAirLedPortAndPin(GPIO_TypeDef *activityLedPort, uint16_t activityLedPin);

extern void switchActivityLed(uint8_t state);

extern uint8_t sendCommand(const char *command);

extern uint8_t sendCommandWithWaitReceive(const char *command, const char *respStr, uint32_t timeout);

extern uint8_t sendCommandWithRepeat(const char *command, uint16_t count);

extern uint8_t waitReceive(const char *respStr, uint32_t timeout);

extern char * getReceivedData();

extern void identifyModule();

extern void readCSQ();

#endif // __SIM900_H__
