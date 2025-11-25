# IoT Risk Logger STM32L4 - Architecture Review & Code Quality Assessment

**Date:** November 3, 2025  
**Reviewer:** Expert in Embedded Software Engineering  
**Project:** IoT Risk Data Logger based on STM32L4

---

## Executive Summary

This project demonstrates a **well-structured embedded systems architecture** using an actor-based (active object) pattern with event-driven communication. The code shows good practices for resource-constrained embedded systems with FreeRTOS. However, there are several areas where code quality, maintainability, and robustness can be significantly improved.

**Overall Architecture Rating:** ⭐⭐⭐⭐ (4/5)  
**Code Quality Rating:** ⭐⭐⭐ (3/5)  
**Maintainability Rating:** ⭐⭐⭐ (3/5)

---

## 1. Architecture Overview

### 1.1 Core Design Pattern: Actor Framework

**Strengths:**
✅ Clean separation of concerns using actor model (active objects)  
✅ Event-driven architecture suitable for low-power IoT applications  
✅ Decoupled components communicating via message queues  
✅ Hierarchical state machines (FSM) for task behavior management  
✅ Centralized event manager for publish-subscribe pattern

**Architecture Components:**
```
┌─────────────────────────────────────────────────────────────┐
│                     Event Manager                            │
│              (Publish-Subscribe Hub)                         │
└──────────────────┬──────────────────────────────────────────┘
                   │
        ┌──────────┼──────────────────────────┐
        │          │                           │
    ┌───▼───┐  ┌──▼────┐  ┌────────┐  ┌──────▼─────┐
    │ CRON  │  │  NFC  │  │ Memory │  │  Sensors   │
    │Actor  │  │Actor  │  │ Actor  │  │  (TH, OPT) │
    └───────┘  └───────┘  └────────┘  └────────────┘
        │          │           │              │
        └──────────┴───────────┴──────────────┘
                   │
            ┌──────▼──────┐
            │   Drivers   │
            │ (W25Q, SHT) │
            └─────────────┘
```

### 1.2 Key Patterns Identified

1. **Actor Model (Active Objects)**: Each task is an independent actor with:
   - Message queue for asynchronous communication
   - State machine for behavior management
   - Dedicated thread (FreeRTOS task)

2. **Publish-Subscribe**: Event Manager acts as broker
   - Static subscription matrix defined at compile-time
   - Efficient for embedded systems (no dynamic allocation)

3. **Finite State Machines**: Each actor implements FSM pattern
   - Clear state transitions
   - Event-driven state changes

---

## 2. Strengths of Current Implementation

### 2.1 Excellent Architectural Decisions

1. **Actor Framework Design**
   - Well-structured base `actor_t` type
   - Message-based communication prevents data races
   - Clean abstraction with message handlers

2. **Static Memory Allocation**
   - Uses static task buffers (no dynamic allocation in critical paths)
   - Compile-time event subscription matrix
   - Predictable memory footprint

3. **Separation of Concerns**
   ```
   app/
   ├── core/          # Reusable core modules
   ├── drivers/       # Hardware abstraction
   ├── tasks/         # Application actors
   ├── config/        # Configuration & lookup tables
   └── middlewares/   # Higher-level services
   ```

4. **Low-Power Design Considerations**
   - Power mode manager actor
   - Sensor sleep/wake capabilities
   - RTC-based wake-up scheduling

### 2.2 Good Coding Practices

✅ Doxygen documentation headers  
✅ Consistent naming conventions  
✅ Debug logging with `fprintf(stdout, ...)`  
✅ State machine pattern for complex behaviors  
✅ Hardware abstraction for drivers (I2C, QSPI)

---

## 3. Critical Issues & Recommendations

### 3.1 🔴 CRITICAL: Error Handling

**Issues:**
1. **Minimal error recovery mechanisms**
2. **Error states often lead to dead ends**
3. **No systematic error logging or diagnostics**

**Examples:**
```c
// temperature_humidity_sensor.c:84
if (handleMessageStatus != osOK) {
    #ifdef DEBUG
        fprintf(stderr, "Error handling event %u in state %ul\n", 
                msg.event, TH_SENS_Actor.state);
    #endif
    osMessageQueuePut(evManagerQueue, 
        &(message_t){GLOBAL_ERROR, .payload.value = TEMPERATURE_HUMIDITY_SENSOR_ACTOR_ID}, 
        0, 0);
    TO_STATE(&TH_SENS_Actor, TH_SENS_STATE_ERROR);
}
```

**Problems:**
- Error state (`TH_SENS_STATE_ERROR`) has no recovery logic
- Error information is lost (only actor ID is sent)
- No retry mechanisms
- System may become unresponsive

**Recommendations:**

1. **Implement Error Recovery State Machine**
```c
// Recommended approach
typedef enum {
    ERROR_SEVERITY_INFO,
    ERROR_SEVERITY_WARNING,
    ERROR_SEVERITY_ERROR,
    ERROR_SEVERITY_CRITICAL
} ErrorSeverity_t;

typedef struct {
    ACTOR_ID actorId;
    event_t failedEvent;
    uint32_t errorCode;
    ErrorSeverity_t severity;
    uint32_t timestamp;
} ErrorReport_t;

// In error state handler
static osStatus_t handleError(TH_SENS_Actor_t *this, message_t *message) {
    switch (message->event) {
        case TH_SENS_RECOVER:
            // Attempt recovery: reset sensor, reinitialize
            HAL_GPIO_WritePin(_TEMP_RESET_GPIO_Port, _TEMP_RESET_Pin, GPIO_PIN_RESET);
            osDelay(1);
            HAL_GPIO_WritePin(_TEMP_RESET_GPIO_Port, _TEMP_RESET_Pin, GPIO_PIN_SET);
            osDelay(10);
            
            if (reinitializeSensor() == osOK) {
                TO_STATE(this, TH_SENS_IDLE_STATE);
                return osOK;
            }
            // Increment retry counter, backoff
            return osError;
            
        default:
            return osOK;
    }
}
```

2. **Add Error Code Enumeration**
```c
typedef enum {
    ERR_OK = 0,
    ERR_I2C_TIMEOUT = 0x0100,
    ERR_I2C_NACK,
    ERR_SENSOR_NOT_RESPONDING,
    ERR_INVALID_DATA,
    ERR_MEMORY_FULL,
    ERR_FLASH_WRITE_FAIL,
    // ... more specific errors
} SystemErrorCode_t;
```

3. **Implement Error Circular Buffer**
```c
// Store last N errors for diagnostics
#define ERROR_LOG_SIZE 16

typedef struct {
    ErrorReport_t errors[ERROR_LOG_SIZE];
    uint8_t writeIndex;
    uint8_t count;
} ErrorLog_t;

void ERROR_Log(ErrorReport_t *error);
ErrorReport_t* ERROR_GetLast(void);
void ERROR_Clear(void);
```

---

### 3.2 🔴 CRITICAL: Magic Numbers and Constants

**Issues:**
Widespread use of magic numbers makes code hard to maintain and understand.

**Examples:**
```c
// event_manager.c:35
const ACTOR_ID EV_MANAGER_SubscribersIdsMatrix[GLOBAL_EVENTS_MAX][MAX_ACTORS] = {
    [GLOBAL_CMD_INITIALIZE] = {CRON_ACTOR_ID, PWRM_MANAGER_ACTOR_ID, MEMORY_ACTOR_ID},
    // What is the meaning of these specific combinations?
};

// nfc/nfc_handlers.c (implied from protocol doc)
// Protocol uses raw hex values: 0xC0, 0xC1, 0xFF, 0xFE
// Should be named constants
```

**Recommendations:**

1. **Define Protocol Constants**
```c
// nfc_protocol.h
#define NFC_PROTOCOL_VERSION        (0x01)

// Response codes
#define NFC_RESPONSE_ACK            (0x00)
#define NFC_RESPONSE_NACK           (0xFF)
#define NFC_RESPONSE_NACK_CRC       (0xFE)

// Command codes
#define NFC_CMD_START_LOGGING       (0xC0)
#define NFC_CMD_STOP_LOGGING        (0xC1)
#define NFC_CMD_WRITE_SETTINGS      (0xC2)
#define NFC_CMD_READ_SETTINGS       (0xC3)
#define NFC_CMD_READ_LOG_CHUNK      (0xC4)

// Protocol limits
#define NFC_MAILBOX_SIZE            (256)
#define NFC_MAX_PAYLOAD_SIZE        (253)
#define NFC_HEADER_SIZE             (3)  // CRC + CMD + SIZE
```

2. **Configuration Header**
```c
// app_config.h
#define SYSTEM_TICK_RATE_HZ         (1000)
#define DEFAULT_SENSOR_READ_PERIOD  (60)    // seconds
#define MAX_LOG_ENTRIES             (10000)
#define WATCHDOG_TIMEOUT_MS         (5000)
```

---

### 3.3 🟡 HIGH PRIORITY: Memory Safety

**Issues:**

1. **No bounds checking in many places**
2. **Potential buffer overflows**
3. **Missing NULL pointer checks**

**Examples:**
```c
// memory.c:145
W25Q_ReadData(&MEMORY_W25QHandle, readBuff, addr, stepSize);
// No check if addr is within valid flash range
// No check if stepSize fits in readBuff

// actor.h:57
typedef struct {
    event_t event;
    union {
        void *ptr;            ///< No size information!
        uint32_t value;
    } payload;
    ssize_t payload_size;     ///< Only documentation, not enforced
} message_t;
```

**Recommendations:**

1. **Add Bounds Checking Macros**
```c
// safety_checks.h
#define CHECK_BUFFER_BOUNDS(buf, size, max_size) \
    do { \
        if ((size) > (max_size)) { \
            fprintf(stderr, "Buffer overflow prevented at %s:%d\n", __FILE__, __LINE__); \
            return osError; \
        } \
    } while(0)

#define CHECK_FLASH_ADDRESS(addr, size, flash_size) \
    do { \
        if ((addr) + (size) > (flash_size)) { \
            fprintf(stderr, "Flash address out of bounds at %s:%d\n", __FILE__, __LINE__); \
            return HAL_ERROR; \
        } \
    } while(0)

#define CHECK_NULL_PTR(ptr) \
    do { \
        if ((ptr) == NULL) { \
            fprintf(stderr, "NULL pointer at %s:%d\n", __FILE__, __LINE__); \
            return osError; \
        } \
    } while(0)
```

2. **Improve Message Structure**
```c
typedef struct {
    event_t event;
    union {
        struct {
            void *ptr;
            size_t size;        // Always track size!
        } buffer;
        uint32_t value;
        struct {
            uint16_t param1;
            uint16_t param2;
        } params;
    } payload;
    uint8_t payloadType;        // Discriminator for union type
} message_t;
```

3. **Safe Flash Access Wrapper**
```c
HAL_StatusTypeDef MEMORY_SafeReadData(W25Q_HandleTypeDef *hflash, 
                                       uint8_t *dataBuffer, 
                                       uint32_t address, 
                                       size_t size) {
    CHECK_NULL_PTR(hflash);
    CHECK_NULL_PTR(dataBuffer);
    CHECK_FLASH_ADDRESS(address, size, hflash->geometry.flashSize);
    
    return W25Q_ReadData(hflash, dataBuffer, address, size);
}
```

---

### 3.4 🟡 HIGH PRIORITY: Code Duplication

**Issues:**
Similar patterns repeated across multiple actors without abstraction.

**Examples:**
```c
// Pattern repeated in every actor: temperature_humidity_sensor.c, light_sensor.c, etc.
void TH_SENS_Task(void *argument) {
    (void) argument;
    message_t msg;
    
    for (;;) {
        if (osMessageQueueGet(TH_SENS_Actor.super.osMessageQueueId, &msg, NULL, osWaitForever) == osOK) {
            osStatus_t status = TH_SENS_Actor.super.messageHandler((actor_t *) &TH_SENS_Actor, &msg);
            
            if (status != osOK) {
                osMessageQueueId_t evManagerQueue = ACTORS_LOOKUP_SystemRegistry[EV_MANAGER_ACTOR_ID]->osMessageQueueId;
                osMessageQueuePut(evManagerQueue, &(message_t){GLOBAL_ERROR, .payload.value = TEMPERATURE_HUMIDITY_SENSOR_ACTOR_ID}, 0, 0);
                TO_STATE(&TH_SENS_Actor, TH_SENS_STATE_ERROR);
            }
        }
    }
}
// This exact pattern is in nfc.c, memory.c, light_sensor.c...
```

**Recommendations:**

1. **Create Generic Actor Task Runner**
```c
// actor.c
void ACTOR_GenericTask(void *actorPtr) {
    actor_t *actor = (actor_t *)actorPtr;
    message_t msg;
    osMessageQueueId_t evManagerQueue = ACTORS_LOOKUP_SystemRegistry[EV_MANAGER_ACTOR_ID]->osMessageQueueId;
    
    #ifdef DEBUG
        fprintf(stdout, "Actor %lu task started\n", actor->actorId);
    #endif
    
    for (;;) {
        if (osMessageQueueGet(actor->osMessageQueueId, &msg, NULL, osWaitForever) == osOK) {
            osStatus_t status = actor->messageHandler(actor, &msg);
            
            if (status != osOK) {
                #ifdef DEBUG
                    fprintf(stderr, "Actor %lu: Error handling event %u\n", 
                            actor->actorId, msg.event);
                #endif
                
                osMessageQueuePut(evManagerQueue, 
                    &(message_t){
                        .event = GLOBAL_ERROR, 
                        .payload.value = actor->actorId
                    }, 
                    0, 0);
                    
                // Optional: call error handler if defined
                if (actor->errorHandler != NULL) {
                    actor->errorHandler(actor, &msg);
                }
            }
        }
    }
}

// Usage in temperature_humidity_sensor.c
actor_t* TH_SENS_TaskInit(void) {
    TH_SENS_Actor.super.osMessageQueueId = osMessageQueueNew(
        DEFAULT_QUEUE_SIZE, 
        DEFAULT_QUEUE_MESSAGE_SIZE, 
        &(osMessageQueueAttr_t){.name = "thSensorQueue"}
    );
    // Use generic task runner instead of custom task function
    TH_SENS_Actor.super.osThreadId = osThreadNew(
        ACTOR_GenericTask, 
        &TH_SENS_Actor,  // Pass actor as argument
        &thSensorTaskDescription
    );
    
    return (actor_t*) &TH_SENS_Actor;
}
```

2. **Extract Common Initialization Pattern**
```c
// actor.c
actor_t* ACTOR_Initialize(actor_t *actor, 
                          const char *queueName,
                          messageHandler_t handler,
                          const osThreadAttr_t *taskAttr) {
    actor->messageHandler = handler;
    actor->osMessageQueueId = osMessageQueueNew(
        DEFAULT_QUEUE_SIZE,
        DEFAULT_QUEUE_MESSAGE_SIZE,
        &(osMessageQueueAttr_t){.name = queueName}
    );
    actor->osThreadId = osThreadNew(ACTOR_GenericTask, actor, taskAttr);
    
    return actor;
}
```

---

### 3.5 🟡 HIGH PRIORITY: TODO Comments & Incomplete Implementation

**Issues:**
Multiple TODO comments indicate incomplete features and technical debt.

**Examples:**
```c
// event_manager.c:36
// TODO: uncomment the full list to initialize all actors
// [GLOBAL_CMD_INITIALIZE] = {CRON_ACTOR_ID, PWRM_MANAGER_ACTOR_ID, NFC_ACTOR_ID, ACCELEROMETER_ACTOR_ID, ...},

// event_manager.c:81
// TODO handle initialize event
// TODO remove from here

// temperature_humidity_sensor.h:38
uint16_t rawHumidity; ///< TODO units

// events_list.h:69
// TODO: Add accelerometer events

// event_manager.c:99
// TODO explain the system in graph
```

**Recommendations:**

1. **Create Technical Debt Tracking**
   - Move TODOs to GitHub Issues with priorities
   - Add issue references in comments
   ```c
   // Issue #42: Implement full actor initialization
   // [GLOBAL_CMD_INITIALIZE] = {...},
   ```

2. **Complete Documentation**
   ```c
   // Before:
   uint16_t rawHumidity; ///< TODO units
   
   // After:
   int16_t temperature_cC;   ///< Temperature in centi-Celsius (100 = 1°C)
   uint16_t humidity_pct10;  ///< Relative humidity in 0.1% units (100 = 10.0%)
   ```

3. **Remove Dead Code or Implement**
   ```c
   // Either implement the feature or remove it
   // Don't leave half-implemented code in production
   ```

---

### 3.6 🟡 MEDIUM PRIORITY: Type Safety & Casting

**Issues:**
Extensive use of explicit casts can hide type errors.

**Examples:**
```c
// temperature_humidity_sensor.c:81
osStatus_t handleMessageStatus = TH_SENS_Actor.super.messageHandler(
    (actor_t *) &TH_SENS_Actor,  // Cast from TH_SENS_Actor_t* to actor_t*
    &msg
);

// actor.h:75
typedef osStatus_t (*messageHandler_t)(struct actor_t *actor, message_t *message);
// Handler receives base type, but implementations need derived type
```

**Recommendations:**

1. **Use Container_of Pattern**
```c
// actor.h
#define ACTOR_FROM_BASE(ptr, type) \
    ((type*)((char*)(ptr) - offsetof(type, super)))

// In handler:
static osStatus_t handleTHSensorFSM(actor_t *base, message_t *message) {
    TH_SENS_Actor_t *this = ACTOR_FROM_BASE(base, TH_SENS_Actor_t);
    // Now 'this' is properly typed
    // ...
}
```

2. **Avoid Void Pointer in Payload**
```c
// Current approach is risky
message_t msg = {
    .event = SOME_EVENT,
    .payload.ptr = someData  // What type? What size?
};

// Better approach: Tagged unions
typedef enum {
    PAYLOAD_TYPE_NONE,
    PAYLOAD_TYPE_VALUE,
    PAYLOAD_TYPE_SENSOR_DATA,
    PAYLOAD_TYPE_SETTINGS,
    PAYLOAD_TYPE_LOG_CHUNK
} PayloadType_t;

typedef struct {
    event_t event;
    PayloadType_t payloadType;
    union {
        uint32_t value;
        SensorData_t sensorData;
        SettingsData_t settings;
        LogChunk_t logChunk;
    } payload;
} message_t;
```

---

### 3.7 🟡 MEDIUM PRIORITY: Testing Infrastructure

**Issues:**
1. No unit tests for core functionality
2. CI pipeline has placeholder test step
3. No hardware-in-the-loop test setup visible

**Current State:**
```yaml
# .github/workflows/ci.yml:96
test:
  runs-on: ubuntu-latest
  needs: compile
  steps:
    - name: Run tests
      run: echo "Run your test commands here"  # Placeholder!
```

**Recommendations:**

1. **Add Unity Test Framework**
```bash
# Add Unity as a git submodule
git submodule add https://github.com/ThrowTheSwitch/Unity.git libraries/Unity
```

2. **Create Test Directory Structure**
```
firmware/iot-risk-logger-stm32l4/
├── app/
│   └── (existing code)
└── test/
    ├── test_actor/
    │   └── test_actor.c
    ├── test_event_manager/
    │   └── test_event_manager.c
    ├── test_memory/
    │   └── test_memory.c
    └── mocks/
        ├── mock_hal.c
        └── mock_freertos.c
```

3. **Example Unit Test**
```c
// test/test_actor/test_actor.c
#include "unity.h"
#include "actor.h"

void setUp(void) {
    // Setup code before each test
}

void tearDown(void) {
    // Cleanup code after each test
}

void test_MessageStructSize(void) {
    // Ensure message_t fits in queue
    TEST_ASSERT_LESS_OR_EQUAL(sizeof(message_t), DEFAULT_QUEUE_MESSAGE_SIZE);
}

void test_ActorIdUniqueness(void) {
    // Verify all actor IDs are unique
    TEST_ASSERT_NOT_EQUAL(EV_MANAGER_ACTOR_ID, CRON_ACTOR_ID);
    TEST_ASSERT_NOT_EQUAL(CRON_ACTOR_ID, NFC_ACTOR_ID);
    // ... more assertions
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_MessageStructSize);
    RUN_TEST(test_ActorIdUniqueness);
    return UNITY_END();
}
```

4. **Mock HAL for Host Testing**
```c
// test/mocks/mock_hal.c
// Provide stub implementations of HAL functions
HAL_StatusTypeDef HAL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) {
    // Log for verification
    return HAL_OK;
}
```

---

### 3.8 🟢 LOW PRIORITY: Documentation

**Current State:**
- Basic Doxygen headers present
- Some README files in subdirectories
- State diagrams in PlantUML format (good!)

**Recommendations:**

1. **Add Architecture Decision Records (ADRs)**
```markdown
# ADR-001: Actor-Based Architecture

## Status
Accepted

## Context
Need concurrent task management with clear separation of concerns for 
low-power IoT device.

## Decision
Use actor model with message passing via FreeRTOS queues.

## Consequences
+ Clear separation of concerns
+ No shared memory between actors
+ Easy to reason about concurrency
- Slightly higher memory overhead per actor
- Message passing latency
```

2. **Expand API Documentation**
```c
/**
 * @brief Reads sensor data via I2C
 * 
 * This function performs a single-shot measurement on the SHT3x sensor.
 * It blocks until measurement is complete (~15ms typical).
 * 
 * @param[in]  this  Pointer to actor instance
 * @param[out] temp  Temperature in centi-Celsius (2500 = 25.00°C)
 * @param[out] hum   Humidity in 0.1% units (655 = 65.5% RH)
 * 
 * @return osOK on success
 * @return osErrorTimeout if I2C timeout
 * @return osError for other errors
 * 
 * @pre  Sensor must be initialized via handleInit()
 * @post On success, temp and hum contain valid measurements
 * 
 * @note This function is blocking. For non-blocking operation, use
 *       TH_SENS_START_SINGLE_SHOT_READ event and wait for
 *       GLOBAL_TEMPERATURE_HUMIDITY_MEASUREMENTS_READY
 * 
 * @warning Not thread-safe. Only call from TH_SENS task context.
 * 
 * @see handleIdle(), SHT3x_ReadMeasurement()
 */
static osStatus_t readSensorData(TH_SENS_Actor_t *this, int16_t *temp, uint16_t *hum);
```

3. **Create System Overview Diagram**
```
docs/
├── architecture/
│   ├── system_overview.png
│   ├── actor_lifecycle.png
│   ├── event_flow.png
│   └── memory_map.png
└── api/
    ├── actor_api.md
    ├── event_api.md
    └── driver_api.md
```

---

### 3.9 🟢 LOW PRIORITY: Code Style Consistency

**Issues:**
Minor inconsistencies in coding style.

**Examples:**
```c
// Inconsistent bracing
if (status != osOK) {
    return osError;
}

if (status != osOK) return osError;  // One-liner

// Inconsistent pointer style
actor_t *actor;    // Some files
actor_t* actor;    // Other files

// Inconsistent naming
TH_SENS_Actor_t    // With type suffix
MEMORY_Actor_t     // With type suffix
message_t          // Lowercase with _t
```

**Recommendations:**

1. **Create Style Guide** (`.clang-format`)
```yaml
# .clang-format
BasedOnStyle: LLVM
IndentWidth: 2
ColumnLimit: 100
PointerAlignment: Right
AllowShortFunctionsOnASingleLine: false
```

2. **Run Formatter in CI**
```yaml
# .github/workflows/ci.yml
lint:
  runs-on: ubuntu-latest
  steps:
    - name: Check code format
      run: |
        find app/ -name "*.c" -o -name "*.h" | xargs clang-format --dry-run -Werror
```

---

## 4. Power Consumption Optimization

### Current Observations:
The project shows awareness of power management (power_mode_manager, sensor sleep modes).

### Recommendations:

1. **Audit Sleep Modes Usage**
```c
// Ensure all peripherals are in sleep between measurements
// Current: Power mode manager exists but implementation is incomplete

// Recommended:
typedef struct {
    uint32_t run_time_ms;
    uint32_t sleep_time_ms;
    uint32_t stop2_time_ms;
    uint32_t wakeup_count;
} PowerMetrics_t;

void PWRM_LogMetrics(PowerMetrics_t *metrics);
```

2. **Optimize Flash Access**
```c
// Current: Flash is accessed frequently
// Recommendation: Batch flash writes

#define FLASH_WRITE_BUFFER_SIZE (256)  // One page
static struct {
    uint8_t buffer[FLASH_WRITE_BUFFER_SIZE];
    uint16_t count;
} flashWriteCache;

// Only write when buffer is full or on demand
void MEMORY_CacheWrite(const uint8_t *data, size_t len);
void MEMORY_FlushCache(void);
```

3. **I2C Clock Optimization**
```c
// Use lowest I2C clock speed that meets requirements
// 100kHz vs 400kHz can make significant power difference
```

---

## 5. Security Considerations

### Current State:
Basic security model with I2C password for NFC tag.

### Recommendations:

1. **Add CRC/Checksum for Flash Data**
```c
typedef struct {
    uint32_t timestamp;
    int16_t temperature;
    uint16_t humidity;
    uint16_t light;
    uint16_t crc16;  // Add CRC for data integrity
} LogEntry_t;
```

2. **Secure Boot Considerations**
```c
// If adding firmware update capability:
// - Implement signature verification
// - Use STM32 built-in security features (RDP, PCROP)
```

3. **NFC Protocol Security**
```c
// Current: No encryption or authentication visible
// Recommendations:
// - Add message sequence numbers (prevent replay)
// - Consider AES-128 for sensitive commands
// - Implement rate limiting to prevent DoS
```

---

## 6. Prioritized Action Plan

### Phase 1: Critical Fixes (Week 1-2)
1. ✅ Implement comprehensive error handling
2. ✅ Add bounds checking and NULL pointer validation
3. ✅ Replace magic numbers with named constants
4. ✅ Create error logging circular buffer

### Phase 2: Code Quality (Week 3-4)
1. ✅ Refactor duplicate code (generic actor task runner)
2. ✅ Improve type safety (container_of, tagged unions)
3. ✅ Add code formatter (clang-format)
4. ✅ Complete TODO items or move to issues

### Phase 3: Infrastructure (Week 5-6)
1. ✅ Set up Unity test framework
2. ✅ Create mock HAL layer
3. ✅ Write unit tests for core modules
4. ✅ Update CI pipeline to run tests

### Phase 4: Documentation (Week 7-8)
1. ✅ Create architecture decision records
2. ✅ Expand API documentation
3. ✅ Create system diagrams
4. ✅ Write developer guide

### Phase 5: Optimization (Ongoing)
1. ✅ Profile power consumption
2. ✅ Optimize flash access patterns
3. ✅ Review and optimize memory usage
4. ✅ Add security features

---

## 7. Metrics & KPIs

### Current Metrics
- **Flash Usage:** ~X KB / 128 KB (visible in CI)
- **SRAM Usage:** ~Y KB / 40 KB (visible in CI)
- **Build Time:** Check CI logs
- **Test Coverage:** 0% (no tests)

### Recommended Metrics to Track
1. **Code Quality**
   - Cyclomatic complexity per function (target: <10)
   - Test coverage (target: >80% for core modules)
   - Static analysis warnings (target: 0)

2. **Performance**
   - Average message processing latency
   - Worst-case event handling time
   - Flash write endurance (cycles)

3. **Power**
   - Average current draw
   - Sleep mode percentage
   - Battery life estimate

---

## 8. Conclusion

### Summary
This is a **solid embedded systems project** with a well-thought-out architecture. The actor-based design is appropriate for the use case, and the separation of concerns is good.

### Key Strengths
✅ Clean actor-based architecture  
✅ Event-driven design suitable for low-power  
✅ Good separation of drivers and application logic  
✅ Static memory allocation for predictability  
✅ CI/CD pipeline basics in place

### Areas for Improvement
⚠️ Error handling and recovery mechanisms  
⚠️ Code duplication across actors  
⚠️ Missing unit tests  
⚠️ Incomplete features (TODOs)  
⚠️ Memory safety (bounds checking)

### Recommendation
**Proceed with incremental improvements following the prioritized action plan.** The architecture is sound; focus on enhancing robustness, testability, and maintainability.

---

## Appendix A: Useful Resources

1. **Design Patterns for Embedded Systems in C** - Bruce Powel Douglass
2. **Making Embedded Systems** - Elecia White
3. **Test Driven Development for Embedded C** - James W. Grenning
4. **STM32 Low-Power Design** - Application Note AN4445
5. **Actor Model Pattern** - https://www.state-machine.com/

## Appendix B: Quick Wins Checklist

- [ ] Add `assert()` calls for critical invariants
- [ ] Replace all magic numbers with `#define` constants
- [ ] Add NULL pointer checks before dereferencing
- [ ] Complete or remove all TODO comments
- [ ] Add input validation to all driver functions
- [ ] Document units for all sensor measurements
- [ ] Add error recovery in all error states
- [ ] Create `.clang-format` and run formatter
- [ ] Set up pre-commit hooks for code formatting
- [ ] Add memory usage tracking in CI

---

**End of Review**

For questions or clarifications, please open a GitHub issue.
