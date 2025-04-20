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
#define RXBUFFERSIZE 21
#define RX_TIMEOUT_MS 2000 //Сброс буфера приема по истечении этого времени

UART_HandleTypeDef huart1;

uint32_t rx_timer = 0;

SerialFlags serialFlags = {0};

/* Buffer used for transmission */
static volatile uint8_t pTxBuffer[TXBUFFERSIZE];
/* Buffer used for receive data */
static volatile uint8_t pRxBuffer[RXBUFFERSIZE];

void (*dateTimeReceived)(DATE_TIME_BCD *date_time) = NULL;

static const char *SET_COMMAND = "SET_DT";

static void MX_USART1_UART_Init(void);

void Error_Handler(void);

void serial_init( void (*dateTimeReceivedCallback)(DATE_TIME_BCD *date_time) )
{
	MX_USART1_UART_Init();

	HAL_UART_Receive_IT(&huart1, (uint8_t*)pRxBuffer, RXBUFFERSIZE);

	dateTimeReceived = dateTimeReceivedCallback;
}

void serial_process()
{
	uint8_t i = 0;
	uint8_t d = 0;
	DATE_TIME_BCD date_time;

	if ( serialFlags.rx_flag ) {
		serialFlags.rx_flag = 0;
		rx_timer = HAL_GetTick();
	}

	if ( HAL_GetTick() - rx_timer > RX_TIMEOUT_MS ) {
		rx_timer = HAL_GetTick();
		HAL_UART_AbortReceive_IT(&huart1);
		HAL_UART_Receive_IT(&huart1, (uint8_t*)pRxBuffer, RXBUFFERSIZE);
	}

	if ( serialFlags.rx_cplt_flag ) {
		serialFlags.rx_cplt_flag = 0;

		for ( i = 0; i < strlen(SET_COMMAND); ++i ) {
			if ( pRxBuffer[i] != SET_COMMAND[i] ) {
				return;
			}
		}

		if ( pRxBuffer[i++] != ' ') {
			return;
		}

		d = pRxBuffer[i++] - 0x30; if ( d >= 0 && d <= 9) date_time.year.year_tens = d;       else return;
		d = pRxBuffer[i++] - 0x30; if ( d >= 0 && d <= 9) date_time.year.year = d;            else return;
		d = pRxBuffer[i++] - 0x30; if ( d >= 0 && d <= 9) date_time.month.month_tens = d;     else return;
		d = pRxBuffer[i++] - 0x30; if ( d >= 0 && d <= 9) date_time.month.month = d;          else return;
		d = pRxBuffer[i++] - 0x30; if ( d >= 0 && d <= 9) date_time.date.date_tens = d;       else return;
		d = pRxBuffer[i++] - 0x30; if ( d >= 0 && d <= 9) date_time.date.date = d;            else return;
		d = pRxBuffer[i++] - 0x30; if ( d >= 0 && d <= 9) date_time.hours.hours_tens = d;     else return;
		d = pRxBuffer[i++] - 0x30; if ( d >= 0 && d <= 9) date_time.hours.hours = d;          else return;
		d = pRxBuffer[i++] - 0x30; if ( d >= 0 && d <= 9) date_time.minutes.minutes_tens = d; else return;
		d = pRxBuffer[i++] - 0x30; if ( d >= 0 && d <= 9) date_time.minutes.minutes = d;      else return;
		d = pRxBuffer[i++] - 0x30; if ( d >= 0 && d <= 9) date_time.seconds.seconds_tens = d; else return;
		d = pRxBuffer[i++] - 0x30; if ( d >= 0 && d <= 9) date_time.seconds.seconds = d;      else return;


		if ( pRxBuffer[i++] == '\r' && pRxBuffer[i] == '\n' ) {
			dateTimeReceived(&date_time);
		}
	}
}

void serial_print_date_time( DATE_TIME_BCD *date_time )
{
	sprintf( (char*) (pTxBuffer),
			"%i%i.%i%i.%i%i %i%i:%i%i:%i%i\n",
			date_time->year.year_tens, date_time->year.year,
			date_time->month.month_tens, date_time->month.month,
			date_time->date.date_tens, date_time->date.date,
			date_time->hours.hours_tens, date_time->hours.hours,
			date_time->minutes.minutes_tens, date_time->minutes.minutes,
			date_time->seconds.seconds_tens, date_time->seconds.seconds );

	if ( HAL_UART_Transmit_IT(&huart1, (uint8_t*)pTxBuffer, strlen( (char*)pTxBuffer) ) != HAL_OK )
	{
		Error_Handler();
	}
}

void serial_print( const char *data )
{
	strcpy( (char*)pTxBuffer, (char*)data );

	if ( HAL_UART_Transmit_IT(&huart1, (uint8_t*)pTxBuffer, strlen( (char*)pTxBuffer) ) != HAL_OK )
	{
		Error_Handler();
	}
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
* @brief UART MSP Initialization
* This function configures the hardware resources used in this example
* @param huart: UART handle pointer
* @retval None
*/
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(huart->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */

  }

}

/**
* @brief UART MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param huart: UART handle pointer
* @retval None
*/
void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
  if(huart->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

    /* USART1 interrupt DeInit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }

}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if ( dateTimeReceived != NULL) {
		serialFlags.rx_cplt_flag = 1;
	}
}
