/*!
 * @file temperature_humidity_sensor.h
 * @brief Brief description of the file.
 *
 * Detailed description of the file.
 *
 * @date 17/07/2024
 * @author artempolisskyi
 */

#ifndef TEMPERATURE_HUMIDITY_SENSOR_H
#define TEMPERATURE_HUMIDITY_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "main.h"

typedef enum {
  TH_SENS_INIT_STATE = 0,
  TH_SENS_READY_TO_READ_STATE,
  TH_SENS_MEASURE_WAIT_STATE,
  TH_SENS_STATE_ERROR,
  TH_SENS_STATE_MAX
} TH_SENS_State_t;

typedef struct {
  actor_t super;
  TH_SENS_State_t state;
  int32_t temperature; ///< in milli degrees Celsius
  int32_t humidity; ///< in milli RH
} TH_SENS_Actor_t;

extern TH_SENS_Actor_t TH_SENS_Actor;

void TH_SENS_TaskInit(void);
void TH_SENS_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif //TEMPERATURE_HUMIDITY_SENSOR_H