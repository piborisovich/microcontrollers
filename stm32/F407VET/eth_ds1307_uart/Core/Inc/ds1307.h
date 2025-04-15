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

#define DS1307_ADDRESS 0b11010000

typedef struct _DS1307REG0
{
	uint8_t seconds      : 4;
	uint8_t seconds_tens : 3;
	uint8_t CH           : 1;
} DS1307REG0;

typedef struct _DS1307REG1
{
	uint8_t minutes      : 4;
	uint8_t minutes_tens : 3;
	uint8_t bit7         : 1;
} DS1307REG1;

typedef struct _DS1307REG2
{
	uint8_t hours      : 4;
	uint8_t hours_tens : 2;
	uint8_t mode12_24  : 1;
	uint8_t bit7       : 1;
} DS1307REG2;

typedef struct _DS1307REG3
{
	uint8_t day   : 3;
	uint8_t bits3 : 1;
	uint8_t bits4 : 1;
	uint8_t bits5 : 1;
	uint8_t bits6 : 1;
	uint8_t bits7 : 1;
} DS1307REG3;

typedef struct _DS1307REG4
{
	uint8_t date      : 4;
	uint8_t date_tens : 2;
	uint8_t bit6      : 1;
	uint8_t bit7      : 1;
} DS1307REG4;

typedef struct _DS1307REG5
{
	uint8_t month      : 4;
	uint8_t month_tens : 1;
	uint8_t bits5      : 1;
	uint8_t bits6      : 1;
	uint8_t bits7      : 1;
} DS1307REG5;

typedef struct _DS1307REG6
{
	uint8_t year      : 4;
	uint8_t year_tens : 4;
} DS1307REG6;

typedef struct _DS1307REG7
{
	uint8_t RS0  : 1;
	uint8_t RS1  : 1;
	uint8_t bit2 : 1;
	uint8_t bit3 : 1;
	uint8_t SQWE : 1;
	uint8_t bit5 : 1;
	uint8_t bit6 : 1;
	uint8_t OUT  : 1;
} DS1307REG7;


/* USER CODE BEGIN PTD */

typedef struct _DS1307TypeDef
{
	DS1307REG0 reg0;
	DS1307REG1 reg1;
	DS1307REG2 reg2;
	DS1307REG3 reg3;
	DS1307REG4 reg4;
	DS1307REG5 reg5;
	DS1307REG6 reg6;
	DS1307REG7 reg7;
	void (*get_date_time) (DATE_TIME_BCD *date_time);
	void (*read_data) ();
} DS1307TypeDef;

typedef union _DS1307
{
	uint8_t data[8];
	DS1307TypeDef regs;

} DS1307;


void DS1307_init(I2C_HandleTypeDef *hi2c1, DS1307 *ds1307);


#ifdef __cplusplus
}
#endif

#endif /* __DS1307_H */
