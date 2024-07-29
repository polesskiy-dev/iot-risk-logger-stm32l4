/*!
 * @file light_sensor.c
 * @brief implementation of light_sensor
 *
 * Detailed description of the implementation file.
 *
 * @date 28/07/2024
 * @author artempolisskyi
 */

#include "light_sensor.h"

static osStatus_t handleLightSensorMessage(LIGHT_SENS_Actor_t *this, message_t *message);

LIGHT_SENS_Actor_t LIGHT_SENS_Actor = {
        .super = {
                .actorId = LIGHT_SENSOR_ACTOR_ID,
                .messageHandler = (messageHandler_t) handleLightSensorMessage,
                .osMessageQueueId = NULL,
                .osThreadId = NULL,
        },
        .state = LIGHT_SENS_INIT_STATE
};

const osThreadAttr_t lightSensorTaskDescription = {
        .name = "lightSensorTask",
        .priority = osPriorityNormal,
        .stack_size = DEFAULT_TASK_STACK_SIZE
};

void LIGHT_SENS_TaskInit(void) {
  LIGHT_SENS_Actor.super.osMessageQueueId = osMessageQueueNew(DEFAULT_QUEUE_SIZE, DEFAULT_QUEUE_MESSAGE_SIZE, &(osMessageQueueAttr_t){
          .name = "lightSensorQueue"
  });
  LIGHT_SENS_Actor.super.osThreadId = osThreadNew(LIGHT_SENS_Task, NULL, &lightSensorTaskDescription);
}

void LIGHT_SENS_Task(void *argument) {
  (void) argument; // Avoid unused parameter warning
  message_t msg;

  osMessageQueuePut(LIGHT_SENS_Actor.super.osMessageQueueId, &(message_t){LIGHT_SENS_INITIALIZE}, 0, 0);

  for (;;) {
    // Wait for messages from the queue
    if (osMessageQueueGet(LIGHT_SENS_Actor.super.osMessageQueueId, &msg, NULL, osWaitForever) == osOK) {
      osStatus_t status = LIGHT_SENS_Actor.super.messageHandler((actor_t *) &LIGHT_SENS_Actor, &msg);
      if (status != osOK) {
        // TODO Handle error, emit common error event and reinitialize module
        LIGHT_SENS_Actor.state = LIGHT_SENS_STATE_ERROR;
      }
    }
  }
}

static osStatus_t handleLightSensorMessage(LIGHT_SENS_Actor_t *this, message_t *message) {
  // TODO implement me
  return osOK;
}