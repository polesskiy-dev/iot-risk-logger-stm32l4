/*!
 * @file events_list.h
 * @brief Brief description of the file.
 *
 * Detailed description of the file.
 *
 * @date 17/07/2024
 * @author artempolisskyi
 */

#ifndef EVENTS_LIST_H
#define EVENTS_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

/**
 * @brief Event type
 * Contains all possible events for the application
 */
typedef enum {
  EVENT_NONE = 0,
  // INFO_LED
  INFO_LED_FLASH,
  // NFC
  NFC_GPO_INTERRUPT,
  NFC_MAILBOX_HAS_NEW_MESSAGE,
  // ACCELEROMETER_SENSOR
  // TEMPERATURE_HUMIDITY_SENSOR
  TH_SENS_INITIALIZE,
  TH_SENS_START_SINGLE_SHOT_READ,
  TH_SENS_READ_MEASUREMENT,
  TH_SENS_ERROR,
  // LIGHT_SENSOR
  LIGHT_SENS_INITIALIZE,
  LIGHT_SENS_INITIALIZE_SUCCESS,
  LIGHT_SENS_SINGLE_SHOT_READ,
  LIGHT_SENS_MEASURE_CONTINUOUSLY,
  LIGHT_SENS_CRON_READ,
  LIGHT_SENS_SET_LIMIT,
  LIGHT_SENS_TURN_OFF,
  LIGHT_SENS_LIMIT_INT,
  LIGHT_SENS_RECOVER,
  LIGHT_SENS_ERROR,
  // RTC
  RTC_CRON_READ_SENSORS,
  //
  MAX_EVENTS
} event_t;

#ifdef __cplusplus
}
#endif

#endif //EVENTS_LIST_H