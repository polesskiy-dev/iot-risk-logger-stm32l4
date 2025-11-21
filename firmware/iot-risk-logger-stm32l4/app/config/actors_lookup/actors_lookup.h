/*!
 * @file actors_list.h
 * @brief Brief description of the file.
 *
 * Detailed description of the file.
 *
 * @date 17/07/2024
 * @author artempolisskyi
 */

#ifndef ACTORS_LIST_H
#define ACTORS_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "actor.h"

typedef enum {
  NO_ACTOR_ID = 0,
  EV_MANAGER_ACTOR_ID, // should receive messages directly
  CRON_ACTOR_ID,
  PWRM_MANAGER_ACTOR_ID,
  NFC_ACTOR_ID,
  IMU_ACTOR_ID,
  TEMPERATURE_HUMIDITY_SENSOR_ACTOR_ID,
  LIGHT_SENSOR_ACTOR_ID,
  MEMORY_ACTOR_ID,
  INFO_LED_ACTOR_ID,
  MAX_ACTORS
} ACTOR_ID;

#ifdef __cplusplus
}
#endif

#endif //ACTORS_LIST_H