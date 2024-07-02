#ifndef NFC_HANDLERS_H
#define NFC_HANDLERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "main.h"
#include "st25dv.h"
#include "custom_bus.h"
#include "cmsis_os2.h"
#include "info_led.h"
#include "nfc.h"

#ifdef __cplusplus
}
#endif

int32_t NFC_ST25DVInit(ST25DV_Object_t *pObj);
void NFC_HandleGPOInterrupt(ST25DV_Object_t *pObj);

#endif //NFC_HANDLERS_H
