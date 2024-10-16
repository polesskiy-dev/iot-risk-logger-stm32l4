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
#include <time.h>

#include "main.h"
#include "rtc.h"

#define YEARS_FROM_1900_TO_2000 100
#define WAKE_UP_AUTO_CLEAR 1 ///< Auto-clear the wake-up event, especially useful in low-power modes.

typedef struct {
  actor_t super;
} CRON_Actor_t;

actor_t* CRON_ActorInit(void);
int32_t CRON_GetCurrentUnixTimestamp(void);
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc);

#ifdef __cplusplus
}
#endif

#endif //CRON_H