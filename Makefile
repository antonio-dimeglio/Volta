# Volta Programming Language - Makefile
# C++ Implementation

# Enable parallel compilation by default (use all CPU cores)
MAKEFLAGS += -j$(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
MAKEFLAGS += --no-print-directory

CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2 -Iinclude -Wno-unused-parameter -Wno-unused-variable -Wno-comment
DEBUG_FLAGS := -fsanitize=address -fsanitize=undefined -g -O0 -DDEBUG
LDFLAGS := -lffi -ldl

# Dependency tracking flags
DEPFLAGS = -MMD -MP

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

# Test files - recursively find all test cpp files
TEST_SOURCES := $(shell find $(TEST_DIR) -name '*.cpp' -type f)
TEST_OBJECTS := $(patsubst $(TEST_DIR)/%.cpp,$(BUILD_DIR)/tests/%.o,$(TEST_SOURCES))
TEST_BINS := $(patsubst $(TEST_DIR)/%.cpp,$(BIN_DIR)/%,$(TEST_SOURCES))

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
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# Compile main
$(MAIN_OBJECT): $(MAIN_SOURCE) | $(BUILD_DIR)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

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
$(BUILD_DIR)/tests/%.o: $(TEST_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling test $<..."
ifeq ($(shell uname), Darwin)
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) -I/opt/homebrew/opt/googletest/include -c $< -o $@
else
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@
endif

# Link individual test executables
$(BIN_DIR)/tests/%: $(BUILD_DIR)/tests/%.o $(filter-out $(BUILD_DIR)/main.o, $(OBJECTS))
	@mkdir -p $(dir $@)
	@echo "Linking test executable $@..."
	@$(CXX) $^ $(GTEST_FLAGS) $(LDFLAGS) -o $@

# Build all tests
.PHONY: build-tests
build-tests: $(TEST_BINS)

# Build and run all tests (combined executable - legacy)
.PHONY: test
test: $(TEST_TARGET)
	@echo "Running all tests..."
	@LSAN_OPTIONS=suppressions=.asan-suppressions $(TEST_TARGET)

.PHONY: run-tests
run-tests: test

$(TEST_TARGET): $(TEST_OBJECTS) $(filter-out $(BUILD_DIR)/main.o, $(OBJECTS)) | $(BIN_DIR)
	@echo "Linking combined test executable..."
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

# Individual C++ test targets
.PHONY: test-lexer test-lexer-missing test-parser test-semantic
.PHONY: test-ir-basicblock test-ir-builder test-ir-function test-ir-generator
.PHONY: test-ir-instruction test-ir-integration test-ir-module test-ir-optimization
.PHONY: test-ir-printer test-ir-value test-ir-verifier
.PHONY: test-bytecode-compiler test-bytecode-module
.PHONY: test-vm-arrays test-vm-end-to-end
.PHONY: test-end-to-end-ir

# Lexer tests
test-lexer: $(BIN_DIR)/tests/lexer/test_lexer
	@echo "Running lexer tests..."
	@$<

test-lexer-missing: $(BIN_DIR)/tests/lexer/test_lexer_missing_features
	@echo "Running lexer missing features tests..."
	@$<

# Parser tests
test-parser: $(BIN_DIR)/tests/parser/test_parser
	@echo "Running parser tests..."
	@$<

# Semantic tests
test-semantic: $(BIN_DIR)/tests/semantic/test_semantic
	@echo "Running semantic tests..."
	@$<

# IR tests
test-ir-basicblock: $(BIN_DIR)/tests/ir/test_ir_basicblock
	@echo "Running IR basic block tests..."
	@$<

test-ir-builder: $(BIN_DIR)/tests/ir/test_ir_builder
	@echo "Running IR builder tests..."
	@$<

test-ir-function: $(BIN_DIR)/tests/ir/test_ir_function
	@echo "Running IR function tests..."
	@$<

test-ir-generator: $(BIN_DIR)/tests/ir/test_ir_generator
	@echo "Running IR generator tests..."
	@$<

test-ir-instruction: $(BIN_DIR)/tests/ir/test_ir_instruction
	@echo "Running IR instruction tests..."
	@$<

test-ir-integration: $(BIN_DIR)/tests/ir/test_ir_integration
	@echo "Running IR integration tests..."
	@$<

test-ir-module: $(BIN_DIR)/tests/ir/test_ir_module
	@echo "Running IR module tests..."
	@$<

test-ir-optimization: $(BIN_DIR)/tests/ir/test_ir_optimization
	@echo "Running IR optimization tests..."
	@$<

test-ir-printer: $(BIN_DIR)/tests/ir/test_ir_printer
	@echo "Running IR printer tests..."
	@$<

test-ir-value: $(BIN_DIR)/tests/ir/test_ir_value
	@echo "Running IR value tests..."
	@$<

test-ir-verifier: $(BIN_DIR)/tests/ir/test_ir_verifier
	@echo "Running IR verifier tests..."
	@$<

# Bytecode tests
test-bytecode-compiler: $(BIN_DIR)/tests/bytecode/test_bytecode_compiler
	@echo "Running bytecode compiler tests..."
	@$<

test-bytecode-module: $(BIN_DIR)/tests/bytecode/test_bytecode_module
	@echo "Running bytecode module tests..."
	@$<

# VM tests
test-vm-arrays: $(BIN_DIR)/tests/vm/test_vm_arrays
	@echo "Running VM arrays tests..."
	@$<

test-vm-end-to-end: $(BIN_DIR)/tests/vm/test_vm_end_to_end
	@echo "Running VM end-to-end tests..."
	@$<

# Integration tests
test-end-to-end-ir: $(BIN_DIR)/tests/integration/test_end_to_end_ir
	@echo "Running end-to-end IR tests..."
	@$<

# Category test targets
.PHONY: test-lexer-all test-parser-all test-semantic-all test-ir-all test-bytecode-all test-vm-all test-integration-all

test-lexer-all: test-lexer test-lexer-missing
test-parser-all: test-parser
test-semantic-all: test-semantic
test-ir-all: test-ir-basicblock test-ir-builder test-ir-function test-ir-generator test-ir-instruction test-ir-integration test-ir-module test-ir-optimization test-ir-printer test-ir-value test-ir-verifier
test-bytecode-all: test-bytecode-compiler test-bytecode-module
test-vm-all: test-vm-arrays test-vm-end-to-end
test-integration-all: test-end-to-end-ir

# Run all individual tests
.PHONY: test-all-individual
test-all-individual: test-lexer-all test-parser-all test-semantic-all test-ir-all test-bytecode-all test-vm-all test-integration-all

# Volta program tests
VOLTA_TEST_DIR := tests/volta_programs

.PHONY: test-volta test-basics test-arrays test-loops test-edge-cases test-all-volta

test-volta: test-all-volta

test-all-volta: $(TARGET)
	@echo "Running all Volta program tests..."
	@./tests/run_tests.sh all

test-basics: $(TARGET)
	@echo "Running basic feature tests..."
	@./tests/run_tests.sh basics

test-arrays: $(TARGET)
	@echo "Running array tests..."
	@./tests/run_tests.sh arrays

test-loops: $(TARGET)
	@echo "Running loop tests..."
	@./tests/run_tests.sh loops

test-edge-cases: $(TARGET)
	@echo "Running edge case tests..."
	@./tests/run_tests.sh edge_cases

# Individual test execution
.PHONY: test-basics-variables test-arrays-comprehensive test-loops-ranges test-loops-nested

test-basics-variables: $(TARGET)
	@echo "Running basics/variables.volta..."
	@$(TARGET) $(VOLTA_TEST_DIR)/basics/variables.volta

test-arrays-comprehensive: $(TARGET)
	@echo "Running arrays/comprehensive.volta..."
	@$(TARGET) $(VOLTA_TEST_DIR)/arrays/comprehensive.volta

test-loops-ranges: $(TARGET)
	@echo "Running loops/for_ranges.volta..."
	@$(TARGET) $(VOLTA_TEST_DIR)/loops/for_ranges.volta

test-loops-nested: $(TARGET)
	@echo "Running loops/nested.volta..."
	@$(TARGET) $(VOLTA_TEST_DIR)/loops/nested.volta

# Help
.PHONY: help
help:
	@echo "Volta Programming Language - Build System"
	@echo ""
	@echo "Build Targets:"
	@echo "  all             - Build the Volta interpreter (default)"
	@echo "  debug           - Build with debug symbols"
	@echo "  build-tests     - Build all individual test executables"
	@echo "  clean           - Remove build artifacts"
	@echo "  install         - Install Volta system-wide"
	@echo "  uninstall       - Remove system-wide installation"
	@echo "  run             - Build and run the interpreter"
	@echo "  format          - Format source code with clang-format"
	@echo ""
	@echo "C++ Unit Tests (Combined):"
	@echo "  test            - Build and run all C++ tests in one executable"
	@echo ""
	@echo "C++ Unit Tests (Individual):"
	@echo "  test-all-individual  - Run all tests individually"
	@echo "  test-lexer-all       - All lexer tests"
	@echo "  test-parser-all      - All parser tests"
	@echo "  test-semantic-all    - All semantic tests"
	@echo "  test-ir-all          - All IR tests"
	@echo "  test-bytecode-all    - All bytecode tests"
	@echo "  test-vm-all          - All VM tests"
	@echo "  test-integration-all - All integration tests"
	@echo ""
	@echo "  test-lexer           - Lexer tests"
	@echo "  test-parser          - Parser tests"
	@echo "  test-semantic        - Semantic analyzer tests"
	@echo "  test-ir-basicblock   - IR basic block tests"
	@echo "  test-ir-builder      - IR builder tests"
	@echo "  test-ir-generator    - IR generator tests"
	@echo "  test-ir-optimization - IR optimization tests"
	@echo "  test-bytecode-compiler - Bytecode compiler tests"
	@echo "  test-vm-arrays       - VM array tests"
	@echo "  test-vm-end-to-end   - VM end-to-end tests"
	@echo "  (and many more... see Makefile for full list)"
	@echo ""
	@echo "Volta Program Tests:"
	@echo "  test-volta              - Run all Volta program tests"
	@echo "  test-basics             - Run basic feature tests"
	@echo "  test-arrays             - Run array tests"
	@echo "  test-loops              - Run loop tests"
	@echo "  test-edge-cases         - Run edge case tests"
	@echo "  test-basics-variables   - Run basics/variables.volta"
	@echo "  test-arrays-comprehensive - Run arrays/comprehensive.volta"
	@echo "  test-loops-ranges       - Run loops/for_ranges.volta"
	@echo "  test-loops-nested       - Run loops/nested.volta"
	@echo ""
	@echo "Other:"
	@echo "  help            - Show this help message"

# Include dependency files (auto-generated by compiler)
# The '-' prefix means: ignore errors if files don't exist yet
-include $(OBJECTS:.o=.d)
-include $(MAIN_OBJECT:.o=.d)
-include $(TEST_OBJECTS:.o=.d)