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

typedef struct _SerialFlags
{
	uint8_t rx_flag      : 1;
	uint8_t rx_cplt_flag : 1;
	uint8_t empty        : 6;
} SerialFlags;

void serial_init( void (*dateTimeReceivedCallback)(DATE_TIME_BCD *date_time) );
void serial_process();
void serial_print_date_time( DATE_TIME_BCD *date_time );
void serial_print( const char *data );

#ifdef __cplusplus
}
#endif

#endif /* __SERIAL_H */
