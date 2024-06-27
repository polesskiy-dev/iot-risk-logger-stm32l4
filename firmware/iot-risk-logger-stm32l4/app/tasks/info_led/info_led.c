#include "info_led.h"

#define BLINK_PERIOD_MS 1000

void infoLedTaskInit(void) {
  osThreadAttr_t attr = {
          .name = "infoLedTask",
          .priority = osPriorityNormal,
          .stack_size = 128 * 4
  };

  osThreadNew(infoLedTask, NULL, &attr);
}

void infoLedTask(void *argument) {
  (void)argument; // Avoid unused parameter warning

  /* init functions call here */
  //  HAL_GPIO_TogglePin(_LED_GPIO_Port, _LED_Pin);

  for (;;) {
    // Toggle LED
    HAL_GPIO_TogglePin(_LED_GPIO_Port, _LED_Pin);

    // Delay for a period
    osDelay (pdMS_TO_TICKS(BLINK_PERIOD_MS));
  }
};
