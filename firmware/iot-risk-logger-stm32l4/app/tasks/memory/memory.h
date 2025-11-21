/*!
 * @file memory.h
 * @brief Brief description of the file.
 *
 * Detailed description of the file.
 *
 * @date 14/08/2024
 * @author artempolisskyi
 */

#ifndef MEMORY_H
#define MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>

#include "main.h"
#include "quadspi.h"
#include "w25q.h"
#include "fs_static.h"

/* W25Q64JV Memory Specifications */
#define W25Q64JV_FLASH_SIZE              (0x800000)  /* 8 MB (64 Mbit) */
#define W25Q64JV_FLASH_ADDR_SIZE_BITS    (24)        /* 24bit size address */
#define W25Q64JV_SECTOR_SIZE             (0x1000)    /* 4 KB */
#define W25Q64JV_SUBSECTOR_SIZE          (0x0100)    /* 256 B */
#define W25Q64JV_PAGE_SIZE               (0x0100)    /* 256 B */
#define W25Q64JV_BLOCK_SIZE_32K          (0x8000)    /* 32 KB */
#define W25Q64JV_BLOCK_SIZE_64K          (0x10000)   /* 64 KB */

#define MEMORY_TIMESTAMP_ENTRY_SIZE                   (0x04)      /* 4 bytes */
#define MEMORY_LUX_ENTRY_SIZE                         (0x02)      /* 2 bytes */
#define MEMORY_TEMPERATURE_ENTRY_SIZE                 (0x02)      /* 2 bytes */
#define MEMORY_HUMIDITY_ENTRY_SIZE                    (0x04)      /* 2 bytes */
#define MEMORY_ACCEL_ENTRY_SIZE                       (0x06)      /* 3 * 2 bytes (X, Y, Z) */
#define RESERVED_ENTRY_SIZE                           (0x04)      /* 4 bytes */
#define MEMORY_LOG_ENTRY_SIZE                         (MEMORY_TIMESTAMP_ENTRY_SIZE + MEMORY_TEMPERATURE_ENTRY_SIZE + MEMORY_HUMIDITY_ENTRY_SIZE + MEMORY_LUX_ENTRY_SIZE + MEMORY_ACCEL_ENTRY_SIZE + RESERVED_ENTRY_SIZE)

#define MEMORY_CHUNKS_ARE_EQUAL                       (0)

typedef enum {
  TEMPERATURE_HUMIDITY_MEASUREMENTS_READY_EVENT_FLAG = 0x01,
  LIGHT_MEASUREMENTS_READY_EVENT_FLAG = 0x02,
  IMU_MEASUREMENTS_READY_EVENT_FLAG = 0x04
} MEMORY_MeasurementEventFlag_t;

typedef enum {
  MEMORY_NO_STATE = 0,
  MEMORY_SLEEP_STATE,
  MEMORY_WRITE_STATE,
  MEMORY_STATE_ERROR,
  MEMORY_MAX_STATE
} MEMORY_State_t;

/**
 * @brief Sensors measurements log entry
 * Contains timestamp, raw temperature, raw humidity, raw lux and reserved fields
 */
typedef struct __attribute__((packed)) {
  int32_t timestamp;
  uint16_t rawTemperature;
  uint16_t rawHumidity;
  uint16_t rawLux;
  int16_t accelX;
  int16_t accelY;
  int16_t accelZ;
  uint32_t reserved; // 4 bytes reserved, can be event type, extra info etc.
} MEMORY_SensorsMeasurementEntry_t;

typedef struct {
  actor_t super;
  MEMORY_State_t state;
  uint32_t logFileTailAddress; ///< Address of the last free space to append into the log file
} MEMORY_Actor_t;

actor_t* MEMORY_TaskInit(void);
void MEMORY_Task(void *argument);
uint32_t MEMORY_SeekFreeSpaceAddress(void);

#ifdef __cplusplus
}
#endif

#endif //MEMORY_H