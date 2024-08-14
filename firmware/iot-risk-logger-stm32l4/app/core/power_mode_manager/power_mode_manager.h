/*!
 * @file PWRM_MANAGER.h
 * @brief Brief description of the file.
 *
 * Detailed description of the file.
 *
 * @date 14/08/2024
 * @author artempolisskyi
 */

#ifndef PWRM_MANAGER_H
#define PWRM_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "main.h"

typedef enum {
  PWRM_NO_STATE = 0,
  PWRM_STANDBY,
  PWRM_STOP2,
  PWRM_LP_RUN,
  PWRM_RUN,
  PWRM_MAX_STATE
} PWRM_MANAGER_State_t;

typedef struct {
  actor_t super;
  PWRM_MANAGER_State_t state;
} PWRM_MANAGER_Actor_t;

extern PWRM_MANAGER_Actor_t PWRM_MANAGER_Actor;

actor_t* PWRM_MANAGER_ActorInit(void);

#ifdef __cplusplus
}
#endif

#endif //PWRM_MANAGER_H