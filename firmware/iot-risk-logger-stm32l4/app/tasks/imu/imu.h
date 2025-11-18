/*!
 * @file imu.h
 * @brief Brief description of the file.
 *
 * This header file defines the interface for the IMU (Inertial Measurement Unit) actor/task
 * in the IoT Risk Logger firmware. It provides type definitions, constants, and function
 * prototypes for managing and interacting with the IMU hardware, specifically the LIS2DW12
 * accelerometer sensor. The IMU actor is responsible for initializing the sensor, handling
 * its state machine, and providing access to sensor data for other system components.
 *
 * @date 19/11/2025
 * @author artempolisskyi
 */

#ifndef IMU_H
#define IMU_H

#ifdef __cplusplus
extern "C" {



#endif

#include <stdio.h>
#include <stdint.h>

#include "main.h"

#define IMU_I2C_ADDRESS (LIS2DW12_I2C_ADD_H << 1) // SA0 connected to VDD

typedef enum {
  IMU_NO_STATE = 0,
  IMU_STATE_IDLE,
  IMU_STATE_ERROR,
  IMU_MAX_STATE
} IMU_State_t;

typedef struct {
  actor_t super;
  IMU_State_t state;
} IMU_Actor_t;

extern IMU_Actor_t IMU_Actor;

actor_t *IMU_TaskInit(void);

void IMU_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif //IMU_H
