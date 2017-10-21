#pragma once
#ifndef __OBJECTS_H_
#define __OBJECTS_H_
//#include "drivers/inc/ssd1306.h"


template<class ForwardIt>
	ForwardIt max_element(ForwardIt first, ForwardIt last)
	{
		if (first == last) {
			return last;
		}
		ForwardIt largest = first;
		++first;
		for (; first != last; ++first) {
			if (*largest < *first) {
				largest = first;
			}
		}
		return largest;
	}

typedef enum
{ 
	LOADING = 0,
	IDLE,
	ACTIVE,
	SLEEP
} SYSTEM_MODE;


extern SYSTEM_MODE systemMode;

extern DMA_HandleTypeDef i2cDmaTx;

/*extern SSD1306 display;*/


#endif //__OBJECTS_H_