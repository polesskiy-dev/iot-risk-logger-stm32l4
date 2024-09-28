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
static osStatus_t writeFAT12BootSector(MEMORY_Actor_t *this);
static osStatus_t handleInit(MEMORY_Actor_t *this, message_t *message);
static osStatus_t handleSleep(MEMORY_Actor_t *this, message_t *message);
static osStatus_t handleWrite(MEMORY_Actor_t *this, message_t *message);

extern actor_t* ACTORS_LIST_SystemRegistry[MAX_ACTORS];
extern uint8_t FAT12_BootSector[FAT12_BOOT_SECTOR_SIZE];

extern USBD_StorageTypeDef USBD_Storage_Interface_fops_FS;

//uint8_t dummyToRead[512] = {0};

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

//  HAL_StatusTypeDef status = HAL_OK;
//  uint8_t id[W25Q_ID_SIZE] = {0x00, 0x00 };
//  const char *dummyToWrite = "Hello, World!";
//
//  status = W25Q_EraseSector(&MEMORY_W25QHandle, 0);
//  if (status != HAL_OK) {
//    fprintf(stderr, "memory error");
//  }
//
//  status = W25Q_WritePageData(&MEMORY_W25QHandle, (uint8_t *)dummyToWrite, 0, 13);
//  if (status != HAL_OK) {
//    fprintf(stderr, "memory error");
//  }
//
//  status = W25Q_ReadData(&MEMORY_W25QHandle, dummyToRead, 0, 16);
//  if (status != HAL_OK) {
//    fprintf(stderr, "memory error");
//  }
//
//  status = W25Q_ReadStatusReg(&MEMORY_W25QHandle);


  for (;;) {
    // Wait for messages from the queue
    if (osMessageQueueGet(MEMORY_Actor.super.osMessageQueueId, &msg, NULL, osWaitForever) == osOK) {
      osStatus_t status = MEMORY_Actor.super.messageHandler((actor_t *) &MEMORY_Actor, &msg);

      if (status != osOK) {
        osMessageQueueId_t evManagerQueue = ACTORS_LIST_SystemRegistry[EV_MANAGER_ACTOR_ID]->osMessageQueueId;
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

// TODO cover by unit tests, check could it be splitted into smaller functions
uint32_t MEMORY_SeekFreeSpaceFirstByteAddress(void) {
  uint8_t logEntry[MEMORY_LOG_ENTRY_SIZE] = {0};
  uint32_t startAddress = FAT12_BOOT_SECTOR_SIZE + 1;
  uint8_t stepSize = MEMORY_LOG_ENTRY_SIZE;
  uint32_t stepNumber = 0;
  bool logEntryHasData = true; // does read log entry contains data other than 0xFF (empty)

  // read until log entry has no data (0xFF)
  while (logEntryHasData) { // TODO check boundaries
    // read log entry chunk to buffer
    uint16_t offset = stepNumber * stepSize;
    W25Q_ReadData(&MEMORY_W25QHandle, logEntry, startAddress + offset, stepSize);

    // check if log entry has data
    for (uint8_t iLogEntry = 0; iLogEntry < MEMORY_LOG_ENTRY_SIZE; iLogEntry++) {
      if (logEntry[iLogEntry] != 0xFF) {
        logEntryHasData = true;
        break;
      }
      logEntryHasData = false; // just to mark algorithm end
      return startAddress + offset;
    }

    stepNumber++;
  };


}

// TODO move to more suitable place
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin == USB_VBUS_SENSE_Pin) {
    GPIO_PinState usbVBusPin = HAL_GPIO_ReadPin(USB_VBUS_SENSE_GPIO_Port, USB_VBUS_SENSE_Pin);
    osMessageQueueId_t evManagerQueue = ACTORS_LIST_SystemRegistry[EV_MANAGER_ACTOR_ID]->osMessageQueueId;

    if (usbVBusPin == GPIO_PIN_SET) {
      osMessageQueuePut(evManagerQueue, &(message_t) {USB_CONNECTED}, 0, 0);
      #if DEBUG
            fprintf(stdout, "USB connected\n");
      #endif
    } else {
      osMessageQueuePut(evManagerQueue, &(message_t) {USB_DISCONNECTED}, 0, 0);
      #if DEBUG
            fprintf(stdout, "USB disconnected\n");
      #endif
    }
  }

  if (GPIO_Pin == _NFC_INT_Pin) {
    #ifdef DEBUG
        fprintf(stdout, "NFC GPO Interrupt\n");
    #endif
    osMessageQueuePut(NFC_Actor.super.osMessageQueueId, &(message_t){NFC_GPO_INTERRUPT}, 0, 0);
  }
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
      fprintf(stderr, "memory error");
    #endif
    return status;
  }

  // flash FAT12 boot sector
  status = W25Q_WriteData(&MEMORY_W25QHandle, FAT12_BootSector, 0, FAT12_BOOT_SECTOR_SIZE);
  if (status != HAL_OK) {
    #ifdef DEBUG
      fprintf(stderr, "memory error");
    #endif
    return status;
  }

  return status;
}

static osStatus_t handleInit(MEMORY_Actor_t *this, message_t *message) {
  if (GLOBAL_CMD_INITIALIZE == message->event) {
    uint8_t norFlashID[W25Q_ID_SIZE] = {0x00, 0x00};

    // read ID
    osStatus_t ioStatus = W25Q_ReadID(&MEMORY_W25QHandle, norFlashID);
    if (ioStatus != osOK) return osError;

    #ifdef DEBUG
        fprintf(stdout, "W25Q NOR Flash ID: %x%x\n", norFlashID[0], norFlashID[1]);
    #endif

    #ifdef ERASE_CHIP_AND_FLASH_FAT12_BOOT_SECTOR
        writeFAT12BootSector(&MEMORY_Actor);
    #endif

    // find first free space address
    uint32_t freeSpaceAddress = MEMORY_SeekFreeSpaceFirstByteAddress();
    MEMORY_Actor.logFileTailAddress = freeSpaceAddress;

    // put memory to sleep
    ioStatus = W25Q_Sleep(&MEMORY_W25QHandle);
    if (ioStatus != osOK) return osError;

    // publish to event manager that memory is initialized
    osMessageQueueId_t evManagerQueue = ACTORS_LIST_SystemRegistry[EV_MANAGER_ACTOR_ID]->osMessageQueueId;
    osMessageQueuePut(evManagerQueue, &(message_t){GLOBAL_INITIALIZE_SUCCESS, .payload.value = MEMORY_ACTOR_ID}, 0, 0);

    #ifdef DEBUG
        fprintf(stdout, "First free space address: %lu\n", freeSpaceAddress);
        fprintf(stdout, "Memory task initialized\n");
    #endif

    TO_STATE(this, MEMORY_SLEEP_STATE);
    return osOK;
  }
};

static osStatus_t handleSleep(MEMORY_Actor_t *this, message_t *message) {
  switch (message->event) {
    case GLOBAL_TEMPERATURE_HUMIDITY_MEASUREMENTS_READY:
      // set appropriate event flag
      osEventFlagsSet(measurementsReadyEventFlags, TEMPERATURE_HUMIDITY_MEASUREMENTS_READY_EVENT_FLAG);

      // if both measurements are ready, emit write to the memory message
      if (osEventFlagsGet(measurementsReadyEventFlags) == (TEMPERATURE_HUMIDITY_MEASUREMENTS_READY_EVENT_FLAG | LIGHT_MEASUREMENTS_READY_EVENT_FLAG)) {
        osEventFlagsClear(measurementsReadyEventFlags, TEMPERATURE_HUMIDITY_MEASUREMENTS_READY_EVENT_FLAG | LIGHT_MEASUREMENTS_READY_EVENT_FLAG);
        osMessageQueuePut(this->super.osMessageQueueId, &(message_t) {MEMORY_MEASUREMENTS_WRITE}, 0, 0);
      }

      TO_STATE(this, MEMORY_SLEEP_STATE);
      return osOK;

    case GLOBAL_LIGHT_MEASUREMENTS_READY:
      // set appropriate event flag
      osEventFlagsSet(measurementsReadyEventFlags, LIGHT_MEASUREMENTS_READY_EVENT_FLAG);

      // if both measurements are ready, emit write to the memory message
      if (osEventFlagsGet(measurementsReadyEventFlags) == (TEMPERATURE_HUMIDITY_MEASUREMENTS_READY_EVENT_FLAG | LIGHT_MEASUREMENTS_READY_EVENT_FLAG)) {
        osEventFlagsClear(measurementsReadyEventFlags, TEMPERATURE_HUMIDITY_MEASUREMENTS_READY_EVENT_FLAG | LIGHT_MEASUREMENTS_READY_EVENT_FLAG);
        osMessageQueuePut(this->super.osMessageQueueId, &(message_t) {MEMORY_MEASUREMENTS_WRITE}, 0, 0);
      }

      TO_STATE(this, MEMORY_SLEEP_STATE);
      return osOK;

    default:
      TO_STATE(this, MEMORY_SLEEP_STATE);
      return osOK;
  }
};

static osStatus_t handleWrite(MEMORY_Actor_t *this, message_t *message) {
  osStatus_t ioStatus;

  // sensors actors pointers from the system registry
  TH_SENS_Actor_t *thSensActor = (TH_SENS_Actor_t *)ACTORS_LIST_SystemRegistry[TEMPERATURE_HUMIDITY_SENSOR_ACTOR_ID];
  LIGHT_SENS_Actor_t *lightSensorActor = (LIGHT_SENS_Actor_t *)ACTORS_LIST_SystemRegistry[LIGHT_SENSOR_ACTOR_ID];

  // current timestamp in UNIX format
  int32_t timestamp = CRON_GetCurrentUnixTimestamp();

  // create measurements log entry
  MEMORY_SensorsMeasurementEntry_t sensorsMeasurementEntry = {
          .timestamp = timestamp,
          .rawTemperature = thSensActor->rawTemperature,
          .rawHumidity = thSensActor->rawHumidity,
          .rawLux = lightSensorActor->rawLux,
          .reserved = 0x00000000
  };

  switch (message->event) {
    case MEMORY_MEASUREMENTS_WRITE:
      #ifdef DEBUG
            fprintf(stdout, "Log entry to write:\n timestamp: %ld\n rawTemperature: %d\n rawHumidity: %d\n rawLux: %d\n reserved: %ld\n",
            sensorsMeasurementEntry.timestamp,
            sensorsMeasurementEntry.rawTemperature,
            sensorsMeasurementEntry.rawHumidity,
            sensorsMeasurementEntry.rawLux,
            sensorsMeasurementEntry.reserved);
            fprintf(stdout, "Memory task initialized\n");
      #endif

      // TODO write measurements to the memory
//      ioStatus = W25Q_WriteData(&MEMORY_W25QHandle, (uint8_t *) &sensorsMeasurementEntry, this->logFileTailAddress, MEMORY_LOG_ENTRY_SIZE);
//      this->logFileTailAddress += MEMORY_LOG_ENTRY_SIZE; // increment tail free space address for the next entry
//      if (ioStatus != osOK) return osError;

      // publish to event manager that measurements has been written, ev manager should publish it back to the memory task
      osMessageQueueId_t evManagerQueue = ACTORS_LIST_SystemRegistry[EV_MANAGER_ACTOR_ID]->osMessageQueueId;
      osMessageQueuePut(evManagerQueue, &(message_t){GLOBAL_MEASUREMENTS_WRITE_SUCCESS}, 0, 0);

      TO_STATE(this, MEMORY_WRITE_STATE);
      return osOK;

    case GLOBAL_MEASUREMENTS_WRITE_SUCCESS:
      ioStatus = W25Q_Sleep(&MEMORY_W25QHandle);
      if (ioStatus != osOK) return osError;

      TO_STATE(this, MEMORY_SLEEP_STATE);
      return osOK;

    default:
      TO_STATE(this, MEMORY_WRITE_STATE);
      return osOK;
  }
}