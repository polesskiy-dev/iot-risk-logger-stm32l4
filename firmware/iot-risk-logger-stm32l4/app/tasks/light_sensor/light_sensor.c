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

static osStatus_t handleMessageFSM(LIGHT_SENS_Actor_t *this, message_t *message);
static osStatus_t handleInit(LIGHT_SENS_Actor_t *this, message_t *message);
static osStatus_t handleReadyWait(LIGHT_SENS_Actor_t *this, message_t *message);

LIGHT_SENS_Actor_t LIGHT_SENS_Actor = {
        .super = {
                .actorId = LIGHT_SENSOR_ACTOR_ID,
                .messageHandler = (messageHandler_t) handleMessageFSM,
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

  // TODO run in global init manager
  osMessageQueuePut(LIGHT_SENS_Actor.super.osMessageQueueId, &(message_t){LIGHT_SENS_INITIALIZE}, 0, 0);
  // debug check
  osMessageQueuePut(LIGHT_SENS_Actor.super.osMessageQueueId, &(message_t){LIGHT_SENS_SINGLE_SHOT_READ}, 0, 0);

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

static osStatus_t handleMessageFSM(LIGHT_SENS_Actor_t *this, message_t *message) {
  switch (this->state) {
    case LIGHT_SENS_INIT_STATE:
      return handleInit(this, message);
    case LIGHT_SENS_READY_WAIT_STATE:
      return handleReadyWait(this, message);
    // TODO case LIGHT_SENS_TURNED_OFF_STATE:
  }

  return osError;
}

static osStatus_t handleInit(LIGHT_SENS_Actor_t *this, message_t *message) {
  if (LIGHT_SENS_INITIALIZE == message->event) {
    // init BSP I2C
    BSP_I2C1_Init(); // TODO think about proper place to init I2C

    // init the driver (io)
    osStatus_t status = OPT3001_InitIO(OPT3001_I2C_ADDRESS, BSP_I2C1_WriteReg, BSP_I2C1_ReadReg);

    if (status != osOK) return osError;

    // test read device id
    uint16_t opt3001Id = 0x0000;
    status = OPT3001_ReadDeviceID(&opt3001Id);

    if (status != osOK) return osError;
    SEGGER_RTT_printf(1, "OPT3001 ID: %x\n", opt3001Id);


//    uint16_t opt3001Config =  0x0000 | \
//                              OPT3001_CONFIG_RANGE_NUMBER_AUTO_SCALE | \
//                              OPT3001_CONFIG_CONVERSION_TIME_800_MS  | \
//                              OPT3001_CONFIG_MODE_CONTINUOUS         | \
//                              OPT3001_CONFIG_LATCH_ENABLED           | \
//                              OPT3001_CONFIG_FAULT_COUNT_4;

    // write default config (OPT3001 remains in turned off state)
    uint16_t opt3001Config = OPT3001_CONFIG_DEFAULT;

    status = OPT3001_WriteConfig(opt3001Config);

    if (status != osOK) return osError;
    SEGGER_RTT_printf(1, "Write OPT3001 Config: %x\n", opt3001Config);

    // read existing config to verify equality
    uint16_t opt3001ConfigFromSensor = 0x0000;
    status = OPT3001_ReadConfig(&opt3001ConfigFromSensor);

    if (status != osOK) return osError;
    SEGGER_RTT_printf(1, "OPT3001 Config: %x\n", opt3001ConfigFromSensor);

    if (opt3001Config != opt3001ConfigFromSensor) {
      SEGGER_RTT_printf(1, "OPT3001 Config mismatch\n");
      return osError;
    }

    this->state = LIGHT_SENS_READY_WAIT_STATE;
  }

  return osOK;
}

static osStatus_t handleReadyWait(LIGHT_SENS_Actor_t *this, message_t *message) {
  osStatus_t status;

  switch (message->event) {
    case LIGHT_SENS_SINGLE_SHOT_READ:
      // start single shot read
      status = OPT3001_WriteConfig(OPT3001_CONFIG_RANGE_NUMBER_AUTO_SCALE | \
                                    OPT3001_CONFIG_CONVERSION_TIME_800_MS  | \
                                    OPT3001_CONFIG_MODE_SINGLE_SHOT        | \
                                    OPT3001_CONFIG_LATCH_ENABLED);

      if (status != osOK) return osError;

      // wait for the sensor to finish the measurement
      osDelay(1000); // TODO replace with 800ms

      // read the measured rawLux
      uint16_t rawLux = 0x0000;
      status = OPT3001_ReadResultRawLux(&rawLux);

      if (status != osOK) return osError;

      // convert rawLux to lux for debug
      this->lux = OPT3001_RawToMilliLux(rawLux);
      SEGGER_RTT_printf(1, "OPT3001 milli Lux: %d\n", this->lux);

      // remains in ready state after successful read
      this->state = LIGHT_SENS_READY_WAIT_STATE;
      return osOK;
    case LIGHT_SENS_CONTINUOUS_READ:
      // start continuous read
      status = OPT3001_WriteConfig(OPT3001_CONFIG_DEFAULT | OPT3001_CONFIG_MODE_CONTINUOUS | OPT3001_CONFIG_FAULT_COUNT_4);

      if (status != osOK) return osError;

      this->state = LIGHT_SENS_CONTINUOUS_READ_WAIT_THRESHOLD_STATE;
      return osOK;
    case LIGHT_SENS_TURN_OFF:
      // turn off the sensor
      status = OPT3001_WriteConfig(OPT3001_CONFIG_DEFAULT | OPT3001_CONFIG_MODE_SHUTDOWN);

      if (status != osOK) return osError;

      this->state = LIGHT_SENS_TURNED_OFF_STATE;
      return osOK;
    default:
      return osError;
  }
}