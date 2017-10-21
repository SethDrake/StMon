#include "stm32f10x.h"

void log_message(const char *message)
{
	SH_SendString(message);
	SH_SendString("\r\n");
}
