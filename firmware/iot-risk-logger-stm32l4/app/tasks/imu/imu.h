/*!
 * @file imu.h
 * @brief IMU task interface for LIS2DW12 accelerometer management in the IoT Risk Logger firmware.
 *
 * This header file defines the interface for the IMU (Inertial Measurement Unit) actor/task
 * in the IoT Risk Logger firmware. It provides type definitions, constants, and function
 * prototypes for managing and interacting with the IMU hardware, specifically the LIS2DW12
 * accelerometer sensor. The IMU actor is responsible for initializing the sensor, handling
 * its state machine, and providing access to sensor data for other system components.
 *
 * @note vendor examples: https://github.com/STMicroelectronics/STMems_Standard_C_drivers/tree/master/lis2dw12_STdC/examples
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
#include "lis2dw12.h"

#define IMU_I2C_ADDRESS (LIS2DW12_I2C_ADD_H) // SA0 connected to VDD
#define IMU_16_SAMPLES_BUFFER_SIZE (16) // number of samples to read from FIFO at once, note DO not set 32 because imu immediately overflows
#define IMU_AXES_COUNT (3)

#define IMU_EMPTY_FIFO_LEVEL (0)

typedef enum {
  IMU_NO_STATE = 0,
  IMU_STATE_IDLE,
  IMU_STATE_ERROR,
  IMU_MAX_STATE
} IMU_State_t;

typedef struct {
  actor_t super;
  IMU_State_t state;
  LIS2DW12_Object_t lis2dw12;
  int16_t lastAcceleration[IMU_AXES_COUNT]; ///< Averaged accelerometer sample from the last FIFO drain
  uint8_t lastFifoLevel; ///< Number of samples processed during the last FIFO drain
} IMU_Actor_t;

extern IMU_Actor_t IMU_Actor;

actor_t *IMU_TaskInit(void);

void IMU_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif //IMU_H
