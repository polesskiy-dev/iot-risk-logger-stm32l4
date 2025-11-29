/**
 * @file nfc_handlers.c
 * @brief NFC GPO interrupt callback implementation for ST25DV.
 *
 * This module provides the GPO interrupt handler that posts events
 * to the NFC task queue for synchronous processing.
 *
 * @date 17/07/2024
 * @author artempolisskyi
 */

#include "nfc_handlers.h"
#include "nfc.h"
#include "events_list.h"

/* External NFC context reference */
extern NFC_Context_t NFC_Context;

/**
 * @brief GPO interrupt callback handler
 *
 * Called from HAL GPIO EXTI callback when ST25DV GPO pin triggers.
 * Posts NFC_GPO_INTERRUPT event to the NFC task queue for processing.
 */
void NFC_GPOInterruptCallback(void)
{
    if (NFC_Context.osMessageQueueId == NULL) {
        return;
    }

    /* Post GPO interrupt event to NFC task queue */
    osMessageQueuePut(NFC_Context.osMessageQueueId, &(message_t){NFC_GPO_INTERRUPT}, 0, 0);

#ifdef DEBUG
    fprintf(stdout, "NFC: GPO interrupt posted to queue\n");
#endif
}