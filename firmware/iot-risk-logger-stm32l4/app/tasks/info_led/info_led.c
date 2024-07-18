#include "info_led.h"

#define BLINK_PERIOD_MS 100
#define BLINK_NFC_GPO_PERIOD_MS 500

/* Queue Handle */
osMessageQueueId_t infoLedQueueHandle;

void INFO_LED_TaskInit(void) {
  /* Create the queue */
  infoLedQueueHandle = osMessageQueueNew(8, sizeof(message_t), NULL);

  osThreadAttr_t attr = {
          .name = "infoLedTask",
          .priority = osPriorityNormal,
          .stack_size = 128 * 4
  };

  osThreadNew(INFO_LED_Task, NULL, &attr);
}

void INFO_LED_Task(void *argument) {
  (void)argument; // Avoid unused parameter warning
  message_t msg;

  /* init functions call here */

  for (;;) {
    // Wait for messages from the queue
    if (osMessageQueueGet(infoLedQueueHandle, &msg, NULL, osWaitForever) == osOK) {
      if (msg.event == INFO_LED_FLASH) {
        // Flash the LED
        HAL_GPIO_TogglePin(_LED_GPIO_Port, _LED_Pin);
        osDelay(pdMS_TO_TICKS(BLINK_NFC_GPO_PERIOD_MS));
        HAL_GPIO_TogglePin(_LED_GPIO_Port, _LED_Pin);
      }
    }
  }
};
