/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_hal.h"

#include "app_conf.h"
#include "app_entry.h"
#include "app_common.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define LED_OFF(n) HAL_GPIO_WritePin(LED##n##_GPIO_Port, LED##n##_Pin, GPIO_PIN_SET)
#define LED_ON(n) HAL_GPIO_WritePin(LED##n##_GPIO_Port, LED##n##_Pin, GPIO_PIN_RESET)
#define LED_TOGGLE(n) HAL_GPIO_TogglePin(LED##n##_GPIO_Port, LED##n##_Pin)

#define TIMER_INTERVAL_MSEC(ms) (ms*1000/CFG_TS_TICK_VAL)
#define TIMER_INTERVAL_SEC(s) (s*1000*1000/CFG_TS_TICK_VAL)

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SENSE_Pin GPIO_PIN_3
#define SENSE_GPIO_Port GPIOC
#define SENSE_EXTI_IRQn EXTI3_IRQn
#define LATCH_Pin GPIO_PIN_2
#define LATCH_GPIO_Port GPIOC
#define SW3_Pin GPIO_PIN_4
#define SW3_GPIO_Port GPIOB
#define SW3_EXTI_IRQn EXTI4_IRQn
#define SW4_Pin GPIO_PIN_10
#define SW4_GPIO_Port GPIOC
#define SW4_EXTI_IRQn EXTI15_10_IRQn
#define SW1_Pin GPIO_PIN_11
#define SW1_GPIO_Port GPIOC
#define SW1_EXTI_IRQn EXTI15_10_IRQn
#define SW2_Pin GPIO_PIN_12
#define SW2_GPIO_Port GPIOC
#define SW2_EXTI_IRQn EXTI15_10_IRQn
#define LED1_Pin GPIO_PIN_0
#define LED1_GPIO_Port GPIOD
#define LED2_Pin GPIO_PIN_1
#define LED2_GPIO_Port GPIOD
#define LED3_Pin GPIO_PIN_3
#define LED3_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
