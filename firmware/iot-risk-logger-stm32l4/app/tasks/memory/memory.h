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
#include "w25q.h"

/* W25Q64JV Memory Specifications */
#define W25Q64JV_FLASH_SIZE              0x800000  /* 8 MB (64 Mbit) */
#define W25Q64JV_SECTOR_SIZE             0x1000    /* 4 KB */
#define W25Q64JV_SUBSECTOR_SIZE          0x0100    /* 256 B */
#define W25Q64JV_PAGE_SIZE               0x0100    /* 256 B */
#define W25Q64JV_BLOCK_SIZE_32K          0x8000    /* 32 KB */
#define W25Q64JV_BLOCK_SIZE_64K          0x10000   /* 64 KB */

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