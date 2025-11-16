/*!
 * @file mock_hal.h
 * @brief Mock STM32 HAL and RTOS types for unit testing
 *
 * @date 16/11/2025
 */

#ifndef MOCK_HAL_H
#define MOCK_HAL_H

#include <stdint.h>
#include <stddef.h>

/* HAL Status */
typedef enum {
    HAL_OK       = 0x00U,
    HAL_ERROR    = 0x01U,
    HAL_BUSY     = 0x02U,
    HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;

/* I2C Handle */
typedef struct {
    void *Instance;
    uint32_t State;
} I2C_HandleTypeDef;

/* I2C Memory Address Size */
#define I2C_MEMADD_SIZE_8BIT    0x00000001U
#define I2C_MEMADD_SIZE_16BIT   0x00000002U

/* I2C Errors */
#define HAL_I2C_ERROR_NONE      0x00000000U
#define HAL_I2C_ERROR_BERR      0x00000001U
#define HAL_I2C_ERROR_ARLO      0x00000002U
#define HAL_I2C_ERROR_AF        0x00000004U
#define HAL_I2C_ERROR_OVR       0x00000008U
#define HAL_I2C_ERROR_DMA       0x00000010U
#define HAL_I2C_ERROR_TIMEOUT   0x00000020U
#define HAL_I2C_ERROR_SIZE      0x00000040U
#define HAL_I2C_ERROR_DMA_PARAM 0x00000080U

/* RTOS Status */
typedef enum {
    osOK                    =  0,
    osError                 = -1,
    osErrorTimeout          = -2,
    osErrorResource         = -3,
    osErrorParameter        = -4,
    osErrorNoMemory         = -5,
    osErrorISR              = -6,
} osStatus_t;

/* RTOS Types */
typedef void * osThreadId_t;
typedef void * osMessageQueueId_t;
typedef void (*osThreadFunc_t)(void *argument);

/* RTOS Attributes */
typedef struct {
    const char *name;
    uint32_t attr_bits;
    void *cb_mem;
    uint32_t cb_size;
    void *stack_mem;
    uint32_t stack_size;
    int32_t priority;
    void *tz_module;
    uint32_t reserved;
} osThreadAttr_t;

typedef struct {
    const char *name;
    uint32_t attr_bits;
    void *cb_mem;
    uint32_t cb_size;
    void *mq_mem;
    uint32_t mq_size;
} osMessageQueueAttr_t;

/* Priority */
typedef enum {
    osPriorityNone          = 0,
    osPriorityIdle          = 1,
    osPriorityLow           = 8,
    osPriorityBelowNormal   = 16,
    osPriorityNormal        = 24,
    osPriorityAboveNormal   = 32,
    osPriorityHigh          = 40,
    osPriorityRealtime      = 48,
    osPriorityISR           = 56,
    osPriorityError         = -1
} osPriority_t;

/* Flags options */
#define osFlagsWaitAny          0x00000000U
#define osFlagsWaitAll          0x00000001U
#define osFlagsNoClear          0x00000002U

#endif /* MOCK_HAL_H */
