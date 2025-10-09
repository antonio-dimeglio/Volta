# Volta Programming Language - Makefile
# Multi-platform build system (Ubuntu/WSL, macOS, Windows/MSYS2)

# =============================================================================
# PLATFORM DETECTION
# =============================================================================

# Detect operating system
UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)
UNAME_M := $(shell uname -m 2>/dev/null || echo x86_64)

# Platform-specific settings
ifeq ($(OS),Windows_NT)
	PLATFORM := Windows
	EXE_EXT := .exe
	RM := del /Q
	RMDIR := rmdir /S /Q
	MKDIR := mkdir
	# Try to detect if we're in MSYS2/MinGW
	ifdef MSYSTEM
		PLATFORM := MSYS2
		RM := rm -f
		RMDIR := rm -rf
		MKDIR := mkdir -p
	endif
else ifeq ($(UNAME_S),Linux)
	PLATFORM := Linux
	EXE_EXT :=
	RM := rm -f
	RMDIR := rm -rf
	MKDIR := mkdir -p
else ifeq ($(UNAME_S),Darwin)
	PLATFORM := macOS
	EXE_EXT :=
	RM := rm -f
	RMDIR := rm -rf
	MKDIR := mkdir -p
else
	PLATFORM := Unknown
	EXE_EXT :=
	RM := rm -f
	RMDIR := rm -rf
	MKDIR := mkdir -p
endif

# Enable parallel compilation (use all CPU cores)
ifeq ($(PLATFORM),Linux)
	MAKEFLAGS += -j$(shell nproc)
else ifeq ($(PLATFORM),macOS)
	MAKEFLAGS += -j$(shell sysctl -n hw.ncpu)
else ifeq ($(PLATFORM),Windows)
	MAKEFLAGS += -j$(NUMBER_OF_PROCESSORS)
else ifeq ($(PLATFORM),MSYS2)
	MAKEFLAGS += -j$(shell nproc)
else
	MAKEFLAGS += -j4
endif
MAKEFLAGS += --no-print-directory

# =============================================================================
# COMPILER & FLAGS
# =============================================================================

CXX := g++

# Platform-specific compiler flags
ifeq ($(PLATFORM),Linux)
	CXXFLAGS := -std=c++20 -Wall -Wextra -O2 -Iinclude -I/usr/include/llvm-18 -I/usr/include/llvm-c-18 -Wno-unused-parameter -Wno-unused-variable -Wno-comment
else ifeq ($(PLATFORM),macOS)
	CXXFLAGS := -std=c++20 -Wall -Wextra -O2 -Iinclude -I/opt/homebrew/opt/llvm@18/include -Wno-unused-parameter -Wno-unused-variable -Wno-comment
else
	CXXFLAGS := -std=c++20 -Wall -Wextra -O2 -Iinclude -I/usr/include/llvm-18 -I/usr/include/llvm-c-18 -Wno-unused-parameter -Wno-unused-variable -Wno-comment
endif

DEBUG_FLAGS := -fsanitize=address -fsanitize=undefined -g -O0 -DDEBUG

# Platform-specific linker flags
ifeq ($(PLATFORM),Linux)
	LDFLAGS := -lffi -ldl -L/usr/lib/llvm-18/lib -lLLVM-18
else ifeq ($(PLATFORM),macOS)
	LDFLAGS := -lffi -ldl -L/opt/homebrew/opt/llvm@18/lib -lLLVM-18
else ifeq ($(PLATFORM),Windows)
	LDFLAGS := -lffi
else ifeq ($(PLATFORM),MSYS2)
	LDFLAGS := -lffi -ldl
else
	LDFLAGS := -lffi
endif

# Dependency tracking
DEPFLAGS = -MMD -MP

# Debug build
ifdef DEBUG
	CXXFLAGS += $(DEBUG_FLAGS)
	LDFLAGS += -fsanitize=address -fsanitize=undefined
endif

# =============================================================================
# GTEST CONFIGURATION (Platform-specific)
# =============================================================================

ifeq ($(PLATFORM),macOS)
	# macOS with Homebrew
	GTEST_FLAGS = -L/opt/homebrew/opt/googletest/lib -I/opt/homebrew/opt/googletest/include -lgtest -lgtest_main -pthread
	GTEST_INCLUDE = -I/opt/homebrew/opt/googletest/include
else ifeq ($(PLATFORM),Linux)
	# Ubuntu/WSL
	GTEST_FLAGS = -lgtest -lgtest_main -pthread
	GTEST_INCLUDE =
else ifeq ($(PLATFORM),MSYS2)
	# MSYS2/MinGW on Windows
	GTEST_FLAGS = -lgtest -lgtest_main -pthread
	GTEST_INCLUDE =
else
	# Fallback
	GTEST_FLAGS = -lgtest -lgtest_main -pthread
	GTEST_INCLUDE =
endif

# =============================================================================
# DIRECTORIES
# =============================================================================

SRC_DIR := src
INCLUDE_DIR := include
BUILD_DIR := build
BIN_DIR := bin
TEST_DIR := tests

# =============================================================================
# OUTPUT BINARIES
# =============================================================================

TARGET := $(BIN_DIR)/volta$(EXE_EXT)
TEST_TARGET := $(BIN_DIR)/test_volta$(EXE_EXT)

# =============================================================================
# SOURCE FILES
# =============================================================================

# Find all source files (excluding main.cpp)
ifeq ($(PLATFORM),Windows)
	# Windows without Unix tools - manual list
	ALL_SOURCES := $(wildcard $(SRC_DIR)/*/*.cpp $(SRC_DIR)/*/*/*.cpp)
	MAIN_SOURCE := $(SRC_DIR)/main.cpp
else
	# Unix-like systems with find
	ALL_SOURCES := $(shell find $(SRC_DIR) -name '*.cpp' ! -name 'main.cpp')
	MAIN_SOURCE := $(SRC_DIR)/main.cpp
endif

# Object files
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(ALL_SOURCES))
MAIN_OBJECT := $(BUILD_DIR)/main.o

# Test files
ifeq ($(PLATFORM),Windows)
	TEST_SOURCES := $(wildcard $(TEST_DIR)/*/*.cpp $(TEST_DIR)/*/*/*.cpp)
else
	TEST_SOURCES := $(shell find $(TEST_DIR) -name '*.cpp' -type f 2>/dev/null || echo "")
endif
TEST_OBJECTS := $(patsubst $(TEST_DIR)/%.cpp,$(BUILD_DIR)/tests/%.o,$(TEST_SOURCES))
TEST_BINS := $(patsubst $(TEST_DIR)/%.cpp,$(BIN_DIR)/%$(EXE_EXT),$(TEST_SOURCES))

# =============================================================================
# DEFAULT TARGET
# =============================================================================

.PHONY: all runtime
all: $(TARGET) runtime

# Runtime library
RUNTIME_LIB := $(BIN_DIR)/libvolta.a
RUNTIME_SRC := src/runtime/volta_runtime.c
RUNTIME_OBJ := $(BUILD_DIR)/runtime/volta_runtime.o

runtime: $(RUNTIME_LIB)

# =============================================================================
# DIRECTORY CREATION
# =============================================================================

$(BUILD_DIR) $(BIN_DIR) $(BUILD_DIR)/tests:
	@$(MKDIR) $@

# =============================================================================
# COMPILATION RULES
# =============================================================================

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@$(MKDIR) $(dir $@)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# Compile main
$(MAIN_OBJECT): $(MAIN_SOURCE) | $(BUILD_DIR)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# Compile runtime
$(RUNTIME_OBJ): $(RUNTIME_SRC) | $(BUILD_DIR)
	@$(MKDIR) $(dir $@)
	@echo "Compiling runtime library..."
ifeq ($(PLATFORM),macOS)
	@gcc -c $(RUNTIME_SRC) -o $(RUNTIME_OBJ) -Iinclude -I/opt/homebrew/opt/bdw-gc/include
else
	@gcc -c $(RUNTIME_SRC) -o $(RUNTIME_OBJ) -Iinclude -I/usr/include
endif

# Create runtime static library
$(RUNTIME_LIB): $(RUNTIME_OBJ) | $(BIN_DIR)
	@echo "Creating runtime library..."
	@ar rcs $(RUNTIME_LIB) $(RUNTIME_OBJ)
	@echo "Runtime library complete: $(RUNTIME_LIB)"

# Link executable
$(TARGET): $(OBJECTS) $(MAIN_OBJECT) | $(BIN_DIR)
	@echo "Linking $(TARGET)..."
	@$(CXX) $(CXXFLAGS) $(OBJECTS) $(MAIN_OBJECT) -o $@ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"
	@echo "Platform: $(PLATFORM) ($(UNAME_M))"

# =============================================================================
# DEBUG BUILD
# =============================================================================

.PHONY: debug
debug: clean
	$(MAKE) DEBUG=1 $(TARGET)

# =============================================================================
# TESTING
# =============================================================================

# Compile test files
$(BUILD_DIR)/tests/%.o: $(TEST_DIR)/%.cpp
	@$(MKDIR) $(dir $@)
	@echo "Compiling test $<..."
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(GTEST_INCLUDE) -c $< -o $@

# Link individual test executables
$(BIN_DIR)/tests/%$(EXE_EXT): $(BUILD_DIR)/tests/%.o $(filter-out $(BUILD_DIR)/main.o, $(OBJECTS))
	@$(MKDIR) $(dir $@)
	@echo "Linking test executable $@..."
	@$(CXX) $^ $(GTEST_FLAGS) $(LDFLAGS) -o $@

# Build all tests
.PHONY: build-tests
build-tests: $(TEST_BINS)

# Run all tests (combined executable)
.PHONY: test
test: $(TEST_TARGET)
	@echo "Running all tests..."
ifeq ($(PLATFORM),Linux)
	@LSAN_OPTIONS=suppressions=.asan-suppressions $(TEST_TARGET)
else
	@$(TEST_TARGET)
endif

.PHONY: run-tests
run-tests: test

$(TEST_TARGET): $(TEST_OBJECTS) $(filter-out $(BUILD_DIR)/main.o, $(OBJECTS)) | $(BIN_DIR)
	@echo "Linking combined test executable..."
	@$(CXX) $^ $(GTEST_FLAGS) $(LDFLAGS) -o $@

# =============================================================================
# CLEAN
# =============================================================================

.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
ifeq ($(PLATFORM),Windows)
	@if exist $(BUILD_DIR) $(RMDIR) $(BUILD_DIR)
	@if exist $(BIN_DIR) $(RMDIR) $(BIN_DIR)
else
	@$(RMDIR) $(BUILD_DIR) $(BIN_DIR)
endif

# =============================================================================
# INSTALL/UNINSTALL (Unix-like only)
# =============================================================================

.PHONY: install
install: $(TARGET)
ifeq ($(PLATFORM),Windows)
	@echo "Install not supported on Windows. Add bin/ to your PATH instead."
else
	@echo "Installing Volta to /usr/local/bin..."
	@install -m 755 $(TARGET) /usr/local/bin/volta
endif

.PHONY: uninstall
uninstall:
ifeq ($(PLATFORM),Windows)
	@echo "Uninstall not supported on Windows."
else
	@echo "Uninstalling Volta..."
	@$(RM) /usr/local/bin/volta
endif

# =============================================================================
# RUN
# =============================================================================

.PHONY: run
run: $(TARGET)
	@$(TARGET)

# =============================================================================
# CODE FORMATTING
# =============================================================================

.PHONY: format
format:
	@echo "Formatting code..."
ifeq ($(PLATFORM),Windows)
	@for /r %%i in ($(SRC_DIR)\*.cpp $(SRC_DIR)\*.hpp $(INCLUDE_DIR)\*.hpp $(TEST_DIR)\*.cpp) do clang-format -i "%%i"
else
	@find $(SRC_DIR) $(INCLUDE_DIR) $(TEST_DIR) -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
endif

# =============================================================================
# INDIVIDUAL TEST TARGETS
# =============================================================================

.PHONY: test-lexer test-parser test-semantic
.PHONY: test-ir-all test-bytecode-all test-vm-all

test-lexer: $(BIN_DIR)/tests/lexer/test_lexer$(EXE_EXT)
	@echo "Running lexer tests..."
	@$<

test-parser: $(BIN_DIR)/tests/parser/test_parser$(EXE_EXT)
	@echo "Running parser tests..."
	@$<

test-semantic: $(BIN_DIR)/tests/semantic/test_semantic$(EXE_EXT)
	@echo "Running semantic tests..."
	@$<

# Volta program tests (Unix-like only - requires bash)
VOLTA_TEST_DIR := tests/volta_programs

.PHONY: test-volta test-all-volta

test-volta: test-all-volta

test-all-volta: $(TARGET)
ifeq ($(PLATFORM),Windows)
	@echo "Volta program tests not supported on Windows (requires bash)"
else
	@echo "Running all Volta program tests..."
	@./tests/run_tests.sh all
endif

# =============================================================================
# HELP
# =============================================================================

.PHONY: help
help:
	@echo "Volta Programming Language - Build System"
	@echo "Platform: $(PLATFORM) ($(UNAME_M))"
	@echo ""
	@echo "Build Targets:"
	@echo "  all             - Build the Volta interpreter (default)"
	@echo "  debug           - Build with debug symbols and sanitizers"
	@echo "  build-tests     - Build all individual test executables"
	@echo "  clean           - Remove build artifacts"
	@echo "  install         - Install Volta system-wide (Unix only)"
	@echo "  uninstall       - Remove system-wide installation (Unix only)"
	@echo "  run             - Build and run the interpreter"
	@echo "  format          - Format source code with clang-format"
	@echo ""
	@echo "Testing:"
	@echo "  test            - Build and run all C++ tests"
	@echo "  test-lexer      - Run lexer tests"
	@echo "  test-parser     - Run parser tests"
	@echo "  test-semantic   - Run semantic analyzer tests"
	@echo "  test-volta      - Run Volta program tests (Unix only)"
	@echo ""
	@echo "Dependencies Setup:"
	@echo "  make deps       - Show dependency installation instructions"
	@echo ""
	@echo "Other:"
	@echo "  help            - Show this help message"

# =============================================================================
# DEPENDENCY INSTALLATION INSTRUCTIONS
# =============================================================================

.PHONY: deps
deps:
	@echo "==================================================================="
	@echo "Volta Dependency Installation Instructions"
	@echo "==================================================================="
	@echo ""
	@echo "Current platform: $(PLATFORM)"
	@echo ""
ifeq ($(PLATFORM),Linux)
	@echo "--- Ubuntu/WSL/Debian ---"
	@echo "sudo apt update"
	@echo "sudo apt install -y build-essential g++ libffi-dev libgtest-dev"
	@echo ""
	@echo "If you plan to migrate to LLVM (recommended):"
	@echo "sudo apt install -y llvm-18-dev clang-18 libgc-dev"
	@echo ""
else ifeq ($(PLATFORM),macOS)
	@echo "--- macOS (Homebrew) ---"
	@echo "# Install Xcode Command Line Tools first:"
	@echo "xcode-select --install"
	@echo ""
	@echo "# Install dependencies:"
	@echo "brew install libffi googletest"
	@echo ""
	@echo "If you plan to migrate to LLVM (recommended):"
	@echo "brew install llvm@18 bdw-gc"
	@echo ""
else ifeq ($(PLATFORM),Windows)
	@echo "--- Windows (MSYS2) ---"
	@echo "1. Install MSYS2 from https://www.msys2.org/"
	@echo "2. Open MSYS2 MINGW64 terminal"
	@echo "3. Run:"
	@echo "   pacman -Syu"
	@echo "   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make"
	@echo "   pacman -S mingw-w64-x86_64-libffi mingw-w64-x86_64-gtest"
	@echo ""
	@echo "If you plan to migrate to LLVM (recommended):"
	@echo "   pacman -S mingw-w64-x86_64-llvm mingw-w64-x86_64-clang"
	@echo "   pacman -S mingw-w64-x86_64-gc"
	@echo ""
	@echo "4. Use 'mingw32-make' instead of 'make'"
	@echo ""
else ifeq ($(PLATFORM),MSYS2)
	@echo "--- MSYS2/MinGW (Windows) ---"
	@echo "pacman -Syu"
	@echo "pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make"
	@echo "pacman -S mingw-w64-x86_64-libffi mingw-w64-x86_64-gtest"
	@echo ""
	@echo "If you plan to migrate to LLVM (recommended):"
	@echo "pacman -S mingw-w64-x86_64-llvm mingw-w64-x86_64-clang"
	@echo "pacman -S mingw-w64-x86_64-gc"
	@echo ""
else
	@echo "Platform not recognized. Please install manually:"
	@echo "  - C++ compiler (g++ or clang++)"
	@echo "  - libffi development headers"
	@echo "  - Google Test (gtest)"
	@echo "  - (Optional) LLVM 18, Boehm GC for LLVM migration"
endif
	@echo "==================================================================="
	@echo ""
	@echo "After installing dependencies, run:"
	@echo "  make all        # Build Volta"
	@echo "  make test       # Run tests"
	@echo ""

# =============================================================================
# DEPENDENCY FILES
# =============================================================================

# Include dependency files (auto-generated by compiler)
-include $(OBJECTS:.o=.d)
-include $(MAIN_OBJECT:.o=.d)
-include $(TEST_OBJECTS:.o=.d)
