#ifndef INFO_LED_H
#define INFO_LED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "main.h"
#include "stm32l4xx_hal.h"

/* Define the queue handle */
extern osMessageQueueId_t infoLedQueueHandle;

void INFO_LED_TaskInit(void);
void INFO_LED_Task(void *argument);

/* Queue message type */
typedef enum {
  INFO_LED_FLASH
} InfoLedMessage_t;

#ifdef __cplusplus
}
#endif

#endif //INFO_LED_H