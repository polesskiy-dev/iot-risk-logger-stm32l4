/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "SEGGER_SYSVIEW_Conf.h"
#include "SEGGER_SYSVIEW.h"

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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define _NFC_INT_Pin GPIO_PIN_0
#define _NFC_INT_GPIO_Port GPIOA
#define _NFC_INT_EXTI_IRQn EXTI0_IRQn
#define _LIGHT_INT_Pin GPIO_PIN_1
#define _LIGHT_INT_GPIO_Port GPIOA
#define _LIGHT_INT_EXTI_IRQn EXTI1_IRQn
#define USB_VBUS_SENSE_Pin GPIO_PIN_8
#define USB_VBUS_SENSE_GPIO_Port GPIOA
#define USB_VBUS_SENSE_EXTI_IRQn EXTI9_5_IRQn
#define _LED_Pin GPIO_PIN_15
#define _LED_GPIO_Port GPIOA
#define _TEMP_RESET_Pin GPIO_PIN_4
#define _TEMP_RESET_GPIO_Port GPIOB
#define TEMP_INT_Pin GPIO_PIN_5
#define TEMP_INT_GPIO_Port GPIOB
#define TEMP_INT_EXTI_IRQn EXTI9_5_IRQn
#define IMU_INT1_Pin GPIO_PIN_6
#define IMU_INT1_GPIO_Port GPIOB
#define IMU_INT1_EXTI_IRQn EXTI9_5_IRQn
#define IMU_INT2_Pin GPIO_PIN_7
#define IMU_INT2_GPIO_Port GPIOB
#define IMU_INT2_EXTI_IRQn EXTI9_5_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
