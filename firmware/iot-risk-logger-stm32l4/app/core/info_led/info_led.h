#ifndef INFO_LED_H
#define INFO_LED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define INFO_LED_OFF HAL_GPIO_WritePin(LED_N_GPIO_Port, LED_N_Pin, GPIO_PIN_SET)
#define INFO_LED_ON HAL_GPIO_WritePin(LED_N_GPIO_Port, LED_N_Pin, GPIO_PIN_RESET)

void INFO_LED_Init(void);
osStatus_t INFO_LED_HandleMessageCMD(message_t *message);

#ifdef __cplusplus
}
#endif

#endif //INFO_LED_H