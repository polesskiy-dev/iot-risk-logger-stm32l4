/*!
 * @file actors_list.c
 * @brief implementation of actors_lookup
 *
 * Detailed description of the implementation file.
 *
 * @date 17/07/2024
 * @author artempolisskyi
 */

#include "actors_lookup.h"

/**
 * @brief Global registry for all actors in the system.
 *
 * This array holds pointers to the initialized actors in the system. Each entry corresponds to
 * an actor identified by a unique actor ID. Actors must be initialized and added to this registry
 * during system startup.
 *
 * Example usage:
 * @code
 * ACTORS_LOOKUP_SystemRegistry[CRON_ACTOR_ID] = CRON_ActorInit();
 * @endcode
 *
 * @warning Ensure that each actor is correctly initialized before adding it to the registry.
 */
actor_t* ACTORS_LOOKUP_SystemRegistry[MAX_ACTORS] = {
  [EV_MANAGER_ACTOR_ID] = NULL,
  [CRON_ACTOR_ID] = NULL,
  [PWRM_MANAGER_ACTOR_ID] = NULL,
  [NFC_ACTOR_ID] = NULL,
  [IMU_ACTOR_ID] = NULL,
  [TEMPERATURE_HUMIDITY_SENSOR_ACTOR_ID] = NULL,
  [LIGHT_SENSOR_ACTOR_ID] = NULL,
  [MEMORY_ACTOR_ID] = NULL,
  [INFO_LED_ACTOR_ID] = NULL,
};
