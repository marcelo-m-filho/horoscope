/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
  ******************************************************************************
  */

#include "usart.h"

// external functions ----------------------------------------------------------
void Error_Handler(void);

// private defines -------------------------------------------------------------

// USART1 GPIO Configuration
// PA10 -> USART1_RX
#define VCP_RX_GPIO_Port  GPIOA
#define VCP_RX_Pin        GPIO_PIN_10

// PA9  -> USART1_TX
#define VCP_TX_GPIO_Port  GPIOA
#define VCP_TX_Pin        GPIO_PIN_9

UART_HandleTypeDef UART1_Handle;

#define SEND_MANY_BUFFER_SIZE 100

void USART1_UART_Init(void)
{
    UART1_Handle.Instance           = USART1; 
    UART1_Handle.Init.BaudRate      = 115200;
    UART1_Handle.Init.WordLength    = UART_WORDLENGTH_8B;
    UART1_Handle.Init.StopBits      = UART_STOPBITS_1;
    UART1_Handle.Init.Parity        = UART_PARITY_NONE;
    UART1_Handle.Init.Mode          = UART_MODE_TX_RX;
    UART1_Handle.Init.HwFlowCtl     = UART_HWCONTROL_NONE;
    UART1_Handle.Init.OverSampling  = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&UART1_Handle) != HAL_OK)
    {
        Error_Handler();
    }
}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef         GPIO_InitStruct     = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  if(uartHandle->Instance == USART1)
  {
    // initializes the peripherals clock
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART1;
    PeriphClkInitStruct.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    // USART1 clock enable
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin       = VCP_RX_Pin;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(VCP_RX_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin       = VCP_TX_Pin;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(VCP_TX_GPIO_Port, &GPIO_InitStruct);
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance == USART1)
  {
    // peripheral clock disable
    __HAL_RCC_USART1_CLK_DISABLE();

    HAL_GPIO_DeInit(GPIOA, VCP_RX_Pin|VCP_TX_Pin);
  }
}
