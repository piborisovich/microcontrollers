/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : serial.c
  * @brief          : serial definitions
  ******************************************************************************
*/

#include "serial.h"

#include "string.h"

#include "stdio.h"

/* Size of Transmission buffer */
#define TXBUFFERSIZE 256

static UART_HandleTypeDef *p_huart = NULL;

/* Buffer used for transmission */
static volatile uint8_t pTxBuffer[TXBUFFERSIZE];

void Error_Handler(void);

void print(DATE_TIME_BCD *date_time)
{
	sprintf( (char*) (pTxBuffer),
			"%i%i.%i%i.%i%i %i%i:%i%i:%i%i\n",
			date_time->year.year_tens, date_time->year.year,
			date_time->month.month_tens, date_time->month.month,
			date_time->date.date_tens, date_time->date.date,
			date_time->hours.hours_tens, date_time->hours.hours,
			date_time->minutes.minutes_tens, date_time->minutes.minutes,
			date_time->seconds.seconds_tens, date_time->seconds.seconds );

	if ( HAL_UART_Transmit_IT(p_huart, (uint8_t*)pTxBuffer, strlen( (char*)pTxBuffer) ) != HAL_OK )
	{
		Error_Handler();
	}
}

void serial_init( UART_HandleTypeDef *huart, Serial* serial )
{
	p_huart = huart;
	serial->printDateTime = print;
}
