/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : ds1307.c
  * @brief          : ds1307 definitions
  ******************************************************************************
*/

#include "ds1307.h"
#include "string.h"

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
} DS1307TypeDef;

typedef union _DS1307
{
	uint8_t data[8];
	DS1307TypeDef regs;

} DS1307;

enum DS1307_STATES
{
	STATE_FREE,
	STATE_DATA_WRITE,
	STATE_DATA_REQUEST
};


I2C_HandleTypeDef hi2c1;

static uint32_t ds307_read_data_period;
static DS1307 ds1307 = {};

static uint8_t state = STATE_FREE;

static uint8_t txBuffer[9];

static void MX_I2C1_Init(void);
void Error_Handler(void);

void DS1307_init()
{
	MX_I2C1_Init();

	memset( (void*)&txBuffer, 0, sizeof(txBuffer) );

	while ( HAL_I2C_Master_Transmit( &hi2c1, (uint16_t)DS1307_ADDRESS, (uint8_t*)&txBuffer, 1, 10000) != HAL_OK )
	{
		/* Error_Handler() function is called when Timeout error occurs.
		       When Acknowledge failure occurs (Slave don't acknowledge it's address)
		       Master restarts communication */
		if ( HAL_I2C_GetError( &hi2c1 ) != HAL_I2C_ERROR_AF)
		{
			Error_Handler();
		}
	}

	while ( HAL_I2C_Master_Receive( &hi2c1, (uint16_t)DS1307_ADDRESS, (uint8_t*)(&ds1307.data), 8, 10000) != HAL_OK )
	{
		/* Error_Handler() function is called when Timeout error occurs.
			       When Acknowledge failure occurs (Slave don't acknowledge it's address)
			       Master restarts communication */
		if ( HAL_I2C_GetError( &hi2c1 ) != HAL_I2C_ERROR_AF)
		{
			Error_Handler();
		}
	}

	ds1307.regs.reg0.CH = 0;

	txBuffer[1] = ds1307.data[0];

	while ( HAL_I2C_Master_Transmit( &hi2c1, (uint16_t)DS1307_ADDRESS, (uint8_t*)&txBuffer, 2, 10000) != HAL_OK )
	{
		/* Error_Handler() function is called when Timeout error occurs.
			       When Acknowledge failure occurs (Slave don't acknowledge it's address)
			       Master restarts communication */
		if ( HAL_I2C_GetError( &hi2c1 ) != HAL_I2C_ERROR_AF)
		{
			Error_Handler();
		}
	}


}

void DS1307_process()
{
	if ( HAL_GetTick() - ds307_read_data_period > READ_DS1307_DATA_PERIOD  ) {
		if ( state == STATE_FREE ) {

			ds307_read_data_period = HAL_GetTick();

			state = STATE_DATA_REQUEST;

			if ( HAL_I2C_Master_Transmit_IT( &hi2c1, (uint16_t)DS1307_ADDRESS, (uint8_t*)&txBuffer, 1 ) != HAL_OK )
			{
				Error_Handler();
			}
		}
	}
}

void DS1307_get_datetime(DATE_TIME_BCD *date_time)
{
	*date_time = *((DATE_TIME_BCD*)&ds1307);
}

void DS1307_set_datetime(DATE_TIME_BCD *datetime)
{
	(*((DATE_TIME_BCD*)&ds1307)) = *datetime;

	ds1307.regs.reg0.CH = 0;

	while ( HAL_I2C_GetState(&hi2c1) == HAL_I2C_STATE_BUSY );

	txBuffer[0] = 0;
	memcpy( (void*)(txBuffer + 1), (void*)ds1307.data, 7);

	state = STATE_DATA_WRITE;

	if ( HAL_I2C_Master_Transmit_IT( &hi2c1, (uint16_t)DS1307_ADDRESS, (uint8_t*)&txBuffer, 8 ) != HAL_OK )
	{
		Error_Handler();
	}
}

static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */
}

/**
* @brief I2C MSP Initialization
* This function configures the hardware resources used in this example
* @param hi2c: I2C handle pointer
* @retval None
*/
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(hi2c->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspInit 0 */

  /* USER CODE END I2C1_MspInit 0 */

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Peripheral clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();
    /* I2C1 interrupt Init */
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_SetPriority(I2C1_ER_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
  /* USER CODE BEGIN I2C1_MspInit 1 */

  /* USER CODE END I2C1_MspInit 1 */

  }

}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	if ( state == STATE_DATA_REQUEST ) {
		while ( HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY );

		if( HAL_I2C_Master_Receive_IT( &hi2c1, (uint16_t)DS1307_ADDRESS, ds1307.data, 8 ) != HAL_OK )
		{
			Error_Handler();
		}
	} else {
		state = STATE_FREE;
	}
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	while ( HAL_I2C_GetState( &hi2c1 ) != HAL_I2C_STATE_READY );
	state = STATE_FREE;
}
