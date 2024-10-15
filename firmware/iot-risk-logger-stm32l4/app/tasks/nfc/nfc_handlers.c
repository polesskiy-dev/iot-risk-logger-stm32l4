#include <assert.h>
#include "nfc_handlers.h"

int32_t NFC_ST25DVInit(ST25DV_Object_t *pObj) {
  ST25DV_IO_t IO;

  IO.Init = BSP_I2C1_Init;
  IO.DeInit = BSP_I2C1_DeInit;
  IO.IsReady = BSP_I2C1_IsReady;
  IO.Read = BSP_I2C1_ReadReg16;
  IO.Write = (ST25DV_Write_Func) BSP_I2C1_WriteReg16;
  IO.GetTick = HAL_GetTick;

  int32_t status = ST25DV_RegisterBusIO(pObj, &IO);
  if (status != NFCTAG_OK) {
    #ifdef DEBUG
      fprintf(stderr,  "ST25DV RegisterBusIO Error: %ld\n", status);
    #endif

    return NFCTAG_ERROR;
  }

  status = St25Dv_Drv.Init(pObj);
  if (status != NFCTAG_OK) {
    #ifdef DEBUG
      fprintf(stderr,  "ST25DV Driver Init Error: %ld\n", status);
    #endif

    return NFCTAG_ERROR;
  }

  return NFCTAG_OK;
}

void NFC_HandleGPOInterrupt(ST25DV_Object_t *pObj) {
  uint8_t ITStatus;
  ST25DV_ReadITSTStatus_Dyn(pObj, &ITStatus);
  if (ITStatus & ST25DV_ITSTS_DYN_RFPUTMSG_MASK) {
    osMessageQueuePut(NFC_Actor.super.osMessageQueueId, &(message_t){NEW_MAILBOX_RF_CMD}, 0, 0);

    #ifdef DEBUG
      fprintf(stdout, "NFC ITStatus: 0x%x\n", ITStatus);
    #endif
  }
}

int32_t NFC_ReadMailboxTo(ST25DV_Object_t *pObj, uint8_t pMailboxBuffer[ST25DV_MAX_MAILBOX_LENGTH]) {
  uint8_t mailboxLength;
  int32_t status = ST25DV_ReadMBLength_Dyn(pObj, &mailboxLength);

  if (status != NFCTAG_OK) {
    #ifdef DEBUG
        fprintf(stdout, "ST25DV ST25DV_ReadMBLength_Dyn Error\n");
    #endif

    return NFCTAG_ERROR;
  }

  #ifdef DEBUG
    fprintf(stdout, "Mailbox length: %d\n", mailboxLength);
  #endif

  status = ST25DV_ReadMailboxData(pObj, pMailboxBuffer, MAILBOX_START_OFFSET, ST25DV_MAX_MAILBOX_LENGTH);

  if (status != NFCTAG_OK) {
    #ifdef DEBUG
      fprintf(stdout, "ST25DV ST25DV_ReadMailboxData Error\n");
    #endif

    return NFCTAG_ERROR;
  }

  return NFCTAG_OK;
}