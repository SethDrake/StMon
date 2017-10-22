/******************************************************************************
 * @file     sim900.h
 * @brief    SimCom SIM900 driver Header File
 * @version  V1.1
 * @date     21.10.2017
 *
 * @note
 * Copyright (C) 2017 Serg Sizov
 *
 ******************************************************************************/
#pragma once
#ifndef __SIM900_H
#define __SIM900_H

#include <sys/_stdint.h>
#include <stm32f1xx_hal.h>

#define RX_BUFF_SIZE 512
#define TX_BUFF_SIZE 64
#define ERROR_STRING "ERROR"

enum RCV_STATUS {
	RS_OK              = 0,
	RS_TIMEOUT         = 1,
	RS_ERROR           = 2,
	RS_BUFFER_OVERFLOW = 3
};

class SIM900 {
public:
	SIM900();
	~SIM900();
	bool initModule(UART_HandleTypeDef* usart);
	void deinitModule();
	void sleepMode(bool enabled);
	bool IsSleep(void);
	bool IsOnline(void);
	uint8_t sendCommand(char *command);
	uint8_t waitReceive(char *respStr, uint32_t timeout);
	uint8_t sendCommandWithWaitReceive(char *command, char *respStr, uint32_t timeout);
	char* getReceivedData();
	void identifyModule();
	void readCSQ();
protected:
	UART_HandleTypeDef* usart;
private:
	uint8_t isOnline;
	uint8_t isSleep;
	uint16_t buf_cnt;
	char sim900_rxbuf[RX_BUFF_SIZE];
	uint8_t sim900_txbuf[TX_BUFF_SIZE];
	char csq[6];
	char op[16];
	char imei[32];
	void USART_SendBlock(uint8_t* data, uint8_t size);
	uint8_t USART_ReadByte(bool* isOk, uint16_t timeout); 
	uint8_t USART_ReadBlock(uint8_t* data, uint8_t size, uint16_t timeout);
	bool USART_ReadFixedBlock(uint8_t* data, uint8_t size, uint16_t timeout); 
};

#endif // __SIM900_H