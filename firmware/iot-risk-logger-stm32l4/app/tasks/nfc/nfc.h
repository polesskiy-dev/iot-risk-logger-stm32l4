#ifndef NFC_H
#define NFC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "main.h"
#include "stm32l4xx_hal.h"
#include "st25dv_reg.h"
#include "st25dv.h"
#include "info_led.h"

/* Define the queue handle */
extern osMessageQueueId_t nfcQueueHandle;

void nfcTaskInit(void);
void nfcTask(void *argument);

/* Queue message type */
typedef enum {
  GPO_INTERRUPT,
  MAILBOX_NEW_MESSAGE,
} nfcMessage_t;

#ifdef __cplusplus
}
#endif

#endif //NFC_H
