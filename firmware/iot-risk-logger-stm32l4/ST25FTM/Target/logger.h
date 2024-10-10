/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    NFC_FTM\Inc\logger.h
  * @author  MMY Application Team
  * @brief   Header file for logger.c
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
/* USER CODE END Header */

#ifndef LOGGER_H
#define LOGGER_H
#include <stdint.h>
#include <stddef.h>
/*
******************************************************************************
* INCLUDES
******************************************************************************
*/

/*
******************************************************************************
* DEFINES
******************************************************************************
*/
#define LOGGER_ON   1
#define LOGGER_OFF  0

/*!
 *****************************************************************************
 *  \brief  helper to convert hex data into formated string
 *
 *  \param[in] data : pointer to buffer to be dumped.
 *
 *  \param[in] dataLen : buffer length
 *
 *  \return hex formated string
 *
 *****************************************************************************
 */
extern char* hex2Str(unsigned char * data, size_t dataLen);

#endif /* LOGGER_H */

