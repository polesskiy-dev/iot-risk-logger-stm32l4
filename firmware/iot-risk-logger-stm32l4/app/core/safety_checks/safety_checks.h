/*!
 * @file safety_checks.h
 * @brief Safety and validation macros for defensive programming
 *
 * This header provides macros and functions for input validation,
 * bounds checking, and defensive programming practices.
 *
 * @date November 3, 2025
 * @author Architecture Review
 */

#ifndef SAFETY_CHECKS_H
#define SAFETY_CHECKS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include "cmsis_os2.h"

/**
 * @brief Check for NULL pointer and return error if found
 * 
 * @param ptr Pointer to check
 * @return osError if NULL, osOK otherwise
 */
#define CHECK_NULL_PTR(ptr) \
    do { \
        if ((ptr) == NULL) { \
            fprintf(stderr, "NULL pointer at %s:%d\n", __FILE__, __LINE__); \
            return osError; \
        } \
    } while(0)

/**
 * @brief Check for NULL pointer and return custom value if found
 * 
 * @param ptr Pointer to check
 * @param ret_val Value to return if NULL
 */
#define CHECK_NULL_PTR_RET(ptr, ret_val) \
    do { \
        if ((ptr) == NULL) { \
            fprintf(stderr, "NULL pointer at %s:%d\n", __FILE__, __LINE__); \
            return (ret_val); \
        } \
    } while(0)

/**
 * @brief Check buffer bounds and return error if exceeded
 * 
 * @param size Requested size
 * @param max_size Maximum allowed size
 */
#define CHECK_BUFFER_BOUNDS(size, max_size) \
    do { \
        if ((size) > (max_size)) { \
            fprintf(stderr, "Buffer overflow prevented at %s:%d: size=%u, max=%u\n", \
                    __FILE__, __LINE__, (unsigned)(size), (unsigned)(max_size)); \
            return osError; \
        } \
    } while(0)

/**
 * @brief Check flash address validity
 * 
 * @param addr Starting address
 * @param size Size of access
 * @param flash_size Total flash size
 */
#define CHECK_FLASH_ADDRESS(addr, size, flash_size) \
    do { \
        if ((addr) + (size) > (flash_size)) { \
            fprintf(stderr, "Flash address out of bounds at %s:%d: addr=0x%08lX, size=%u, max=0x%08lX\n", \
                    __FILE__, __LINE__, (unsigned long)(addr), (unsigned)(size), (unsigned long)(flash_size)); \
            return HAL_ERROR; \
        } \
        if ((addr) < 0) { \
            fprintf(stderr, "Invalid flash address at %s:%d: addr=0x%08lX\n", \
                    __FILE__, __LINE__, (unsigned long)(addr)); \
            return HAL_ERROR; \
        } \
    } while(0)

/**
 * @brief Check if value is within valid range
 * 
 * @param val Value to check
 * @param min Minimum valid value (inclusive)
 * @param max Maximum valid value (inclusive)
 */
#define CHECK_RANGE(val, min, max) \
    do { \
        if ((val) < (min) || (val) > (max)) { \
            fprintf(stderr, "Value out of range at %s:%d: val=%d, min=%d, max=%d\n", \
                    __FILE__, __LINE__, (int)(val), (int)(min), (int)(max)); \
            return osError; \
        } \
    } while(0)

/**
 * @brief Assert condition is true, log error if false
 * 
 * @param condition Condition to check
 * @param msg Error message to print if condition is false
 */
#define ASSERT_CONDITION(condition, msg) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "Assertion failed at %s:%d: %s\n", \
                    __FILE__, __LINE__, (msg)); \
            return osError; \
        } \
    } while(0)

/**
 * @brief Check HAL status and convert to osStatus_t
 * 
 * @param hal_status HAL status to check
 */
#define CHECK_HAL_STATUS(hal_status) \
    do { \
        if ((hal_status) != HAL_OK) { \
            fprintf(stderr, "HAL error at %s:%d: status=%d\n", \
                    __FILE__, __LINE__, (int)(hal_status)); \
            return osError; \
        } \
    } while(0)

/**
 * @brief Check queue status and log error if not OK
 * 
 * @param queue_status Queue status to check
 */
#define CHECK_QUEUE_STATUS(queue_status) \
    do { \
        if ((queue_status) != osOK) { \
            fprintf(stderr, "Queue operation failed at %s:%d: status=%d\n", \
                    __FILE__, __LINE__, (int)(queue_status)); \
            return (queue_status); \
        } \
    } while(0)

/**
 * @brief Validate actor pointer and ID
 * 
 * @param actor Actor pointer to validate
 */
#define VALIDATE_ACTOR(actor) \
    do { \
        CHECK_NULL_PTR(actor); \
    } while(0)

/**
 * @brief Validate message structure
 * 
 * @param msg Message pointer to validate
 */
#define VALIDATE_MESSAGE(msg) \
    do { \
        CHECK_NULL_PTR(msg); \
    } while(0)

/**
 * @brief Safe memory copy with bounds checking
 * 
 * @param dest Destination buffer
 * @param dest_size Size of destination buffer
 * @param src Source buffer
 * @param count Number of bytes to copy
 * @return osOK on success, osError on failure
 */
static inline osStatus_t SafeMemcpy(void *dest, size_t dest_size, const void *src, size_t count) {
    if (dest == NULL || src == NULL) {
        fprintf(stderr, "SafeMemcpy: NULL pointer\n");
        return osError;
    }
    
    if (count > dest_size) {
        fprintf(stderr, "SafeMemcpy: Buffer overflow prevented: count=%u > dest_size=%u\n",
                (unsigned)count, (unsigned)dest_size);
        return osError;
    }
    
    memcpy(dest, src, count);
    return osOK;
}

/**
 * @brief Safe string copy with null termination guarantee
 * 
 * @param dest Destination buffer
 * @param dest_size Size of destination buffer
 * @param src Source string
 * @return osOK on success, osError on failure
 */
static inline osStatus_t SafeStrcpy(char *dest, size_t dest_size, const char *src) {
    if (dest == NULL || src == NULL) {
        fprintf(stderr, "SafeStrcpy: NULL pointer\n");
        return osError;
    }
    
    if (dest_size == 0) {
        fprintf(stderr, "SafeStrcpy: Zero-size destination buffer\n");
        return osError;
    }
    
    size_t src_len = strlen(src);
    if (src_len >= dest_size) {
        fprintf(stderr, "SafeStrcpy: String truncation would occur: src_len=%u >= dest_size=%u\n",
                (unsigned)src_len, (unsigned)dest_size);
        return osError;
    }
    
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';  // Guarantee null termination
    return osOK;
}

/**
 * @brief Check if pointer is aligned to specified boundary
 * 
 * @param ptr Pointer to check
 * @param alignment Required alignment (must be power of 2)
 * @return true if aligned, false otherwise
 */
static inline bool IsAligned(const void *ptr, size_t alignment) {
    return ((uintptr_t)ptr & (alignment - 1)) == 0;
}

/**
 * @brief Validate I2C address
 * 
 * @param addr I2C address to validate (7-bit or 8-bit format)
 * @return true if valid, false otherwise
 */
static inline bool IsValidI2CAddress(uint16_t addr) {
    // Check if 7-bit address (0x00-0x7F) or 8-bit address (0x00-0xFE, even)
    if (addr <= 0x7F) {
        return true;  // Valid 7-bit address
    } else if (addr <= 0xFE && (addr & 0x01) == 0) {
        return true;  // Valid 8-bit address (must be even)
    }
    return false;
}

/**
 * @brief Validate flash sector address alignment
 * 
 * @param addr Address to check
 * @param sector_size Sector size in bytes
 * @return true if aligned to sector boundary, false otherwise
 */
static inline bool IsFlashSectorAligned(uint32_t addr, uint32_t sector_size) {
    return (addr % sector_size) == 0;
}

#ifdef __cplusplus
}
#endif

#endif // SAFETY_CHECKS_H
