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

static osStatus_t handleLightSensorFSM(LIGHT_SENS_Actor_t *this, message_t *message);
/** states handlers */
static osStatus_t handleInit(LIGHT_SENS_Actor_t *this, message_t *message);
static osStatus_t handleTurnedOff(LIGHT_SENS_Actor_t *this, message_t *message);
static osStatus_t handleContinuousMeasure(LIGHT_SENS_Actor_t *this, message_t *message);
static osStatus_t handleOutOfRange(LIGHT_SENS_Actor_t *this, message_t *message);

extern actor_t* ACTORS_LIST_SystemRegistry[MAX_ACTORS];

LIGHT_SENS_Actor_t LIGHT_SENS_Actor = {
        .super = {
                .actorId = LIGHT_SENSOR_ACTOR_ID,
                .messageHandler = (messageHandler_t) handleLightSensorFSM,
                .osMessageQueueId = NULL,
                .osThreadId = NULL,
        },
        .state = LIGHT_SENS_NO_STATE,
        .rawLux = 0x0000,
        .highLimit = OPT3001_CONFIG_LIMIT_MAX
};

uint32_t lightSensorTaskBuffer[DEFAULT_TASK_STACK_SIZE_WORDS];
StaticTask_t lightSensorTaskControlBlock;
const osThreadAttr_t lightSensorTaskDescription = {
        .name = "lightSensorTask",
        .cb_mem = &lightSensorTaskControlBlock,
        .cb_size = sizeof(lightSensorTaskControlBlock),
        .stack_mem = &lightSensorTaskBuffer[0],
        .stack_size = sizeof(lightSensorTaskBuffer),
        .priority = (osPriority_t) osPriorityNormal,
};

actor_t* LIGHT_SENS_TaskInit(void) {
  LIGHT_SENS_Actor.super.osMessageQueueId = osMessageQueueNew(DEFAULT_QUEUE_SIZE, DEFAULT_QUEUE_MESSAGE_SIZE, &(osMessageQueueAttr_t){
          .name = "lightSensorQueue"
  });
  LIGHT_SENS_Actor.super.osThreadId = osThreadNew(LIGHT_SENS_Task, NULL, &lightSensorTaskDescription);

  return &LIGHT_SENS_Actor.super;
}

void LIGHT_SENS_Task(void *argument) {
  (void) argument; // Avoid unused parameter warning
  message_t msg;

  fprintf(stdout, "Task %s started\n", lightSensorTaskDescription.name);

  for (;;) {
    // Wait for messages from the queue
    if (osMessageQueueGet(LIGHT_SENS_Actor.super.osMessageQueueId, &msg, NULL, osWaitForever) == osOK) {
      osStatus_t handleMessageStatus = LIGHT_SENS_Actor.super.messageHandler((actor_t *) &LIGHT_SENS_Actor, &msg);

      if (handleMessageStatus != osOK) {
        fprintf(stderr, "%s: Error handling event %u in state %ul\n", lightSensorTaskDescription.name, msg.event, LIGHT_SENS_Actor.state);
        osMessageQueueId_t evManagerQueue = ACTORS_LIST_SystemRegistry[EV_MANAGER_ACTOR_ID]->osMessageQueueId;
        osMessageQueuePut(evManagerQueue, &(message_t){GLOBAL_ERROR, .payload.value = LIGHT_SENSOR_ACTOR_ID}, 0, 0);
        TO_STATE(&LIGHT_SENS_Actor, LIGHT_SENS_STATE_ERROR);
      }
    }
  }
}

static osStatus_t handleLightSensorFSM(LIGHT_SENS_Actor_t *this, message_t *message) {
  switch (this->state) {
    case LIGHT_SENS_NO_STATE:
      return osOK;
//      return handleInit(this, message);
    case LIGHT_SENS_TURNED_OFF_STATE:
      return handleTurnedOff(this, message);
    case LIGHT_SENS_CONTINUOUS_MEASURE_STATE:
      return handleContinuousMeasure(this, message);
    case LIGHT_SENS_OUT_OF_RANGE_STATE:
      return handleOutOfRange(this, message);
  }

  return osOK;
}
/**
 * @brief Initializes the sensor, configures I2C, writes default settings, and transitions to the TURNED_OFF state
 */
static osStatus_t handleInit(LIGHT_SENS_Actor_t *this, message_t *message) {
  if (GLOBAL_CMD_INITIALIZE == message->event) {
    // init the driver (io)
    osStatus_t ioStatus = OPT3001_InitIO(LIGHT_SENS_I2C_ADDRESS, BSP_I2C1_WriteReg, BSP_I2C1_ReadReg);

    if (ioStatus != osOK) return osError;

    // test read device id
    uint16_t opt3001Id = 0x0000;
    ioStatus = OPT3001_ReadDeviceID(&opt3001Id);

    if (ioStatus != osOK) return osError;
    fprintf(stdout, "OPT3001 ID: %x\n", opt3001Id);

    // write default config (OPT3001 remains in turned off state)
    uint16_t opt3001Config = OPT3001_CONFIG_DEFAULT;

    ioStatus = OPT3001_WriteConfig(opt3001Config);

    if (ioStatus != osOK) return osError;
    fprintf(stdout, "Write OPT3001 Config: %x\n", opt3001Config);

    // read existing config to verify equality
    uint16_t opt3001ConfigFromSensor = 0x0000;
    ioStatus = OPT3001_ReadConfig(&opt3001ConfigFromSensor);

    if (ioStatus != osOK) return osError;
    fprintf(stdout, "OPT3001 Config: %x\n", opt3001ConfigFromSensor);

    if (opt3001Config != opt3001ConfigFromSensor) {
      fprintf(stderr, "OPT3001 Config mismatch\n");
      return osError;
    }

    // TODO read it from NOR flash (implement settings manager)
    // set high limit and minimal low limit so that the sensor never triggers the interrupt on low limit
    ioStatus = OPT3001_WriteHighLimit(this->highLimit);
    if (ioStatus != osOK) return osError;

    ioStatus = OPT3001_WriteLowLimit(OPT3001_CONFIG_LIMIT_MIN);
    if (ioStatus != osOK) return osError;

    // publish to event manager that the sensor is initialized
    osMessageQueueId_t evManagerQueue = ACTORS_LIST_SystemRegistry[EV_MANAGER_ACTOR_ID]->osMessageQueueId;
    osMessageQueuePut(evManagerQueue, &(message_t){GLOBAL_INITIALIZE_SUCCESS, .payload.value = LIGHT_SENSOR_ACTOR_ID}, 0, 0);

    fprintf(stdout, "Light sensor %ul initialized\n", LIGHT_SENSOR_ACTOR_ID);
    TO_STATE(this, LIGHT_SENS_TURNED_OFF_STATE);
  }

  return osOK;
}

/**
 * @brief Perform single-shot measurements, set limits, or switch to continuous measurement mode.
 */
static osStatus_t handleTurnedOff(LIGHT_SENS_Actor_t *this, message_t *message) {
  osStatus_t ioStatus;

  switch (message->event) {
    case LIGHT_SENS_SINGLE_SHOT_READ:
      // set single shot mode
      ioStatus = OPT3001_WriteConfig(OPT3001_CONFIG_RANGE_NUMBER_AUTO_SCALE | \
                                    OPT3001_CONFIG_CONVERSION_TIME_800_MS | \
                                    OPT3001_CONFIG_MODE_SINGLE_SHOT | \
                                    OPT3001_CONFIG_LATCH_ENABLED);

      if (ioStatus != osOK) return osError;

      // wait for the sensor to finish the measurement
      osDelay(1000); // TODO replace with 800ms

      // read the measured rawLux
      ioStatus = OPT3001_ReadResultRawLux(&this->rawLux);

      if (ioStatus != osOK) return osError;

      // convert rawLux to lux for debug
      fprintf(stdout, "OPT3001 milli Lux: %ld\n", OPT3001_RawToMilliLux(this->rawLux));

      // remains in turned off state after successful read, opt3001 turns off automatically after single shot read
      TO_STATE(this, LIGHT_SENS_TURNED_OFF_STATE);
      return osOK;
    case LIGHT_SENS_MEASURE_CONTINUOUSLY:
      // set continuous measurements mode
      ioStatus = OPT3001_WriteConfig(OPT3001_CONFIG_RANGE_NUMBER_AUTO_SCALE | \
                                    OPT3001_CONFIG_CONVERSION_TIME_800_MS | \
                                    OPT3001_CONFIG_MODE_CONTINUOUS | \
                                    OPT3001_CONFIG_FAULT_COUNT_4 | \
                                    OPT3001_CONFIG_LATCH_ENABLED);

      if (ioStatus != osOK) return osError;

      TO_STATE(this, LIGHT_SENS_CONTINUOUS_MEASURE_STATE);
      return osOK;
    case LIGHT_SENS_SET_LIMIT:
      // set high limit from message payload
      this->highLimit = message->payload.value;
      ioStatus = OPT3001_WriteHighLimit(this->highLimit);

      if (ioStatus != osOK) return osError;

      TO_STATE(this, LIGHT_SENS_TURNED_OFF_STATE);
      return osOK;
  }

  return osOK;
};

/**
 * @brief Cron read sensor data periodically, handle limit interrupts, or turn off the sensor.
 */
static osStatus_t handleContinuousMeasure(LIGHT_SENS_Actor_t *this, message_t *message) {
  osStatus_t ioStatus;

  switch (message->event) {
    case GLOBAL_WAKE_N_READ:
      // read the measured rawLux on RTC cron
      ioStatus = OPT3001_ReadResultRawLux(&this->rawLux);
      if (ioStatus != osOK) return osError;

      TO_STATE(this, LIGHT_SENS_CONTINUOUS_MEASURE_STATE);
      return osOK;
    case LIGHT_SENS_TURN_OFF:
      // turn off the sensor
      ioStatus = OPT3001_WriteConfig(OPT3001_CONFIG_DEFAULT | OPT3001_CONFIG_MODE_SHUTDOWN);

      if (ioStatus != osOK) return osError;

      TO_STATE(this, LIGHT_SENS_TURNED_OFF_STATE);
      return osOK;
    case LIGHT_SENS_LIMIT_INT:
      // read the measured rawLux overvalue
      ioStatus = OPT3001_ReadResultRawLux(&this->rawLux);
      if (ioStatus != osOK) return osError;

      // TODO emit to event manager LIGHT_SENS_LIMIT_INT with this->rawLux payload

      // swap limits to wait for lux returning below the high limit
      ioStatus = OPT3001_WriteHighLimit(OPT3001_CONFIG_LIMIT_MAX)
      | OPT3001_WriteLowLimit(this->highLimit);
      if (ioStatus != osOK) return osError;

      TO_STATE(this, LIGHT_SENS_OUT_OF_RANGE_STATE);
      return osOK;
  }

  return osOK;
}

/**
 * @brief Cron read sensor data, handle limit interrupts, or turn off the sensor.
 */
static osStatus_t handleOutOfRange(LIGHT_SENS_Actor_t *this, message_t *message) {
  osStatus_t ioStatus;

  switch (message->event) {
    case GLOBAL_WAKE_N_READ:
      // still read the measured rawLux on RTC cron
      ioStatus = OPT3001_ReadResultRawLux(&this->rawLux);
      if (ioStatus != osOK) return osError;

      TO_STATE(this, LIGHT_SENS_OUT_OF_RANGE_STATE);
      return osOK;
    case LIGHT_SENS_TURN_OFF:
      // turn off the sensor
      ioStatus = OPT3001_WriteConfig(OPT3001_CONFIG_DEFAULT | OPT3001_CONFIG_MODE_SHUTDOWN);

      if (ioStatus != osOK) return osError;

      TO_STATE(this, LIGHT_SENS_TURNED_OFF_STATE);
      return osOK;
    case LIGHT_SENS_LIMIT_INT:
      // read the measured rawLux OK overvalue
      ioStatus = OPT3001_ReadResultRawLux(&this->rawLux);
      if (ioStatus != osOK) return osError;
      // TODO emit to event manager LIGHT_SENS_LIMIT_INT with this->rawLux payload, handle that this is OK value

      // swap limits back to normal
      ioStatus = OPT3001_WriteHighLimit(this->highLimit)
               | OPT3001_WriteLowLimit(OPT3001_CONFIG_LIMIT_MIN);
      if (ioStatus != osOK) return osError;

      // back to continuous measure state
      TO_STATE(this, LIGHT_SENS_CONTINUOUS_MEASURE_STATE);
      return osOK;
  }

  return osOK;
}
