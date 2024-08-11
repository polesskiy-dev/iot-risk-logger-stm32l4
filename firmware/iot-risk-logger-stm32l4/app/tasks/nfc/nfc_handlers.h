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
#include "nfc.h"

#ifdef __cplusplus
}
#endif

#define MAILBOX_START_OFFSET 0x00

int32_t NFC_ST25DVInit(ST25DV_Object_t *pObj);
void NFC_HandleGPOInterrupt(ST25DV_Object_t *pObj);
int32_t NFC_ReadMailboxTo(ST25DV_Object_t *pObj, uint8_t pMailboxBuffer[ST25DV_MAX_MAILBOX_LENGTH]);

#endif //NFC_HANDLERS_H
