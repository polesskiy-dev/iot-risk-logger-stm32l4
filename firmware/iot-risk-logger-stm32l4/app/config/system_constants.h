/*!
 * @file system_constants.h
 * @brief System-wide constants and configuration values
 *
 * This file centralizes all magic numbers and configuration constants
 * used throughout the application to improve maintainability.
 *
 * @date November 3, 2025
 * @author Architecture Review
 */

#ifndef SYSTEM_CONSTANTS_H
#define SYSTEM_CONSTANTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ============================================================================
 * System Configuration
 * ============================================================================ */

/** @defgroup System_Timing System Timing Constants
 * @{
 */
#define SYSTEM_TICK_RATE_HZ                 (1000)      ///< System tick rate in Hz
#define MS_TO_TICKS(ms)                     ((ms) * SYSTEM_TICK_RATE_HZ / 1000)
#define SECONDS_TO_TICKS(sec)               ((sec) * SYSTEM_TICK_RATE_HZ)
/** @} */

/** @defgroup Watchdog Watchdog Configuration
 * @{
 */
#define WATCHDOG_TIMEOUT_MS                 (5000)      ///< Watchdog timeout in milliseconds
#define WATCHDOG_ENABLED                    (0)         ///< Enable/disable watchdog (1/0)
/** @} */

/* ============================================================================
 * NFC Protocol Constants
 * ============================================================================ */

/** @defgroup NFC_Protocol NFC Protocol Constants
 * @{
 */
#define NFC_PROTOCOL_VERSION                (0x01)      ///< Protocol version

// Response codes
#define NFC_RESPONSE_ACK                    (0x00)      ///< Acknowledgment (OK)
#define NFC_RESPONSE_NACK                   (0xFF)      ///< Negative acknowledgment (Error)
#define NFC_RESPONSE_NACK_CRC               (0xFE)      ///< CRC error

// Command codes (matching events_list.h GLOBAL_CMD_* values)
#define NFC_CMD_START_LOGGING               (0xC0)      ///< Start logging measurements
#define NFC_CMD_STOP_LOGGING                (0xC1)      ///< Stop logging measurements
#define NFC_CMD_WRITE_SETTINGS              (0xC2)      ///< Write settings to device
#define NFC_CMD_READ_SETTINGS               (0xC3)      ///< Read settings from device
#define NFC_CMD_READ_LOG_CHUNK              (0xC4)      ///< Read log chunk from device

// Protocol limits
#define NFC_MAILBOX_SIZE                    (256)       ///< ST25DV mailbox size in bytes
#define NFC_MAX_PAYLOAD_SIZE                (253)       ///< Maximum payload size (256 - 3 header bytes)
#define NFC_HEADER_SIZE                     (3)         ///< Header size: CRC + CMD + SIZE
#define NFC_CRC_SIZE                        (1)         ///< CRC field size
#define NFC_CMD_SIZE                        (1)         ///< Command ID field size
#define NFC_PAYLOAD_SIZE_SIZE               (1)         ///< Payload size field size

// Packet offsets
#define NFC_OFFSET_CRC                      (0)         ///< CRC byte offset
#define NFC_OFFSET_CMD                      (1)         ///< Command byte offset
#define NFC_OFFSET_PAYLOAD_SIZE             (2)         ///< Payload size byte offset
#define NFC_OFFSET_PAYLOAD                  (3)         ///< Payload start offset

// Timing
#define NFC_MAILBOX_TIMEOUT_MS              (1000)      ///< Mailbox operation timeout
#define NFC_GPO_DEBOUNCE_MS                 (50)        ///< GPO interrupt debounce time
/** @} */

/* ============================================================================
 * Sensor Configuration
 * ============================================================================ */

/** @defgroup Sensor_Config Sensor Configuration
 * @{
 */

// Default measurement periods (in seconds)
#define DEFAULT_TEMP_HUMIDITY_PERIOD_SEC    (60)        ///< Default T/H measurement period
#define DEFAULT_LIGHT_PERIOD_SEC            (60)        ///< Default light measurement period
#define DEFAULT_ACCEL_PERIOD_SEC            (1)         ///< Default accelerometer period

// Sensor measurement timeouts (in milliseconds)
#define TEMP_HUMIDITY_TIMEOUT_MS            (100)       ///< Temperature/humidity read timeout
#define LIGHT_SENSOR_TIMEOUT_MS             (100)       ///< Light sensor read timeout
#define ACCEL_SENSOR_TIMEOUT_MS             (50)        ///< Accelerometer read timeout

// Sensor power-down delays (in milliseconds)
#define TEMP_HUMIDITY_POWERDOWN_DELAY_MS    (1)         ///< Time to wait after sensor power down
#define LIGHT_SENSOR_POWERDOWN_DELAY_MS     (1)         ///< Time to wait after sensor power down

// Sensor reset timing
#define SENSOR_RESET_PULSE_MS               (1)         ///< Reset pulse duration
#define SENSOR_RESET_RECOVERY_MS            (10)        ///< Time to wait after reset

// Temperature/Humidity sensor units and scaling
#define TEMP_UNITS_CENTI_CELSIUS            (100)       ///< Temperature units: 100 = 1°C
#define HUMIDITY_UNITS_TENTH_PERCENT        (10)        ///< Humidity units: 10 = 1%
/** @} */

/* ============================================================================
 * Memory/Flash Configuration
 * ============================================================================ */

/** @defgroup Memory_Config Memory Configuration
 * @{
 */

// Flash timing
#define FLASH_TIMEOUT_DEFAULT_MS            (1000)      ///< Default flash operation timeout
#define FLASH_PAGE_PROGRAM_TIMEOUT_MS       (3)         ///< Page program timeout
#define FLASH_SECTOR_ERASE_TIMEOUT_MS       (300)       ///< Sector erase timeout
#define FLASH_BLOCK_ERASE_32K_TIMEOUT_MS    (1200)      ///< 32K block erase timeout
#define FLASH_BLOCK_ERASE_64K_TIMEOUT_MS    (2000)      ///< 64K block erase timeout
#define FLASH_CHIP_ERASE_TIMEOUT_MS         (10000)     ///< Chip erase timeout

// Flash busy wait
#define FLASH_BUSY_WAIT_MAX_CYCLES          (0xFFFFFF)  ///< Max cycles to wait for flash ready
#define FLASH_BUSY_WAIT_CYCLES_DEPLETED     (0)         ///< Busy wait cycles depleted indicator

// Log entry configuration
#define LOG_ENTRY_SIZE_BYTES                (16)        ///< Size of one log entry
#define MAX_LOG_ENTRIES                     (10000)     ///< Maximum number of log entries
#define LOG_CHUNK_SIZE_BYTES                (128)       ///< Size of log chunk for NFC transfer

// Flash write batching
#define FLASH_WRITE_CACHE_SIZE              (256)       ///< Flash write cache size (1 page)
/** @} */

/* ============================================================================
 * Power Management
 * ============================================================================ */

/** @defgroup Power_Config Power Management Configuration
 * @{
 */

// Power modes
#define POWER_MODE_RUN                      (0)         ///< Normal run mode
#define POWER_MODE_LOW_POWER_RUN            (1)         ///< Low power run mode
#define POWER_MODE_STOP2                    (2)         ///< Stop 2 mode
#define POWER_MODE_STANDBY                  (3)         ///< Standby mode

// Battery monitoring
#define BATTERY_LOW_THRESHOLD_MV            (2000)      ///< Low battery threshold in mV
#define BATTERY_CRITICAL_THRESHOLD_MV       (1800)      ///< Critical battery threshold in mV
#define BATTERY_CHECK_PERIOD_SEC            (3600)      ///< Battery check period (1 hour)
/** @} */

/* ============================================================================
 * Communication Timeouts
 * ============================================================================ */

/** @defgroup Comm_Timeouts Communication Timeouts
 * @{
 */
#define I2C_TIMEOUT_MS                      (100)       ///< I2C operation timeout
#define SPI_TIMEOUT_MS                      (100)       ///< SPI operation timeout
#define UART_TIMEOUT_MS                     (1000)      ///< UART operation timeout
#define USB_TIMEOUT_MS                      (5000)      ///< USB operation timeout
/** @} */

/* ============================================================================
 * Actor/Queue Configuration
 * ============================================================================ */

/** @defgroup Actor_Config Actor/Queue Configuration
 * @{
 */

// Default queue sizes
#define DEFAULT_ACTOR_QUEUE_SIZE            (8)         ///< Default message queue depth
#define EVENT_MANAGER_QUEUE_SIZE            (16)        ///< Event manager queue depth (larger)

// Default stack sizes (in words, not bytes!)
#define DEFAULT_ACTOR_STACK_SIZE_WORDS      (128)       ///< Default actor task stack (512 bytes)
#define LARGE_ACTOR_STACK_SIZE_WORDS        (256)       ///< Large actor task stack (1024 bytes)

// Message retry configuration
#define MESSAGE_RETRY_MAX_ATTEMPTS          (3)         ///< Max attempts to send a message
#define MESSAGE_RETRY_DELAY_MS              (100)       ///< Delay between retry attempts
/** @} */

/* ============================================================================
 * Debug and Diagnostics
 * ============================================================================ */

/** @defgroup Debug_Config Debug Configuration
 * @{
 */

// Error logging
#define ERROR_LOG_BUFFER_SIZE               (16)        ///< Number of errors to keep in log

// Debug output
#define DEBUG_BUFFER_SIZE                   (256)       ///< Debug string buffer size
#define DEBUG_UART_BAUD_RATE                (115200)    ///< Debug UART baud rate

// Performance monitoring
#define ENABLE_PERFORMANCE_MONITORING       (0)         ///< Enable performance counters (1/0)
#define ENABLE_STACK_USAGE_MONITORING       (0)         ///< Enable stack usage monitoring (1/0)
/** @} */

/* ============================================================================
 * Validation Macros
 * ============================================================================ */

/** @brief Compile-time assertion to verify constants */
#define STATIC_ASSERT(expr, msg) \
    typedef char static_assertion_##msg[(expr) ? 1 : -1]

// Validate that NFC payload size is correct
STATIC_ASSERT(NFC_MAX_PAYLOAD_SIZE == (NFC_MAILBOX_SIZE - NFC_HEADER_SIZE), 
              nfc_payload_size_matches_mailbox);

// Validate that log entry size is reasonable for flash page size
STATIC_ASSERT(LOG_ENTRY_SIZE_BYTES <= FLASH_WRITE_CACHE_SIZE,
              log_entry_fits_in_cache);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_CONSTANTS_H
