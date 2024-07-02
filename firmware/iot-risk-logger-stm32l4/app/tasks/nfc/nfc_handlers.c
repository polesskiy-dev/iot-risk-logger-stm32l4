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
    SEGGER_RTT_printf(1, "ST25DV RegisterBusIO Error: %d\n", status);
    return NFCTAG_ERROR;
  }

  status = St25Dv_Drv.Init(pObj);
  if (status != NFCTAG_OK) {
    SEGGER_RTT_printf(1, "ST25DV Driver Init Error: %d\n", status);
    return NFCTAG_ERROR;
  }

  return NFCTAG_OK;
}

void NFC_HandleGPOInterrupt(ST25DV_Object_t *pObj) {
  uint8_t ITStatus;
  ST25DV_ReadITSTStatus_Dyn(pObj, &ITStatus);
  if (ITStatus & ST25DV_ITSTS_DYN_RFPUTMSG_MASK) {
    osMessageQueuePut(nfcQueueHandle, &(nfcMessage_t){MAILBOX_HAS_NEW_MESSAGE}, 0, 0);
    SEGGER_RTT_printf(0, "NFC ITStatus: 0x%x\n", ITStatus);
  }
}