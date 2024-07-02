#include "nfc.h"
#include "custom_bus.h"

/**
 * @see https://www.st.com/resource/en/application_note/an5512-st25-fast-transfer-mode-embedded-library-stmicroelectronics.pdf
 * @see https://www.st.com/resource/en/application_note/an4910-data-exchange-between-wired-ic-and-wireless-rf-iso-15693-using-fast-transfer-mode-supported-by-st25dvi2c-series-stmicroelectronics.pdf
 */

// TODO put NFC descriptor into Actor
static ST25DV_Object_t st25dv;

static int32_t initST25DV(void);
static void handleGPOInterrupt(void);

void nfcTaskInit(void) {
  osThreadAttr_t attr = {
          .name = "nfcTask",
          .priority = osPriorityNormal,
          .stack_size = 128 * 4
  };

  osThreadNew(nfcTask, NULL, &attr);
}

void nfcTask(void *argument) {
  (void) argument; // Avoid unused parameter warning
  nfcMessage_t msg;
  uint8_t ITStatus = 0x00;

  /* init functions call here */

  /* Init ST25DV driver */
  if (initST25DV() != NFCTAG_OK) {
    // TODO handle and log NFC init error
    /* Error */
    Error_Handler();
  }

  /* Reset Mailbox enable to allow write to EEPROM */
  ST25DV_ResetMBEN_Dyn(&st25dv);

  SEGGER_RTT_printf(0, "NFC initialized\n");

  for (;;) {
    // Wait for messages from the queue
    if (osMessageQueueGet(infoLedQueueHandle, &msg, NULL, osWaitForever) == osOK) {
      switch (msg) {
        case GPO_INTERRUPT:
          // recon GPO interrupt type
          // TODO handle error
          ST25DV_ReadITSTStatus_Dyn(&st25dv, &ITStatus);
          if (ITStatus & (ST25DV_ITSTS_DYN_RFPUTMSG_MASK<<ST25DV_ITSTS_DYN_RFPUTMSG_SHIFT)) {
            // TODO log
          }

          break;
        case MAILBOX_NEW_MESSAGE:
          // Handle new message in mailbox
          break;
      }
    }
  }
}

int32_t initST25DV(void) {
  int32_t status;
  ST25DV_IO_t IO;

  /* Configure the component */
  IO.Init         = BSP_I2C1_Init;
  IO.DeInit       = BSP_I2C1_DeInit;
  IO.IsReady      = BSP_I2C1_IsReady;
  IO.Read         = BSP_I2C1_ReadReg16;

  IO.Write        = (ST25DV_Write_Func) BSP_I2C1_WriteReg16;
  IO.GetTick      = HAL_GetTick;

  status = ST25DV_RegisterBusIO (&st25dv, &IO);
  if(status != NFCTAG_OK)
    return NFCTAG_ERROR;

  status = St25Dv_Drv.Init(&st25dv);
  if(status != NFCTAG_OK)
    return NFCTAG_ERROR;

  return NFCTAG_OK;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin == _NFC_INT_Pin) {
    handleGPOInterrupt();
  }
}


void handleGPOInterrupt(void) {
    /* Handle GPO interrupt */
    InfoLedMessage_t msg = INFO_LED_FLASH;

    /* Send the message to the queue */
    osMessageQueuePut(infoLedQueueHandle, &msg, 0, 0);
  }