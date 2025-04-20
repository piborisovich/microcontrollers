/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : ds1307.h
  * @brief          : Header for ds1307.c file.
  *                   This file contains defines of the ds1307.
  ******************************************************************************
  ******************************************************************************
*/
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DS1307_H
#define __DS1307_H

#include "stm32f4xx_hal.h"
#include "global.h"

#ifdef __cplusplus
extern "C" {
#endif

#define READ_DS1307_DATA_PERIOD 100

void DS1307_init();
void DS1307_process();
void DS1307_get_datetime(DATE_TIME_BCD *datetime);
void DS1307_set_datetime(DATE_TIME_BCD *datetime);


#ifdef __cplusplus
}
#endif

#endif /* __DS1307_H */
