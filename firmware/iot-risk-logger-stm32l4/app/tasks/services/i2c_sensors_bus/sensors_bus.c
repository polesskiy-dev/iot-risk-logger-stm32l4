/*!
 * @file sensors_bus.c
 * @brief implementation of Sensors I2C Bus Service
 *
 * A dedicated RTOS task that serializes all I2C transactions.
 * All modules submit I2C requests to this service via a message queue.
 * Only this service is allowed to call HAL_I2C_* on the owned I2C handle.
 * Other modules submit requests via I2C_BusService_RequestSync() or by
 * putting I2C_BusService_Request_t into the service queue.
 *
 * @date 16/11/2025
 * @author artempolisskyi
 */

#include "sensors_bus.h"
#include "sensors_bus_conf.h"

static I2C_HandleTypeDef *s_hi2c = NULL;
static osMessageQueueId_t s_i2cQueue = NULL;
static osThreadId_t       s_i2cTaskHandle = NULL;

osStatus_t I2C_BusService_Init(I2C_HandleTypeDef *hi2c)
{
    assert(hi2c != NULL); // "I2C_BusService_Init: hi2c cannot be NULL"

    s_hi2c = hi2c;

    /* Create queue */
    s_i2cQueue = osMessageQueueNew(I2C_BUS_SERVICE_QUEUE_DEPTH,
                                   sizeof(I2C_BusService_Request_t),
                                   NULL);
    if (s_i2cQueue == NULL) {
        return osError;
    }

    /* Create task */
    const osThreadAttr_t taskAttr = {
        .name       = "I2C_BusService",
        .stack_size = I2C_BUS_SERVICE_TASK_STACK,
        .priority   = I2C_BUS_SERVICE_TASK_PRIO,
    };

    s_i2cTaskHandle = osThreadNew(I2C_BusService_Task, NULL, &taskAttr);

    if (s_i2cTaskHandle == NULL) {
        return osError;
    }

    return osOK;
}

HAL_StatusTypeDef I2C_BusService_RequestSync(I2C_BusService_Request_t *req, uint32_t waitMs)
{
    assert(req != NULL); // "I2C_BusService_RequestSync: req cannot be NULL"
    assert(req->pData != NULL); // "I2C_BusService_RequestSync: req->pData cannot be NULL
    assert(req->size != 0U); // "I2C_BusService_RequestSync: req->size cannot be 0"

    HAL_StatusTypeDef status = HAL_ERROR;

    // Clear old completion flag if any
    osThreadFlagsClear(I2C_BUS_SERVICE_FLAG_DONE);

    req->requester   = osThreadGetId();
    req->pStatusOut  = &status;

    /* 1) Put request into queue */
    osStatus_t qst = osMessageQueuePut(s_i2cQueue, req, 0U, waitMs);
    if (qst != osOK) {
        return HAL_BUSY;
    }

    /* 2) Wait for completion flag */
    uint32_t flags = osThreadFlagsWait(I2C_BUS_SERVICE_FLAG_DONE,
                                       osFlagsWaitAny,
                                       waitMs);

    if ((int32_t)flags < 0) {
        /* Timeout or other RTOS error – service did not answer */
        return HAL_ERROR;
    }

    /* status was written by the service task */
    return status;
}

/**
 * @brief Execute a single I2C transaction with retry logic.
 *
 * @param hi2c I2C handle
 * @param req Request descriptor
 * @return HAL_StatusTypeDef Final status after retries
 */
static HAL_StatusTypeDef I2C_ExecuteTransaction(I2C_HandleTypeDef *hi2c, I2C_BusService_Request_t *req)
{
    HAL_StatusTypeDef status = HAL_ERROR;
    uint32_t retries = 0;

    do {
        switch (req->type) {
            case I2C_BUS_SERVICE_REQ_WRITE:
                status = HAL_I2C_Master_Transmit(hi2c,
                                              req->devAddr,
                                              req->pData,
                                              req->size,
                                              req->halTimeoutMs);
                break;

            case I2C_BUS_SERVICE_REQ_READ:
                status = HAL_I2C_Master_Receive(hi2c,
                                             req->devAddr,
                                             req->pData,
                                             req->size,
                                             req->halTimeoutMs);
                break;

            case I2C_BUS_SERVICE_REQ_MEM_WRITE:
                status = HAL_I2C_Mem_Write(hi2c,
                                        req->devAddr,
                                        req->memAddr,
                                        req->memAddrSize,
                                        req->pData,
                                        req->size,
                                        req->halTimeoutMs);
                break;

            case I2C_BUS_SERVICE_REQ_MEM_READ:
                status = HAL_I2C_Mem_Read(hi2c,
                                       req->devAddr,
                                       req->memAddr,
                                       req->memAddrSize,
                                       req->pData,
                                       req->size,
                                       req->halTimeoutMs);
                break;

            default:
                /* Unknown type */
                return HAL_ERROR;
        }

        /* Success - return immediately */
        if (status == HAL_OK) {
            return HAL_OK;
        }

        /* HAL_BUSY - retry after delay */
        if (status == HAL_BUSY && retries < I2C_BUS_SERVICE_MAX_RETRIES) {
            osDelay(I2C_BUS_SERVICE_RETRY_DELAY_MS);
            retries++;
            continue;
        }

        /* HAL_ERROR or HAL_TIMEOUT - check error type */
        if (status == HAL_ERROR || status == HAL_TIMEOUT) {
            uint32_t error = HAL_I2C_GetError(hi2c);
            
            /* Bus error, arbitration lost - try bus reset once */
            if ((error & (HAL_I2C_ERROR_BERR | HAL_I2C_ERROR_ARLO)) && retries == 0) {
                I2C_BusService_Reset();
                retries++;
                continue;
            }
            
            /* Other errors - return immediately */
            return status;
        }

        /* Other status codes */
        return status;

    } while (retries < I2C_BUS_SERVICE_MAX_RETRIES);

    return status;
}

void I2C_BusService_Task(void *argument)
{
    (void)argument;

    I2C_BusService_Request_t req;

    for (;;) {
        if (osMessageQueueGet(s_i2cQueue, &req, NULL, osWaitForever) != osOK) {
            continue;
        }

        HAL_StatusTypeDef status = HAL_ERROR;

        /* If caller forgot hi2c, fall back to default */
        I2C_HandleTypeDef *hi2c = (req.hi2c != NULL) ? req.hi2c : s_hi2c;

        if (hi2c == NULL) {
            if (req.pStatusOut) {
                *req.pStatusOut = HAL_ERROR;
            }
            if (req.requester) {
                osThreadFlagsSet(req.requester, I2C_BUS_SERVICE_FLAG_DONE);
            }
            continue;
        }

        /* Execute transaction with retry and error recovery */
        status = I2C_ExecuteTransaction(hi2c, &req);

        if (req.pStatusOut != NULL) {
            *req.pStatusOut = status;
        }

        if (req.requester != NULL) {
            osThreadFlagsSet(req.requester, I2C_BUS_SERVICE_FLAG_DONE);
        }
    }
}

HAL_StatusTypeDef I2C_BusService_Reset(void)
{
    if (s_hi2c == NULL) {
        return HAL_ERROR;
    }

    /* Disable and deinitialize I2C */
    if (HAL_I2C_DeInit(s_hi2c) != HAL_OK) {
        return HAL_ERROR;
    }

    /* Small delay to ensure peripheral reset */
    osDelay(10);

    /* Reinitialize I2C using the MX_I2C1_Init function */
    extern HAL_StatusTypeDef MX_I2C1_Init(I2C_HandleTypeDef* hi2c);
    if (MX_I2C1_Init(s_hi2c) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}