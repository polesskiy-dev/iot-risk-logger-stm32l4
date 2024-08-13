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

static osStatus_t handleMemoryFSM(MEMORY_Actor_t *this, message_t *message);

MEMORY_Actor_t MEMORY_Actor = {
        .super = {
                .actorId = MEMORY_ACTOR_ID,
                .messageHandler = (messageHandler_t) handleMemoryFSM,
                .osMessageQueueId = NULL,
                .osThreadId = NULL,
        },
        .state = MEMORY_NO_STATE
};

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

actor_t* MEMORY_TaskInit(void) {
  MEMORY_Actor.super.osMessageQueueId = osMessageQueueNew(DEFAULT_QUEUE_SIZE, DEFAULT_QUEUE_MESSAGE_SIZE, &(osMessageQueueAttr_t){
          .name = "memoryQueue"
  });
  MEMORY_Actor.super.osThreadId = osThreadNew(MEMORY_Task, NULL, &memoryTaskDescription);

  // TODO move to driver

  QSPI_CommandTypeDef com;

  com.InstructionMode = QSPI_INSTRUCTION_1_LINE; // QSPI_INSTRUCTION_...
  com.Instruction = W25Q_POWERDOWN;	 // Command

  com.AddressMode = QSPI_ADDRESS_NONE;
  com.AddressSize = QSPI_ADDRESS_NONE;
  com.Address = 0x0U;

  com.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  com.AlternateBytes = QSPI_ALTERNATE_BYTES_NONE;
  com.AlternateBytesSize = QSPI_ALTERNATE_BYTES_NONE;

  com.DummyCycles = 0;
  com.DataMode = QSPI_DATA_NONE;
  com.NbData = 0;

  com.DdrMode = QSPI_DDR_MODE_DISABLE;
  com.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
  com.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  HAL_StatusTypeDef status = HAL_QSPI_Command(&hqspi, &com, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  fprintf(stdout, "Memory put to sleep status: %u\n", status);

  return &MEMORY_Actor.super;
}

void MEMORY_Task(void *argument) {
  (void) argument; // Avoid unused parameter warning
  message_t msg;

  fprintf(stdout, "Memory task initialized\n");

  for (;;) {
    // Wait for messages from the queue
    if (osMessageQueueGet(MEMORY_Actor.super.osMessageQueueId, &msg, NULL, osWaitForever) == osOK) {
      osStatus_t status = MEMORY_Actor.super.messageHandler((actor_t *) &MEMORY_Actor, &msg);
      if (status != osOK) {
        // TODO Handle error, emit common error event and reinitialize module
        MEMORY_Actor.state = MEMORY_STATE_ERROR;
      }
    }
  }
}

static osStatus_t handleMemoryFSM(MEMORY_Actor_t *this, message_t *message) {
  return osOK;
}