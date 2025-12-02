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
#include "SEGGER_RTT.h"

#include "actor.h"
#include "event_manager.h"
#include "power_mode_manager.h"
#include "gpio_ext_interrupts.h"
#include "nfc.h"
#include "memory.h"
#include "temperature_humidity_sensor.h"
#include "light_sensor.h"
#include "cron.h"
#include "info_led.h"
#include "bsp_bus.h"

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
#define USB_VBUS_SENSE_WKUP2_Pin GPIO_PIN_13
#define USB_VBUS_SENSE_WKUP2_GPIO_Port GPIOC
#define PH0_Disabled_Pin GPIO_PIN_0
#define PH0_Disabled_GPIO_Port GPIOH
#define PH1_Disabled_Pin GPIO_PIN_1
#define PH1_Disabled_GPIO_Port GPIOH
#define NFC_INT_WKUP_Pin GPIO_PIN_0
#define NFC_INT_WKUP_GPIO_Port GPIOA
#define NFC_INT_WKUP_EXTI_IRQn EXTI0_IRQn
#define PA1_Disabled_Pin GPIO_PIN_1
#define PA1_Disabled_GPIO_Port GPIOA
#define MEMORY_EN_N_Pin GPIO_PIN_4
#define MEMORY_EN_N_GPIO_Port GPIOA
#define PA5_Disabled_Pin GPIO_PIN_5
#define PA5_Disabled_GPIO_Port GPIOA
#define VDD_NFC_Pin GPIO_PIN_2
#define VDD_NFC_GPIO_Port GPIOB
#define I2C2_SCL_NFC_Pin GPIO_PIN_10
#define I2C2_SCL_NFC_GPIO_Port GPIOB
#define I2C2_SDA_NFC_Pin GPIO_PIN_11
#define I2C2_SDA_NFC_GPIO_Port GPIOB
#define PB12_Disabled_Pin GPIO_PIN_12
#define PB12_Disabled_GPIO_Port GPIOB
#define PB13_Disabled_Pin GPIO_PIN_13
#define PB13_Disabled_GPIO_Port GPIOB
#define PB14_Disabled_Pin GPIO_PIN_14
#define PB14_Disabled_GPIO_Port GPIOB
#define PB15_Disabled_Pin GPIO_PIN_15
#define PB15_Disabled_GPIO_Port GPIOB
#define PA8_Disabled_Pin GPIO_PIN_8
#define PA8_Disabled_GPIO_Port GPIOA
#define I2C1_SCL_SENS_Pin GPIO_PIN_9
#define I2C1_SCL_SENS_GPIO_Port GPIOA
#define I2C1_SDA_SENS_Pin GPIO_PIN_10
#define I2C1_SDA_SENS_GPIO_Port GPIOA
#define USB_DM_N_Pin GPIO_PIN_11
#define USB_DM_N_GPIO_Port GPIOA
#define USB_DP_P_Pin GPIO_PIN_12
#define USB_DP_P_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define LED_N_Pin GPIO_PIN_15
#define LED_N_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define TEMP_RESET_N_Pin GPIO_PIN_4
#define TEMP_RESET_N_GPIO_Port GPIOB
#define TEMP_INT_Pin GPIO_PIN_5
#define TEMP_INT_GPIO_Port GPIOB
#define TEMP_INT_EXTI_IRQn EXTI9_5_IRQn
#define IMU_INT1_Pin GPIO_PIN_6
#define IMU_INT1_GPIO_Port GPIOB
#define IMU_INT1_EXTI_IRQn EXTI9_5_IRQn
#define IMU_INT2_Pin GPIO_PIN_7
#define IMU_INT2_GPIO_Port GPIOB
#define IMU_INT2_EXTI_IRQn EXTI9_5_IRQn
#define PH3_BOOT0_Disabled_Pin GPIO_PIN_3
#define PH3_BOOT0_Disabled_GPIO_Port GPIOH
#define LIGHT_INT_N_Pin GPIO_PIN_8
#define LIGHT_INT_N_GPIO_Port GPIOB
#define LIGHT_INT_N_EXTI_IRQn EXTI9_5_IRQn
#define PB9_Disabled_Pin GPIO_PIN_9
#define PB9_Disabled_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
