#include "nfc.h"
#include "custom_bus.h"

EXTI_HandleTypeDef hexti;

// TODO put NFC descriptor into Actor
static ST25DV_Object_t st25dv;

static int32_t initST25DV(void);
static void initGPOInterrupt(void);
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

  /* init functions call here */

  /* Init ST25DV driver */
  if (initST25DV() != NFCTAG_OK) {
    // TODO handle and log NFC init error
    /* Error */
    Error_Handler();
  }

  /* Reset Mailbox enable to allow write to EEPROM */
//  CUSTOM_NFCTAG_ResetMBEN_Dyn(CUSTOM_NFCTAG_INSTANCE);

  for (;;) {
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