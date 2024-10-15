/**
 * @file nfc.c
 * @brief NFC task and initialization for ST25DV NFC tag.
 *
 * @see https://www.st.com/resource/en/application_note/an5512-st25-fast-transfer-mode-embedded-library-stmicroelectronics.pdf
 * @see https://www.st.com/resource/en/application_note/an4910-data-exchange-between-wired-ic-and-wireless-rf-iso-15693-using-fast-transfer-mode-supported-by-st25dvi2c-series-stmicroelectronics.pdf
 */

#include "nfc.h"
#include "nfc_handlers.h"

static osStatus_t handleNFCFSM(NFC_Actor_t *this, message_t *message);
static osStatus_t handleInit(NFC_Actor_t *this, message_t *message);
static osStatus_t handleStandby(NFC_Actor_t *this, message_t *message);
static osStatus_t handleMailboxReceiveCMD(NFC_Actor_t *this, message_t *message);
static osStatus_t handleMailboxValidate(NFC_Actor_t *this, message_t *message);
static osStatus_t handleMailboxWriteResponse(NFC_Actor_t *this, message_t *message);

extern actor_t* ACTORS_LIST_SystemRegistry[MAX_ACTORS];

NFC_Actor_t NFC_Actor = {
        .super = {
                .actorId = NFC_ACTOR_ID,
                .messageHandler = (messageHandler_t) handleNFCFSM,
                .osMessageQueueId = NULL,
                .osThreadId = NULL,
        },
        .state = NFC_NO_STATE
};

uint32_t nfcTaskBuffer[DEFAULT_TASK_STACK_SIZE_WORDS];
StaticTask_t nfcTaskControlBlock;
const osThreadAttr_t nfcTaskDescription = {
        .name = "nfcTask",
        .cb_mem = &nfcTaskControlBlock,
        .cb_size = sizeof(nfcTaskControlBlock),
        .stack_mem = &nfcTaskBuffer[0],
        .stack_size = sizeof(nfcTaskBuffer),
        .priority = (osPriority_t) osPriorityNormal,
};

actor_t* NFC_TaskInit(void) {
  NFC_Actor.super.osMessageQueueId = osMessageQueueNew(DEFAULT_QUEUE_SIZE, DEFAULT_QUEUE_MESSAGE_SIZE, &(osMessageQueueAttr_t){
    .name = "nfcQueue"
  });
  NFC_Actor.super.osThreadId = osThreadNew(NFC_Task, NULL, &nfcTaskDescription);

  return (actor_t*) &NFC_Actor;
}

void NFC_Task(void *argument) {
  (void) argument; // Avoid unused parameter warning
  message_t msg;

  /* Reset Mailbox enable to allow write to EEPROM */
//  ST25DV_ResetMBEN_Dyn(&st25dv);

  for (;;) {
    // Wait for messages from the queue
    if (osMessageQueueGet(NFC_Actor.super.osMessageQueueId, &msg, NULL, osWaitForever) == osOK) {
      osStatus_t status = NFC_Actor.super.messageHandler((actor_t *) &NFC_Actor, &msg);

      if (status != osOK) {
        osMessageQueuePut(EV_MANAGER_Actor.super.osMessageQueueId, &(message_t){GLOBAL_ERROR, .payload.value = NFC_ACTOR_ID}, 0, 0);
        TO_STATE(&NFC_Actor, NFC_STATE_ERROR);
      }
    }
  }
}

static osStatus_t handleNFCFSM(NFC_Actor_t *this, message_t *message) {
  switch (this->state) {
    case NFC_NO_STATE:
      return handleInit(this, message);
    case NFC_STANDBY_STATE:
      return handleStandby(this, message);
    case NFC_MAILBOX_RECEIVE_CMD_STATE:
      return handleMailboxReceiveCMD(this, message);
    case NFC_VALIDATE_MAILBOX_STATE:
      return handleMailboxValidate(this, message);
    case NFC_MAILBOX_WRITE_RESPONSE_STATE:
      return handleMailboxWriteResponse(this, message);
    default:
      return osOK;
  }
}

static osStatus_t handleInit(NFC_Actor_t *this, message_t *message) {
  osStatus_t ioStatus;
  ST25DV_UID uid = {0x00000000, 0x00000000};
  const ST25DV_PASSWD i2cPwd = {0x00000000, 0x00000000};

  switch (message->event) {
    case GLOBAL_CMD_INITIALIZE:
      ioStatus = NFC_ST25DVInit(&this->st25dv);
      if (ioStatus != NFCTAG_OK)
        return osError;

      ioStatus = ST25DV_PresentI2CPassword(&this->st25dv, i2cPwd);
      if (ioStatus != NFCTAG_OK)
        return osError;

      ioStatus = ST25DV_ReadUID(&this->st25dv, &uid);
      if (ioStatus != NFCTAG_OK)
        return osError;

      #ifdef DEBUG
            fprintf(stdout, "NFC task initialized, UID: 0x%x %x\n", uid.MsbUid, uid.LsbUid);
      #endif

      TO_STATE(this, NFC_STANDBY_STATE);
      return osOK;
    default:
      return osOK;
  }
}

static osStatus_t handleStandby(NFC_Actor_t *this, message_t *message) {
  switch (message->event) {
    case NFC_GPO_INTERRUPT:
      NFC_HandleGPOInterrupt(&this->st25dv);

      TO_STATE(this, NFC_MAILBOX_RECEIVE_CMD_STATE);
      return osOK;
    default:
      return osOK;
  }
}

static osStatus_t handleMailboxReceiveCMD(NFC_Actor_t *this, message_t *message) {
  uint8_t *mailboxPayload   = NULL;
  uint8_t mailboxSize       = 0;

  switch (message->event) {
    case NEW_MAILBOX_RF_CMD:
      NFC_ReadMailboxTo(&this->st25dv, this->mailboxBuffer);

      // TODO validate mailbox CRC8 this->mailboxBuffer
      bool isValidCRC8 = true;

      if (isValidCRC8) {
        // get CMD from read mailbox
        event_t cmdEvent = this->mailboxBuffer[NFC_MAILBOX_PROTOCOL_CMD_ADDR];
        assert_param(cmdEvent >= GLOBAL_CMD_START_LOGGING && cmdEvent < GLOBAL_EVENTS_MAX);

        #ifdef DEBUG
          fprintf(stdout, "RF CMD: 0x%x\n", cmdEvent);
        #endif

        // dispatch received CMD to EV_MANAGER (globally)
        mailboxPayload = this->mailboxBuffer + NFC_MAILBOX_PROTOCOL_PAYLOAD_ADDR;
        mailboxSize = this->mailboxBuffer[NFC_MAILBOX_PROTOCOL_PAYLOAD_SIZE_ADDR];

        osMessageQueuePut(EV_MANAGER_Actor.super.osMessageQueueId,
        &(message_t){.event = cmdEvent, .payload.ptr = mailboxPayload, .payload_size = mailboxSize}, 0, 0);
      }

      if (!isValidCRC8) {
        osMessageQueuePut(this->super.osMessageQueueId, &(message_t) {NFC_CRC_ERROR}, 0, 0);
      }

      TO_STATE(this, NFC_VALIDATE_MAILBOX_STATE);
      return osOK;
    default:
      return osOK;
  }
}

static osStatus_t handleMailboxValidate(NFC_Actor_t *this, message_t *message) {
  if (NFC_CRC_ERROR == message->event) {
    osMessageQueuePut(this->super.osMessageQueueId, &(message_t) {GLOBAL_CMD_NFC_MAILBOX_WRITE}, 0, 0);
    // TODO: handle CRC error, write e.g. NACK to mailbox
    // TODO: maybe put this message as a static?
    this->mailboxBuffer[NFC_MAILBOX_PROTOCOL_CRC8_ADDR] = 0xF4; // CRC-8/NRSC-5 Standard from [0xFE, 0x00]
    this->mailboxBuffer[NFC_MAILBOX_PROTOCOL_CMD_ADDR] = NFC_RESPONSE_NACK_CRC_ERROR;
    this->mailboxBuffer[NFC_MAILBOX_PROTOCOL_PAYLOAD_SIZE_ADDR] = 0x00;

    TO_STATE(this, NFC_MAILBOX_WRITE_RESPONSE_STATE);
  }

  // All Commands transmit NFC FSM to the write data to mailbox state
  if (message->event >= GLOBAL_CMD_START_LOGGING && message->event < GLOBAL_EVENTS_MAX) {
    /**
     * @note No events are published here
     * NFC module will wait for GLOBAL_CMD_NFC_MAILBOX_WRITE from Event Manager
     * as a result of the command processing
     */
    // TODO remove it, it's a temporary solution to send only ACK
    osMessageQueuePut(this->super.osMessageQueueId, &(message_t) {GLOBAL_CMD_NFC_MAILBOX_WRITE}, 0, 0);

    TO_STATE(this, NFC_MAILBOX_WRITE_RESPONSE_STATE);
  }

  return osOK;
}

static osStatus_t handleMailboxWriteResponse(NFC_Actor_t *this, message_t *message) {
  osStatus_t ioStatus = osOK;
  uint8_t *payloadData = NULL;

  switch (message->event) {
    case GLOBAL_CMD_NFC_MAILBOX_WRITE:
      // TODO enhance protocol with ACK and ERROR CODES, Error codes could follow HTTP status codes

      // TODO copy data from event to mailbox buf
      // TODO check utilization f double buffer technique
      payloadData = (uint8_t *) message->payload.ptr;
      memcpy(this->mailboxBuffer, payloadData, message->payload_size);

      // TODO move to a separate "protocol" module
      // ST25DV_WriteMailboxData(&this->st25dv, this->mailboxBuffer, ST25DV_MAX_MAILBOX_LENGTH);

      // TODO it's a temporary debug solution to send only ACK
      this->mailboxBuffer[NFC_MAILBOX_PROTOCOL_CRC8_ADDR] = 0x81; // CRC-8/NRSC-5 Standard from [0x00, 0x00]
      this->mailboxBuffer[NFC_MAILBOX_PROTOCOL_CMD_ADDR] = NFC_RESPONSE_ACK_OK;
      this->mailboxBuffer[NFC_MAILBOX_PROTOCOL_PAYLOAD_SIZE_ADDR] = 0x00;

      ioStatus = ST25DV_WriteMailboxData(&this->st25dv, this->mailboxBuffer, NFC_MAILBOX_PROTOCOL_HEADER_SIZE);

      TO_STATE(this, NFC_STANDBY_STATE);
      return ioStatus;
  }
}