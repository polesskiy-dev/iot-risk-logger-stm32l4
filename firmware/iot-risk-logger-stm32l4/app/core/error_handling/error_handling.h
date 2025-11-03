/*!
 * @file error_handling.h
 * @brief Centralized error handling and logging system
 *
 * This module provides a robust error handling framework for the IoT Risk Logger.
 * It includes error logging, recovery mechanisms, and diagnostic capabilities.
 *
 * @date November 3, 2025
 * @author Architecture Review
 */

#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "cmsis_os2.h"
#include "actors_lookup.h"
#include "events_list.h"

/**
 * @brief Error severity levels
 */
typedef enum {
    ERROR_SEVERITY_INFO = 0,      ///< Informational, no action needed
    ERROR_SEVERITY_WARNING,       ///< Warning, system can continue
    ERROR_SEVERITY_ERROR,         ///< Error, feature unavailable but system operational
    ERROR_SEVERITY_CRITICAL       ///< Critical, system stability compromised
} ErrorSeverity_t;

/**
 * @brief System-wide error codes
 * 
 * Error codes are organized by subsystem (upper byte) and specific error (lower byte)
 * Format: 0xSSEE where SS = subsystem, EE = error code
 */
typedef enum {
    ERR_OK = 0x0000,
    
    // I2C Errors (0x01xx)
    ERR_I2C_TIMEOUT             = 0x0100,
    ERR_I2C_NACK                = 0x0101,
    ERR_I2C_BUS_ERROR           = 0x0102,
    ERR_I2C_ARBITRATION_LOST    = 0x0103,
    
    // SPI/QSPI Errors (0x02xx)
    ERR_QSPI_TIMEOUT            = 0x0200,
    ERR_QSPI_BUSY               = 0x0201,
    ERR_QSPI_ERROR              = 0x0202,
    
    // Sensor Errors (0x03xx)
    ERR_SENSOR_NOT_RESPONDING   = 0x0300,
    ERR_SENSOR_INVALID_ID       = 0x0301,
    ERR_SENSOR_INVALID_DATA     = 0x0302,
    ERR_SENSOR_CALIBRATION      = 0x0303,
    
    // Memory/Flash Errors (0x04xx)
    ERR_FLASH_WRITE_FAIL        = 0x0400,
    ERR_FLASH_ERASE_FAIL        = 0x0401,
    ERR_FLASH_VERIFY_FAIL       = 0x0402,
    ERR_FLASH_FULL              = 0x0403,
    ERR_FLASH_ADDRESS_INVALID   = 0x0404,
    
    // NFC Errors (0x05xx)
    ERR_NFC_TIMEOUT             = 0x0500,
    ERR_NFC_MAILBOX_FULL        = 0x0501,
    ERR_NFC_CRC_ERROR           = 0x0502,
    ERR_NFC_INVALID_CMD         = 0x0503,
    
    // Actor/Queue Errors (0x06xx)
    ERR_QUEUE_FULL              = 0x0600,
    ERR_QUEUE_TIMEOUT           = 0x0601,
    ERR_INVALID_MESSAGE         = 0x0602,
    ERR_INVALID_STATE           = 0x0603,
    
    // System Errors (0x07xx)
    ERR_INVALID_PARAMETER       = 0x0700,
    ERR_BUFFER_OVERFLOW         = 0x0701,
    ERR_NULL_POINTER            = 0x0702,
    ERR_OUT_OF_MEMORY           = 0x0703,
    ERR_RESOURCE_LOCKED         = 0x0704,
    
    // Power Management Errors (0x08xx)
    ERR_POWER_MODE_TRANSITION   = 0x0800,
    ERR_LOW_BATTERY             = 0x0801,
    
    ERR_MAX
} SystemErrorCode_t;

/**
 * @brief Error report structure
 * Contains all information about an error occurrence
 */
typedef struct {
    uint32_t timestamp;             ///< RTC timestamp when error occurred
    ACTOR_ID actorId;               ///< Actor that reported the error
    SystemErrorCode_t errorCode;    ///< Specific error code
    ErrorSeverity_t severity;       ///< Error severity level
    event_t failedEvent;            ///< Event that was being processed when error occurred
    uint32_t contextData;           ///< Additional context-specific data
    uint16_t lineNumber;            ///< Source line number
} ErrorReport_t;

/**
 * @brief Error statistics structure
 */
typedef struct {
    uint32_t totalErrors;
    uint32_t criticalErrors;
    uint32_t errorsByActor[MAX_ACTORS];
    uint32_t lastErrorTimestamp;
} ErrorStatistics_t;

// Error log configuration
#define ERROR_LOG_SIZE 16  ///< Number of errors to keep in circular buffer

/**
 * @brief Initialize error handling system
 * Must be called during system initialization
 */
void ERROR_Init(void);

/**
 * @brief Log an error to the error buffer
 * 
 * @param actorId Actor reporting the error
 * @param errorCode Specific error code
 * @param severity Error severity level
 * @param failedEvent Event being processed when error occurred
 * @param contextData Additional context information
 * @param lineNumber Source file line number
 */
void ERROR_Log(ACTOR_ID actorId, 
               SystemErrorCode_t errorCode,
               ErrorSeverity_t severity,
               event_t failedEvent,
               uint32_t contextData,
               uint16_t lineNumber);

/**
 * @brief Get the most recent error
 * 
 * @return Pointer to most recent error, or NULL if no errors logged
 */
const ErrorReport_t* ERROR_GetLast(void);

/**
 * @brief Get error by index (0 = most recent)
 * 
 * @param index Index into error history (0 to ERROR_LOG_SIZE-1)
 * @return Pointer to error at index, or NULL if invalid index
 */
const ErrorReport_t* ERROR_GetByIndex(uint8_t index);

/**
 * @brief Get total number of errors logged (since last clear)
 * 
 * @return Number of errors in buffer
 */
uint8_t ERROR_GetCount(void);

/**
 * @brief Clear all errors from log
 */
void ERROR_Clear(void);

/**
 * @brief Get error statistics
 * 
 * @param stats Pointer to statistics structure to fill
 */
void ERROR_GetStatistics(ErrorStatistics_t *stats);

/**
 * @brief Convert error code to human-readable string
 * 
 * @param errorCode Error code to convert
 * @return String description of error
 */
const char* ERROR_ToString(SystemErrorCode_t errorCode);

/**
 * @brief Convert severity to string
 * 
 * @param severity Severity level
 * @return String description of severity
 */
const char* ERROR_SeverityToString(ErrorSeverity_t severity);

/**
 * @brief Print error report to stdout
 * 
 * @param error Pointer to error report
 */
void ERROR_Print(const ErrorReport_t *error);

/**
 * @brief Dump all errors in buffer to stdout
 */
void ERROR_DumpAll(void);

/**
 * @brief Macro to simplify error logging with automatic line number
 */
#define ERROR_REPORT(actorId, errorCode, severity, failedEvent, context) \
    ERROR_Log((actorId), (errorCode), (severity), (failedEvent), (context), __LINE__)

/**
 * @brief Macro for critical error reporting with immediate diagnostics
 */
#define ERROR_CRITICAL(actorId, errorCode, failedEvent, context) \
    do { \
        ERROR_REPORT((actorId), (errorCode), ERROR_SEVERITY_CRITICAL, (failedEvent), (context)); \
        fprintf(stderr, "CRITICAL ERROR at %s:%d - Actor %u, Code 0x%04X\n", \
                __FILE__, __LINE__, (actorId), (errorCode)); \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif // ERROR_HANDLING_H
