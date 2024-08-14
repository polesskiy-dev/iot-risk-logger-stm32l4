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
#include "custom_bus.h"
#include "nfc_handlers.h"

typedef enum {
  NFC_NO_STATE = 0,
  NFC_STANDBY_STATE,
  NFC_STATE_ERROR,
  NFC_MAX_STATE
} NFC_State_t;

typedef struct {
  actor_t super;
  NFC_State_t state;
  ST25DV_Object_t st25dv;
  uint8_t mailboxBuffer[ST25DV_MAX_MAILBOX_LENGTH];
} NFC_Actor_t;

extern NFC_Actor_t NFC_Actor;

actor_t* NFC_TaskInit(void);
void NFC_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif //NFC_H
