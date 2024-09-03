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

extern actor_t* ACTORS_LIST_SystemRegistry[MAX_ACTORS];

extern USBD_StorageTypeDef USBD_Storage_Interface_fops_FS;

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

  HAL_StatusTypeDef status = HAL_OK;
  uint8_t id[W25Q_ID_SIZE] = {0x00, 0x00 };
  const char *dummyToWrite = "Hello, World!";
  uint8_t dummyToRead[16] = {0};

  status = W25Q_EraseSector(&MEMORY_W25QHandle, 0);
  if (status != HAL_OK) {
    fprintf(stderr, "memory error");
  }

  status = W25Q_WriteData(&MEMORY_W25QHandle, (uint8_t *)dummyToWrite, 0, 13);
  if (status != HAL_OK) {
    fprintf(stderr, "memory error");
  }

  status = W25Q_ReadData(&MEMORY_W25QHandle, dummyToRead, 0, 16);
  if (status != HAL_OK) {
    fprintf(stderr, "memory error");
  }

  status = W25Q_ReadStatusReg(&MEMORY_W25QHandle);

  // init usb msd TODO move to more suitable place


  #ifdef DEBUG
    fprintf(stdout, "Memory task initialized\n");
  #endif

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
  return osOK;
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