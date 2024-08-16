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
        // TODO Handle error, emit common error event and reinitialize module
        NFC_Actor.state = NFC_STATE_ERROR;
      }
    }
  }
}

/** Handle GPO interrupt */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin == _NFC_INT_Pin) {
    fprintf(stdout, "NFC GPO Interrupt\n");
    osMessageQueuePut(NFC_Actor.super.osMessageQueueId, &(message_t){NFC_GPO_INTERRUPT}, 0, 0);
  }
}

static osStatus_t handleNFCFSM(NFC_Actor_t *this, message_t *message) {
  switch (this->state) {
    case NFC_NO_STATE:
      handleInit(this, message);
      return osOK;
    case NFC_STANDBY_STATE:
      // TODO handle low power state
      return osOK;
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
  osStatus_t status;

  switch (message->event) {
    case GLOBAL_CMD_INITIALIZE:
      status = NFC_ST25DVInit(&this->st25dv);
      if (status != NFCTAG_OK)
        return osError;
      this->state = NFC_STANDBY_STATE;

      return osOK;
    default:
      return osOK;
  }
}