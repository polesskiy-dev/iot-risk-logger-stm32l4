# Unit Tests for IoT Risk Logger STM32L4

This directory contains unit tests for the firmware components using the Unity test framework.

## Structure

```
tests/
├── unity_framework/        # Unity test framework (submodule)
├── services/
│   └── i2c_sensors_bus/   # I2C Bus Service tests
│       └── test_sensors_bus.c
├── Makefile               # Test build system
└── README.md             # This file
```

## Prerequisites

- GCC compiler (standard C compiler)
- Make build system
- Unity test framework (automatically cloned)

## Running Tests

### Run all tests:
```bash
cd tests
make test
```

### Run specific test:
```bash
cd tests
make
./build/test_sensors_bus
```

### Clean build artifacts:
```bash
cd tests
make clean
```

## Test Coverage

### I2C Sensors Bus Service (`test_sensors_bus.c`)

Tests cover:
- ✅ Initialization with valid I2C handle
- ✅ Synchronous request handling
- ✅ Memory write operations
- ✅ Memory read operations
- ✅ HAL_BUSY retry logic configuration
- ✅ Bus error recovery (BERR)
- ✅ Arbitration lost recovery (ARLO)
- ✅ Reset function for power management
- ✅ Timeout handling
- ✅ Edge cases and error conditions

## Adding New Tests

1. Create a new test file in the appropriate subdirectory:
   ```c
   #include "unity.h"
   #include "your_module.h"
   
   void setUp(void) { /* Setup before each test */ }
   void tearDown(void) { /* Cleanup after each test */ }
   
   void test_YourFunction_Success(void) {
       TEST_ASSERT_EQUAL(expected, actual);
   }
   
   int main(void) {
       UNITY_BEGIN();
       RUN_TEST(test_YourFunction_Success);
       return UNITY_END();
   }
   ```

2. Add the test to the Makefile:
   ```makefile
   TEST_SRCS += your_module/test_your_module.c
   ```

3. Run tests:
   ```bash
   make test
   ```

## Unity Test Framework

Unity is a lightweight unit testing framework for C. Documentation: https://github.com/ThrowTheSwitch/Unity

### Common Assertions

```c
TEST_ASSERT_EQUAL(expected, actual)
TEST_ASSERT_NOT_EQUAL(expected, actual)
TEST_ASSERT_TRUE(condition)
TEST_ASSERT_FALSE(condition)
TEST_ASSERT_NULL(pointer)
TEST_ASSERT_NOT_NULL(pointer)
TEST_ASSERT_EQUAL_MEMORY(expected, actual, length)
```

## CI Integration

Tests are automatically run in the CI pipeline. See `.github/workflows/ci.yml` for configuration.

## Notes

- Tests use mocked RTOS and HAL functions to avoid hardware dependencies
- Each test file should be self-contained and independent
- Use descriptive test names following pattern: `test_ModuleName_FunctionName_Scenario`
- All tests should pass before committing code changes
