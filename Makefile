# Volta Compiler Makefile
# Auto-detecting Makefile with proper incremental builds

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
CXXFLAGS := -std=c++20 -Wall -Wextra -pedantic -fexceptions -Wno-unused-parameter -Wno-unused-variable
CFLAGS := -std=c11 -Wall -Wextra -pedantic
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
    LLVM_INCLUDE_DIR := $(shell $(LLVM_CONFIG) --includedir)
    CXXFLAGS += -isystem $(LLVM_INCLUDE_DIR) -DLLVM_AVAILABLE
    LDFLAGS += $(LLVM_LDFLAGS)
    LIBS += $(LLVM_LIBS)
    # Add ncurses for LLVM on Linux
    ifeq ($(PLATFORM),Linux)
        LIBS += -lncurses
    endif
    $(info Found LLVM 18: $(LLVM_CONFIG))
else
    $(info LLVM 18 not found - building without LLVM support)
endif

# Directories
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
TEST_DIR := tests

# ============================================================================
# AUTO-DETECT all source files and modules
# ============================================================================

# Find all subdirectories in src/ (these are our modules)
SRC_MODULES := $(filter-out $(SRC_DIR)/main.cpp, $(wildcard $(SRC_DIR)/*))
SRC_MODULES := $(filter %/, $(SRC_MODULES))

# Auto-discover all C++ source files (recursively)
LIB_SRCS := $(shell find $(SRC_DIR) -name '*.cpp' -not -name 'main.cpp' 2>/dev/null)

# Auto-discover all C source files in runtime (recursively)
RUNTIME_SRCS := $(shell find $(SRC_DIR)/runtime -name '*.c' 2>/dev/null)

# Auto-discover all include directories
INC_DIRS := $(shell find $(INC_DIR) -type d 2>/dev/null)
CXXFLAGS += $(foreach dir,$(INC_DIRS),-I$(dir))
CFLAGS += $(foreach dir,$(INC_DIRS),-I$(dir))

# Generate object file paths
LIB_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(LIB_SRCS))
RUNTIME_OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(RUNTIME_SRCS))

MAIN_SRC := $(SRC_DIR)/main.cpp
MAIN_OBJ := $(BUILD_DIR)/main.o

# Test sources and binaries - search in subdirectories
TEST_SRCS := $(shell find $(TEST_DIR) -name 'test_*.cpp' 2>/dev/null)
# Extract just the test name from the full path
TEST_BINS := $(patsubst %.cpp,%,$(notdir $(TEST_SRCS)))
TEST_BINS := $(addprefix $(BUILD_DIR)/,$(TEST_BINS))

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

.PHONY: all clean test help build-dir release

# Default target
all: $(EXECUTABLE)

# Help target
help:
	@echo "Volta Compiler Makefile"
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

# Create build directories as needed
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Auto-create subdirectories for each module
define CREATE_BUILD_DIR
$(BUILD_DIR)/$(1):
	@mkdir -p $(BUILD_DIR)/$(1)
endef

# Generate rules for all module directories
MODULE_DIRS := $(sort $(dir $(LIB_OBJS) $(RUNTIME_OBJS)))
$(foreach dir,$(MODULE_DIRS),$(eval $(dir): ; @mkdir -p $(dir)))

# Library (force rebuild if sources change to avoid linking errors)
$(LIB): $(LIB_OBJS) $(RUNTIME_OBJS) | $(BUILD_DIR)
	@echo "Creating static library: $@"
	@rm -f $@
	@ar rcs $@ $(LIB_OBJS) $(RUNTIME_OBJS)
	@ranlib $@

# Main executable
$(EXECUTABLE): $(LIB) $(MAIN_OBJ)
	@echo "Linking executable: $@"
	@$(CXX) $(CXXFLAGS) $(MAIN_OBJ) -L$(BUILD_DIR) -lvolta $(LDFLAGS) $(LIBS) -o $@
	@echo "✓ Build successful! Executable: $(EXECUTABLE)"

# Pattern rule for C++ object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "Compiling: $<"
	@$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# Pattern rule for C object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Test executables - link against library after it's fully built
$(BUILD_DIR)/test_%: $(TEST_DIR)/unit/test_%.cpp $(LIB)
	@echo "Building unit test: $@"
	@$(CXX) $(CXXFLAGS) $(TEST_CXXFLAGS) $< -L$(BUILD_DIR) -lvolta $(TEST_LDFLAGS) $(LDFLAGS) $(LIBS) -o $@

$(BUILD_DIR)/test_%: $(TEST_DIR)/integration/test_%.cpp $(LIB)
	@echo "Building integration test: $@"
	@$(CXX) $(CXXFLAGS) $(TEST_CXXFLAGS) $< -L$(BUILD_DIR) -lvolta $(TEST_LDFLAGS) $(LDFLAGS) $(LIBS) -o $@

$(BUILD_DIR)/test_%: $(TEST_DIR)/e2e/test_%.cpp $(LIB)
	@echo "Building e2e test: $@"
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

# Include dependency files for incremental builds
-include $(LIB_OBJS:.o=.d)
-include $(RUNTIME_OBJS:.o=.d)
-include $(MAIN_OBJ:.o=.d)
