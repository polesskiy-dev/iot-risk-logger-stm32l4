/**
 * @file nfc.c
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

#include "nfc.h"
#include "nfc_handlers.h"
#include "fs_static.h"
#include "memory.h"
#include "w25q.h"
#include "events_list.h"

/* CRC-8/NRSC-5 lookup table (polynomial 0x31, init 0xFF, no reflect, no xor) */
static const uint8_t crc8Table[256] = {
    0x00, 0x31, 0x62, 0x53, 0xC4, 0xF5, 0xA6, 0x97,
    0xB9, 0x88, 0xDB, 0xEA, 0x7D, 0x4C, 0x1F, 0x2E,
    0x43, 0x72, 0x21, 0x10, 0x87, 0xB6, 0xE5, 0xD4,
    0xFA, 0xCB, 0x98, 0xA9, 0x3E, 0x0F, 0x5C, 0x6D,
    0x86, 0xB7, 0xE4, 0xD5, 0x42, 0x73, 0x20, 0x11,
    0x3F, 0x0E, 0x5D, 0x6C, 0xFB, 0xCA, 0x99, 0xA8,
    0xC5, 0xF4, 0xA7, 0x96, 0x01, 0x30, 0x63, 0x52,
    0x7C, 0x4D, 0x1E, 0x2F, 0xB8, 0x89, 0xDA, 0xEB,
    0x3D, 0x0C, 0x5F, 0x6E, 0xF9, 0xC8, 0x9B, 0xAA,
    0x84, 0xB5, 0xE6, 0xD7, 0x40, 0x71, 0x22, 0x13,
    0x7E, 0x4F, 0x1C, 0x2D, 0xBA, 0x8B, 0xD8, 0xE9,
    0xC7, 0xF6, 0xA5, 0x94, 0x03, 0x32, 0x61, 0x50,
    0xBB, 0x8A, 0xD9, 0xE8, 0x7F, 0x4E, 0x1D, 0x2C,
    0x02, 0x33, 0x60, 0x51, 0xC6, 0xF7, 0xA4, 0x95,
    0xF8, 0xC9, 0x9A, 0xAB, 0x3C, 0x0D, 0x5E, 0x6F,
    0x41, 0x70, 0x23, 0x12, 0x85, 0xB4, 0xE7, 0xD6,
    0x7A, 0x4B, 0x18, 0x29, 0xBE, 0x8F, 0xDC, 0xED,
    0xC3, 0xF2, 0xA1, 0x90, 0x07, 0x36, 0x65, 0x54,
    0x39, 0x08, 0x5B, 0x6A, 0xFD, 0xCC, 0x9F, 0xAE,
    0x80, 0xB1, 0xE2, 0xD3, 0x44, 0x75, 0x26, 0x17,
    0xFC, 0xCD, 0x9E, 0xAF, 0x38, 0x09, 0x5A, 0x6B,
    0x45, 0x74, 0x27, 0x16, 0x81, 0xB0, 0xE3, 0xD2,
    0xBF, 0x8E, 0xDD, 0xEC, 0x7B, 0x4A, 0x19, 0x28,
    0x06, 0x37, 0x64, 0x55, 0xC2, 0xF3, 0xA0, 0x91,
    0x47, 0x76, 0x25, 0x14, 0x83, 0xB2, 0xE1, 0xD0,
    0xFE, 0xCF, 0x9C, 0xAD, 0x3A, 0x0B, 0x58, 0x69,
    0x04, 0x35, 0x66, 0x57, 0xC0, 0xF1, 0xA2, 0x93,
    0xBD, 0x8C, 0xDF, 0xEE, 0x79, 0x48, 0x1B, 0x2A,
    0xC1, 0xF0, 0xA3, 0x92, 0x05, 0x34, 0x67, 0x56,
    0x78, 0x49, 0x1A, 0x2B, 0xBC, 0x8D, 0xDE, 0xEF,
    0x82, 0xB3, 0xE0, 0xD1, 0x46, 0x77, 0x24, 0x15,
    0x3B, 0x0A, 0x59, 0x68, 0xFF, 0xCE, 0x9D, 0xAC
};

/* Forward declarations for internal functions */
static int nfc_handle_command(uint8_t cmd, const uint8_t *payload, size_t payloadLen);
static int nfc_dispatch_start_logging(void);
static int nfc_dispatch_stop_logging(void);
static int nfc_dispatch_write_settings(const uint8_t *payload, size_t payloadLen);
static int nfc_dispatch_read_settings(void);
static int nfc_dispatch_read_log_chunk(const uint8_t *payload, size_t payloadLen);

/* External references */
extern actor_t* ACTORS_LOOKUP_SystemRegistry[MAX_ACTORS];
extern W25Q_HandleTypeDef MEMORY_W25QHandle;

/* Global NFC context */
NFC_Context_t NFC_Context = {
    .st25dv = {0},
    .mailboxBuffer = {0},
    .osMessageQueueId = NULL,
    .osThreadId = NULL
};

/* Actor wrapper for system registry compatibility */
static actor_t nfc_actor_wrapper = {
    .actorId = NFC_ACTOR_ID,
    .messageHandler = NULL,
    .osMessageQueueId = NULL,
    .osThreadId = NULL
};

/* Task stack and control block */
static uint32_t nfcTaskBuffer[DEFAULT_TASK_STACK_SIZE_WORDS];
static StaticTask_t nfcTaskControlBlock;
static const osThreadAttr_t nfcTaskDescription = {
    .name = "nfcTask",
    .cb_mem = &nfcTaskControlBlock,
    .cb_size = sizeof(nfcTaskControlBlock),
    .stack_mem = &nfcTaskBuffer[0],
    .stack_size = sizeof(nfcTaskBuffer),
    .priority = (osPriority_t) osPriorityNormal,
};

/* ============================================================================
 * POSIX-style NFC API Implementation
 * ============================================================================ */

int NFC_Init(void)
{
    ST25DV_IO_t io;
    ST25DV_UID uid = {0x00000000, 0x00000000};
    const ST25DV_PASSWD i2cPwd = {0x00000000, 0x00000000};
    int32_t status;

    /* Configure I/O callbacks for ST25DV driver */
    io.Init = BSP_I2C1_Init;
    io.DeInit = BSP_I2C1_DeInit;
    io.IsReady = BSP_I2C1_IsReady;
    io.Read = BSP_I2C1_ReadReg16;
    io.Write = (ST25DV_Write_Func) BSP_I2C1_WriteReg16;
    io.GetTick = HAL_GetTick;

    /* Register bus I/O */
    status = ST25DV_RegisterBusIO(&NFC_Context.st25dv, &io);
    if (status != NFCTAG_OK) {
#ifdef DEBUG
        fprintf(stderr, "NFC: ST25DV RegisterBusIO failed: %ld\n", status);
#endif
        return NFC_ERROR_IO;
    }

    /* Initialize ST25DV driver */
    status = St25Dv_Drv.Init(&NFC_Context.st25dv);
    if (status != NFCTAG_OK) {
#ifdef DEBUG
        fprintf(stderr, "NFC: ST25DV Init failed: %ld\n", status);
#endif
        return NFC_ERROR_IO;
    }

    /* Present I2C password to enable mailbox access */
    status = ST25DV_PresentI2CPassword(&NFC_Context.st25dv, i2cPwd);
    if (status != NFCTAG_OK) {
#ifdef DEBUG
        fprintf(stderr, "NFC: I2C password presentation failed: %ld\n", status);
#endif
        return NFC_ERROR_IO;
    }

    /* Read UID for verification */
    status = ST25DV_ReadUID(&NFC_Context.st25dv, &uid);
    if (status != NFCTAG_OK) {
#ifdef DEBUG
        fprintf(stderr, "NFC: UID read failed: %ld\n", status);
#endif
        return NFC_ERROR_IO;
    }

#ifdef DEBUG
    fprintf(stdout, "NFC: Initialized, UID: 0x%08lX%08lX\n",
            (unsigned long)uid.MsbUid, (unsigned long)uid.LsbUid);
#endif

    return NFC_OK;
}

ssize_t NFC_Read(uint8_t *buffer, size_t size)
{
    uint8_t mailboxLength;
    int32_t status;

    if (buffer == NULL || size == 0) {
        return NFC_ERROR;
    }

    /* Read mailbox length */
    status = ST25DV_ReadMBLength_Dyn(&NFC_Context.st25dv, &mailboxLength);
    if (status != NFCTAG_OK) {
#ifdef DEBUG
        fprintf(stderr, "NFC: ReadMBLength failed\n");
#endif
        return NFC_ERROR_IO;
    }

    /* Clamp to requested size */
    size_t readSize = (mailboxLength < size) ? mailboxLength : size;

    /* Read mailbox data */
    status = ST25DV_ReadMailboxData(&NFC_Context.st25dv, buffer, 0, readSize);
    if (status != NFCTAG_OK) {
#ifdef DEBUG
        fprintf(stderr, "NFC: ReadMailboxData failed\n");
#endif
        return NFC_ERROR_IO;
    }

#ifdef DEBUG
    fprintf(stdout, "NFC: Read %zu bytes from mailbox\n", readSize);
#endif

    return (ssize_t)readSize;
}

ssize_t NFC_Write(const uint8_t *buffer, size_t size)
{
    int32_t status;

    if (buffer == NULL || size == 0 || size > ST25DV_MAX_MAILBOX_LENGTH) {
        return NFC_ERROR;
    }

    /* Write data to mailbox */
    status = ST25DV_WriteMailboxData(&NFC_Context.st25dv, buffer, size);
    if (status != NFCTAG_OK) {
#ifdef DEBUG
        fprintf(stderr, "NFC: WriteMailboxData failed\n");
#endif
        return NFC_ERROR_IO;
    }

#ifdef DEBUG
    fprintf(stdout, "NFC: Wrote %zu bytes to mailbox\n", size);
#endif

    return (ssize_t)size;
}

uint8_t NFC_ComputeCRC8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;  /* CRC-8/NRSC-5 initial value */

    for (size_t i = 0; i < len; i++) {
        crc = crc8Table[crc ^ data[i]];
    }

    return crc;
}

bool NFC_ValidateCRC8(const uint8_t *buffer)
{
    uint8_t receivedCrc = buffer[NFC_MAILBOX_PROTOCOL_CRC8_ADDR];
    uint8_t payloadSize = buffer[NFC_MAILBOX_PROTOCOL_PAYLOAD_SIZE_ADDR];

    /* CRC covers CMD + Payload Size + Payload */
    size_t crcLen = NFC_MAILBOX_PROTOCOL_CMD_SIZE +
                    NFC_MAILBOX_PROTOCOL_PAYLOAD_SIZE_SIZE +
                    payloadSize;

    uint8_t computedCrc = NFC_ComputeCRC8(&buffer[NFC_MAILBOX_PROTOCOL_CMD_ADDR], crcLen);

    return (receivedCrc == computedCrc);
}

size_t NFC_BuildResponse(uint8_t *buffer, uint8_t cmd, uint8_t status,
                         const uint8_t *payload, size_t payloadLen)
{
    /* Build response packet */
    buffer[NFC_MAILBOX_PROTOCOL_CMD_ADDR] = cmd;
    buffer[NFC_MAILBOX_PROTOCOL_PAYLOAD_SIZE_ADDR] = (uint8_t)(payloadLen + 1); /* +1 for status byte */
    buffer[NFC_MAILBOX_PROTOCOL_PAYLOAD_ADDR] = status;

    /* Copy payload if present */
    if (payload != NULL && payloadLen > 0) {
        memcpy(&buffer[NFC_MAILBOX_PROTOCOL_PAYLOAD_ADDR + 1], payload, payloadLen);
    }

    /* Compute CRC over CMD + Payload Size + Status + Payload */
    size_t crcLen = NFC_MAILBOX_PROTOCOL_CMD_SIZE +
                    NFC_MAILBOX_PROTOCOL_PAYLOAD_SIZE_SIZE +
                    1 + payloadLen;  /* 1 for status byte */

    buffer[NFC_MAILBOX_PROTOCOL_CRC8_ADDR] = NFC_ComputeCRC8(&buffer[NFC_MAILBOX_PROTOCOL_CMD_ADDR], crcLen);

    return NFC_MAILBOX_PROTOCOL_HEADER_SIZE + 1 + payloadLen;  /* Header + status + payload */
}

/* ============================================================================
 * NFC Task Implementation
 * ============================================================================ */

actor_t* NFC_TaskInit(void)
{
    /* Create message queue for GPO events */
    NFC_Context.osMessageQueueId = osMessageQueueNew(DEFAULT_QUEUE_SIZE, DEFAULT_QUEUE_MESSAGE_SIZE,
        &(osMessageQueueAttr_t){ .name = "nfcQueue" });

    /* Create NFC task */
    NFC_Context.osThreadId = osThreadNew(NFC_Task, NULL, &nfcTaskDescription);

    /* Update actor wrapper for system registry */
    nfc_actor_wrapper.osMessageQueueId = NFC_Context.osMessageQueueId;
    nfc_actor_wrapper.osThreadId = NFC_Context.osThreadId;

    return &nfc_actor_wrapper;
}

void NFC_Task(void *argument)
{
    (void)argument;
    message_t msg;
    int result;

    /* Initialize NFC hardware */
    result = NFC_Init();
    if (result != NFC_OK) {
#ifdef DEBUG
        fprintf(stderr, "NFC: Initialization failed with error %d\n", result);
#endif
        /* Notify error to event manager */
        osMessageQueueId_t evManagerQueue = ACTORS_LOOKUP_SystemRegistry[EV_MANAGER_ACTOR_ID]->osMessageQueueId;
        osMessageQueuePut(evManagerQueue, &(message_t){GLOBAL_ERROR, .payload.value = NFC_ACTOR_ID}, 0, 0);
        return;
    }

#ifdef DEBUG
    fprintf(stdout, "NFC: Task started, waiting for GPO events\n");
#endif

    /* Main task loop - synchronous request/response */
    for (;;) {
        /* Wait for GPO interrupt event */
        if (osMessageQueueGet(NFC_Context.osMessageQueueId, &msg, NULL, osWaitForever) != osOK) {
            continue;
        }

        /* Handle GPO interrupt */
        if (msg.event == NFC_GPO_INTERRUPT) {
            uint8_t itStatus;

            /* Read interrupt status */
            ST25DV_ReadITSTStatus_Dyn(&NFC_Context.st25dv, &itStatus);

            /* Check if RF put message to mailbox */
            if (!(itStatus & ST25DV_ITSTS_DYN_RFPUTMSG_MASK)) {
                continue;
            }

#ifdef DEBUG
            fprintf(stdout, "NFC: GPO interrupt, ITStatus: 0x%02X\n", itStatus);
#endif

            /* Read mailbox data */
            ssize_t bytesRead = NFC_Read(NFC_Context.mailboxBuffer, ST25DV_MAX_MAILBOX_LENGTH);
            if (bytesRead < NFC_MAILBOX_PROTOCOL_HEADER_SIZE) {
#ifdef DEBUG
                fprintf(stderr, "NFC: Mailbox read failed or too short\n");
#endif
                continue;
            }

            /* Validate CRC */
            if (!NFC_ValidateCRC8(NFC_Context.mailboxBuffer)) {
#ifdef DEBUG
                fprintf(stderr, "NFC: CRC validation failed\n");
#endif
                /* Send CRC error response */
                size_t respLen = NFC_BuildResponse(NFC_Context.mailboxBuffer,
                                                   NFC_RESPONSE_CRC_ERROR,
                                                   NFC_RESPONSE_ERROR,
                                                   NULL, 0);
                NFC_Write(NFC_Context.mailboxBuffer, respLen);
                continue;
            }

            /* Extract command and payload */
            uint8_t cmd = NFC_Context.mailboxBuffer[NFC_MAILBOX_PROTOCOL_CMD_ADDR];
            uint8_t payloadSize = NFC_Context.mailboxBuffer[NFC_MAILBOX_PROTOCOL_PAYLOAD_SIZE_ADDR];
            uint8_t *payload = &NFC_Context.mailboxBuffer[NFC_MAILBOX_PROTOCOL_PAYLOAD_ADDR];

#ifdef DEBUG
            fprintf(stdout, "NFC: Received CMD: 0x%02X, PayloadSize: %u\n", cmd, payloadSize);
#endif

            /* Dispatch command synchronously */
            result = nfc_handle_command(cmd, payload, payloadSize);

            /* Build and send response only if handler didn't already send one */
            if (result != NFC_OK_RESPONSE_SENT) {
                uint8_t status = (result == NFC_OK) ? NFC_RESPONSE_OK : NFC_RESPONSE_ERROR;
                size_t respLen = NFC_BuildResponse(NFC_Context.mailboxBuffer, cmd, status, NULL, 0);
                NFC_Write(NFC_Context.mailboxBuffer, respLen);
            }
        }
    }
}

/* ============================================================================
 * Command Dispatch Implementation
 * ============================================================================ */

/**
 * @brief Handle received command and dispatch to appropriate handler
 */
static int nfc_handle_command(uint8_t cmd, const uint8_t *payload, size_t payloadLen)
{
    switch (cmd) {
        case GLOBAL_CMD_START_LOGGING:
            return nfc_dispatch_start_logging();

        case GLOBAL_CMD_STOP_LOGGING:
            return nfc_dispatch_stop_logging();

        case GLOBAL_CMD_WRITE_SETTINGS:
            return nfc_dispatch_write_settings(payload, payloadLen);

        case GLOBAL_CMD_READ_SETTINGS:
            return nfc_dispatch_read_settings();

        case GLOBAL_CMD_READ_LOG_CHUNK:
            return nfc_dispatch_read_log_chunk(payload, payloadLen);

        default:
#ifdef DEBUG
            fprintf(stderr, "NFC: Unknown command: 0x%02X\n", cmd);
#endif
            return NFC_ERROR_INVALID_CMD;
    }
}

/**
 * @brief Start logging command handler
 */
static int nfc_dispatch_start_logging(void)
{
    osMessageQueueId_t evManagerQueue = ACTORS_LOOKUP_SystemRegistry[EV_MANAGER_ACTOR_ID]->osMessageQueueId;
    osMessageQueuePut(evManagerQueue, &(message_t){GLOBAL_CMD_START_LOGGING}, 0, 0);

#ifdef DEBUG
    fprintf(stdout, "NFC: Start logging command dispatched\n");
#endif

    return NFC_OK;
}

/**
 * @brief Stop logging command handler
 */
static int nfc_dispatch_stop_logging(void)
{
    osMessageQueueId_t evManagerQueue = ACTORS_LOOKUP_SystemRegistry[EV_MANAGER_ACTOR_ID]->osMessageQueueId;
    osMessageQueuePut(evManagerQueue, &(message_t){GLOBAL_CMD_STOP_LOGGING}, 0, 0);

#ifdef DEBUG
    fprintf(stdout, "NFC: Stop logging command dispatched\n");
#endif

    return NFC_OK;
}

/**
 * @brief Write settings command handler
 * @note Settings data is expected in the payload
 */
static int nfc_dispatch_write_settings(const uint8_t *payload, size_t payloadLen)
{
    if (payload == NULL || payloadLen == 0 || payloadLen > SETTINGS_DATA_SIZE) {
        return NFC_ERROR;
    }

    /* Wake up NOR flash */
    W25Q_WakeUp(&MEMORY_W25QHandle);

    /* Erase settings sector */
    HAL_StatusTypeDef status = W25Q_EraseSector(&MEMORY_W25QHandle, SETTINGS_FILE_ADDR);
    if (status != HAL_OK) {
#ifdef DEBUG
        fprintf(stderr, "NFC: Settings sector erase failed\n");
#endif
        W25Q_Sleep(&MEMORY_W25QHandle);
        return NFC_ERROR_IO;
    }

    /* Write settings to flash */
    status = W25Q_WriteData(&MEMORY_W25QHandle, payload, SETTINGS_FILE_ADDR, payloadLen);
    if (status != HAL_OK) {
#ifdef DEBUG
        fprintf(stderr, "NFC: Settings write failed\n");
#endif
        W25Q_Sleep(&MEMORY_W25QHandle);
        return NFC_ERROR_IO;
    }

    /* Put flash back to sleep */
    W25Q_Sleep(&MEMORY_W25QHandle);

#ifdef DEBUG
    fprintf(stdout, "NFC: Settings written (%zu bytes)\n", payloadLen);
#endif

    return NFC_OK;
}

/**
 * @brief Read settings command handler
 * @note Settings data will be placed in response payload
 * @return NFC_OK_RESPONSE_SENT on success (response already sent), negative error code on failure
 */
static int nfc_dispatch_read_settings(void)
{
    uint8_t settingsBuffer[SETTINGS_DATA_SIZE];

    /* Wake up NOR flash */
    W25Q_WakeUp(&MEMORY_W25QHandle);

    /* Read settings from flash */
    HAL_StatusTypeDef status = W25Q_ReadData(&MEMORY_W25QHandle, settingsBuffer,
                                              SETTINGS_FILE_ADDR, SETTINGS_DATA_SIZE);

    /* Put flash back to sleep */
    W25Q_Sleep(&MEMORY_W25QHandle);

    if (status != HAL_OK) {
#ifdef DEBUG
        fprintf(stderr, "NFC: Settings read failed\n");
#endif
        return NFC_ERROR_IO;
    }

    /* Build response with settings data */
    size_t respLen = NFC_BuildResponse(NFC_Context.mailboxBuffer,
                                        GLOBAL_CMD_READ_SETTINGS,
                                        NFC_RESPONSE_OK,
                                        settingsBuffer,
                                        SETTINGS_DATA_SIZE);

    /* Write response to mailbox */
    ssize_t written = NFC_Write(NFC_Context.mailboxBuffer, respLen);
    if (written < 0) {
        return NFC_ERROR_IO;
    }

#ifdef DEBUG
    fprintf(stdout, "NFC: Settings read and sent (%zu bytes)\n", SETTINGS_DATA_SIZE);
#endif

    return NFC_OK_RESPONSE_SENT;
}

/**
 * @brief Read log chunk command handler
 * @note Payload should contain the log address offset (4 bytes, little-endian)
 * @note End-of-log is detected by checking if all bytes are 0xFF (erased flash state)
 *       This is standard for NOR flash memory where erased state is 0xFF
 * @return NFC_OK_RESPONSE_SENT on success (response already sent), negative error code on failure
 */
static int nfc_dispatch_read_log_chunk(const uint8_t *payload, size_t payloadLen)
{
    uint32_t logAddr;
    uint8_t logBuffer[NFC_MAX_LOG_CHUNK_SIZE];

    /* Extract log address from payload (4 bytes, little-endian) */
    if (payloadLen >= sizeof(uint32_t)) {
        memcpy(&logAddr, payload, sizeof(uint32_t));  /* Safe on little-endian STM32 */
    } else {
        logAddr = INITIAL_LOG_START_ADDR;
    }

    /* Calculate available space for log data in response */
    size_t maxLogSize = sizeof(logBuffer);

    /* Wake up NOR flash */
    W25Q_WakeUp(&MEMORY_W25QHandle);

    /* Read log data from flash */
    HAL_StatusTypeDef status = W25Q_ReadData(&MEMORY_W25QHandle, logBuffer, logAddr, maxLogSize);

    /* Put flash back to sleep */
    W25Q_Sleep(&MEMORY_W25QHandle);

    if (status != HAL_OK) {
#ifdef DEBUG
        fprintf(stderr, "NFC: Log chunk read failed\n");
#endif
        return NFC_ERROR_IO;
    }

    /*
     * Check if we've reached end of log (all 0xFF means empty/erased flash)
     * This is the standard erased state for NOR flash memory
     */
    bool isEndOfLog = true;
    for (size_t i = 0; i < maxLogSize; i++) {
        if (logBuffer[i] != 0xFF) {
            isEndOfLog = false;
            break;
        }
    }

    /* Build response with log data */
    uint8_t responseStatus = isEndOfLog ? NFC_RESPONSE_END_OF_LOG : NFC_RESPONSE_OK;
    size_t respLen = NFC_BuildResponse(NFC_Context.mailboxBuffer,
                                        GLOBAL_CMD_READ_LOG_CHUNK,
                                        responseStatus,
                                        logBuffer,
                                        maxLogSize);

    /* Write response to mailbox */
    ssize_t written = NFC_Write(NFC_Context.mailboxBuffer, respLen);
    if (written < 0) {
        return NFC_ERROR_IO;
    }

#ifdef DEBUG
    fprintf(stdout, "NFC: Log chunk read from 0x%08lX (%zu bytes, endOfLog=%d)\n",
            (unsigned long)logAddr, maxLogSize, isEndOfLog);
#endif

    return NFC_OK_RESPONSE_SENT;
}