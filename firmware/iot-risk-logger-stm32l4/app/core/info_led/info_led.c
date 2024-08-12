#include "info_led.h"

void INFO_LED_Init(void) {
  return INFO_LED_OFF;
}

osStatus_t INFO_LED_HandleMessageCMD(message_t *message) {
  switch (message->event) {
    case GLOBAL_CMD_INFO_LED_ON:
      INFO_LED_ON;
      break;
    case GLOBAL_CMD_INFO_LED_OFF:
      INFO_LED_OFF;
      break;
  }
  return osOK;
}
