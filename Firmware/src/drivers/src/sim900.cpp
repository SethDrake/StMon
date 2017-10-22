/******************************************************************************
 * @file     sim900.c
 * @brief    SimCom SIM900 driver File
 * @version  V1.1
 * @date     21.10.2017
 *
 * @note
 * Copyright (C) 2017 Serg Sizov
 *
 ******************************************************************************/
#include "src/drivers/inc/sim900.h"
#include <string.h>
#include <algorithm>
#include "delay.h"


SIM900::SIM900() {
	this->buf_cnt = 0;
	this->isOnline = false;
	this->isSleep = false;
	this->usart = NULL;
	std::fill_n(this->csq, 6, 0x00);
	std::fill_n(this->op, 16, 0x00);
	std::fill_n(this->imei, 32, 0x00);
}

SIM900::~SIM900() {
}


bool SIM900::initModule(UART_HandleTypeDef* usart)
{
	this->usart = usart;
	sendCommand("AT");
	waitReceive("OK", 10000);
	if (sendCommandWithWaitReceive("AT+CSCLK=0", "OK", 5000) != RS_OK)
	{
		return this->isOnline;		
	}
	if (sendCommandWithWaitReceive("AT", "OK", 5000) != RS_OK) {
		return this->isOnline;
	}
	sendCommand("ATE0");
	DelayManager::DelayMs(20);
	sendCommand("AT+GSMBUSY=1");
	DelayManager::DelayMs(20);
	this->isOnline = true;
	return this->isOnline;
}

void SIM900::deinitModule()
{	
	this->isOnline = false;
}

void SIM900::sleepMode(bool enabled)
{
	if (this->isSleep != enabled)
	{
		this->isSleep = enabled;	
	}
}

bool SIM900::IsSleep()
{
	return this->isSleep;
}

bool SIM900::IsOnline()
{
	return this->isOnline;
}

char* SIM900::getReceivedData()
{
}

uint8_t SIM900::sendCommand(char *command) {
	uint16_t len = strlen(command);
	memset(sim900_txbuf, 0x00, TX_BUFF_SIZE);
	memcpy(sim900_txbuf, command, len);
	sim900_txbuf[len] = '\r';
	HAL_StatusTypeDef result = HAL_UART_Transmit_DMA(usart, sim900_txbuf, len+1);
	return result == HAL_OK;
}

uint8_t SIM900::waitReceive(char *respStr, uint32_t timeout)
{
	memset(sim900_rxbuf, 0x00, RX_BUFF_SIZE);
	uint16_t size = strlen(respStr);
	HAL_StatusTypeDef status = HAL_UART_Receive(usart, (uint8_t*)&sim900_rxbuf, size, timeout);
	if (status == HAL_OK)
	{
		if (strcmp(respStr, sim900_rxbuf) == 0)
		{
			return RS_OK;	
		}	
	}
	return RS_ERROR;
}

uint8_t SIM900::sendCommandWithWaitReceive(char *command, char *respStr, uint32_t timeout) {
	sendCommand(command);
	return waitReceive(respStr, timeout);
}


void SIM900::identifyModule() {
	std::fill_n(imei, 32, 0x00);
	if (sendCommandWithWaitReceive("AT+GSN", "OK", 1000) == RS_OK) {
		strcpy(imei, strtok(getReceivedData(), "\r\n"));
	}
}

void SIM900::readCSQ() {
	std::fill_n(csq, 16, 0x00);
	if (sendCommandWithWaitReceive("AT+CSQ", "OK", 1000) == RS_OK) {
		char *csqStr = strtok(getReceivedData(), "\r\n");
//		char *csq = strrchr(' ', csqStr);
//		strcpy(this->csq = atoi(strtok(csq, ",")));
	}
}

void SIM900::USART_SendBlock(uint8_t* data, uint8_t size) 
{
	//HAL_UART_Transmit(usart, data, size, 5000);
}

uint8_t SIM900::USART_ReadByte(bool* isOk, uint16_t timeout) 
{
	volatile uint32_t nCount;
	
	*isOk = true;
	nCount = ((uint64_t)HAL_RCC_GetHCLKFreq() * timeout / 10000);
	if (!(usart->RxState == HAL_UART_STATE_READY) && !(usart->RxState == HAL_UART_STATE_BUSY_TX))
	{
		*isOk = false;
		return 0x00;
	}
	if (usart->Lock == HAL_LOCKED)
	{
		*isOk = false;
		return 0x00;
	}
	__HAL_LOCK(usart);
	usart->ErrorCode = HAL_UART_ERROR_NONE;
	if (usart->RxState == HAL_UART_STATE_BUSY_TX)
	{
		usart->RxState = HAL_UART_STATE_BUSY_TX_RX;
	}
	else
	{
		usart->RxState = HAL_UART_STATE_BUSY_RX;
	}
	while (!(usart->Instance->SR & USART_SR_RXNE))
	{
		nCount--;
		if (nCount == 0) //break if await longer timeout in ms
		{
			*isOk = false;
			break;
		}
	}
	uint8_t result = usart->Instance->DR;
	if (usart->RxState == HAL_UART_STATE_BUSY_TX_RX) 
	{
		usart->RxState = HAL_UART_STATE_BUSY_TX;
	}
	else
	{
		usart->RxState = HAL_UART_STATE_READY;
	}
	__HAL_UNLOCK(usart);
	return result;
}

uint8_t SIM900::USART_ReadBlock(uint8_t* data, uint8_t size, uint16_t timeout) 
{
	uint8_t cnt = 0;
	bool isOk = true;
	for (int i = 0; i < size; i++)
	{
		data[i] = USART_ReadByte(&isOk, timeout);
		if (!isOk)
		{
			return cnt;
		}
		cnt++;
	}
	return cnt;
}

bool SIM900::USART_ReadFixedBlock(uint8_t* data, uint8_t size, uint16_t timeout) 
{
	return HAL_UART_Receive(usart, data, size, timeout) == HAL_OK;
}

