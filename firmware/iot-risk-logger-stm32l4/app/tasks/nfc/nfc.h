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

/**
 * NFC exchange protocol description
 * | CRC8 | CMD | Payload Size | Payload |
 */
#define NFC_MAILBOX_PROTOCOL_CRC8_ADDR              0
#define NFC_MAILBOX_PROTOCOL_CRC8_SIZE              1
#define NFC_MAILBOX_PROTOCOL_CMD_ADDR               (NFC_MAILBOX_PROTOCOL_CRC8_ADDR + NFC_MAILBOX_PROTOCOL_CRC8_SIZE)
#define NFC_MAILBOX_PROTOCOL_CMD_SIZE               1
#define NFC_MAILBOX_PROTOCOL_PAYLOAD_SIZE_ADDR      (NFC_MAILBOX_PROTOCOL_CMD_ADDR + NFC_MAILBOX_PROTOCOL_CMD_SIZE)
#define NFC_MAILBOX_PROTOCOL_PAYLOAD_SIZE_SIZE      1
#define NFC_MAILBOX_PROTOCOL_PAYLOAD_ADDR           (NFC_MAILBOX_PROTOCOL_PAYLOAD_SIZE_ADDR + NFC_MAILBOX_PROTOCOL_PAYLOAD_SIZE_SIZE)
#define NFC_MAILBOX_PROTOCOL_HEADER_SIZE            (NFC_MAILBOX_PROTOCOL_CRC8_SIZE + NFC_MAILBOX_PROTOCOL_CMD_SIZE + NFC_MAILBOX_PROTOCOL_PAYLOAD_SIZE_SIZE)

/**
 * Exchange protocol response codes
 */
#define NFC_RESPONSE_ACK_OK           0x00
#define NFC_RESPONSE_NACK_ERROR       0xFF
#define NFC_RESPONSE_NACK_CRC_ERROR   0xFE

typedef enum {
  NFC_NO_STATE = 0,
  NFC_STANDBY_STATE,
  NFC_MAILBOX_RECEIVE_CMD_STATE,
  NFC_VALIDATE_MAILBOX_STATE,
  NFC_MAILBOX_WRITE_RESPONSE_STATE,
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
