#ifndef NFC_H
#define NFC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "main.h"
#include "stm32l4xx_hal.h"

void nfcTaskInit(void);
void nfcTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif //NFC_H
