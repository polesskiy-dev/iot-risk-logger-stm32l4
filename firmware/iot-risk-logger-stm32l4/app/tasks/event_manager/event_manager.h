/*!
 * @file event_manager.h
 * @brief Brief description of the file.
 *
 * Detailed description of the file.
 *
 * @date 17/07/2024
 * @author artempolisskyi
 */

#ifndef EVENT_MANAGER_H
#define EVENT_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "main.h"
#include "actor.h"
#include "cmsis_os2.h"

typedef struct {
  actor_t super;
} EV_MANAGER_Actor_t;

extern EV_MANAGER_Actor_t EV_MANAGER_Actor;

actor_t* EV_MANAGER_TaskInit(void);
void EV_MANAGER_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif //EVENT_MANAGER_H