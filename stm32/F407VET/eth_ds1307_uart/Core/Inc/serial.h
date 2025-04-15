/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : serial.h
  * @brief          : Header for serial.c file.
  *                   This file contains defines of serial data read/write.
  ******************************************************************************
*/
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SERIAL_H
#define __SERIAL_H

#include "global.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Serial
{
	void (*printDateTime)(DATE_TIME_BCD *date_time);
} Serial;

void serial_init( UART_HandleTypeDef *huart, Serial* serial );


#ifdef __cplusplus
}
#endif

#endif /* __SERIAL_H */
