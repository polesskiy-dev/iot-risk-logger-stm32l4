#ifndef INFO_LED_H
#define INFO_LED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "main.h"
#include "stm32l4xx_hal.h"

typedef struct {
  actor_t super;
} INFO_LED_Actor_t;

extern const INFO_LED_Actor_t INFO_LED_Actor;

/* Define the queue handle */
extern osMessageQueueId_t infoLedQueueHandle;

INFO_LED_Actor_t *INFO_LED_ActorConstructor(void);
void INFO_LED_TaskInit(void);
void INFO_LED_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif //INFO_LED_H