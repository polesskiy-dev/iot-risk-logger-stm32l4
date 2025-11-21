# Implementation Guide - Architecture Improvements

This guide provides step-by-step instructions for implementing the recommendations from the Architecture Review.

---

## Quick Start: Implementing the New Headers

### Step 1: Integrate Error Handling Framework

**1.1 Add error_handling.c implementation**

Create `firmware/iot-risk-logger-stm32l4/app/core/error_handling/error_handling.c`:

```c
#include "error_handling.h"
#include <string.h>

// Error log circular buffer
static ErrorReport_t errorLog[ERROR_LOG_SIZE];
static uint8_t errorLogIndex = 0;
static uint8_t errorLogCount = 0;
static ErrorStatistics_t errorStats = {0};

void ERROR_Init(void) {
    memset(errorLog, 0, sizeof(errorLog));
    memset(&errorStats, 0, sizeof(errorStats));
    errorLogIndex = 0;
    errorLogCount = 0;
}

void ERROR_Log(ACTOR_ID actorId, 
               SystemErrorCode_t errorCode,
               ErrorSeverity_t severity,
               event_t failedEvent,
               uint32_t contextData,
               uint16_t lineNumber) {
    
    ErrorReport_t *error = &errorLog[errorLogIndex];
    
    error->timestamp = HAL_GetTick();  // Or use RTC timestamp
    error->actorId = actorId;
    error->errorCode = errorCode;
    error->severity = severity;
    error->failedEvent = failedEvent;
    error->contextData = contextData;
    error->lineNumber = lineNumber;
    
    // Update circular buffer index
    errorLogIndex = (errorLogIndex + 1) % ERROR_LOG_SIZE;
    if (errorLogCount < ERROR_LOG_SIZE) {
        errorLogCount++;
    }
    
    // Update statistics
    errorStats.totalErrors++;
    if (severity == ERROR_SEVERITY_CRITICAL) {
        errorStats.criticalErrors++;
    }
    if (actorId < MAX_ACTORS) {
        errorStats.errorsByActor[actorId]++;
    }
    errorStats.lastErrorTimestamp = error->timestamp;
}

const ErrorReport_t* ERROR_GetLast(void) {
    if (errorLogCount == 0) return NULL;
    uint8_t lastIndex = (errorLogIndex == 0) ? (ERROR_LOG_SIZE - 1) : (errorLogIndex - 1);
    return &errorLog[lastIndex];
}

const ErrorReport_t* ERROR_GetByIndex(uint8_t index) {
    if (index >= errorLogCount) return NULL;
    uint8_t actualIndex = (errorLogIndex - 1 - index + ERROR_LOG_SIZE) % ERROR_LOG_SIZE;
    return &errorLog[actualIndex];
}

uint8_t ERROR_GetCount(void) {
    return errorLogCount;
}

void ERROR_Clear(void) {
    ERROR_Init();
}

void ERROR_GetStatistics(ErrorStatistics_t *stats) {
    if (stats != NULL) {
        memcpy(stats, &errorStats, sizeof(ErrorStatistics_t));
    }
}

const char* ERROR_ToString(SystemErrorCode_t errorCode) {
    switch (errorCode) {
        case ERR_OK: return "OK";
        case ERR_I2C_TIMEOUT: return "I2C Timeout";
        case ERR_I2C_NACK: return "I2C NACK";
        case ERR_SENSOR_NOT_RESPONDING: return "Sensor Not Responding";
        case ERR_FLASH_WRITE_FAIL: return "Flash Write Failed";
        // Add more as needed
        default: return "Unknown Error";
    }
}

const char* ERROR_SeverityToString(ErrorSeverity_t severity) {
    switch (severity) {
        case ERROR_SEVERITY_INFO: return "INFO";
        case ERROR_SEVERITY_WARNING: return "WARNING";
        case ERROR_SEVERITY_ERROR: return "ERROR";
        case ERROR_SEVERITY_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

void ERROR_Print(const ErrorReport_t *error) {
    if (error == NULL) return;
    
    fprintf(stderr, "[%lu] %s: Actor %u, Event %u, Code 0x%04X (line %u)\n",
            error->timestamp,
            ERROR_SeverityToString(error->severity),
            error->actorId,
            error->failedEvent,
            error->errorCode,
            error->lineNumber);
}

void ERROR_DumpAll(void) {
    fprintf(stdout, "\n=== Error Log (%u errors) ===\n", errorLogCount);
    for (uint8_t i = 0; i < errorLogCount; i++) {
        const ErrorReport_t *error = ERROR_GetByIndex(i);
        if (error != NULL) {
            ERROR_Print(error);
        }
    }
    fprintf(stdout, "=========================\n\n");
}
```

**1.2 Update Makefile**

Add to `firmware/iot-risk-logger-stm32l4/Makefile`:

```makefile
# In C_SOURCES section, add:
app/core/error_handling/error_handling.c \

# In C_INCLUDES section, add:
-Iapp/core/error_handling \
-Iapp/core/safety_checks \
```

**1.3 Use in Existing Code**

Update `app/tasks/temperature_humidity_sensor/temperature_humidity_sensor.c`:

```c
// Add includes at top
#include "error_handling.h"
#include "safety_checks.h"

// In handleInit() function, replace error handling:
// Before:
if (ioStatus != osOK) return osError;

// After:
if (ioStatus != osOK) {
    ERROR_REPORT(TEMPERATURE_HUMIDITY_SENSOR_ACTOR_ID,
                 ERR_SENSOR_NOT_RESPONDING,
                 ERROR_SEVERITY_ERROR,
                 GLOBAL_CMD_INITIALIZE,
                 ioStatus);
    return osError;
}

// In handleIdle() for I2C errors:
if (ioStatus != osOK) {
    ERROR_REPORT(TEMPERATURE_HUMIDITY_SENSOR_ACTOR_ID,
                 ERR_I2C_TIMEOUT,
                 ERROR_SEVERITY_WARNING,
                 TH_SENS_START_SINGLE_SHOT_READ,
                 0);
    return osError;
}
```

---

### Step 2: Add Safety Checks to Memory Operations

**2.1 Update memory.c**

In `app/tasks/memory/memory.c`, add safety checks:

```c
#include "safety_checks.h"
#include "system_constants.h"

// In MEMORY_SeekFreeSpaceAddress():
HAL_StatusTypeDef MEMORY_SafeReadData(W25Q_HandleTypeDef *hflash, 
                                       uint8_t *dataBuffer, 
                                       uint32_t address, 
                                       size_t size) {
    CHECK_NULL_PTR_RET(hflash, HAL_ERROR);
    CHECK_NULL_PTR_RET(dataBuffer, HAL_ERROR);
    CHECK_FLASH_ADDRESS(address, size, hflash->geometry.flashSize);
    
    return W25Q_ReadData(hflash, dataBuffer, address, size);
}

// Replace direct W25Q_ReadData calls:
// Before:
W25Q_ReadData(&MEMORY_W25QHandle, readBuff, addr, stepSize);

// After:
HAL_StatusTypeDef status = MEMORY_SafeReadData(&MEMORY_W25QHandle, readBuff, addr, stepSize);
if (status != HAL_OK) {
    ERROR_REPORT(MEMORY_ACTOR_ID,
                 ERR_FLASH_ADDRESS_INVALID,
                 ERROR_SEVERITY_ERROR,
                 MEMORY_MEASUREMENTS_WRITE,
                 addr);
    return osError;
}
```

---

### Step 3: Replace Magic Numbers with Constants

**3.1 Update NFC Protocol Code**

In `app/tasks/nfc/nfc_handlers.c` (or wherever protocol is handled):

```c
#include "system_constants.h"

// Before:
#define RESPONSE_ACK 0x00
#define RESPONSE_NACK 0xFF

// After: Remove these, use from system_constants.h

// In protocol parsing code:
// Before:
if (response == 0x00) { /* ACK */ }

// After:
if (response == NFC_RESPONSE_ACK) { /* ACK */ }

// Before:
uint8_t mailbox[256];

// After:
uint8_t mailbox[NFC_MAILBOX_SIZE];

// Before:
if (payload_size > 253) { error(); }

// After:
if (payload_size > NFC_MAX_PAYLOAD_SIZE) { error(); }
```

**3.2 Update Sensor Timing**

In `app/tasks/temperature_humidity_sensor/temperature_humidity_sensor.c`:

```c
#include "system_constants.h"

// Before:
osDelay(1);

// After:
osDelay(SENSOR_RESET_PULSE_MS);

// Before:
osDelay(10);

// After:
osDelay(SENSOR_RESET_RECOVERY_MS);
```

---

### Step 4: Improve Error Recovery

**4.1 Add Recovery Handler to Temperature Sensor**

In `app/tasks/temperature_humidity_sensor/temperature_humidity_sensor.c`:

```c
static osStatus_t handleError(TH_SENS_Actor_t *this, message_t *message) {
    static uint8_t retryCount = 0;
    const uint8_t MAX_RETRIES = 3;
    
    switch (message->event) {
        case TH_SENS_RECOVER:
            if (retryCount >= MAX_RETRIES) {
                fprintf(stderr, "Temperature sensor: Max recovery attempts reached\n");
                // Notify system of permanent failure
                osMessageQueueId_t evManagerQueue = ACTORS_LOOKUP_SystemRegistry[EV_MANAGER_ACTOR_ID]->osMessageQueueId;
                osMessageQueuePut(evManagerQueue, 
                    &(message_t){GLOBAL_ERROR, .payload.value = TEMPERATURE_HUMIDITY_SENSOR_ACTOR_ID}, 
                    0, 0);
                return osError;
            }
            
            retryCount++;
            fprintf(stdout, "Attempting sensor recovery (attempt %u/%u)\n", retryCount, MAX_RETRIES);
            
            // Reset sensor
            HAL_GPIO_WritePin(_TEMP_RESET_GPIO_Port, _TEMP_RESET_Pin, GPIO_PIN_RESET);
            osDelay(SENSOR_RESET_PULSE_MS);
            HAL_GPIO_WritePin(_TEMP_RESET_GPIO_Port, _TEMP_RESET_Pin, GPIO_PIN_SET);
            osDelay(SENSOR_RESET_RECOVERY_MS);
            
            // Attempt reinitialization
            uint32_t sht3xId = 0x00000000;
            osStatus_t status = SHT3x_ReadDeviceID(&sht3xId);
            
            if (status == osOK) {
                fprintf(stdout, "Sensor recovery successful\n");
                retryCount = 0;  // Reset retry counter on success
                TO_STATE(this, TH_SENS_IDLE_STATE);
                return osOK;
            }
            
            // Schedule another retry after delay
            osDelay(1000 * retryCount);  // Exponential backoff
            osMessageQueuePut(this->super.osMessageQueueId, 
                &(message_t){TH_SENS_RECOVER}, 
                0, 0);
            return osOK;
            
        default:
            // Stay in error state
            return osOK;
    }
}

// Update FSM to handle error state:
static osStatus_t handleTHSensorFSM(TH_SENS_Actor_t *this, message_t *message) {
    switch (this->state) {
        case TH_SENS_NO_STATE:
            return handleInit(this, message);
        case TH_SENS_IDLE_STATE:
            return handleIdle(this, message);
        case TH_SENS_CONTINUOUS_MEASURE_STATE:
            return handleContinuousMeasure(this, message);
        case TH_SENS_STATE_ERROR:
            return handleError(this, message);  // Now has implementation!
        default:
            return osOK;
    }
}

// In task error handling:
if (handleMessageStatus != osOK) {
    ERROR_REPORT(TEMPERATURE_HUMIDITY_SENSOR_ACTOR_ID,
                 ERR_SENSOR_NOT_RESPONDING,
                 ERROR_SEVERITY_ERROR,
                 msg.event,
                 this->state);
    TO_STATE(&TH_SENS_Actor, TH_SENS_STATE_ERROR);
    
    // Trigger recovery attempt
    osMessageQueuePut(TH_SENS_Actor.super.osMessageQueueId, 
        &(message_t){TH_SENS_RECOVER}, 
        0, 100);  // 100ms timeout
}
```

---

### Step 5: Add Unit Tests

**5.1 Set up Unity Framework**

```bash
cd firmware/iot-risk-logger-stm32l4
git submodule add https://github.com/ThrowTheSwitch/Unity.git libraries/Unity
```

**5.2 Create Test Structure**

```bash
mkdir -p test/test_error_handling
mkdir -p test/mocks
```

**5.3 Create First Test**

Create `test/test_error_handling/test_error_handling.c`:

```c
#include "unity.h"
#include "error_handling.h"

void setUp(void) {
    ERROR_Init();
}

void tearDown(void) {
    ERROR_Clear();
}

void test_ErrorLogInitialization(void) {
    TEST_ASSERT_EQUAL(0, ERROR_GetCount());
    TEST_ASSERT_NULL(ERROR_GetLast());
}

void test_ErrorLogSingleEntry(void) {
    ERROR_REPORT(MEMORY_ACTOR_ID, ERR_FLASH_WRITE_FAIL, ERROR_SEVERITY_ERROR, MEMORY_MEASUREMENTS_WRITE, 0x1000);
    
    TEST_ASSERT_EQUAL(1, ERROR_GetCount());
    
    const ErrorReport_t *lastError = ERROR_GetLast();
    TEST_ASSERT_NOT_NULL(lastError);
    TEST_ASSERT_EQUAL(MEMORY_ACTOR_ID, lastError->actorId);
    TEST_ASSERT_EQUAL(ERR_FLASH_WRITE_FAIL, lastError->errorCode);
    TEST_ASSERT_EQUAL(ERROR_SEVERITY_ERROR, lastError->severity);
}

void test_ErrorLogCircularBuffer(void) {
    // Fill buffer beyond capacity
    for (uint8_t i = 0; i < ERROR_LOG_SIZE + 5; i++) {
        ERROR_REPORT(i % MAX_ACTORS, ERR_OK, ERROR_SEVERITY_INFO, EVENT_NONE, i);
    }
    
    // Should only have ERROR_LOG_SIZE entries
    TEST_ASSERT_EQUAL(ERROR_LOG_SIZE, ERROR_GetCount());
    
    // Most recent should be the last one logged
    const ErrorReport_t *lastError = ERROR_GetLast();
    TEST_ASSERT_EQUAL((ERROR_LOG_SIZE + 5 - 1), lastError->contextData);
}

void test_ErrorStatistics(void) {
    ERROR_REPORT(MEMORY_ACTOR_ID, ERR_FLASH_WRITE_FAIL, ERROR_SEVERITY_ERROR, EVENT_NONE, 0);
    ERROR_REPORT(MEMORY_ACTOR_ID, ERR_FLASH_ERASE_FAIL, ERROR_SEVERITY_CRITICAL, EVENT_NONE, 0);
    ERROR_REPORT(NFC_ACTOR_ID, ERR_NFC_TIMEOUT, ERROR_SEVERITY_WARNING, EVENT_NONE, 0);
    
    ErrorStatistics_t stats;
    ERROR_GetStatistics(&stats);
    
    TEST_ASSERT_EQUAL(3, stats.totalErrors);
    TEST_ASSERT_EQUAL(1, stats.criticalErrors);
    TEST_ASSERT_EQUAL(2, stats.errorsByActor[MEMORY_ACTOR_ID]);
    TEST_ASSERT_EQUAL(1, stats.errorsByActor[NFC_ACTOR_ID]);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ErrorLogInitialization);
    RUN_TEST(test_ErrorLogSingleEntry);
    RUN_TEST(test_ErrorLogCircularBuffer);
    RUN_TEST(test_ErrorStatistics);
    return UNITY_END();
}
```

**5.4 Create Test Makefile**

Create `test/Makefile`:

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
INCLUDES = -I../libraries/Unity/src \
           -I../app/core/error_handling \
           -I../app/core/safety_checks \
           -I../app/config \
           -I../app/config/actors_lookup \
           -I../app/config/events_list \
           -Imocks

UNITY_SRC = ../libraries/Unity/src/unity.c
TEST_SRC = test_error_handling/test_error_handling.c
APP_SRC = ../app/core/error_handling/error_handling.c
MOCK_SRC = mocks/mock_cmsis.c

all: test_error_handling
	./test_error_handling

test_error_handling: $(UNITY_SRC) $(TEST_SRC) $(APP_SRC) $(MOCK_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

clean:
	rm -f test_error_handling *.o

.PHONY: all clean
```

**5.5 Create Mock CMSIS**

Create `test/mocks/mock_cmsis.c`:

```c
#include <stdint.h>

// Mock HAL_GetTick
uint32_t HAL_GetTick(void) {
    static uint32_t tick = 0;
    return tick++;
}
```

**5.6 Run Tests**

```bash
cd test
make
```

---

## Phase-by-Phase Implementation Timeline

### Week 1-2: Critical Fixes
- [ ] Implement error_handling.c
- [ ] Add error recovery to all actors
- [ ] Replace magic numbers with system_constants.h
- [ ] Add safety_checks to memory operations

### Week 3-4: Code Quality
- [ ] Create generic actor task runner
- [ ] Refactor duplicate initialization code
- [ ] Add .clang-format file
- [ ] Format all code

### Week 5-6: Testing Infrastructure
- [ ] Set up Unity framework
- [ ] Create mocks for HAL/FreeRTOS
- [ ] Write tests for error_handling
- [ ] Write tests for safety_checks
- [ ] Update CI to run tests

### Week 7-8: Documentation
- [ ] Complete all TODO items
- [ ] Add units to all sensor variables
- [ ] Create API documentation
- [ ] Add architecture diagrams

---

## Testing Your Changes

### Manual Testing Checklist

- [ ] Project compiles without errors
- [ ] Project compiles without warnings
- [ ] Flash usage doesn't increase significantly
- [ ] RAM usage doesn't increase significantly
- [ ] Error logging works (test by inducing errors)
- [ ] Error recovery works (disconnect sensor, reconnect)
- [ ] NFC communication still works
- [ ] Sensor readings are correct
- [ ] Flash writes/reads work correctly

### Validation Steps

1. **Compile and check memory usage:**
   ```bash
   cd firmware/iot-risk-logger-stm32l4
   make clean
   make all
   arm-none-eabi-size build/iot-risk-logger-stm32l4.elf
   ```

2. **Run unit tests:**
   ```bash
   cd test
   make clean
   make
   ```

3. **Check code formatting:**
   ```bash
   clang-format --dry-run -Werror app/**/*.c app/**/*.h
   ```

---

## Common Issues and Solutions

### Issue: Undefined reference to ERROR_Init
**Solution:** Make sure error_handling.c is added to Makefile C_SOURCES

### Issue: Cannot find error_handling.h
**Solution:** Add `-Iapp/core/error_handling` to Makefile C_INCLUDES

### Issue: Tests won't compile
**Solution:** Check that mock_cmsis.c provides all needed HAL functions

### Issue: Flash/RAM usage increased too much
**Solution:** Consider reducing ERROR_LOG_SIZE or using compile-time conditionals:
```c
#ifdef ENABLE_ERROR_LOGGING
    ERROR_REPORT(...);
#endif
```

---

## Getting Help

If you encounter issues:

1. Check the ARCHITECTURE_REVIEW.md for detailed explanations
2. Review existing similar code in the codebase
3. Open a GitHub issue with:
   - What you're trying to implement
   - The error you're seeing
   - Steps to reproduce

---

## Summary

This guide provides practical steps to implement the architecture improvements. Start with the critical fixes (error handling and safety checks), then move to code quality improvements, and finally add testing infrastructure.

Remember: **Make incremental changes and test frequently!**

Each improvement should be:
1. Implemented in a focused way
2. Tested (manually or with unit tests)
3. Committed separately
4. Reviewed before moving to the next

Good luck with your improvements! 🚀
