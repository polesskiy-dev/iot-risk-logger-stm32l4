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

#include "main.h"

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
  TH_SENS_DATA_SHOULD_BE_READY,
  // RTC
  RTC_CRON_READ_SENSORS,
  //
  MAX_EVENTS
} event_t;

#ifdef __cplusplus
}
#endif

#endif //EVENTS_LIST_H