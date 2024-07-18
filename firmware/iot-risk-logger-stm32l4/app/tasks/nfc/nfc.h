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
#include "info_led.h"
#include "nfc_handlers.h"

typedef enum {
  NFC_STATE_INIT = 0,
  NFC_STATE_READY,
  NFC_STATE_ERROR,
  NFC_STATE_MAX
} NFC_State_t;

typedef struct {
  actor_t super;
  NFC_State_t state;
} NFC_Actor_t;

extern NFC_Actor_t NFC_Actor;

void NFC_TaskInit(void);
void NFC_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif //NFC_H
