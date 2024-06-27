#include "nfc.h"
#include "custom_nfc04a1_nfctag.h"

void nfcTaskInit(void) {
  osThreadAttr_t attr = {
          .name = "nfcTask",
          .priority = osPriorityNormal,
          .stack_size = 128 * 4
  };

  osThreadNew(nfcTask, NULL, &attr);
}

void nfcTask(void *argument) {
  (void)argument; // Avoid unused parameter warning

  /* init functions call here */
  /* Init ST25DV driver */
  while(CUSTOM_NFCTAG_Init(CUSTOM_NFCTAG_INSTANCE) != NFCTAG_OK );

  /* Reset Mailbox enable to allow write to EEPROM */
  CUSTOM_NFCTAG_ResetMBEN_Dyn(CUSTOM_NFCTAG_INSTANCE);

  for (;;) {
  }
}
