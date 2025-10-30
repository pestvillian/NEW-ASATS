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
#include "stm32f4xx_hal.h"

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

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define T_CS_Pin GPIO_PIN_4
#define T_CS_GPIO_Port GPIOA
#define T_SCK_Pin GPIO_PIN_5
#define T_SCK_GPIO_Port GPIOA
#define FAN_EN_Pin GPIO_PIN_5
#define FAN_EN_GPIO_Port GPIOC
#define DEBUG_LED_Pin GPIO_PIN_10
#define DEBUG_LED_GPIO_Port GPIOC
#define LCD_NRST_Pin GPIO_PIN_6
#define LCD_NRST_GPIO_Port GPIOD
#define T_IRQ_Pin GPIO_PIN_4
#define T_IRQ_GPIO_Port GPIOB
#define T_IRQ_EXTI_IRQn EXTI4_IRQn
#define BL_PWM_Pin GPIO_PIN_5
#define BL_PWM_GPIO_Port GPIOB
#define OTG_FS_OC_Pin GPIO_PIN_8
#define OTG_FS_OC_GPIO_Port GPIOB
#define OTG_FS_OC_EXTI_IRQn EXTI9_5_IRQn
#define OTG_FS_PSO_Pin GPIO_PIN_9
#define OTG_FS_PSO_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
