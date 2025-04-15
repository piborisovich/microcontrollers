/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : ds1307.c
  * @brief          : ds1307 definitions
  ******************************************************************************
*/

#include "ds1307.h"
#include "string.h"

enum DS1307_STATES
{
	STATE_NONE,
	STATE_DATA_REQUEST
};

static I2C_HandleTypeDef *p_hi2c1 = NULL;
static DS1307* p_ds1307 = NULL;

uint8_t state = STATE_NONE;

static uint8_t txBuffer[9];


void Error_Handler(void);

void get_date_time(DATE_TIME_BCD *date_time)
{
	*date_time = *((DATE_TIME_BCD*)p_ds1307);
}

void read_data(void)
{
	state = STATE_DATA_REQUEST;

	if ( HAL_I2C_Master_Transmit_IT( p_hi2c1, (uint16_t)DS1307_ADDRESS, (uint8_t*)&txBuffer, 1 ) != HAL_OK )
	{
		Error_Handler();
	}
}


void DS1307_init(I2C_HandleTypeDef *hi2c1, DS1307 *ds1307)
{
	p_hi2c1 = hi2c1;
	p_ds1307 = ds1307;

	p_ds1307->regs.get_date_time = get_date_time;
	p_ds1307->regs.read_data = read_data;

	p_ds1307->regs.reg0.CH = 0;

	memset( (void*)&txBuffer, 0, sizeof(txBuffer) );

	txBuffer[1] = p_ds1307->data[0];

	while ( HAL_I2C_Master_Transmit( p_hi2c1, (uint16_t)DS1307_ADDRESS, (uint8_t*)&txBuffer, 2, 10000) != HAL_OK )
	{
		/* Error_Handler() function is called when Timeout error occurs.
	       When Acknowledge failure occurs (Slave don't acknowledge it's address)
	       Master restarts communication */
		if ( HAL_I2C_GetError(p_hi2c1) != HAL_I2C_ERROR_AF)
		{
			Error_Handler();
		}
	}
}


void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	if ( state == STATE_DATA_REQUEST ) {
		while ( HAL_I2C_GetState(p_hi2c1) != HAL_I2C_STATE_READY );

		if( HAL_I2C_Master_Receive_IT( p_hi2c1, (uint16_t)DS1307_ADDRESS, p_ds1307->data, 8 ) != HAL_OK )
		{

			Error_Handler();
		}
	}
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	state = STATE_NONE;
	while ( HAL_I2C_GetState(p_hi2c1) != HAL_I2C_STATE_READY );
}
