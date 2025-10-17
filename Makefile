# Volta Compiler Makefile
# Simple Makefile for building without CMake

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    PLATFORM := Linux
    NPROC := nproc
endif
ifeq ($(UNAME_S),Darwin)
    PLATFORM := macOS
    NPROC := sysctl -n hw.ncpu
endif

# Compiler settings
CXX := g++
CC := gcc
CXXFLAGS := -std=c++20 -Wall -Wextra -pedantic -fexceptions -Wno-unused-parameter -Wno-unused-variable -I./include -I./include/Lexer -I./include/Parser -I./include/HIR -I./include/MIR -I./include/Semantic -I./include/Error -I./include/runtime
CFLAGS := -std=c11 -Wall -Wextra -pedantic -I./include/runtime
LDFLAGS :=
LIBS :=

# GC Configuration (Boehm GC by default)
GC_MODE ?= VOLTA_GC_BOEHM
CFLAGS += -D$(GC_MODE)
CXXFLAGS += -D$(GC_MODE)

# If using Boehm GC, link against it
ifeq ($(GC_MODE),VOLTA_GC_BOEHM)
    LIBS += -lgc
endif

# Parallel compilation
MAKEFLAGS += -j$(shell $(NPROC))

# Build type (debug or release)
BUILD_TYPE ?= debug

ifeq ($(BUILD_TYPE),release)
    CXXFLAGS += -O3 -DNDEBUG
else
    CXXFLAGS += -g -O0
endif

# LLVM 18 detection (platform-agnostic)
LLVM_CONFIG := $(shell \
    which llvm-config-18 2>/dev/null || \
    which llvm-config-18.0 2>/dev/null || \
    which llvm-config@18 2>/dev/null || \
    which llvm-config 2>/dev/null | xargs -I{} sh -c '{} --version 2>/dev/null | grep -q "^18\." && echo {}' \
)

ifneq ($(LLVM_CONFIG),)
    LLVM_CXXFLAGS := $(shell $(LLVM_CONFIG) --cxxflags)
    LLVM_LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags)
    LLVM_LIBS := $(shell $(LLVM_CONFIG) --libs core support irreader)
    # Suppress LLVM warnings by treating them as system headers
    CXXFLAGS += -isystem $(shell $(LLVM_CONFIG) --includedir) -DLLVM_AVAILABLE
    LDFLAGS += $(LLVM_LDFLAGS)
    LIBS += $(LLVM_LIBS)
    $(info Found LLVM 18: $(LLVM_CONFIG))
else
    $(info LLVM 18 not found - building without LLVM support)
endif

# Directories
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
TEST_DIR := tests

# Find all source files
LEXER_SRCS := $(wildcard $(SRC_DIR)/Lexer/*.cpp)
PARSER_SRCS := $(wildcard $(SRC_DIR)/Parser/*.cpp)
HIR_SRCS := $(wildcard $(SRC_DIR)/HIR/*.cpp)
MIR_SRCS := $(wildcard $(SRC_DIR)/MIR/*.cpp)
SEMANTIC_SRCS := $(wildcard $(SRC_DIR)/Semantic/*.cpp)
ERROR_SRCS := $(wildcard $(SRC_DIR)/Error/*.cpp)
CODEGEN_SRCS := $(wildcard $(SRC_DIR)/Codegen/*.cpp)
RUNTIME_SRCS := $(wildcard $(SRC_DIR)/runtime/*.c)

LIB_SRCS := $(LEXER_SRCS) $(PARSER_SRCS) $(HIR_SRCS) $(MIR_SRCS) $(SEMANTIC_SRCS) $(ERROR_SRCS) $(CODEGEN_SRCS)
LIB_OBJS := $(LIB_SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
RUNTIME_OBJS := $(RUNTIME_SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

MAIN_SRC := $(SRC_DIR)/main.cpp
MAIN_OBJ := $(BUILD_DIR)/main.o

# Test sources
TEST_SRCS := $(wildcard $(TEST_DIR)/test_*.cpp)
TEST_BINS := $(TEST_SRCS:$(TEST_DIR)/test_%.cpp=$(BUILD_DIR)/test_%)

# Main executable
EXECUTABLE := $(BUILD_DIR)/volta

# Library archive
LIB := $(BUILD_DIR)/libvolta.a

# GTest detection (platform-agnostic)
GTEST_CONFIG := $(shell \
    pkg-config --exists gtest 2>/dev/null && echo "pkg-config" || \
    (test -d /usr/local/opt/googletest 2>/dev/null && echo "homebrew") || \
    (test -d /opt/homebrew/opt/googletest 2>/dev/null && echo "homebrew-arm") || \
    echo "system" \
)

ifeq ($(GTEST_CONFIG),pkg-config)
    TEST_CXXFLAGS := $(shell pkg-config --cflags gtest)
    TEST_LDFLAGS := $(shell pkg-config --libs gtest gtest_main) -pthread
    $(info Found GTest via pkg-config)
else ifeq ($(GTEST_CONFIG),homebrew)
    TEST_CXXFLAGS := -I/usr/local/opt/googletest/include
    TEST_LDFLAGS := -L/usr/local/opt/googletest/lib -lgtest_main -lgtest -pthread
    $(info Found GTest via Homebrew (Intel))
else ifeq ($(GTEST_CONFIG),homebrew-arm)
    TEST_CXXFLAGS := -I/opt/homebrew/opt/googletest/include
    TEST_LDFLAGS := -L/opt/homebrew/opt/googletest/lib -lgtest_main -lgtest -pthread
    $(info Found GTest via Homebrew (ARM))
else
    TEST_CXXFLAGS :=
    TEST_LDFLAGS := -lgtest_main -lgtest -pthread
    $(info Using system GTest (default paths))
endif

.PHONY: all clean test help build-dir

# Default target
all: $(EXECUTABLE)

# Help target
help:
	@echo "Reverie Compiler Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  make          - Build the compiler (default)"
	@echo "  make test     - Build and run all tests"
	@echo "  make clean    - Remove all build artifacts"
	@echo "  make release  - Build optimized release version"
	@echo "  make help     - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  BUILD_TYPE=debug|release  - Set build type (default: debug)"
	@echo ""
	@echo "Examples:"
	@echo "  make BUILD_TYPE=release"
	@echo "  make test"

# Build directory
build-dir:
	@mkdir -p $(BUILD_DIR)/Lexer
	@mkdir -p $(BUILD_DIR)/Parser
	@mkdir -p $(BUILD_DIR)/HIR
	@mkdir -p $(BUILD_DIR)/MIR
	@mkdir -p $(BUILD_DIR)/Semantic
	@mkdir -p $(BUILD_DIR)/Error
	@mkdir -p $(BUILD_DIR)/Codegen
	@mkdir -p $(BUILD_DIR)/runtime

# Library
$(LIB): build-dir $(LIB_OBJS) $(RUNTIME_OBJS)
	@echo "Creating static library: $@"
	@ar rcs $@ $(LIB_OBJS) $(RUNTIME_OBJS)

# Main executable
$(EXECUTABLE): $(LIB) $(MAIN_OBJ)
	@echo "Linking executable: $@"
	@$(CXX) $(CXXFLAGS) $(MAIN_OBJ) -L$(BUILD_DIR) -lvolta $(LDFLAGS) $(LIBS) -o $@
	@echo "✓ Build successful! Executable: $(EXECUTABLE)"

# Compile library object files (C++)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling: $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile runtime object files (C)
$(BUILD_DIR)/runtime/%.o: $(SRC_DIR)/runtime/%.c
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Compile main object file
$(MAIN_OBJ): $(MAIN_SRC)
	@echo "Compiling: $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Test executables
$(BUILD_DIR)/test_%: $(TEST_DIR)/test_%.cpp $(LIB)
	@echo "Building test: $@"
	@$(CXX) $(CXXFLAGS) $(TEST_CXXFLAGS) $< -L$(BUILD_DIR) -lvolta $(TEST_LDFLAGS) $(LDFLAGS) $(LIBS) -o $@

# Build all tests
build-tests: $(TEST_BINS)
	@echo "✓ All tests built successfully!"

# Run tests
test: build-tests
	@echo ""
	@echo "======================================"
	@echo "Running Test Suite"
	@echo "======================================"
	@passed=0; total=0; \
	for test in $(TEST_BINS); do \
		total=$$((total + 1)); \
		echo ""; \
		echo "Running: $$test"; \
		echo "--------------------------------------"; \
		if [ "$(PLATFORM)" = "Linux" ]; then \
			if LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:$$LD_LIBRARY_PATH $$test; then \
				passed=$$((passed + 1)); \
			fi; \
		else \
			if $$test; then \
				passed=$$((passed + 1)); \
			fi; \
		fi; \
	done; \
	echo ""; \
	echo "======================================"; \
	echo "Test Results: $$passed/$$total passed"; \
	echo "======================================"; \
	if [ $$passed -eq $$total ]; then \
		echo "✓ All tests passed!"; \
		exit 0; \
	else \
		echo "✗ Some tests failed!"; \
		exit 1; \
	fi

# Release build
release:
	@$(MAKE) BUILD_TYPE=release all

# Clean
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@echo "✓ Clean complete!"

# Dependency tracking (optional, for incremental builds)
-include $(LIB_OBJS:.o=.d)
-include $(MAIN_OBJ:.o=.d)

$(BUILD_DIR)/%.d: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -MM -MT $(@:.d=.o) $< > $@
