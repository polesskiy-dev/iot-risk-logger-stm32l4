#include "info_led.h"

#define BLINK_PERIOD_MS 100

/* Queue Handle */
osMessageQueueId_t infoLedQueueHandle;

void infoLedTaskInit(void) {
  /* Turn off the LED */
//  HAL_GPIO_WritePin(_LED_GPIO_Port, _LED_Pin, GPIO_PIN_SET);

  /* Create the queue */
  infoLedQueueHandle = osMessageQueueNew(10, sizeof(InfoLedMessage_t), NULL);

  osThreadAttr_t attr = {
          .name = "infoLedTask",
          .priority = osPriorityNormal,
          .stack_size = 128 * 4
  };

  osThreadNew(infoLedTask, NULL, &attr);
}

void infoLedTask(void *argument) {
  (void)argument; // Avoid unused parameter warning
  InfoLedMessage_t msg;

  /* init functions call here */

  for (;;) {
    // Wait for messages from the queue
    if (osMessageQueueGet(infoLedQueueHandle, &msg, NULL, osWaitForever) == osOK) {
      if (msg == INFO_LED_FLASH) {
        // Flash the LED
        HAL_GPIO_TogglePin(_LED_GPIO_Port, _LED_Pin);
        osDelay(pdMS_TO_TICKS(BLINK_PERIOD_MS));
        HAL_GPIO_TogglePin(_LED_GPIO_Port, _LED_Pin);
      }
    }
  }
};
