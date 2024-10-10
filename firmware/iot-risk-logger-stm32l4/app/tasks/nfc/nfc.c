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
static osStatus_t handleMailboxTransmit(NFC_Actor_t *this, message_t *message);

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
        osMessageQueueId_t evManagerQueue = ACTORS_LIST_SystemRegistry[EV_MANAGER_ACTOR_ID]->osMessageQueueId;
        osMessageQueuePut(evManagerQueue, &(message_t){GLOBAL_ERROR, .payload.value = NFC_ACTOR_ID}, 0, 0);
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
    case NFC_MAILBOX_TRANSMIT_STATE:
      return handleMailboxTransmit(this, message);
    default:
      return osOK;
  }

//  switch (message->event) {
//    case NFC_GPO_INTERRUPT:
//      NFC_HandleGPOInterrupt(&st25dv);
//      return osOK;
//    case NFC_MAILBOX_HAS_NEW_MESSAGE:
//      fprintf(stdout, "Mailbox has new message\n");
//      NFC_ReadMailboxTo(&st25dv, mailboxBuffer);
//      // Print for debug purposes
//      for (int i = 0; i < ST25DV_MAX_MAILBOX_LENGTH; i++) {
//        fprintf(stdout, "0x%x ", mailboxBuffer[i]);
//      }
//      return osOK;
//    default:
//      return osError;
//  }
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

      TO_STATE(this, NFC_MAILBOX_TRANSMIT_STATE);
      return osOK;
    default:
      return osOK;
  }
}

static osStatus_t handleMailboxTransmit(NFC_Actor_t *this, message_t *message) {
  switch (message->event) {
    case NFC_MAILBOX_HAS_NEW_MESSAGE:
      NFC_ReadMailboxTo(&this->st25dv, this->mailboxBuffer);

      // Print for debug purposes
      for (int i = 0; i < ST25DV_MAX_MAILBOX_LENGTH; i++) {
        fprintf(stdout, "0x%x ", this->mailboxBuffer[i]);
      }

      return osOK;
    default:
      return osOK;
  }
}