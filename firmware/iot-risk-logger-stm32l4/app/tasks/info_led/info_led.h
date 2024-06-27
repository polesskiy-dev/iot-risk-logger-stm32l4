#ifndef INFO_LED_H
#define INFO_LED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "main.h"
#include "stm32l4xx_hal.h"

void infoLedTaskInit(void);
void infoLedTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif //INFO_LED_H