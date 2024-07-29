/*!
 * @file light_sensor.h
 * @brief Brief description of the file.
 *
 * Detailed description of the file.
 *
 * @date 28/07/2024
 * @author artempolisskyi
 */

#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "main.h"
#include "actor.h"

#define OPT3001_I2C_ADDRESS 0x45 // ADDR connected to VDD due to SHT3x 0x44 address conflict

typedef enum {
  LIGHT_SENS_INIT_STATE = 0,
  LIGHT_SENS_READ_READY_STATE,
  LIGHT_SENS_STATE_ERROR,
  LIGHT_SENS_STATE_MAX
} LIGHT_SENS_State_t;

typedef struct {
  actor_t super;
  LIGHT_SENS_State_t state;
  int32_t lux; ///< in milli lux
} LIGHT_SENS_Actor_t;

extern LIGHT_SENS_Actor_t LIGHT_SENS_Actor;

void LIGHT_SENS_TaskInit(void);
void LIGHT_SENS_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif //LIGHT_SENSOR_H