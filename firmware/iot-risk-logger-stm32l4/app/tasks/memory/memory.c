/*!
 * @file memory.c
 * @brief implementation of memory
 *
 * Detailed description of the implementation file.
 *
 * @date 14/08/2024
 * @author artempolisskyi
 */

#include "memory.h"
#include "usbd_msc.h"

static osStatus_t handleMemoryFSM(MEMORY_Actor_t *this, message_t *message);
static osStatus_t handleInit(MEMORY_Actor_t *this, message_t *message);
static osStatus_t handleSleep(MEMORY_Actor_t *this, message_t *message);
static osStatus_t handleWrite(MEMORY_Actor_t *this, message_t *message);

static osStatus_t writeFAT12BootSector(MEMORY_Actor_t *this);
static osStatus_t writeSettingsToMemory(MEMORY_Actor_t *this, uint8_t *settingsWriteBuff);
static void publishMemoryWriteOnMeasurementsReady(MEMORY_Actor_t *this);
static osStatus_t appendMeasurementsToNORFlashLogTail(MEMORY_Actor_t *this);

extern actor_t* ACTORS_LOOKUP_SystemRegistry[MAX_ACTORS];
extern uint8_t FAT12_BootSector[FAT12_BOOT_SECTOR_SIZE];

extern USBD_StorageTypeDef USBD_Storage_Interface_fops_FS;

osEventFlagsId_t measurementsReadyEventFlags;

/**
 * @brief Memory actor struct representing NOR Flash storage
 * @extends actor_t
 */
MEMORY_Actor_t MEMORY_Actor = {
        .super = {
                .actorId = MEMORY_ACTOR_ID,
                .messageHandler = (messageHandler_t) handleMemoryFSM,
                .osMessageQueueId = NULL,
                .osThreadId = NULL,
        },
        .logFileTailAddress = FAT12_BOOT_SECTOR_SIZE + 1,
        .state = MEMORY_NO_STATE
};

// task description required for static task creation
uint32_t memoryTaskBuffer[DEFAULT_TASK_STACK_SIZE_WORDS];
StaticTask_t memoryTaskControlBlock;
const osThreadAttr_t memoryTaskDescription = {
        .name = "memoryTask",
        .cb_mem = &memoryTaskControlBlock,
        .cb_size = sizeof(memoryTaskControlBlock),
        .stack_mem = &memoryTaskBuffer[0],
        .stack_size = sizeof(memoryTaskBuffer),
        .priority = (osPriority_t) osPriorityNormal,
};

W25Q_HandleTypeDef MEMORY_W25QHandle = {
        .hqspi = &hqspi,
        .geometry = {
                .flashSize      = W25Q64JV_FLASH_SIZE,
                .sectorSize     = W25Q64JV_SECTOR_SIZE,
                .subSectorSize  = W25Q64JV_SUBSECTOR_SIZE,
                .pageSize       = W25Q64JV_PAGE_SIZE,
                .blockSize32K   = W25Q64JV_BLOCK_SIZE_32K,
                .blockSize64K   = W25Q64JV_BLOCK_SIZE_64K
        },
        .status.status1Reg      = 0x00,
        .busyWaitCycles         = FLASH_BUSY_WAIT_CYCLES
};

/**
 * @brief Initializes the Memory Sensor task.
 * @return {actor_t*} - pointer to the actor base struct
 */
actor_t* MEMORY_TaskInit(void) {
  MEMORY_Actor.super.osMessageQueueId = osMessageQueueNew(DEFAULT_QUEUE_SIZE, DEFAULT_QUEUE_MESSAGE_SIZE, &(osMessageQueueAttr_t){
          .name = "memoryQueue"
  });
  MEMORY_Actor.super.osThreadId = osThreadNew(MEMORY_Task, NULL, &memoryTaskDescription);

  measurementsReadyEventFlags = osEventFlagsNew(NULL);

  return (actor_t*) &MEMORY_Actor;
}

/**
 * @brief Memory (NOR Flash) task
 * Waits for message from the queue and proceed it in FSM
 * Enters ERROR state if message handling failed
 */
void MEMORY_Task(void *argument) {
  (void) argument; // Avoid unused parameter warning
  message_t msg;

  for (;;) {
    // Wait for messages from the queue
    if (osMessageQueueGet(MEMORY_Actor.super.osMessageQueueId, &msg, NULL, osWaitForever) == osOK) {
      osStatus_t status = MEMORY_Actor.super.messageHandler((actor_t *) &MEMORY_Actor, &msg);

      if (status != osOK) {
        osMessageQueueId_t evManagerQueue = ACTORS_LOOKUP_SystemRegistry[EV_MANAGER_ACTOR_ID]->osMessageQueueId;
        osMessageQueuePut(evManagerQueue, &(message_t){GLOBAL_ERROR, .payload.value = MEMORY_ACTOR_ID}, 0, 0);
        TO_STATE(&MEMORY_Actor, MEMORY_STATE_ERROR);
      }
    }
  }
}

static osStatus_t handleMemoryFSM(MEMORY_Actor_t *this, message_t *message) {
  switch (this->state) {
    case MEMORY_NO_STATE:
      return handleInit(this, message);
    case MEMORY_SLEEP_STATE:
      return handleSleep(this, message);
    case MEMORY_WRITE_STATE:
      return handleWrite(this, message);
    default:
      return osOK;
  }

  return osOK;
}

uint32_t MEMORY_SeekFreeSpaceAddress(void) {
  uint8_t readBuff[MEMORY_LOG_ENTRY_SIZE] = {0};
  uint8_t emptyLogEntryTemplate[MEMORY_LOG_ENTRY_SIZE] = {0xFF};
  const uint32_t startAddress = INITIAL_LOG_START_ADDR;
  const uint8_t stepSize = MEMORY_LOG_ENTRY_SIZE;

  uint32_t offset = 0;
  uint32_t stepNumber = 0;
  uint32_t addr = 0;
  bool isFreeSpaceForLog = false; // does read log entry contains data other than 0xFF (0xFF means empty)

  // create an empty log entry template to compare with
  memset(emptyLogEntryTemplate, 0xFF, MEMORY_LOG_ENTRY_SIZE);

  do {
    // address offset from FS boot sector
    offset = stepNumber * stepSize;
    addr = startAddress + offset;

    // read data chunk equal to log entry
    W25Q_ReadData(&MEMORY_W25QHandle, readBuff, addr, stepSize); // @warning: io status check is omitted here

    // check if space for log entry don't contain any data
    isFreeSpaceForLog = MEMORY_CHUNKS_ARE_EQUAL == memcmp(readBuff, emptyLogEntryTemplate, MEMORY_LOG_ENTRY_SIZE);

    if (isFreeSpaceForLog) {
      return addr;
    }

    stepNumber++;

  } while (addr < W25Q64JV_FLASH_SIZE);

  return addr;
}


/**
 * @brief Writes FAT12 boot sector to the NOR Flash
 * Required for USB MSD to work
 */
static osStatus_t writeFAT12BootSector(MEMORY_Actor_t *this) {
  // erase chip
  HAL_StatusTypeDef status = W25Q_EraseChip(&MEMORY_W25QHandle);
  if (status != HAL_OK) {
    #ifdef DEBUG
      fprintf(stderr,  "memory error on chip erase\n");
    #endif
    return status;
  }

  #ifdef DEBUG
    fprintf(stdout,  "Chip erased\n");
  #endif

  // flash FAT12 boot sector
  status = W25Q_WriteData(&MEMORY_W25QHandle, FAT12_BootSector, 0, FAT12_BOOT_SECTOR_SIZE);
  if (status != HAL_OK) {
    #ifdef DEBUG
      fprintf(stderr,  "memory error on FAT12 write\n");
    #endif
    return status;
  }

  #ifdef DEBUG
    fprintf(stdout,  "FS FAT12 boot sector has been written on NOR Flash\n");
  #endif

  return status;
}

static osStatus_t handleInit(MEMORY_Actor_t *this, message_t *message) {
  if (GLOBAL_CMD_INITIALIZE == message->event) {
    uint8_t norFlashID[W25Q_ID_SIZE] = {0x00, 0x00};

    // wake up the chip
    osStatus_t ioStatus = W25Q_WakeUp(&MEMORY_W25QHandle);
    if (ioStatus != osOK) return osError;

    // read ID
    ioStatus = W25Q_ReadID(&MEMORY_W25QHandle, norFlashID);
    if (ioStatus != osOK) return osError;

    #ifdef DEBUG
        fprintf(stdout, "W25Q NOR MF ID: 0x%x, Device ID: 0x%x\n", norFlashID[0], norFlashID[1]);
    #endif

    #ifdef FLASH_ERASE_CHIP_AND_WRITE_FAT12_BOOT_SECTOR
        writeFAT12BootSector(&MEMORY_Actor);
    #endif

    // find the first free space address on NOR flash (to append log to)
    uint32_t freeSpaceAddress = MEMORY_SeekFreeSpaceAddress();
    MEMORY_Actor.logFileTailAddress = freeSpaceAddress;

    // put memory to sleep
    ioStatus = W25Q_Sleep(&MEMORY_W25QHandle);
    if (ioStatus != osOK) return osError;

    // publish to event manager that memory is initialized
    osMessageQueueId_t evManagerQueue = ACTORS_LOOKUP_SystemRegistry[EV_MANAGER_ACTOR_ID]->osMessageQueueId;
    osMessageQueuePut(evManagerQueue, &(message_t){GLOBAL_INITIALIZE_SUCCESS, .payload.value = MEMORY_ACTOR_ID}, 0, 0);

    #ifdef DEBUG
        fprintf(stdout, "First free space address: %x\n", freeSpaceAddress);
        fprintf(stdout, "Memory task initialized\n");
    #endif

    TO_STATE(this, MEMORY_SLEEP_STATE);
    return osOK;
  }
};

static osStatus_t handleSleep(MEMORY_Actor_t *this, message_t *message) {
  osStatus_t ioStatus = osOK;
  uint8_t *measurementsLogReadBuff = NULL;
  uint8_t *settingsWriteBuff = NULL;
  uint8_t *settingsReadBuff = NULL;

  osMessageQueueId_t evManagerQueue = ACTORS_LOOKUP_SystemRegistry[EV_MANAGER_ACTOR_ID]->osMessageQueueId;

  switch (message->event) {
    case GLOBAL_TEMPERATURE_HUMIDITY_MEASUREMENTS_READY:
      // set appropriate event flag
      osEventFlagsSet(measurementsReadyEventFlags, TEMPERATURE_HUMIDITY_MEASUREMENTS_READY_EVENT_FLAG);

      // if both measurements are ready, emit write to the memory message
      publishMemoryWriteOnMeasurementsReady(this);

      TO_STATE(this, MEMORY_SLEEP_STATE);
      return osOK;

    case GLOBAL_LIGHT_MEASUREMENTS_READY:
      // set appropriate event flag
      osEventFlagsSet(measurementsReadyEventFlags, LIGHT_MEASUREMENTS_READY_EVENT_FLAG);

      // if both measurements are ready, emit write to the memory message
      publishMemoryWriteOnMeasurementsReady(this);

      TO_STATE(this, MEMORY_SLEEP_STATE);
      return osOK;

    case GLOBAL_IMU_MEASUREMENTS_READY:
      // set appropriate event flag
      osEventFlagsSet(measurementsReadyEventFlags, IMU_MEASUREMENTS_READY_EVENT_FLAG);

      // if both measurements are ready, emit write to the memory message
      publishMemoryWriteOnMeasurementsReady(this);

      TO_STATE(this, MEMORY_SLEEP_STATE);
      return osOK;

    case MEMORY_MEASUREMENTS_WRITE:
      // wake up the chip
      W25Q_WakeUp(&MEMORY_W25QHandle);

      // save measurements to the memory, increment log tail address
      ioStatus = appendMeasurementsToNORFlashLogTail(this);

      // TODO if ioStatus is not OK return it

      osMessageQueuePut(evManagerQueue, &(message_t) {GLOBAL_MEASUREMENTS_WRITE_SUCCESS}, 0, 0);

      TO_STATE(this, MEMORY_WRITE_STATE);
      return ioStatus;

    case GLOBAL_CMD_READ_LOG_CHUNK:
      // TODO implement settings and log chunk read/write
      assert_param(false);

      osMessageQueuePut(evManagerQueue, &(message_t) {GLOBAL_LOG_CHUNK_READ_SUCCESS}, 0, 0);

      TO_STATE(this, MEMORY_SLEEP_STATE);
      return osOK;

    case GLOBAL_CMD_WRITE_SETTINGS:
      // wake up the chip
      W25Q_WakeUp(&MEMORY_W25QHandle);

      settingsWriteBuff = (uint8_t *) message->payload.ptr;

      // write settings to the memory
      ioStatus = writeSettingsToMemory(this, settingsWriteBuff);

      // TODO if ioStatus is not OK return it

      osMessageQueuePut(evManagerQueue, &(message_t) {GLOBAL_SETTINGS_WRITE_SUCCESS}, 0, 0);

      TO_STATE(this, MEMORY_WRITE_STATE);
      return ioStatus;

    case GLOBAL_CMD_READ_SETTINGS:
      // wake up the chip
      W25Q_WakeUp(&MEMORY_W25QHandle);

      // other module is responsible to provide correct buffer address to write to
      measurementsLogReadBuff = (uint8_t *) message->payload.ptr;

      // read settings from the memory
      ioStatus = W25Q_ReadData(&MEMORY_W25QHandle, measurementsLogReadBuff, SETTINGS_FILE_ADDR, SETTINGS_DATA_SIZE);

      osMessageQueuePut(evManagerQueue, &(message_t) {GLOBAL_SETTINGS_READ_SUCCESS}, 0, 0);

      TO_STATE(this, MEMORY_SLEEP_STATE);
      return ioStatus;

    default:
      TO_STATE(this, MEMORY_SLEEP_STATE);
      return osOK;
  }
};

static osStatus_t handleWrite(MEMORY_Actor_t *this, message_t *message) {
  osStatus_t ioStatus = osOK;

  switch (message->event) {
    case GLOBAL_MEASUREMENTS_WRITE_SUCCESS:
    case GLOBAL_SETTINGS_WRITE_SUCCESS:
      ioStatus = W25Q_Sleep(&MEMORY_W25QHandle);

      TO_STATE(this, MEMORY_SLEEP_STATE);
      return ioStatus;

    default:
      TO_STATE(this, MEMORY_WRITE_STATE);
      return osOK;
  }
}

static osStatus_t writeSettingsToMemory(MEMORY_Actor_t *this, uint8_t *settingsWriteBuff) {
  osStatus_t ioStatus = osOK;

  // clear previous settings - erase the sector
  ioStatus = W25Q_EraseSector(&MEMORY_W25QHandle, SETTINGS_FILE_ADDR);

  // write settings to the memory
  ioStatus = W25Q_WriteData(&MEMORY_W25QHandle, settingsWriteBuff, SETTINGS_FILE_ADDR, SETTINGS_DATA_SIZE) && ioStatus;

  return ioStatus;
}

/**
 * @brief Publishes MEMORY_MEASUREMENTS_WRITE message to the memory task on both measurements ready
 */
static void publishMemoryWriteOnMeasurementsReady(MEMORY_Actor_t *this) {
  const uint32_t readyMask = TEMPERATURE_HUMIDITY_MEASUREMENTS_READY_EVENT_FLAG |
                             LIGHT_MEASUREMENTS_READY_EVENT_FLAG |
                             IMU_MEASUREMENTS_READY_EVENT_FLAG;

  const uint32_t currentFlags = osEventFlagsGet(measurementsReadyEventFlags);

  if ((currentFlags & readyMask) == readyMask) {
    osEventFlagsClear(measurementsReadyEventFlags, readyMask);
    osMessageQueuePut(this->super.osMessageQueueId, &(message_t) {MEMORY_MEASUREMENTS_WRITE}, 0, 0);
  }
}

static osStatus_t appendMeasurementsToNORFlashLogTail(MEMORY_Actor_t *this) {
  osStatus_t ioStatus = osOK;

  // sensors actors pointers from the system registry
  TH_SENS_Actor_t *thSensActor = (TH_SENS_Actor_t *)ACTORS_LOOKUP_SystemRegistry[TEMPERATURE_HUMIDITY_SENSOR_ACTOR_ID];
  LIGHT_SENS_Actor_t *lightSensorActor = (LIGHT_SENS_Actor_t *)ACTORS_LOOKUP_SystemRegistry[LIGHT_SENSOR_ACTOR_ID];
  IMU_Actor_t *imuActor = (IMU_Actor_t *)ACTORS_LOOKUP_SystemRegistry[IMU_ACTOR_ID];

  // current timestamp in UNIX format
  int32_t timestamp = CRON_GetCurrentUnixTimestamp();

  // create measurements log entry
  MEMORY_SensorsMeasurementEntry_t sensorsMeasurementEntry = {
          .timestamp = timestamp,
          .rawTemperature = thSensActor->rawTemperature,
          .rawHumidity = thSensActor->rawHumidity,
          .rawLux = lightSensorActor->rawLux,
          .accelX = imuActor->lastAcceleration[0],
          .accelY = imuActor->lastAcceleration[1],
          .accelZ = imuActor->lastAcceleration[2],
          .reserved = 0
  };

  #ifdef DEBUG
  fprintf(stdout, "Log entry to write:\n timestamp: %ld\n rawTemperature: 0x%x\n rawHumidity: 0x%x\n rawLux: 0x%x\n accelX: 0x%x\n accelY: 0x%x\n accelZ: 0x%x\n lastFifoLevel: %d\n",
            sensorsMeasurementEntry.timestamp,
            sensorsMeasurementEntry.rawTemperature,
            sensorsMeasurementEntry.rawHumidity,
            sensorsMeasurementEntry.rawLux,
            sensorsMeasurementEntry.accelX,
            sensorsMeasurementEntry.accelY,
            sensorsMeasurementEntry.accelZ,
            imuActor->lastFifoLevel & 0x000000FF);
  #endif

  // write measurements to the memory
  #ifdef FLASH_WRITE_ENABLED
  ioStatus = W25Q_WriteData(&MEMORY_W25QHandle, (uint8_t *) &sensorsMeasurementEntry, this->logFileTailAddress, MEMORY_LOG_ENTRY_SIZE);
  #endif

  // increment tail free space address for the next entry
  this->logFileTailAddress += MEMORY_LOG_ENTRY_SIZE;

  return ioStatus;
}
