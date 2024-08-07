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

// state handlers
static osStatus_t handleMessageFSM(LIGHT_SENS_Actor_t *this, message_t *message);
static osStatus_t handleInit(LIGHT_SENS_Actor_t *this, message_t *message);
static osStatus_t handleTurnedOff(LIGHT_SENS_Actor_t *this, message_t *message);
static osStatus_t handleContinuousMeasure(LIGHT_SENS_Actor_t *this, message_t *message);
static osStatus_t handleOutOfRange(LIGHT_SENS_Actor_t *this, message_t *message);

LIGHT_SENS_Actor_t LIGHT_SENS_Actor = {
        .super = {
                .actorId = LIGHT_SENSOR_ACTOR_ID,
                .messageHandler = (messageHandler_t) handleMessageFSM,
                .osMessageQueueId = NULL,
                .osThreadId = NULL,
        },
        .state = LIGHT_SENS_NO_STATE,
        .rawLux = 0x0000,
        .highLimit = OPT3001_CONFIG_LIMIT_MAX
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

  SEGGER_SYSVIEW_PrintfTarget("Light Sensor initialized\n");

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
    case LIGHT_SENS_NO_STATE:
      return handleInit(this, message);
    case LIGHT_SENS_TURNED_OFF_STATE:
      return handleTurnedOff(this, message);
    case LIGHT_SENS_CONTINUOUS_MEASURE_STATE:
      return handleContinuousMeasure(this, message);
    case LIGHT_SENS_OUT_OF_RANGE_STATE:
      return handleOutOfRange(this, message);
  }

  return osError;
}
/**
 * @brief Initializes the sensor, configures I2C, writes default settings, and transitions to the TURNED_OFF state
 */
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
    SEGGER_SYSVIEW_PrintfTarget("OPT3001 ID: %x\n", opt3001Id);

    // write default config (OPT3001 remains in turned off state)
    uint16_t opt3001Config = OPT3001_CONFIG_DEFAULT;

    status = OPT3001_WriteConfig(opt3001Config);

    if (status != osOK) return osError;
    SEGGER_SYSVIEW_PrintfTarget("Write OPT3001 Config: %x\n", opt3001Config);

    // read existing config to verify equality
    uint16_t opt3001ConfigFromSensor = 0x0000;
    status = OPT3001_ReadConfig(&opt3001ConfigFromSensor);

    if (status != osOK) return osError;
    SEGGER_SYSVIEW_PrintfTarget("OPT3001 Config: %x\n", opt3001ConfigFromSensor);

    if (opt3001Config != opt3001ConfigFromSensor) {
      SEGGER_SYSVIEW_PrintfTarget("OPT3001 Config mismatch\n");
      return osError;
    }

    // TODO read it from NOR flash e.g. emit LIGHT_SENS_INITIALIZE_SUCCESS to global events manager
    // set high limit and minimal low limit so that the sensor never triggers the interrupt on low limit
    status = OPT3001_WriteHighLimit(this->highLimit)
    | OPT3001_WriteLowLimit(OPT3001_CONFIG_LIMIT_MIN);
    if (status != osOK) return osError;

    this->state = LIGHT_SENS_TURNED_OFF_STATE;
  }

  return osOK;
}

/**
 * @brief Perform single-shot measurements, set limits, or switch to continuous measurement mode.
 */
static osStatus_t handleTurnedOff(LIGHT_SENS_Actor_t *this, message_t *message) {
  osStatus_t status;

  switch (message->event) {
    case LIGHT_SENS_SINGLE_SHOT_READ:
      // set single shot mode
      status = OPT3001_WriteConfig(OPT3001_CONFIG_RANGE_NUMBER_AUTO_SCALE | \
                                    OPT3001_CONFIG_CONVERSION_TIME_800_MS | \
                                    OPT3001_CONFIG_MODE_SINGLE_SHOT | \
                                    OPT3001_CONFIG_LATCH_ENABLED);

      if (status != osOK) return osError;

      // wait for the sensor to finish the measurement
      osDelay(1000); // TODO replace with 800ms

      // read the measured rawLux
      status = OPT3001_ReadResultRawLux(&this->rawLux);

      if (status != osOK) return osError;

      // convert rawLux to lux for debug
      SEGGER_SYSVIEW_PrintfTarget("OPT3001 milli Lux: %d\n", OPT3001_RawToMilliLux(this->rawLux));

      // remains in turned off state after successful read, opt3001 turns off automatically after single shot read
      this->state = LIGHT_SENS_TURNED_OFF_STATE;
      return osOK;
    case LIGHT_SENS_MEASURE_CONTINUOUSLY:
      // set continuous measurements mode
      status = OPT3001_WriteConfig(OPT3001_CONFIG_RANGE_NUMBER_AUTO_SCALE | \
                                    OPT3001_CONFIG_CONVERSION_TIME_800_MS | \
                                    OPT3001_CONFIG_MODE_CONTINUOUS | \
                                    OPT3001_CONFIG_FAULT_COUNT_4 | \
                                    OPT3001_CONFIG_LATCH_ENABLED);

      if (status != osOK) return osError;

      this->state = LIGHT_SENS_CONTINUOUS_MEASURE_STATE;
      return osOK;
    case LIGHT_SENS_SET_LIMIT:
      // set high limit from message payload
      this->highLimit = message->payload.value;
      status = OPT3001_WriteHighLimit(this->highLimit);

      if (status != osOK) return osError;

      this->state = LIGHT_SENS_TURNED_OFF_STATE;
      return osOK;
  }

  return osOK;
};

/**
 * @brief Cron read sensor data periodically, handle limit interrupts, or turn off the sensor.
 */
static osStatus_t handleContinuousMeasure(LIGHT_SENS_Actor_t *this, message_t *message) {
  osStatus_t status;

  switch (message->event) {
    case LIGHT_SENS_CRON_READ:
      // read the measured rawLux on RTC cron
      status = OPT3001_ReadResultRawLux(&this->rawLux);
      if (status != osOK) return osError;

      this->state = LIGHT_SENS_CONTINUOUS_MEASURE_STATE;
      return osOK;
    case LIGHT_SENS_TURN_OFF:
      // turn off the sensor
      status = OPT3001_WriteConfig(OPT3001_CONFIG_DEFAULT | OPT3001_CONFIG_MODE_SHUTDOWN);

      if (status != osOK) return osError;

      this->state = LIGHT_SENS_TURNED_OFF_STATE;
      return osOK;
    case LIGHT_SENS_LIMIT_INT:
      // read the measured rawLux overvalue
      status = OPT3001_ReadResultRawLux(&this->rawLux);
      if (status != osOK) return osError;

      // TODO emit to event manager LIGHT_SENS_LIMIT_INT with this->rawLux payload

      // swap limits to wait for lux returning below the high limit
      status = OPT3001_WriteHighLimit(OPT3001_CONFIG_LIMIT_MAX)
      | OPT3001_WriteLowLimit(this->highLimit);
      if (status != osOK) return osError;

      this->state = LIGHT_SENS_OUT_OF_RANGE_STATE;
      return osOK;
  }

  return osOK;
}

/**
 * @brief Cron read sensor data, handle limit interrupts, or turn off the sensor.
 */
static osStatus_t handleOutOfRange(LIGHT_SENS_Actor_t *this, message_t *message) {
  osStatus_t status;

  switch (message->event) {
    case LIGHT_SENS_CRON_READ:
      // still read the measured rawLux on RTC cron
      status = OPT3001_ReadResultRawLux(&this->rawLux);
      if (status != osOK) return osError;

      this->state = LIGHT_SENS_OUT_OF_RANGE_STATE;
      return osOK;
    case LIGHT_SENS_TURN_OFF:
      // turn off the sensor
      status = OPT3001_WriteConfig(OPT3001_CONFIG_DEFAULT | OPT3001_CONFIG_MODE_SHUTDOWN);

      if (status != osOK) return osError;

      this->state = LIGHT_SENS_TURNED_OFF_STATE;
      return osOK;
    case LIGHT_SENS_LIMIT_INT:
      // read the measured rawLux OK overvalue
      status = OPT3001_ReadResultRawLux(&this->rawLux);
      if (status != osOK) return osError;
      // TODO emit to event manager LIGHT_SENS_LIMIT_INT with this->rawLux payload, handle that this is OK value

      // swap limits back to normal
      status = OPT3001_WriteHighLimit(this->highLimit)
               | OPT3001_WriteLowLimit(OPT3001_CONFIG_LIMIT_MIN);
      if (status != osOK) return osError;

      // back to continuous measure state
      this->state = LIGHT_SENS_CONTINUOUS_MEASURE_STATE;
      return osOK;
  }

  return osOK;
}