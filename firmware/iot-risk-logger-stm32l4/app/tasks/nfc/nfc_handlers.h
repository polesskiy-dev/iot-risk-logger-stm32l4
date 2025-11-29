/**
 * @file nfc_handlers.h
 * @brief NFC GPO interrupt and mailbox helper functions for ST25DV.
 *
 * This module provides low-level helper functions for NFC operations.
 *
 * @date 17/07/2024
 * @author artempolisskyi
 */

#ifndef NFC_HANDLERS_H
#define NFC_HANDLERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>

#include "main.h"
#include "st25dv.h"
#include "custom_bus.h"
#include "cmsis_os2.h"

/* Mailbox read offset */
#define MAILBOX_START_OFFSET 0x00

/* Forward declaration */
struct NFC_Context_t;

/**
 * @brief GPO interrupt callback handler
 *
 * Called from HAL GPIO EXTI callback when ST25DV GPO pin triggers.
 * Posts NFC_GPO_INTERRUPT event to the NFC task queue.
 */
void NFC_GPOInterruptCallback(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_HANDLERS_H */
