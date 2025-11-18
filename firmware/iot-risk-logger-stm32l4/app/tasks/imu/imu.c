/*!
 * @file imu.c
 * @brief implementation of imu
 *
 * Detailed description of the implementation file.
 *
 * @date 19/11/2025
 * @author artempolisskyi
 */

#include "imu.h"

static osStatus_t handleImuFSM(IMU_Actor_t *this, message_t *message);

/** states handlers */
static osStatus_t handleInit(IMU_Actor_t *this, message_t *message);

extern actor_t *ACTORS_LOOKUP_SystemRegistry[MAX_ACTORS];

/**
 * @brief IMU Accelerometer actor struct
 * @extends actor_t
 */
IMU_Actor_t IMU_Actor = {
  .super = {
    .actorId = IMU_ACTOR_ID,
    .messageHandler = (messageHandler_t) handleImuFSM,
    .osMessageQueueId = NULL,
    .osThreadId = NULL,
  },
  .state = IMU_NO_STATE,
};

// task description required for static task creation
uint32_t imuTaskBuffer[DEFAULT_TASK_STACK_SIZE_WORDS];
StaticTask_t imuTaskControlBlock;
const osThreadAttr_t imuTaskDescription = {
        .name = "imuTask",
        .cb_mem = &imuTaskControlBlock,
        .cb_size = sizeof(imuTaskControlBlock),
        .stack_mem = &imuTaskBuffer[0],
        .stack_size = sizeof(imuTaskBuffer),
        .priority = (osPriority_t) osPriorityNormal,
};

/**
 * @brief Initializes the IMU task.
 * @return {actor_t*} - pointer to the actor base struct
 */
actor_t* IMU_TaskInit(void) {
  IMU_Actor.super.osMessageQueueId = osMessageQueueNew(DEFAULT_QUEUE_SIZE, DEFAULT_QUEUE_MESSAGE_SIZE, &(osMessageQueueAttr_t){
          .name = "imuQueue"
  });
  IMU_Actor.super.osThreadId = osThreadNew(IMU_Task, NULL, &imuTaskDescription);

  return &IMU_Actor.super;
}

/**
 * @brief Light Sensor task
 * Waits for a message from the queue and processes it in FSM
 * Enters ERROR state if message handling failed
 */
void IMU_Task(void *argument) {
  (void) argument; // Avoid unused parameter warning
  message_t msg;

  fprintf(stdout, "Task %s started\n", imuTaskDescription.name);

  for (;;) {
    // Wait for messages from the queue
    if (osMessageQueueGet(IMU_Actor.super.osMessageQueueId, &msg, NULL, osWaitForever) == osOK) {
      osStatus_t handleMessageStatus = IMU_Actor.super.messageHandler((actor_t *) &IMU_Actor, &msg);

      if (handleMessageStatus != osOK) {
        fprintf(stderr,  "%s: Error handling event %u in state %u\n", imuTaskDescription.name, msg.event, IMU_Actor.state);
        osMessageQueueId_t evManagerQueue = ACTORS_LOOKUP_SystemRegistry[EV_MANAGER_ACTOR_ID]->osMessageQueueId;
        osMessageQueuePut(evManagerQueue, &(message_t){GLOBAL_ERROR, .payload.value = IMU_ACTOR_ID}, 0, 0);
        TO_STATE(&IMU_Actor, IMU_STATE_ERROR);
      }
    }
  }
}

static osStatus_t handleImuFSM(IMU_Actor_t *this, message_t *message) {
  switch (this->state) {
    case IMU_NO_STATE:
      return handleInit(this, message);
    case IMU_STATE_IDLE:
    case IMU_STATE_ERROR:
      // TODO implement other states
      return osOK;
  }

  return osOK;
}

/**
 * @brief Initializes the sensor, configures I2C, writes default settings, and transitions to the TURNED_OFF state
 */
static osStatus_t handleInit(IMU_Actor_t *this, message_t *message) {
  if (GLOBAL_CMD_INITIALIZE == message->event) {
    // init the driver (io)
    // TODO osStatus_t ioStatus =

    // if (ioStatus != osOK) return osError;

    // TODO
    // test read device id, should be 0x44 (LIS2DW12_ID)
    // ioStatus =

    // if (ioStatus != osOK) return osError;


    #ifdef DEBUG
      // fprintf(stdout, "LIS2DW ID: %x\n", lis2dw);
    #endif

    // publish to event manager that the IMU is initialized
    osMessageQueueId_t evManagerQueue = ACTORS_LOOKUP_SystemRegistry[EV_MANAGER_ACTOR_ID]->osMessageQueueId;
    osMessageQueuePut(evManagerQueue, &(message_t){GLOBAL_INITIALIZE_SUCCESS, .payload.value = IMU_ACTOR_ID}, 0, 0);

    fprintf(stdout, "IMU %u initialized\n", IMU_ACTOR_ID);
    TO_STATE(this, IMU_STATE_IDLE);
  }

  return osOK;
}