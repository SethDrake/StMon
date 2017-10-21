#include <sys/_stdint.h>

#include <stm32f1xx_hal.h>

#include "cmsis_os.h"

//#include "drivers/inc/ssd1306.h"
//#include "src/drivers/inc/fonts.h"
#include "periph_config.h"
#include "objects.h"
#include "delay.h"
#include <cerrno>
#include "stm32f1xx_ll_i2c.h"

DMA_HandleTypeDef i2cDmaTx;
UART_HandleTypeDef uart;
I2C_HandleTypeDef i2c;

/*SSD1306 display;*/

SYSTEM_MODE systemMode;
bool gsmIsActive;

osThreadId statusTaskHandle;
osThreadId listenModemTaskHandle;

void switchSystemMode(SYSTEM_MODE mode);

void switchGSM(bool enable);

void errorHandler(void);

void DBG_Configuration()
{
	//Enable debug in powersafe modes
	HAL_DBGMCU_EnableDBGSleepMode();
	HAL_DBGMCU_EnableDBGStopMode();
	HAL_DBGMCU_EnableDBGStandbyMode();

	//hard fault on div by zero
	SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;
}

void SystemClock_Configuration()
{
	
	/*RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
	RCC_OscInitStruct.PLL.PLLState    = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource   = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL      = RCC_PLL_MUL9;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		while (true); //error on init clock
	}*/

	/*RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
	{
		errorHandler(); //error on init clock
	}*/

	RCC_PeriphCLKInitTypeDef PeriphClkInit;
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
	PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
	{
		errorHandler(); //error on init clock
	}

	/**Configure the Systick interrupt time */
	HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);
	/**Configure the Systick */
	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

	SystemCoreClockUpdate();
}

void RCC_Configuration()
{

#if (PREFETCH_ENABLE != 0)
	__HAL_FLASH_PREFETCH_BUFFER_ENABLE();
#endif
	
	__HAL_RCC_BKP_CLK_ENABLE();
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_I2C1_CLK_ENABLE();
	__HAL_RCC_DMA1_CLK_ENABLE();
	__HAL_RCC_USART2_CLK_ENABLE();
	__HAL_RCC_AFIO_CLK_ENABLE();
}

void I2C_Configuration(I2C_HandleTypeDef* i2cHandle)
{
	i2cHandle->Instance             = I2C1;
	i2cHandle->Init.ClockSpeed      = LL_I2C_MAX_SPEED_FAST; //400000
	i2cHandle->Init.DutyCycle       = I2C_DUTYCYCLE_2;
	i2cHandle->Init.OwnAddress1     = 0x00;
	i2cHandle->Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
	i2cHandle->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	i2cHandle->Init.OwnAddress2     = 0x00;
	i2cHandle->Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	i2cHandle->Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(i2cHandle) != HAL_OK)
	{
		errorHandler();
	}
	__HAL_I2C_ENABLE(i2cHandle);
}

void USART_Configuration(UART_HandleTypeDef* usartHandle)
{
	usartHandle->Instance			= USART2;
	usartHandle->Init.BaudRate		= 115200;
	usartHandle->Init.WordLength	= UART_WORDLENGTH_8B;
	usartHandle->Init.StopBits		= UART_STOPBITS_1;
	usartHandle->Init.Parity		= UART_PARITY_NONE;
	usartHandle->Init.HwFlowCtl		= UART_HWCONTROL_NONE;
	usartHandle->Init.Mode			= UART_MODE_TX_RX;
	usartHandle->Init.OverSampling  = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(usartHandle) != HAL_OK)
	{
		errorHandler();
	}
	__HAL_UART_ENABLE(usartHandle);
}

void DMA_I2C_TX_Configuration(DMA_HandleTypeDef* dmaHandle)
{
	dmaHandle->Instance                 = DMA1_Channel6;
	dmaHandle->Init.Direction           = DMA_MEMORY_TO_PERIPH;
	dmaHandle->Init.PeriphInc           = DMA_PINC_DISABLE;
	dmaHandle->Init.MemInc              = DMA_MINC_ENABLE;
	dmaHandle->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	dmaHandle->Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
	dmaHandle->Init.Mode                = DMA_NORMAL;
	dmaHandle->Init.Priority            = DMA_PRIORITY_VERY_HIGH;
	if (HAL_DMA_Init(dmaHandle) != HAL_OK)
	{
		errorHandler();
	}
}

void GPIO_Configuration(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;
		/* Configure I2C pins: SCL and SDA */
	GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Pin = I2C_SCL | I2C_SDA;
	HAL_GPIO_Init(I2C_PORT, &GPIO_InitStruct);

		/* USART2 PINS */
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Pin = USART_RX | USART_TX;
	HAL_GPIO_Init(USART_PORT, &GPIO_InitStruct);

		/* Configure LED pins */
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	/*GPIO_InitStruct.Pull = GPIO_PULLUP;*/
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Pin = LED_STAT_PIN | LED_AIR_PIN;
	HAL_GPIO_Init(LEDS_PORT, &GPIO_InitStruct);
	
	/* Configure GSM PWR pins */
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Pin = GSM_PWR_PIN;
	HAL_GPIO_Init(GSM_PWR_PORT, &GPIO_InitStruct);

	__HAL_AFIO_REMAP_SWJ_NOJTAG(); //disable JTAG
}

void EXTI_Configuration()
{

}

void NVIC_Configuration(void) {
	HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /* System interrupt init*/
  /* MemoryManagement_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(MemoryManagement_IRQn, 0, 0);
	/* BusFault_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(BusFault_IRQn, 0, 0);
	/* UsageFault_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(UsageFault_IRQn, 0, 0);
	/* SVCall_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(SVCall_IRQn, 0, 0);
	/* DebugMonitor_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DebugMonitor_IRQn, 0, 0);
	/* PendSV_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);
	/* SysTick_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(SysTick_IRQn, 14, 0);

	HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 15, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);
}


void switchSystemMode(SYSTEM_MODE mode)
{
	if (mode == systemMode)
	{
		return;	
	}
	systemMode = mode;
	if (mode == ACTIVE)
	{
		//enable GSM module
		switchGSM(true);
	}
	else if(mode == IDLE)
	{
		//disable GSM module
		switchGSM(true);
	}
	else if(mode == SLEEP)
	{
		//sleep controller
	}
}

void switchGSM(bool enable)
{
	if (gsmIsActive == enable)
	{
		return;
	}
	gsmIsActive = enable;
	if (gsmIsActive)
	{
		//switch on gsm
		HAL_GPIO_WritePin(GSM_PWR_PORT, GSM_PWR_PIN, GPIO_PIN_SET);
		HAL_GPIO_WritePin(LEDS_PORT, LED_AIR_PIN, GPIO_PIN_SET);
	}
	else 
	{
		//switch off gsm
		HAL_GPIO_WritePin(GSM_PWR_PORT, GSM_PWR_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LEDS_PORT, LED_AIR_PIN, GPIO_PIN_RESET);
	}
}


void statusTask(void const * argument)
{
	while (true)
	{
		if (systemMode == LOADING)
		{
			HAL_GPIO_WritePin(LEDS_PORT, LED_STAT_PIN, GPIO_PIN_SET);
			osDelay(2000);
			HAL_GPIO_WritePin(LEDS_PORT, LED_STAT_PIN, GPIO_PIN_RESET);
			osDelay(100);
			HAL_GPIO_WritePin(LEDS_PORT, LED_AIR_PIN, GPIO_PIN_SET);
			osDelay(200);
			HAL_GPIO_WritePin(LEDS_PORT, LED_AIR_PIN, GPIO_PIN_RESET);
			osDelay(150);
		}
		else if (systemMode == IDLE)
		{
			HAL_GPIO_WritePin(LEDS_PORT, LED_STAT_PIN, GPIO_PIN_SET);
			osDelay(1500);
			HAL_GPIO_WritePin(LEDS_PORT, LED_STAT_PIN, GPIO_PIN_RESET);
			osDelay(3000);
		}
		else if (systemMode == ACTIVE)
		{
			HAL_GPIO_WritePin(LEDS_PORT, LED_STAT_PIN, GPIO_PIN_SET);
			osDelay(500);
			HAL_GPIO_WritePin(LEDS_PORT, LED_STAT_PIN, GPIO_PIN_RESET);
			osDelay(500);
		}
		else if (systemMode == SLEEP)
		{
			HAL_GPIO_WritePin(LEDS_PORT, LED_STAT_PIN, GPIO_PIN_SET);
			osDelay(500);
			HAL_GPIO_WritePin(LEDS_PORT, LED_STAT_PIN, GPIO_PIN_RESET);
			osDelay(10000);
		}
	}
}

void listenModemTask(void const * argument)
{
	while (true)
	{
		if (systemMode == IDLE)
		{
			switchSystemMode(ACTIVE); //enable gsm
			
		}
		osDelay(60000);
	}
}


int main()
{
	DBG_Configuration();
	SystemClock_Configuration();
	DelayManager::DelayMs(150);
	RCC_Configuration();
	GPIO_Configuration();
	EXTI_Configuration();
	NVIC_Configuration();
	I2C_Configuration(&i2c);
	USART_Configuration(&uart);
	DMA_I2C_TX_Configuration(&i2cDmaTx);

	systemMode = LOADING;

	/*display.initDisplay(&i2c);
	display.setFont(font5x7);
	display.clearScreen();
	display.printf(12, 50, logoStr);
	display.drawLine(0, 44, 127, 44);
	display.printf(12, 15, ".... LOADING ....");
	display.drawFramebuffer();*/

	osThreadDef(statusThread, statusTask, osPriorityNormal, 0, 128);
	osThreadDef(listenModemThread, listenModemTask, osPriorityNormal, 0, 256);

	statusTaskHandle = osThreadCreate(osThread(statusThread), NULL);
	listenModemTaskHandle = osThreadCreate(osThread(listenModemThread), NULL);
	
	osKernelStart();
	
	switchSystemMode(IDLE);

	while (true)
	{
	}
}

void errorHandler()
{
	/*if (display.IsActive() && reason != NULL)
	{
		display.printf(0, 56, "SYSTEM ERROR!");
		display.printf(0, 40, "%s", *reason);
	}*/

	while (true)
	{
		HAL_GPIO_WritePin(LEDS_PORT, LED_STAT_PIN, GPIO_PIN_SET);
		HAL_GPIO_WritePin(LEDS_PORT, LED_AIR_PIN, GPIO_PIN_RESET);
		DelayManager::DelayMs(300);
		HAL_GPIO_WritePin(LEDS_PORT, LED_STAT_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LEDS_PORT, LED_AIR_PIN, GPIO_PIN_SET);
		DelayManager::DelayMs(300);
		HAL_GPIO_WritePin(LEDS_PORT, LED_STAT_PIN, GPIO_PIN_SET);
		HAL_GPIO_WritePin(LEDS_PORT, LED_AIR_PIN, GPIO_PIN_RESET);
		DelayManager::DelayMs(300);
		HAL_GPIO_WritePin(LEDS_PORT, LED_STAT_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LEDS_PORT, LED_AIR_PIN, GPIO_PIN_SET);
		DelayManager::DelayMs(300);
		HAL_GPIO_WritePin(LEDS_PORT, LED_STAT_PIN, GPIO_PIN_SET);
		HAL_GPIO_WritePin(LEDS_PORT, LED_AIR_PIN, GPIO_PIN_RESET);
		DelayManager::DelayMs(300);
		HAL_GPIO_WritePin(LEDS_PORT, LED_STAT_PIN, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LEDS_PORT, LED_AIR_PIN, GPIO_PIN_SET);
		DelayManager::DelayMs(500);
		HAL_GPIO_WritePin(LEDS_PORT, LED_STAT_PIN, GPIO_PIN_SET);
		HAL_GPIO_WritePin(LEDS_PORT, LED_AIR_PIN, GPIO_PIN_RESET);
		DelayManager::DelayMs(1000);
		HAL_GPIO_WritePin(LEDS_PORT, LED_STAT_PIN, GPIO_PIN_RESET);
		DelayManager::DelayMs(500);
	}
}
