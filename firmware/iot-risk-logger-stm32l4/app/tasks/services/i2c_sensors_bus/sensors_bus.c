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

void I2C_BusService_Task(void *argument)
{
    (void)argument;

    /** @warning should never be allocated on the stack! */
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

        switch (req.type) {
            case I2C_BUS_SERVICE_REQ_WRITE:
                status = HAL_I2C_Master_Transmit(hi2c,
                                              req.devAddr,
                                              req.pData,
                                              req.size,
                                              req.halTimeoutMs);
                break;

            case I2C_BUS_SERVICE_REQ_READ:
                status = HAL_I2C_Master_Receive(hi2c,
                                             req.devAddr,
                                             req.pData,
                                             req.size,
                                             req.halTimeoutMs);
                break;

            case I2C_BUS_SERVICE_REQ_MEM_WRITE:
                status = HAL_I2C_Mem_Write(hi2c,
                                        req.devAddr,
                                        req.memAddr,
                                        req.memAddrSize,
                                        req.pData,
                                        req.size,
                                        req.halTimeoutMs);
                break;

            case I2C_BUS_SERVICE_REQ_MEM_READ:
                status = HAL_I2C_Mem_Read(hi2c,
                                       req.devAddr,
                                       req.memAddr,
                                       req.memAddrSize,
                                       req.pData,
                                       req.size,
                                       req.halTimeoutMs);
                break;

            default:
                /* Unknown type */
                status = HAL_ERROR;
                break;

        }

        // TODO handle status == HAL_BUSY by retrying? and erorr?

        if (req.pStatusOut != NULL) {
            *req.pStatusOut = status;
        }

        if (req.requester != NULL) {
            osThreadFlagsSet(req.requester, I2C_BUS_SERVICE_FLAG_DONE);
        }
    }
}