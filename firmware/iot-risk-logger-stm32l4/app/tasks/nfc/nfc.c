/**
 * @file nfc.c
 * @brief NFC task and initialization for ST25DV NFC tag.
 *
 * @see https://www.st.com/resource/en/application_note/an5512-st25-fast-transfer-mode-embedded-library-stmicroelectronics.pdf
 * @see https://www.st.com/resource/en/application_note/an4910-data-exchange-between-wired-ic-and-wireless-rf-iso-15693-using-fast-transfer-mode-supported-by-st25dvi2c-series-stmicroelectronics.pdf
 */

#include "nfc.h"
#include "nfc_handlers.h"

// TODO put NFC descriptor into Actor
static ST25DV_Object_t st25dv;

/* Queue Handle */
osMessageQueueId_t nfcQueueHandle;


void NFC_TaskInit(void) {
  /* Create the queue */
  nfcQueueHandle = osMessageQueueNew(8, sizeof(nfcMessage_t), &(osMessageQueueAttr_t){
    .name = "nfcQueue"
  });

  osThreadAttr_t attr = {
          .name = "nfcTask",
          .priority = osPriorityNormal,
          .stack_size = 128 * 4
  };

  osThreadNew(NFC_Task, NULL, &attr);
}

void NFC_Task(void *argument) {
  (void) argument; // Avoid unused parameter warning
  nfcMessage_t msg;
  uint8_t ITStatus = 0x00;

  /* init functions call here */

  /* Init ST25DV driver */
  if (NFC_ST25DVInit(&st25dv) != NFCTAG_OK) {
    // TODO handle and log NFC init error
    SEGGER_RTT_printf(1, "NFC initialization Error\n");
    /* Error */
    Error_Handler();
  }

  /* Reset Mailbox enable to allow write to EEPROM */
  ST25DV_ResetMBEN_Dyn(&st25dv);
  // RTT_CTRL_TEXT_GREEN
  SEGGER_RTT_printf(0, "NFC initialized\n");

  for (;;) {
    // Wait for messages from the queue
    if (osMessageQueueGet(nfcQueueHandle, &msg, NULL, osWaitForever) == osOK) {
      switch (msg) {
        case GPO_INTERRUPT:
          NFC_HandleGPOInterrupt(&st25dv);
          break;
        case MAILBOX_HAS_NEW_MESSAGE:
          SEGGER_RTT_printf(0, "Mailbox has new message\n");
          // Handle new message in mailbox
          break;
      }
    }
  }
}

/** Handle GPO interrupt */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin == _NFC_INT_Pin) {
    osMessageQueuePut(infoLedQueueHandle, &(InfoLedMessage_t){INFO_LED_FLASH}, 0, 0);
    osMessageQueuePut(nfcQueueHandle, &(nfcMessage_t){GPO_INTERRUPT}, 0, 0);
  }
}
