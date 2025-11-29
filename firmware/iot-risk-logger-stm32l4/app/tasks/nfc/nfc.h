/**
 * @file nfc.h
 * @brief Synchronous NFC task and initialization for ST25DV NFC tag.
 *
 * This implementation uses a synchronous request/response model. The NFC task waits
 * for events on its message queue. Upon receiving a GPO interrupt from the ST25DV,
 * it reads the entire mailbox, validates the CRC8 and command, dispatches the command
 * synchronously and writes the response back to the mailbox before returning to standby.
 *
 * Exchange protocol:
 * | CRC8 | CMD | Payload Size | Payload |
 * where the CRC8 covers CMD, Payload Size and the Payload bytes. The payload of responses
 * always begins with a status byte (0x00 for OK, 0x01 for end-of-log, 0xFF for error).
 *
 * @see https://www.st.com/resource/en/application_note/an5512-st25-fast-transfer-mode-embedded-library-stmicroelectronics.pdf
 * @see https://www.st.com/resource/en/application_note/an4910-data-exchange-between-wired-ic-and-wireless-rf-iso-15693-using-fast-transfer-mode-supported-by-st25dvi2c-series-stmicroelectronics.pdf
 */

#ifndef NFC_H
#define NFC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "main.h"
#include "stm32l4xx_hal.h"
#include "st25dv_reg.h"
#include "st25dv.h"
#include "custom_bus.h"

/**
 * @defgroup NFC_Protocol NFC Mailbox Protocol Definitions
 * @brief Protocol layout: | CRC8 | CMD | Payload Size | Payload |
 * @{
 */
#define NFC_MAILBOX_PROTOCOL_CRC8_ADDR              0
#define NFC_MAILBOX_PROTOCOL_CRC8_SIZE              1
#define NFC_MAILBOX_PROTOCOL_CMD_ADDR               (NFC_MAILBOX_PROTOCOL_CRC8_ADDR + NFC_MAILBOX_PROTOCOL_CRC8_SIZE)
#define NFC_MAILBOX_PROTOCOL_CMD_SIZE               1
#define NFC_MAILBOX_PROTOCOL_PAYLOAD_SIZE_ADDR      (NFC_MAILBOX_PROTOCOL_CMD_ADDR + NFC_MAILBOX_PROTOCOL_CMD_SIZE)
#define NFC_MAILBOX_PROTOCOL_PAYLOAD_SIZE_SIZE      1
#define NFC_MAILBOX_PROTOCOL_PAYLOAD_ADDR           (NFC_MAILBOX_PROTOCOL_PAYLOAD_SIZE_ADDR + NFC_MAILBOX_PROTOCOL_PAYLOAD_SIZE_SIZE)
#define NFC_MAILBOX_PROTOCOL_HEADER_SIZE            (NFC_MAILBOX_PROTOCOL_CRC8_SIZE + NFC_MAILBOX_PROTOCOL_CMD_SIZE + NFC_MAILBOX_PROTOCOL_PAYLOAD_SIZE_SIZE)
/** @} */

/**
 * @defgroup NFC_Response NFC Response Codes
 * @brief Response status codes used in mailbox response payload
 * @{
 */
#define NFC_RESPONSE_OK               0x00  /**< Success */
#define NFC_RESPONSE_END_OF_LOG       0x01  /**< End of log reached */
#define NFC_RESPONSE_ERROR            0xFF  /**< General error */
#define NFC_RESPONSE_CRC_ERROR        0xFE  /**< CRC validation failed */
/** @} */

/**
 * @defgroup NFC_POSIX NFC POSIX-style Return Codes
 * @brief POSIX-style return codes for NFC functions
 * @{
 */
#define NFC_OK                        0     /**< Operation successful */
#define NFC_OK_RESPONSE_SENT          1     /**< Success, response already sent by handler */
#define NFC_ERROR                    -1     /**< General error */
#define NFC_ERROR_CRC                -2     /**< CRC validation error */
#define NFC_ERROR_INVALID_CMD        -3     /**< Invalid command received */
#define NFC_ERROR_IO                 -4     /**< I/O error (I2C communication) */
#define NFC_ERROR_TIMEOUT            -5     /**< Operation timed out */
/** @} */

/**
 * @brief NFC context structure
 * Contains all state and buffers for NFC operations
 */
typedef struct {
    ST25DV_Object_t st25dv;                              /**< ST25DV driver object */
    uint8_t mailboxBuffer[ST25DV_MAX_MAILBOX_LENGTH];    /**< Mailbox data buffer */
    osMessageQueueId_t osMessageQueueId;                 /**< Message queue for GPO events */
    osThreadId_t osThreadId;                             /**< Task thread handle */
} NFC_Context_t;

/* Global NFC context */
extern NFC_Context_t NFC_Context;

/**
 * @defgroup NFC_API NFC POSIX-style API
 * @brief Synchronous NFC operations with POSIX-style return codes
 * @{
 */

/**
 * @brief Initialize NFC hardware and ST25DV tag
 * @return NFC_OK on success, negative error code on failure
 */
int NFC_Init(void);

/**
 * @brief Read data from NFC mailbox
 * @param[out] buffer Buffer to store read data
 * @param[in] size Maximum number of bytes to read
 * @return Number of bytes read on success, negative error code on failure
 */
ssize_t NFC_Read(uint8_t *buffer, size_t size);

/**
 * @brief Write data to NFC mailbox
 * @param[in] buffer Data to write
 * @param[in] size Number of bytes to write
 * @return Number of bytes written on success, negative error code on failure
 */
ssize_t NFC_Write(const uint8_t *buffer, size_t size);

/**
 * @brief Compute CRC8 for data validation (CRC-8/NRSC-5)
 * @param[in] data Data to compute CRC over
 * @param[in] len Length of data
 * @return Computed CRC8 value
 */
uint8_t NFC_ComputeCRC8(const uint8_t *data, size_t len);

/**
 * @brief Validate CRC8 of received mailbox data
 * @param[in] buffer Mailbox buffer with protocol header
 * @return true if CRC is valid, false otherwise
 */
bool NFC_ValidateCRC8(const uint8_t *buffer);

/**
 * @brief Build response packet in mailbox format
 * @param[out] buffer Output buffer for response
 * @param[in] cmd Command/response code
 * @param[in] status Response status byte
 * @param[in] payload Optional payload data (can be NULL)
 * @param[in] payloadLen Payload length (0 if no payload)
 * @return Total response size including header
 */
size_t NFC_BuildResponse(uint8_t *buffer, uint8_t cmd, uint8_t status,
                         const uint8_t *payload, size_t payloadLen);

/** @} */

/**
 * @defgroup NFC_Task NFC Task Interface
 * @brief FreeRTOS task functions for NFC handling
 * @{
 */

/**
 * @brief Initialize NFC task and create message queue
 * @return Pointer to actor structure (for compatibility)
 */
actor_t* NFC_TaskInit(void);

/**
 * @brief NFC task main function
 * @param[in] argument Unused task argument
 */
void NFC_Task(void *argument);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NFC_H */
