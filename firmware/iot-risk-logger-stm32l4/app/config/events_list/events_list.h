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
  //
  MAX_EVENTS
} event_t;

#ifdef __cplusplus
}
#endif

#endif //EVENTS_LIST_H