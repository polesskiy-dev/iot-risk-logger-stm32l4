/*!
 * @file cron.h
 * @brief Brief description of the file.
 *
 * Detailed description of the file.
 *
 * @date 09/08/2024
 * @author artempolisskyi
 */

#ifndef CRON_H
#define CRON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "rtc.h"

HAL_StatusTypeDef CRON_Init(void);

#ifdef __cplusplus
}
#endif

#endif //CRON_H