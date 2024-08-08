/*!
 * @file light_sensor.h
 * @brief Brief description of the file.
 *
 * TODO Detailed description of the file.
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
#include "custom_bus.h"
#include "actor.h"
#include "opt3001.h"

#define OPT3001_I2C_ADDRESS (0x45 << 1) // ADDR connected to VDD due to SHT3x 0x44 address conflict

typedef enum {
  LIGHT_SENS_NO_STATE = 0,
  LIGHT_SENS_TURNED_OFF_STATE, ///< Initialized, turned off, ready for commands, low power mode
  LIGHT_SENS_CONTINUOUS_MEASURE_STATE, ///< Device is measuring continuously, generates interrupt on threshold exceed
  LIGHT_SENS_OUT_OF_RANGE_STATE, ///< Lux is out of range, limits are swapped, return to measurements after lux returns in limits
  LIGHT_SENS_STATE_ERROR, ///< Error state
  LIGHT_SENS_MAX_STATE
} LIGHT_SENS_State_t;

typedef struct {
  actor_t super;
  LIGHT_SENS_State_t state;
  uint16_t rawLux; ///< raw lux (exponent + mantissa)
  uint16_t highLimit; ///< high limit for lux (in raw) TODO verify if it ir's in raw
} LIGHT_SENS_Actor_t;

extern LIGHT_SENS_Actor_t LIGHT_SENS_Actor;

actor_t* LIGHT_SENS_TaskInit(void);
void LIGHT_SENS_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif //LIGHT_SENSOR_H