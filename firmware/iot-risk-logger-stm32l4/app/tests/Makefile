# Makefile for Unity Unit Tests
# I2C Sensors Bus Service Tests

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -DUNIT_TEST
INCLUDES = -I./unity_framework/src \
           -I./mocks

# Unity source
UNITY_SRC = ./unity_framework/src/unity.c

# Test sources
TEST_SRCS = services/i2c_sensors_bus/test_sensors_bus.c

# Output directory
BUILD_DIR = build

# Test executables
TEST_EXES = $(BUILD_DIR)/test_sensors_bus

# Default target
all: $(BUILD_DIR) $(TEST_EXES)
	@echo "Running tests..."
	@for test in $(TEST_EXES); do \
		echo "Running $$test..."; \
		$$test || exit 1; \
	done
	@echo "All tests passed!"

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build test executable
$(BUILD_DIR)/test_sensors_bus: services/i2c_sensors_bus/test_sensors_bus.c $(UNITY_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Run tests
test: all

.PHONY: all clean test
