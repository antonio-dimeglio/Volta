# Volta Programming Language - Makefile
# C++ Implementation

CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Iinclude -Wno-unused-parameter -Wno-unused-variable -Wno-comment
DEBUG_FLAGS := -fsanitize=address -fsanitize=undefined -g -O0 -DDEBUG
LDFLAGS := -lffi -ldl

# Debug build
ifdef DEBUG
	CXXFLAGS += $(DEBUG_FLAGS)
	LDFLAGS += -fsanitize=address -fsanitize=undefined
endif

# GTest configuration - different for macOS vs Linux
ifeq ($(shell uname), Darwin)
	GTEST_FLAGS = -L/opt/homebrew/opt/googletest/lib -I/opt/homebrew/opt/googletest/include -lgtest -lgtest_main -pthread
else
	GTEST_FLAGS = -lgtest -lgtest_main -pthread
endif

# Directories
SRC_DIR := src
INCLUDE_DIR := include
BUILD_DIR := build
BIN_DIR := bin
TEST_DIR := tests

# Output binary
TARGET := $(BIN_DIR)/volta
TEST_TARGET := $(BIN_DIR)/test_volta

# Source files - automatically detect all subdirectories
ALL_SOURCES := $(shell find $(SRC_DIR) -name '*.cpp' ! -name 'main.cpp')
MAIN_SOURCE := $(SRC_DIR)/main.cpp

# Object files
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(ALL_SOURCES))
MAIN_OBJECT := $(BUILD_DIR)/main.o

# Test files
TEST_SOURCES := $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJECTS := $(patsubst $(TEST_DIR)/%.cpp,$(BUILD_DIR)/tests/%.o,$(TEST_SOURCES))

# Default target
.PHONY: all
all: $(TARGET)

# Create directories
$(BUILD_DIR) $(BIN_DIR) $(BUILD_DIR)/tests:
	@mkdir -p $@

# Compile source files - automatically create subdirectories
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile main
$(MAIN_OBJECT): $(MAIN_SOURCE) | $(BUILD_DIR)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Link executable
$(TARGET): $(OBJECTS) $(MAIN_OBJECT) | $(BIN_DIR)
	@echo "Linking $(TARGET)..."
	@$(CXX) $(CXXFLAGS) $(OBJECTS) $(MAIN_OBJECT) -o $@ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Debug build
.PHONY: debug
debug: clean
	$(MAKE) DEBUG=1 $(TARGET)

# Compile test files with GTest includes
$(BUILD_DIR)/tests/%.o: $(TEST_DIR)/%.cpp | $(BUILD_DIR)/tests
	@echo "Compiling test $<..."
ifeq ($(shell uname), Darwin)
	@$(CXX) $(CXXFLAGS) -I/opt/homebrew/opt/googletest/include -c $< -o $@
else
	@$(CXX) $(CXXFLAGS) -c $< -o $@
endif

# Build and run tests
.PHONY: test
test: $(TEST_TARGET)
	@echo "Running tests..."
	@LSAN_OPTIONS=suppressions=.asan-suppressions $(TEST_TARGET)

.PHONY: run-tests
run-tests: test

$(TEST_TARGET): $(TEST_OBJECTS) $(filter-out $(BUILD_DIR)/main.o, $(OBJECTS)) | $(BIN_DIR)
	@echo "Linking test executable..."
	@$(CXX) $^ $(GTEST_FLAGS) $(LDFLAGS) -o $@

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)

# Install (optional - for system-wide installation)
.PHONY: install
install: $(TARGET)
	@echo "Installing Volta to /usr/local/bin..."
	@install -m 755 $(TARGET) /usr/local/bin/volta

# Uninstall
.PHONY: uninstall
uninstall:
	@echo "Uninstalling Volta..."
	@rm -f /usr/local/bin/volta

# Run the interpreter
.PHONY: run
run: $(TARGET)
	@$(TARGET)

# Format code (requires clang-format)
.PHONY: format
format:
	@echo "Formatting code..."
	@find $(SRC_DIR) $(INCLUDE_DIR) $(TEST_DIR) -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Help
.PHONY: help
help:
	@echo "Volta Programming Language - Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build the Volta interpreter (default)"
	@echo "  debug     - Build with debug symbols"
	@echo "  test      - Build and run tests"
	@echo "  clean     - Remove build artifacts"
	@echo "  install   - Install Volta system-wide"
	@echo "  uninstall - Remove system-wide installation"
	@echo "  run       - Build and run the interpreter"
	@echo "  format    - Format source code with clang-format"
	@echo "  help      - Show this help message"