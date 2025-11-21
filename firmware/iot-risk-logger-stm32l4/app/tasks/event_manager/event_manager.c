/*!
 * @file event_manager.c
 * @brief implementation of event_manager
 *
 * Detailed description of the implementation file.
 *
 * @date 17/07/2024
 * @author artempolisskyi
 */

#include "event_manager.h"

static osStatus_t handleEvManagerMessage(EV_MANAGER_Actor_t *this, message_t *message);
static osStatus_t publishEventToSubscribers(message_t *message);

extern actor_t* ACTORS_LOOKUP_SystemRegistry[MAX_ACTORS];

/**
 * @brief Event subscription matrix for the system's global events.
 *
 * This 2D array maps each global event to the list of actors that are subscribed to it.
 * Each row corresponds to a specific global event, and each column contains the IDs of actors
 * that are interested in that event.
 *
 * @note The matrix is initialized at compile-time and stored in Flash memory for efficiency.
 * @note The matrix includes Actors IDs, not Actors themselves to decouple the event manager from the actors.
 *
 * Example usage:
 * @code
 * EV_MANAGER_SubscribersIdsMatrix[GLOBAL_CMD_INITIALIZE] = {CRON_ACTOR_ID, PWRM_MANAGER_ACTOR_ID, ...};
 * @endcode
 *
 * @warning Ensure that the matrix is kept up-to-date whenever new events or actors are added to the system.
 */
const ACTOR_ID EV_MANAGER_SubscribersIdsMatrix[GLOBAL_EVENTS_MAX][MAX_ACTORS] = {
  // TODO: uncomment the full list to initialize all actors
//  [GLOBAL_CMD_INITIALIZE]                           = {CRON_ACTOR_ID, PWRM_MANAGER_ACTOR_ID, NFC_ACTOR_ID, IMU_ACTOR_ID, TEMPERATURE_HUMIDITY_SENSOR_ACTOR_ID, LIGHT_SENSOR_ACTOR_ID, MEMORY_ACTOR_ID},
  [GLOBAL_CMD_INITIALIZE]                           = {CRON_ACTOR_ID, LIGHT_SENSOR_ACTOR_ID, TEMPERATURE_HUMIDITY_SENSOR_ACTOR_ID, IMU_ACTOR_ID, MEMORY_ACTOR_ID},
  [GLOBAL_INITIALIZE_SUCCESS]                       = {},
  [GLOBAL_WAKE_N_READ]                              = {LIGHT_SENSOR_ACTOR_ID, TEMPERATURE_HUMIDITY_SENSOR_ACTOR_ID},
  [GLOBAL_TEMPERATURE_HUMIDITY_MEASUREMENTS_READY]  = {MEMORY_ACTOR_ID},
  [GLOBAL_LIGHT_MEASUREMENTS_READY]                 = {MEMORY_ACTOR_ID},
  [GLOBAL_IMU_MEASUREMENTS_READY]                   = {MEMORY_ACTOR_ID},
  [GLOBAL_MEASUREMENTS_WRITE_SUCCESS]               = {MEMORY_ACTOR_ID, NFC_ACTOR_ID},
  [GLOBAL_LOG_CHUNK_READ_SUCCESS]                   = { NFC_ACTOR_ID},
  [GLOBAL_SETTINGS_WRITE_SUCCESS]                   = {MEMORY_ACTOR_ID, NFC_ACTOR_ID},
  [GLOBAL_SETTINGS_READ_SUCCESS]                    = { NFC_ACTOR_ID},
  [GLOBAL_CMD_READ_SETTINGS]                        = { MEMORY_ACTOR_ID},
  [GLOBAL_CMD_START_CONTINUOUS_SENSING]             = {TEMPERATURE_HUMIDITY_SENSOR_ACTOR_ID, LIGHT_SENSOR_ACTOR_ID},
  [GLOBAL_CMD_SET_TIME_DATE]                        = {CRON_ACTOR_ID},
  [GLOBAL_CMD_SET_WAKE_UP_PERIOD]                   = {CRON_ACTOR_ID},
  [GLOBAL_CMD_TURN_OFF]                             = {TEMPERATURE_HUMIDITY_SENSOR_ACTOR_ID, LIGHT_SENSOR_ACTOR_ID, PWRM_MANAGER_ACTOR_ID},
};

// TODO simplify it from the task [DFT-24](https://www.notion.so/recycle-refactor-Event-Manager-transform-from-task-to-plain-function-2ad109abe35680949db7d59a1498757d?source=copy_link)

EV_MANAGER_Actor_t EV_MANAGER_Actor = {
        .super = {
                .actorId = EV_MANAGER_ACTOR_ID,
                .messageHandler = (messageHandler_t) handleEvManagerMessage,
                .osMessageQueueId = NULL,
                .osThreadId = NULL,
        }
};

actor_t* EV_MANAGER_ActorInit(osThreadId_t defaultTaskHandle) {
  EV_MANAGER_Actor.super.osMessageQueueId = osMessageQueueNew(DEFAULT_QUEUE_SIZE, DEFAULT_QUEUE_MESSAGE_SIZE, &(osMessageQueueAttr_t){
          .name = "eventManagerQueue"
  });
  EV_MANAGER_Actor.super.osThreadId = defaultTaskHandle;

  fprintf(stdout, "Event Manager initialized\n");

  // TODO move to init manager
  osMessageQueuePut(EV_MANAGER_Actor.super.osMessageQueueId, &(message_t){GLOBAL_CMD_INITIALIZE}, 0, 0);

  return &EV_MANAGER_Actor.super;
}

static osStatus_t handleEvManagerMessage(EV_MANAGER_Actor_t *this, message_t *message) {
  switch (message->event) {
    case GLOBAL_CMD_INITIALIZE:
      publishEventToSubscribers(message);
      // TODO handle initialize event
      // TODO remove from here
      osMessageQueuePut(EV_MANAGER_Actor.super.osMessageQueueId, &(message_t){GLOBAL_INITIALIZE_SUCCESS}, 0, 0);
      return osOK;
    case GLOBAL_INITIALIZE_SUCCESS:
      publishEventToSubscribers(message);
      // TODO remove from here, emit only in NFC
      osMessageQueuePut(EV_MANAGER_Actor.super.osMessageQueueId, &(message_t){GLOBAL_CMD_START_CONTINUOUS_SENSING}, 0, 0);
      return osOK;
    // all not specially dedicated to evManager events are published to subscribers
    default:
      publishEventToSubscribers(message);
      return osOK;
  }

  return osOK;
}

// TODO explain the system in graph: queueId -> actor -> ACTORS_LIST_SystemRegistry -> EV_MANAGER_SubscribersMatrix
/**
 * @brief Publishes an event to all subscribed actors.
 *
 * This function traverses the list of actors subscribed to a given event and dispatches the event message
 * to each actor's message queue. If an actor does not have a message queue, the message is processed immediately
 * using the actor's message handler.
 *
 * @param[in] message Pointer to the message to be published.
 *
 * Example usage:
 * @code
 * message_t msg = { .event = GLOBAL_CMD_INITIALIZE, .payload.value = 0 };
 * publishEventToSubscribers(&msg);
 * @endcode
 *
 * @warning This function assumes that the message queue for each actor has been properly initialized.
 *          If the queue is full, the message will be lost.
 */
static osStatus_t publishEventToSubscribers(message_t *message) {
  const ACTOR_ID* subscribersIds = EV_MANAGER_SubscribersIdsMatrix[message->event];

  // traverse all subscribers IDs and put message to their queues/handlers, subscribers are [NO_ACTOR, SOME_ACTOR, NO_ACTOR, ..., MAX_ACTORS]
  for (int32_t i = 0; i < MAX_ACTORS; i++) {
    if (subscribersIds[i] == NO_ACTOR_ID)
      continue;

    ACTOR_ID subscribedActorId = subscribersIds[i];
    actor_t* subscribedActor = ACTORS_LOOKUP_SystemRegistry[subscribedActorId];

    if (subscribedActor == NULL) {
      fprintf(stderr,  "Actor with ID %d subscribed on event %d is not found, check ACTORS_LIST_SystemRegistry\n", subscribedActorId, message->event);
      continue;
    }

    bool isTask = subscribedActor->osThreadId != NULL;

    // actor doesn't have task (not the OS thread), hence it can simply process message immediately
    if (!isTask) {
      subscribedActor->messageHandler(subscribedActor, message);
      continue;
    }

    // actor has task, hence we should put message to its queue
    osMessageQueueId_t actorsOsMessageQueueId = subscribedActor->osMessageQueueId;

    // Make a local copy of the message
//    message_t messageCopy;
//    memcpy(&messageCopy, &message, sizeof(message_t));

    // Try to put the message into the queue
//    osStatus_t status = osMessageQueuePut(actorsOsMessageQueueId, &messageCopy, 0, 0);
    osStatus_t status = osMessageQueuePut(actorsOsMessageQueueId, message, 0, 0);
    if (status != osOK) {
      fprintf(stderr, "Failed to enqueue message for actor ID %d (queue full or other error)\n", subscribedActorId);
    }
  }

  return osOK;
}