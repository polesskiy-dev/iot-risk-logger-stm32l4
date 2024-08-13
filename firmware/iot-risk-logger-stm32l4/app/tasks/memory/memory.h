/*!
 * @file memory.h
 * @brief Brief description of the file.
 *
 * Detailed description of the file.
 *
 * @date 14/08/2024
 * @author artempolisskyi
 */

#ifndef MEMORY_H
#define MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "main.h"
#include "quadspi.h"

// TODO move to driver
#define W25Q_POWERDOWN 0xB9

typedef enum {
  MEMORY_NO_STATE = 0,
  MEMORY_STATE_ERROR,
  MEMORY_MAX_STATE
} MEMORY_State_t;

typedef struct {
  actor_t super;
  MEMORY_State_t state;
} MEMORY_Actor_t;

actor_t* MEMORY_TaskInit(void);
void MEMORY_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif //MEMORY_H