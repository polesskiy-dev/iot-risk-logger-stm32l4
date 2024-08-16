/*!
 * @file actor.h
 * @brief Actor (Active object) pico framework
 *
 * This file contains the definitions and structures needed for the actor framework,
 * which facilitates message handling in a CMSIS-RTOS2 environment.
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

#include "cmsis_os2.h"
#include "../config/events_list/events_list.h"
#include "../config/actors_list/actors_list.h"

#define DEFAULT_QUEUE_SIZE 8
#define DEFAULT_QUEUE_MESSAGE_SIZE sizeof(message_t)
#define DEFAULT_TASK_STACK_SIZE_WORDS (128)

/**
 * @brief Sets the state of an inherited actor and logs the action.
 *
 * @warning inherited actor from actor_t should have a `state` member.
 *
 * @param actorPointer A pointer to inherited the actor whose state is to be set.
 *                     This should be a pointer to a struct that has a `state` member.
 * @param stateEnum    state to set
 *
 * @example
 * typedef struct {
 *  actor_t super;
 *  StateEnum_t state;
 * } My_Actor_t;
 *
 * My_Actor_t myActor;
 * TO_STATE(&myActor, FIRST_STATE);
 */
#define TO_STATE(actorPointer, stateEnum)                                     \
  do {                                                                        \
    (actorPointer)->state = (stateEnum);                                      \
    fprintf(stdout, "%lu: %s\n", (actorPointer)->super.actorId, #stateEnum);  \
  } while (0);

/**
* @brief Structure representing a message in the actor framework.
*/
typedef struct {
  event_t event;          ///< Event associated with the message
  union {
    void *ptr;            ///< Pointer to the message payload
    uint32_t value;       ///< payload value
  } payload;
  ssize_t payload_size;   ///< Size of the message payload
} message_t;

// Forward declare struct actor_t
struct actor_t;

/**
 * @brief Function pointer type for message handlers.
 *
 * @param actor Pointer to the actor that is handling the message.
 * @param message Pointer to the message to be handled.
 * @return osStatus_t Status of the message handling.
 */
typedef osStatus_t (*messageHandler_t)(struct actor_t *actor, message_t *message);

/**
 * @brief Structure representing a base actor in the actor framework.
 */
typedef struct actor_t {
  uint32_t actorId;                ///< Unique actor ID
  osThreadId_t osThreadId;         ///< CMSIS-RTOS2 Thread ID
  osMessageQueueId_t osMessageQueueId; ///< Message queue ID
  messageHandler_t messageHandler; ///< Message handler, most likely a state machine
} actor_t;

#ifdef __cplusplus
}
#endif

#endif //ACTOR_H