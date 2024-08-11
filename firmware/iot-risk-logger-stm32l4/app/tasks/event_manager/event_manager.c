/*!
 * @file event_manager.c
 * @brief implementation of event_manager
 *
 * Detailed description of the implementation file.
 *
 * @date 17/07/2024
 * @author artempolisskyi
 */

#include "event_manager.h"

static osStatus_t handleMessage(EV_MANAGER_Actor_t *this, message_t *message);

EV_MANAGER_Actor_t EV_MANAGER_Actor = {
        .super = {
                .actorId = EV_MANAGER_ACTOR_ID,
                .messageHandler = (messageHandler_t) handleMessage,
                .osMessageQueueId = NULL,
                .osThreadId = NULL,
        }
};

const osThreadAttr_t eventManagerTaskDescription = {
        .name = "eventManagerTask",
        .priority = osPriorityNormal,
        .stack_size = DEFAULT_TASK_STACK_SIZE
};

actor_t* EV_MANAGER_TaskInit(void) {
  EV_MANAGER_Actor.super.osMessageQueueId = osMessageQueueNew(DEFAULT_QUEUE_SIZE, DEFAULT_QUEUE_MESSAGE_SIZE, &(osMessageQueueAttr_t){
          .name = "eventManagerQueue"
  });
  EV_MANAGER_Actor.super.osThreadId = osThreadNew(EV_MANAGER_Task, NULL, &eventManagerTaskDescription);

  return &EV_MANAGER_Actor.super;
}
void EV_MANAGER_Task(void *argument) {
  (void) argument; // Avoid unused parameter warning
  message_t msg;

  SEGGER_SYSVIEW_PrintfTarget("Event Manager initialized\n");
  // TODO move to init manager
  osMessageQueuePut(EV_MANAGER_Actor.super.osMessageQueueId, &(message_t){GLOBAL_INITIALIZE}, 0, 0);

  for (;;) {
    // Wait for messages from the queue
    if (osMessageQueueGet(EV_MANAGER_Actor.super.osMessageQueueId, &msg, NULL, osWaitForever) == osOK) {
      // TODO verify correct execution?
      EV_MANAGER_Actor.super.messageHandler(&EV_MANAGER_Actor.super, &msg);
    }
  }
}

static osStatus_t handleMessage(EV_MANAGER_Actor_t *this, message_t *message) {
  switch (message->event) {
    case GLOBAL_INITIALIZE:
      // TODO handle initialize event
      // TODO remove from here
      osMessageQueuePut(EV_MANAGER_Actor.super.osMessageQueueId, &(message_t){GLOBAL_INITIALIZE_SUCCESS}, 0, 0);
      return osOK;
    case GLOBAL_INITIALIZE_SUCCESS:
      // TODO remove from here, emit only in NFC
      osMessageQueuePut(EV_MANAGER_Actor.super.osMessageQueueId, &(message_t){GLOBAL_CMD_START_CONTINUOUS_SENSING}, 0, 0);
      return osOK;
    case GLOBAL_CMD_SET_TIME_DATE:
    case GLOBAL_CMD_SET_WAKE_UP_PERIOD:
      return CRON_HandleMessageCMD(message);
    case GLOBAL_RTC_WAKE_UP:
      // TODO take from initializer
      osMessageQueuePut(LIGHT_SENS_Actor.super.osMessageQueueId, &(message_t){LIGHT_SENS_CRON_READ}, 0, 0);
      osMessageQueuePut(TH_SENS_Actor.super.osMessageQueueId, &(message_t){TH_SENS_CRON_READ}, 0, 0);
      return osOK;
    case GLOBAL_CMD_TURN_OFF:
      // TODO take from initializer
      osMessageQueuePut(LIGHT_SENS_Actor.super.osMessageQueueId, &(message_t){LIGHT_SENS_TURN_OFF}, 0, 0);
      osMessageQueuePut(TH_SENS_Actor.super.osMessageQueueId, &(message_t){TH_SENS_TURN_OFF}, 0, 0);
      return osOK;
    case GLOBAL_CMD_START_CONTINUOUS_SENSING:
      // TODO take from initializer
//      osMessageQueuePut(LIGHT_SENS_Actor.super.osMessageQueueId, &(message_t){LIGHT_SENS_MEASURE_CONTINUOUSLY}, 0, 0);
//      osMessageQueuePut(TH_SENS_Actor.super.osMessageQueueId, &(message_t){TH_SENS_MEASURE_CONTINUOUSLY}, 0, 0);
      return osOK;
  }

  return osOK;
}
