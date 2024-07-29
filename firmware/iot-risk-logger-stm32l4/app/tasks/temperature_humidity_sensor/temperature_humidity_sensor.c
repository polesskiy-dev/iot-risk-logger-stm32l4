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
#include "sht3x_i2c.h"
#include "custom_bus.h"

static osStatus_t handleTHSensorMessage(TH_SENS_Actor_t *this, message_t *message);
static osStatus_t initTHSensor(TH_SENS_Actor_t *this, message_t *message);
static osStatus_t startSingleShotRead(TH_SENS_Actor_t *this, message_t *message);

TH_SENS_Actor_t TH_SENS_Actor = {
        .super = {
                .actorId = TEMPERATURE_HUMIDITY_SENSOR_ACTOR_ID,
                .messageHandler = (messageHandler_t) handleTHSensorMessage,
                .osMessageQueueId = NULL,
                .osThreadId = NULL,
        },
        .state = TH_SENS_INIT_STATE
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
      osStatus_t status = TH_SENS_Actor.super.messageHandler((actor_t *) &TH_SENS_Actor, &msg);
      if (status != osOK) {
        // TODO Handle error, emit common error event and reinitialize module
        TH_SENS_Actor.state = TH_SENS_STATE_ERROR;
      }
    }
  }
}

static osStatus_t handleTHSensorMessage(TH_SENS_Actor_t *this, message_t *message) {
  switch (this->state) {
    case TH_SENS_INIT_STATE:
      return initTHSensor(this, message);
    case TH_SENS_READY_TO_READ_STATE:
      return startSingleShotRead(this, message);
//    case TH_SENS_MEASURE_WAIT_STATE:
//      return collectMeasurements(this, message);
    case TH_SENS_STATE_ERROR:
    // TODO handle reinit after error
    default:
      return osError;
  }
}

static osStatus_t initTHSensor(TH_SENS_Actor_t *this, message_t *message) {
  if (this->state == TH_SENS_INIT_STATE) {
    BSP_I2C1_Init(); // TODO think about proper place to init I2C
    // TODO soft reset of the sensor by pulling down _TEMP_RESET for 1uS minimum
    sht3x_init(SHT31_I2C_ADDR_44 << 1);
    osMessageQueuePut(TH_SENS_Actor.super.osMessageQueueId, &(message_t){TH_SENS_START_SINGLE_SHOT_READ}, 0, 0);
    // TODO check sensor ID
    this->state = TH_SENS_READY_TO_READ_STATE;
    SEGGER_RTT_printf(0, "temperature humidity sensor initialized\n");
    return osOK;
  }
  return osError;
}

static osStatus_t startSingleShotRead(TH_SENS_Actor_t *this, message_t *message) {
  if (TH_SENS_START_SINGLE_SHOT_READ == message->event) {
    osStatus_t status = sht3x_measure_single_shot(REPEATABILITY_MEDIUM, false, &this->temperature, &this->humidity);
    if (status != osOK) return osError;

    SEGGER_RTT_printf(0, "temperature: %d humidity %d\n", this->temperature, this->humidity);

    osDelay(5000);
    osMessageQueuePut(TH_SENS_Actor.super.osMessageQueueId, &(message_t){TH_SENS_START_SINGLE_SHOT_READ}, 0, 0);

    this->state = TH_SENS_READY_TO_READ_STATE;
  }

  return osOK;
}
