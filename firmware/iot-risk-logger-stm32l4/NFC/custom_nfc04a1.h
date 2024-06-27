/**
  ******************************************************************************
  * @file    custom_nfc04a1.h
  * @author  MMY-SRA Application Team
  * @brief   This file contains definitions for the custom
  *          board specific functions.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CUSTOM_NFC04A1_H__
#define __CUSTOM_NFC04A1_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "st25dv_conf.h"

#include "st25dv.h"

/** @addtogroup CUSTOM
  * @{
  */

/* Exported types ------------------------------------------------------------*/

/**
 * @brief  NFC04A1 Ack Nack enumerator definition
 */
typedef enum
{
  I2CANSW_ACK = 0,
  I2CANSW_NACK
}CUSTOM_I2CANSW_E;

/* External variables --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/** @defgroup ST25DV_NUCLEO_Exported_Functions
  * @{
  */
int32_t CUSTOM_GPO_Init( void );
int32_t CUSTOM_GPO_DeInit( void );
int32_t CUSTOM_GPO_ReadPin( void );
int32_t CUSTOM_LPD_Init( void );
int32_t CUSTOM_LPD_DeInit( void );
int32_t CUSTOM_LPD_ReadPin( void );
int32_t CUSTOM_LPD_On( void );
int32_t CUSTOM_LPD_Off( void );
int32_t CUSTOM_LPD_Toggle( void );
void BSP_GPO_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* __CUSTOM_NFC04A1_H__ */

