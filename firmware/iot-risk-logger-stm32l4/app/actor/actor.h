/*!
 * @file actor.h
 * @brief Brief description of the file.
 *
 * Detailed description of the file.
 *
 * @date 17/07/2024
 * @author artempolisskyi
 */

#ifndef ACTOR_H
#define ACTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "main.h"
#include "cmsis_os2.h"
#include "../config/events_list/events_list.h"

typedef struct {
  event_t event;
  void *payload;
  ssize_t payload_size;
} message_t;

/**
 * @brief Function pointer type for message handlers.
 *
 * @param message Pointer to the message to be handled.
 * @return osStatus_t Status of the message handling.
 */
typedef osStatus_t (*messageHandler_t)(message_t *message);

/**
 * @brief Base actor structure to inherit from.
 */
typedef struct {
  uint8_t actorId; ///< Unique actor ID
  osThreadId_t osThreadId; ///< CMSIS-RTOS2 Thread ID
  osMessageQueueId_t osMessageQueueId; ///< Message queue ID
  messageHandler_t messageHandler; ///< Message handler, most likely a state machine
  uint8_t state; ///< Actor state
} actor_t;

#ifdef __cplusplus
}
#endif

#endif //ACTOR_H