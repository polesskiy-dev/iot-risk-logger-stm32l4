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
#include "sensors_bus.h"
#include "sht3x.h"

#define TH_SENS_I2C_ADDRESS (SHT3x_I2C_ADDR_44 << 1) // ADDR connected to GND due to OPT3001 address conflict

typedef enum {
  TH_SENS_NO_STATE = 0,
  TH_SENS_IDLE_STATE,
  TH_SENS_MEASURE_WAIT_STATE,
  TH_SENS_CONTINUOUS_MEASURE_STATE,
  TH_SENS_STATE_ERROR,
  TH_SENS_STATE_MAX
} TH_SENS_State_t;

typedef struct {
  actor_t super;
  TH_SENS_State_t state;
  int16_t rawTemperature; ///< TODO units
  uint16_t rawHumidity; ///< TODO units
} TH_SENS_Actor_t;

extern TH_SENS_Actor_t TH_SENS_Actor;

actor_t* TH_SENS_TaskInit(void);
void TH_SENS_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif //TEMPERATURE_HUMIDITY_SENSOR_H