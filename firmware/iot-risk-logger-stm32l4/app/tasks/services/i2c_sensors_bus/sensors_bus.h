#ifndef SENSORS_BUS_H
#define SENSORS_BUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <assert.h>

#include "main.h"

typedef enum {
    I2C_BUS_SERVICE_REQ_WRITE = 0,
    I2C_BUS_SERVICE_REQ_READ,
    I2C_BUS_SERVICE_REQ_MEM_WRITE,
    I2C_BUS_SERVICE_REQ_MEM_READ,
} I2C_BusServiceReqType_t;

/**
 * @brief Request descriptor for a single I2C transaction.
 *
 * This is what callers send to the I2C Bus Service.
 */
typedef struct {
    I2C_BusServiceReqType_t type;

    I2C_HandleTypeDef *hi2c;   /**< Target I2C handle, e.g. &hi2c1 */
    uint16_t devAddr;          /**< Device address (HAL-style, already shifted) */

    uint16_t memAddr;          /**< Memory/register address for *_MEM_* types */
    uint16_t memAddrSize;      /**< I2C_MEMADD_SIZE_8BIT / 16BIT */

    uint8_t  *pData;           /**< Data buffer */
    uint16_t size;             /**< Data length */
    uint32_t halTimeoutMs;     /**< Timeout passed to HAL */

    /* Completion / status */
    osThreadId_t requester;               /**< Thread to notify when done */
    HAL_StatusTypeDef *pStatusOut;   /**< Pointer to write status into */
} I2C_BusService_Request_t;

/* ---------- Public API ---------- */

/**
 * @brief Initialize the I2C Bus Service.
 *
 * Creates the queue and the service task. Should be called once at startup,
 * after HAL and RTOS are initialized, and after hi2c is configured.
 *
 * @param hi2c Pointer to I2C handle the service will own (e.g. &hi2c1).
 * @return osStatus_t
 */
osStatus_t I2C_BusService_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief Synchronous helper: send a request and wait for completion.
 *
 * This function:
 *  1) fills in requester + pStatusOut
 *  2) enqueues the request to the service queue
 *  3) waits on a thread flag until the service completes the transaction
 *
 * @param req        Pointer to prepared request structure.
 * @param waitMs     Max time to wait for completion (RTOS wait).
 * @return I2C_BusServiceStatus_t  Final status of the transaction.
 */
HAL_StatusTypeDef I2C_BusService_RequestSync(I2C_BusService_Request_t *req, uint32_t waitMs);

/**
 * @brief The service task entry function.
 *
 * Normally, you don't call this directly; it's passed to osThreadNew in Init.
 */
void I2C_BusService_Task(void *argument);

/**
 * @brief Reset and reinitialize the I2C bus.
 *
 * This function performs a hardware reset of the I2C peripheral.
 * Useful for recovery from bus errors or after wake from low-power modes.
 *
 * @return HAL_StatusTypeDef Status of the reset operation.
 */
HAL_StatusTypeDef I2C_BusService_Reset(void);

#ifdef __cplusplus
}
#endif

#endif //SENSORS_BUS_H