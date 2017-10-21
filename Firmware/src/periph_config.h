
#pragma once
#ifndef __PERIPH_CONFIG_H_
#define __PERIPH_CONFIG_H_

/* PA13, PA14 - SWD - DON'T USE!!! */
/* PA11, PA12 - USB */

#define I2C_PORT		GPIOB
#define I2C_SCL			GPIO_PIN_6
#define I2C_SDA			GPIO_PIN_7

#define LEDS_PORT		GPIOB
#define LED_STAT_PIN	GPIO_PIN_0
#define LED_AIR_PIN		GPIO_PIN_1

#define USART_PORT		GPIOA
#define USART_TX	    GPIO_PIN_2
#define USART_RX		GPIO_PIN_3

#define BAT_CHECK_PORT	GPIOA
#define BAT_CHECK_PIN	GPIO_PIN_0

#define GSM_PWR_PORT	GPIOA
#define GSM_PWR_PIN		GPIO_PIN_1

#endif //__PERIPH_CONFIG_H_
