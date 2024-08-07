/**
 * @file nfc.c
 * @brief NFC task and initialization for ST25DV NFC tag.
 *
 * @see https://www.st.com/resource/en/application_note/an5512-st25-fast-transfer-mode-embedded-library-stmicroelectronics.pdf
 * @see https://www.st.com/resource/en/application_note/an4910-data-exchange-between-wired-ic-and-wireless-rf-iso-15693-using-fast-transfer-mode-supported-by-st25dvi2c-series-stmicroelectronics.pdf
 */

#include "nfc.h"
#include "nfc_handlers.h"

// TODO put into Actor
static ST25DV_Object_t st25dv;
static uint8_t mailboxBuffer[ST25DV_MAX_MAILBOX_LENGTH];

static osStatus_t handleNFCMessage(NFC_Actor_t *this, message_t *message);

NFC_Actor_t NFC_Actor = {
        .super = {
                .actorId = NFC_ACTOR_ID,
                .messageHandler = (messageHandler_t) handleNFCMessage,
                .osMessageQueueId = NULL,
                .osThreadId = NULL,
        },
        .state = NFC_STATE_READY
};

const osThreadAttr_t nfcTaskDescription = {
        .name = "nfcTask",
        .priority = osPriorityNormal,
        .stack_size = DEFAULT_TASK_STACK_SIZE
};

void NFC_TaskInit(void) {
  NFC_Actor.super.osMessageQueueId = osMessageQueueNew(DEFAULT_QUEUE_SIZE, DEFAULT_QUEUE_MESSAGE_SIZE, &(osMessageQueueAttr_t){
    .name = "nfcQueue"
  });
  NFC_Actor.super.osThreadId = osThreadNew(NFC_Task, NULL, &nfcTaskDescription);
}

void NFC_Task(void *argument) {
  (void) argument; // Avoid unused parameter warning
  message_t msg;
  uint8_t ITStatus = 0x00;

  /* init functions call here */

  /* Init ST25DV driver */
  if (NFC_ST25DVInit(&st25dv) != NFCTAG_OK) {
    // TODO handle and log NFC init error
    SEGGER_SYSVIEW_PrintfTarget("NFC initialization Error\n");
    /* Error */
    Error_Handler();
  }

  /* Reset Mailbox enable to allow write to EEPROM */
  ST25DV_ResetMBEN_Dyn(&st25dv);
  SEGGER_SYSVIEW_PrintfTarget("NFC initialized\n");

  for (;;) {
    // Wait for messages from the queue
    if (osMessageQueueGet(NFC_Actor.super.osMessageQueueId, &msg, NULL, osWaitForever) == osOK) {
      NFC_Actor.super.messageHandler((actor_t *) &NFC_Actor, &msg);
    }
  }
}

static osStatus_t handleNFCMessage(NFC_Actor_t *this, message_t *message) {
  switch (message->event) {
    case NFC_GPO_INTERRUPT:
      NFC_HandleGPOInterrupt(&st25dv);
      return osOK;
    case NFC_MAILBOX_HAS_NEW_MESSAGE:
      SEGGER_SYSVIEW_PrintfTarget("Mailbox has new message\n");
      NFC_ReadMailboxTo(&st25dv, mailboxBuffer);
      // Print for debug purposes
      for (int i = 0; i < ST25DV_MAX_MAILBOX_LENGTH; i++) {
        SEGGER_SYSVIEW_PrintfTarget("0x%x ", mailboxBuffer[i]);
      }
      return osOK;
    default:
      return osError;
  }
}

/** Handle GPO interrupt */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin == _NFC_INT_Pin) {
    osMessageQueuePut(infoLedQueueHandle, &(message_t){INFO_LED_FLASH}, 0, 0);
    osMessageQueuePut(NFC_Actor.super.osMessageQueueId, &(message_t){NFC_GPO_INTERRUPT}, 0, 0);
  }
}
