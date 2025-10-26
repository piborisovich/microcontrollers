/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : uart.c
  * @brief          : UART body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "uart.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RX_BUFFER_SIZE 1024
#define RX_INACTIVITY_TIME 700
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;

typedef struct _UART_STRUCT
{
	uint8_t rx_bufferByte[1];
	uint8_t rx_buffer[RX_BUFFER_SIZE];
	uint16_t rx_head;
	uint16_t rx_tail;

	uint32_t lastReceiveTick;

	uint8_t isTransmit;
} UART_STRUCT;


UART_STRUCT uartStr = {0};

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void Error_Handler(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */


/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
void MX_USART1_UART_Init(void)
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

  uartStr.rx_head = 0;
  uartStr.rx_tail = 0;

  HAL_UART_Receive_IT(&huart1, (uint8_t*)uartStr.rx_bufferByte, 1);
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief  The application entry point.
  * @retval int
  */
void uart_main(uint8_t *led_enable)
{
	if ( HAL_UART_GetState(&huart1) != HAL_UART_STATE_BUSY_TX && uartStr.rx_head != uartStr.rx_tail )
	{
		if ( led_enable != NULL ) *led_enable = 1;

		HAL_UART_AbortReceive_IT(&huart1);
		HAL_UART_Transmit_IT(&huart1, (uint8_t*)(uartStr.rx_buffer + uartStr.rx_head), 1);
	}

	if ( HAL_GetTick() - uartStr.lastReceiveTick > RX_INACTIVITY_TIME )
	{
		uartStr.lastReceiveTick = HAL_GetTick();
		if ( led_enable != NULL ) *led_enable = 0;
	}
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(huart);
  /* NOTE: This function should not be modified, when the callback is needed,
           the HAL_UART_RxCpltCallback could be implemented in the user file
   */
  uartStr.rx_buffer[uartStr.rx_tail] = uartStr.rx_bufferByte[0];

  uartStr.rx_tail = (uartStr.rx_tail + 1) % RX_BUFFER_SIZE;

  if ( uartStr.rx_head == uartStr.rx_tail )
  {
	  //error
  }

  uartStr.lastReceiveTick = HAL_GetTick();

  HAL_UART_Receive_IT(&huart1, (uint8_t*)uartStr.rx_bufferByte, 1);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(huart);
  /* NOTE: This function should not be modified, when the callback is needed,
           the HAL_UART_TxCpltCallback could be implemented in the user file
   */
  uartStr.rx_head = (uartStr.rx_head + 1) % RX_BUFFER_SIZE;
  HAL_UART_Receive_IT(&huart1, (uint8_t*)uartStr.rx_bufferByte, 1);
}


