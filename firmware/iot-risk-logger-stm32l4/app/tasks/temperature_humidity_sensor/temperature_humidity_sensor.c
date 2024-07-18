/*!
 * @file temperature_humidity_sensor.c
 * @brief implementation of temperature_humidity_sensor
 *
 * Detailed description of the implementation file.
 *
 * @date 17/07/2024
 * @author artempolisskyi
 */

#include "temperature_humidity_sensor.h"

static osStatus_t handleTHSensorMessage(TH_SENS_Actor_t *this, message_t *message);
static osStatus_t initTHSensor(TH_SENS_Actor_t *this);
static osStatus_t readTHSensor(TH_SENS_Actor_t *this);

TH_SENS_Actor_t TH_SENS_Actor = {
        .super = {
                .actorId = TEMPERATURE_HUMIDITY_SENSOR_ACTOR_ID,
                .messageHandler = (messageHandler_t) handleTHSensorMessage,
                .osMessageQueueId = NULL,
                .osThreadId = NULL,
        },
        .state = TH_SENS_STATE_INIT
};

const osThreadAttr_t thSensorTaskDescription = {
        .name = "thSensorTask",
        .priority = osPriorityNormal,
        .stack_size = DEFAULT_TASK_STACK_SIZE
};

void TH_SENS_TaskInit(void) {
  TH_SENS_Actor.super.osMessageQueueId = osMessageQueueNew(DEFAULT_QUEUE_SIZE, DEFAULT_QUEUE_MESSAGE_SIZE, &(osMessageQueueAttr_t){
          .name = "thSensorQueue"
  });
  TH_SENS_Actor.super.osThreadId = osThreadNew(TH_SENS_Task, NULL, &thSensorTaskDescription);
 }

void TH_SENS_Task(void *argument) {
  (void) argument; // Avoid unused parameter warning
  message_t msg;

  osMessageQueuePut(TH_SENS_Actor.super.osMessageQueueId, &(message_t){TH_SENS_INITIALIZE}, 0, 0);

  for (;;) {
    // Wait for messages from the queue
    if (osMessageQueueGet(TH_SENS_Actor.super.osMessageQueueId, &msg, NULL, osWaitForever) == osOK) {
      TH_SENS_Actor.super.messageHandler((actor_t *) &TH_SENS_Actor, &msg);
    }
  }
}

static osStatus_t handleTHSensorMessage(TH_SENS_Actor_t *this, message_t *message) {
  switch (message->event) {
    case TH_SENS_INITIALIZE:
      initTHSensor(this);
      return osOK;
    case RTC_CRON_READ_SENSORS:
      // TODO
      return osOK;
    case TH_SENS_DATA_SHOULD_BE_READY:
      // TODO
      return osOK;
    default:
      // Handle unknown message
      return osError;
  }
}

static osStatus_t initTHSensor(TH_SENS_Actor_t *this) {
  if (this->state == TH_SENS_STATE_INIT) {
    // TODO check sensor ID
    this->state = TH_SENS_STATE_READY;
    SEGGER_RTT_printf(0, "temperature humidity sensor initialized\n");
    return osOK;
  }
  return osError;
}

static osStatus_t readTHSensor(TH_SENS_Actor_t *this) {
  switch (this->state) {
    case TH_SENS_STATE_READY:
    case TH_SENS_DATA_READY_WAIT:
      // TODO read sensor data
      return osOK;
    default:
      return osError;
  }
}