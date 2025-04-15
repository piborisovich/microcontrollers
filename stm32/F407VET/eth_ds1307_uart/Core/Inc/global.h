/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GLOBAL_H
#define __GLOBAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

typedef struct _SECONDS_BCD
{
	uint8_t seconds      : 4;
	uint8_t seconds_tens : 3;
	uint8_t empty        : 1;
} SECONDS_BCD;

typedef struct _MINUTES_BCD
{
	uint8_t minutes      : 4;
	uint8_t minutes_tens : 3;
	uint8_t empty        : 1;
} MINUTES_BCD;

typedef struct _HOURS_BCD
{
	uint8_t hours      : 4;
	uint8_t hours_tens : 2;
	uint8_t empty      : 2;
} HOURS_BCD;

typedef struct _DAY_BCD
{
	uint8_t day   : 3;
	uint8_t empty : 5;
} DAY_BCD;

typedef struct _DATE_BCD
{
	uint8_t date      : 4;
	uint8_t date_tens : 2;
	uint8_t empty     : 2;
} DATE_BCD;

typedef struct _MONTH_BCD
{
	uint8_t month      : 4;
	uint8_t month_tens : 1;
	uint8_t empty      : 3;
} MONTH_BCD;

typedef struct _YEAR_BCD
{
	uint8_t year      : 4;
	uint8_t year_tens : 4;
} YEAR_BCD;


/* USER CODE BEGIN PTD */

typedef struct _DATE_TIME_BCD
{
	SECONDS_BCD seconds;
	MINUTES_BCD minutes;
	HOURS_BCD   hours;
	DAY_BCD     day;
	DATE_BCD    date;
	MONTH_BCD   month;
	YEAR_BCD    year;
} DATE_TIME_BCD;


#ifdef __cplusplus
}
#endif

#endif /* __GLOBAL_H */
