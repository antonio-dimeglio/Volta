# Volta LLVM Migration: Complete Implementation Plan

**Date:** 2025-10-08
**Goal:** Replace bytecode VM with LLVM-based AOT compiler
**Timeline:** 12-16 weeks to production-ready compiler
**LLVM Version:** 18.x (latest stable as of 2025)

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [LLVM Crash Course](#llvm-crash-course)
3. [Project Setup](#project-setup)
4. [Phase 1: Hello World (Week 1)](#phase-1-hello-world-week-1)
5. [Phase 2: Core Codegen (Weeks 2-3)](#phase-2-core-codegen-weeks-2-3)
6. [Phase 3: Runtime & GC (Week 4)](#phase-3-runtime--gc-week-4)
7. [Phase 4: Advanced Features (Weeks 5-8)](#phase-4-advanced-features-weeks-5-8)
8. [Phase 5: Standard Library (Weeks 9-10)](#phase-5-standard-library-weeks-9-10)
9. [Phase 6: C Interop & FFI (Week 11)](#phase-6-c-interop--ffi-week-11)
10. [Phase 7: Error Reporting (Week 12)](#phase-7-error-reporting-week-12)
11. [Phase 8: Optimization & Polish (Weeks 13-16)](#phase-8-optimization--polish-weeks-13-16)
12. [Testing Strategy](#testing-strategy)
13. [Performance Targets](#performance-targets)
14. [Appendix: Code Examples](#appendix-code-examples)

---

## Architecture Overview

### What We're Building

```
┌──────────────────────────────────────────────────────────┐
│                    Volta Source (.vlt)                   │
└──────────────────────────────────────────────────────────┘
                          ↓
┌──────────────────────────────────────────────────────────┐
│  Frontend (KEEP - Already Working)                       │
│  ├─ Lexer (lexer.cpp)                                   │
│  ├─ Parser (Parser.cpp)                                 │
│  ├─ Semantic Analyzer (SemanticAnalyzer.cpp)           │
│  └─ Typed AST                                           │
└──────────────────────────────────────────────────────────┘
                          ↓
┌──────────────────────────────────────────────────────────┐
│  LLVM Backend (NEW - Build This)                        │
│  ├─ LLVM IR Codegen (LLVMCodegen.cpp)                  │
│  ├─ Type Lowering (Volta types → LLVM types)           │
│  ├─ Function Codegen                                    │
│  ├─ Expression Codegen                                  │
│  └─ Control Flow Codegen                               │
└──────────────────────────────────────────────────────────┘
                          ↓
┌──────────────────────────────────────────────────────────┐
│                    LLVM IR (.ll file)                    │
│  - Human-readable intermediate representation            │
│  - Can be inspected, debugged, optimized                │
└──────────────────────────────────────────────────────────┘
                          ↓
┌──────────────────────────────────────────────────────────┐
│  LLVM Optimizer (Built-in)                              │
│  ├─ -O0: No optimization (fast compile)                │
│  ├─ -O1: Basic optimization                            │
│  ├─ -O2: Aggressive optimization                       │
│  └─ -O3: Max optimization + vectorization              │
└──────────────────────────────────────────────────────────┘
                          ↓
┌──────────────────────────────────────────────────────────┐
│              Native Object Code (.o file)                │
└──────────────────────────────────────────────────────────┘
                          ↓
┌──────────────────────────────────────────────────────────┐
│  Linker (clang/gcc)                                     │
│  Links with:                                            │
│  ├─ Volta Runtime (libvolta.a)                         │
│  ├─ Boehm GC (libgc.a)                                 │
│  ├─ C Standard Library (libc)                          │
│  └─ User C libraries (for FFI)                         │
└──────────────────────────────────────────────────────────┘
                          ↓
┌──────────────────────────────────────────────────────────┐
│              Native Executable (./program)               │
│  - No VM needed at runtime                              │
│  - Fully optimized native code                          │
│  - Standard debugging tools work (gdb, lldb)            │
└──────────────────────────────────────────────────────────┘
```

### What We're Deleting

```
DELETE (~10,000 LOC):
├─ src/vm/VM.cpp                    (~2,000 LOC)
├─ src/vm/BytecodeCompiler.cpp      (~1,500 LOC)
├─ src/vm/BytecodeModule.cpp        (~800 LOC)
├─ src/vm/BytecodeDecompiler.cpp    (~500 LOC)
├─ src/gc/GarbageCollector.cpp      (~3,000 LOC)
├─ include/vm/OPCode.hpp            (~200 LOC)
├─ include/vm/VM.hpp                (~400 LOC)
├─ include/vm/Value.hpp             (~300 LOC)
├─ include/gc/GarbageCollector.hpp  (~800 LOC)
└─ All VM tests                     (~500 LOC)

ARCHIVE IN GIT:
$ git checkout -b archive/bytecode-vm
$ git add -A
$ git commit -m "Archive bytecode VM before LLVM migration"
$ git checkout main
```

### What We're Building

```
BUILD (~6,000 LOC):
├─ src/llvm/LLVMCodegen.cpp         (~2,500 LOC)
├─ src/llvm/TypeLowering.cpp        (~500 LOC)
├─ src/runtime/volta_runtime.c      (~1,000 LOC)
├─ src/runtime/volta_array.c        (~300 LOC)
├─ src/runtime/volta_string.c       (~300 LOC)
├─ src/runtime/volta_io.c           (~200 LOC)
├─ include/runtime/volta_runtime.h  (~400 LOC)
├─ include/llvm/LLVMCodegen.hpp     (~300 LOC)
└─ New LLVM tests                   (~500 LOC)

TOTAL: Net -4,000 LOC (simpler codebase!)
```

---

## LLVM Crash Course

### Key Concepts You Need to Know

#### 1. **LLVMContext**
- Container for all LLVM state
- Thread-local (one per compilation unit)
- Owns all types, constants, metadata

```cpp
LLVMContext context;  // Create once, use everywhere
```

#### 2. **Module**
- Container for all functions, globals, types
- One module = one .ll file
- Can be compiled independently

```cpp
Module module("my_program", context);
```

#### 3. **IRBuilder**
- Helper for creating LLVM instructions
- Tracks insertion point (where to add instructions)
- Like your current IR builder, but for LLVM IR

```cpp
IRBuilder<> builder(context);
```

#### 4. **Types**
- `IntegerType::get(context, 64)` → i64
- `Type::getDoubleTy(context)` → double
- `Type::getInt1Ty(context)` → i1 (bool)
- `PointerType::get(context, 0)` → ptr (opaque pointer in LLVM 18)

#### 5. **Values**
- Everything is a `Value*` (like your IR)
- `Constant*` - compile-time constants
- `Instruction*` - runtime computations
- `Argument*` - function parameters

#### 6. **BasicBlocks**
- Sequence of instructions ending with terminator
- Terminators: ret, br (branch), switch
- Same as your IR basic blocks

```cpp
BasicBlock* entry = BasicBlock::Create(context, "entry", function);
builder.SetInsertPoint(entry);
```

#### 7. **Functions**
```cpp
// fn add(x: int, y: int) -> int
FunctionType* ft = FunctionType::get(
    Type::getInt64Ty(context),        // return type
    {Type::getInt64Ty(context),       // param 1
     Type::getInt64Ty(context)},      // param 2
    false                              // not varargs
);

Function* f = Function::Create(
    ft,
    Function::ExternalLinkage,
    "add",
    module
);
```

### LLVM IR Example

**Volta code:**
```volta
fn add(x: int, y: int) -> int {
    return x + y
}
```

**LLVM IR:**
```llvm
define i64 @add(i64 %x, i64 %y) {
entry:
  %result = add i64 %x, %y
  ret i64 %result
}
```

**Generated by C++ code:**
```cpp
// Create function
FunctionType* ft = FunctionType::get(
    Type::getInt64Ty(context),
    {Type::getInt64Ty(context), Type::getInt64Ty(context)},
    false
);
Function* f = Function::Create(ft, Function::ExternalLinkage, "add", module);

// Set parameter names
Function::arg_iterator args = f->arg_begin();
Value* x = args++;
x->setName("x");
Value* y = args++;
y->setName("y");

// Create basic block
BasicBlock* entry = BasicBlock::Create(context, "entry", f);
builder.SetInsertPoint(entry);

// Generate: %result = add i64 %x, %y
Value* result = builder.CreateAdd(x, y, "result");

// Generate: ret i64 %result
builder.CreateRet(result);
```

---

## Project Setup

### 1. Install LLVM 18

#### Ubuntu/Debian:
```bash
# Add LLVM apt repository
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 18

# Install LLVM development packages
sudo apt install llvm-18 llvm-18-dev clang-18 libclang-18-dev

# Verify installation
llvm-config-18 --version  # Should show 18.x.x
```

#### macOS:
```bash
brew install llvm@18

# Add to PATH
echo 'export PATH="/opt/homebrew/opt/llvm@18/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc

# Verify
llvm-config --version
```

#### Arch Linux:
```bash
sudo pacman -S llvm clang

# LLVM 18 should be default in recent Arch
llvm-config --version
```

### 2. Install Boehm GC

The Boehm-Demers-Weiser conservative garbage collector - battle-tested, used by GCC, Mono, etc.

#### Ubuntu/Debian:
```bash
sudo apt install libgc-dev
```

#### macOS:
```bash
brew install bdw-gc
```

#### Arch Linux:
```bash
sudo pacman -S gc
```

### 3. Update Makefile

**Create new `Makefile.llvm`:**
```makefile
CXX := clang++-18
CXXFLAGS := -std=c++17 -Wall -Wextra -g
LLVM_CXXFLAGS := $(shell llvm-config-18 --cxxflags)
LLVM_LDFLAGS := $(shell llvm-config-18 --ldflags --libs core support)

INCLUDES := -I./include -I./include/llvm
SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin

# Source files
FRONTEND_SRCS := \
    $(SRC_DIR)/lexer/lexer.cpp \
    $(SRC_DIR)/lexer/token.cpp \
    $(SRC_DIR)/parser/Parser.cpp \
    $(SRC_DIR)/semantic/SemanticAnalyzer.cpp \
    $(SRC_DIR)/semantic/Type.cpp \
    $(SRC_DIR)/semantic/SymbolTable.cpp \
    $(SRC_DIR)/error/ErrorReporter.cpp \
    $(SRC_DIR)/error/ErrorTypes.cpp

LLVM_SRCS := \
    $(SRC_DIR)/llvm/LLVMCodegen.cpp \
    $(SRC_DIR)/llvm/TypeLowering.cpp

MAIN_SRC := $(SRC_DIR)/main.cpp

SRCS := $(FRONTEND_SRCS) $(LLVM_SRCS) $(MAIN_SRC)
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Runtime library (separate compilation)
RUNTIME_SRCS := \
    src/runtime/volta_runtime.c \
    src/runtime/volta_array.c \
    src/runtime/volta_string.c \
    src/runtime/volta_io.c

RUNTIME_OBJS := $(RUNTIME_SRCS:src/%.c=$(BUILD_DIR)/%.o)
RUNTIME_LIB := $(BIN_DIR)/libvolta.a

# Compiler executable
VOLTA_COMPILER := $(BIN_DIR)/volta

.PHONY: all clean runtime

all: $(VOLTA_COMPILER) $(RUNTIME_LIB)

# Compile Volta compiler
$(VOLTA_COMPILER): $(OBJS) | $(BIN_DIR)
    $(CXX) $(CXXFLAGS) $(OBJS) $(LLVM_LDFLAGS) -o $@

# Compile C++ sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
    @mkdir -p $(dir $@)
    $(CXX) $(CXXFLAGS) $(LLVM_CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile runtime library (C code)
$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
    @mkdir -p $(dir $@)
    clang -c -O2 -fPIC $< -o $@

# Create runtime archive
$(RUNTIME_LIB): $(RUNTIME_OBJS) | $(BIN_DIR)
    ar rcs $@ $^

$(BUILD_DIR):
    mkdir -p $(BUILD_DIR)

$(BIN_DIR):
    mkdir -p $(BIN_DIR)

clean:
    rm -rf $(BUILD_DIR) $(BIN_DIR)

# Usage example:
# volta compile program.vlt -o program
```

### 4. Create Directory Structure

```bash
mkdir -p src/llvm
mkdir -p src/runtime
mkdir -p include/llvm
mkdir -p include/runtime
mkdir -p tests/llvm
```

---

## Phase 1: Hello World (Week 1)

**Goal:** Compile and run the simplest possible Volta program.

### Target Program

```volta
fn main() -> int {
    print("Hello, LLVM!")
    return 0
}
```

### Step 1.1: Create LLVMCodegen Skeleton

**File: `include/llvm/LLVMCodegen.hpp`**
```cpp
#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include "ast/Statement.hpp"
#include "ast/Expression.hpp"
#include "semantic/SemanticAnalyzer.hpp"

namespace volta::llvm_codegen {

class LLVMCodegen {
public:
    explicit LLVMCodegen(const std::string& moduleName);
    ~LLVMCodegen();

    // Main entry point: generate LLVM IR from typed AST
    bool generate(const volta::ast::Program* program,
                  const volta::semantic::SemanticAnalyzer* analyzer);

    // Output LLVM IR to file
    bool emitLLVMIR(const std::string& filename);

    // Compile to object file
    bool compileToObject(const std::string& filename);

    // Get the LLVM module (for inspection)
    ::llvm::Module* getModule() { return module_.get(); }

private:
    // LLVM core objects
    std::unique_ptr<::llvm::LLVMContext> context_;
    std::unique_ptr<::llvm::Module> module_;
    std::unique_ptr<::llvm::IRBuilder<>> builder_;

    // Code generation
    void generateFunction(const volta::ast::FnDeclaration* fn);
    ::llvm::Value* generateExpression(const volta::ast::Expression* expr);
    void generateStatement(const volta::ast::Statement* stmt);

    // Type lowering
    ::llvm::Type* lowerType(std::shared_ptr<volta::semantic::Type> voltaType);

    // Symbol tables
    std::unordered_map<std::string, ::llvm::Value*> namedValues_;
    std::unordered_map<std::string, ::llvm::Function*> functions_;

    // Current function being generated
    ::llvm::Function* currentFunction_;

    // Semantic analyzer (for type information)
    const volta::semantic::SemanticAnalyzer* analyzer_;

    // Error tracking
    bool hasErrors_;
    std::vector<std::string> errors_;

    void reportError(const std::string& message);
};

} // namespace volta::llvm_codegen
```

**File: `src/llvm/LLVMCodegen.cpp`**
```cpp
#include "llvm/LLVMCodegen.hpp"
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetOptions.h>

using namespace llvm;

namespace volta::llvm_codegen {

LLVMCodegen::LLVMCodegen(const std::string& moduleName)
    : context_(std::make_unique<LLVMContext>()),
      module_(std::make_unique<Module>(moduleName, *context_)),
      builder_(std::make_unique<IRBuilder<>>(*context_)),
      currentFunction_(nullptr),
      analyzer_(nullptr),
      hasErrors_(false) {
}

LLVMCodegen::~LLVMCodegen() = default;

bool LLVMCodegen::generate(const volta::ast::Program* program,
                           const volta::semantic::SemanticAnalyzer* analyzer) {
    if (!program || !analyzer) {
        reportError("Null program or analyzer");
        return false;
    }

    analyzer_ = analyzer;

    // Generate code for all top-level declarations
    for (const auto& stmt : program->statements) {
        if (auto* fnDecl = dynamic_cast<const volta::ast::FnDeclaration*>(stmt.get())) {
            generateFunction(fnDecl);
        }
        // TODO: Handle other top-level declarations
    }

    // Verify the module
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    if (verifyModule(*module_, &errorStream)) {
        reportError("Module verification failed: " + errorMsg);
        return false;
    }

    return !hasErrors_;
}

void LLVMCodegen::generateFunction(const volta::ast::FnDeclaration* fn) {
    // TODO: Implement in Phase 1.2
}

Value* LLVMCodegen::generateExpression(const volta::ast::Expression* expr) {
    // TODO: Implement in Phase 2
    return nullptr;
}

void LLVMCodegen::generateStatement(const volta::ast::Statement* stmt) {
    // TODO: Implement in Phase 2
}

Type* LLVMCodegen::lowerType(std::shared_ptr<volta::semantic::Type> voltaType) {
    // TODO: Implement in Phase 1.3
    return Type::getInt64Ty(*context_);  // Placeholder
}

bool LLVMCodegen::emitLLVMIR(const std::string& filename) {
    std::error_code ec;
    raw_fd_ostream dest(filename, ec, sys::fs::OF_None);

    if (ec) {
        reportError("Could not open file: " + ec.message());
        return false;
    }

    module_->print(dest, nullptr);
    return true;
}

bool LLVMCodegen::compileToObject(const std::string& filename) {
    // TODO: Implement in Phase 1.4
    return false;
}

void LLVMCodegen::reportError(const std::string& message) {
    hasErrors_ = true;
    errors_.push_back(message);
    // TODO: Integrate with ErrorReporter
    std::cerr << "LLVM Codegen Error: " << message << std::endl;
}

} // namespace volta::llvm_codegen
```

### Step 1.2: Generate First Function

**Update `generateFunction()` in LLVMCodegen.cpp:**
```cpp
void LLVMCodegen::generateFunction(const volta::ast::FnDeclaration* fn) {
    // Get function name
    std::string funcName = fn->identifier;

    // Lower return type
    auto semReturnType = analyzer_->resolveTypeAnnotation(fn->returnType.get());
    Type* returnType = lowerType(semReturnType);

    // Lower parameter types
    std::vector<Type*> paramTypes;
    for (const auto& param : fn->parameters) {
        auto semType = analyzer_->resolveTypeAnnotation(param->type.get());
        paramTypes.push_back(lowerType(semType));
    }

    // Create function type
    FunctionType* funcType = FunctionType::get(returnType, paramTypes, false);

    // Create function
    Function* function = Function::Create(
        funcType,
        Function::ExternalLinkage,
        funcName,
        module_.get()
    );

    // Store in function table
    functions_[funcName] = function;

    // Set parameter names
    unsigned idx = 0;
    for (auto& arg : function->args()) {
        arg.setName(fn->parameters[idx]->identifier);
        idx++;
    }

    // Create entry basic block
    BasicBlock* entry = BasicBlock::Create(*context_, "entry", function);
    builder_->SetInsertPoint(entry);

    // Track current function
    currentFunction_ = function;

    // Clear named values (function scope)
    namedValues_.clear();

    // Add parameters to symbol table
    for (auto& arg : function->args()) {
        namedValues_[std::string(arg.getName())] = &arg;
    }

    // Generate function body
    if (fn->body) {
        for (const auto& stmt : fn->body->statements) {
            generateStatement(stmt.get());
        }
    }

    // If no return was generated and function returns void, add implicit return
    if (!builder_->GetInsertBlock()->getTerminator()) {
        if (returnType->isVoidTy()) {
            builder_->CreateRetVoid();
        } else {
            // Non-void function without return - error
            // (semantic analyzer should have caught this)
            reportError("Function '" + funcName + "' missing return statement");
        }
    }

    // Verify function
    std::string errorMsg;
    raw_string_ostream errorStream(errorMsg);
    if (verifyFunction(*function, &errorStream)) {
        reportError("Function verification failed: " + errorMsg);
        function->eraseFromParent();  // Delete the function
    }

    currentFunction_ = nullptr;
}
```

### Step 1.3: Implement Type Lowering

**File: `src/llvm/TypeLowering.cpp`**
```cpp
#include "llvm/LLVMCodegen.hpp"

using namespace llvm;

namespace volta::llvm_codegen {

Type* LLVMCodegen::lowerType(std::shared_ptr<volta::semantic::Type> voltaType) {
    if (!voltaType) {
        return Type::getVoidTy(*context_);
    }

    switch (voltaType->kind()) {
        case volta::semantic::Type::Kind::Int:
            return Type::getInt64Ty(*context_);

        case volta::semantic::Type::Kind::Float:
            return Type::getDoubleTy(*context_);

        case volta::semantic::Type::Kind::Bool:
            return Type::getInt1Ty(*context_);

        case volta::semantic::Type::Kind::String:
            // String is a pointer to GC-allocated data
            return PointerType::get(*context_, 0);  // Opaque pointer

        case volta::semantic::Type::Kind::Void:
            return Type::getVoidTy(*context_);

        case volta::semantic::Type::Kind::Array: {
            // Array is a pointer to GC-allocated array structure
            return PointerType::get(*context_, 0);
        }

        case volta::semantic::Type::Kind::Struct: {
            // Struct is a pointer to GC-allocated struct
            return PointerType::get(*context_, 0);
        }

        default:
            reportError("Unsupported type in type lowering");
            return Type::getVoidTy(*context_);
    }
}

} // namespace volta::llvm_codegen
```

### Step 1.4: Create Minimal Runtime

**File: `include/runtime/volta_runtime.h`**
```c
#ifndef VOLTA_RUNTIME_H
#define VOLTA_RUNTIME_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// GC functions
void* volta_gc_alloc(size_t size);
void volta_gc_collect(void);

// Print functions
void volta_print_int(int64_t value);
void volta_print_float(double value);
void volta_print_bool(int8_t value);
void volta_print_string(const char* str);

// String functions
typedef struct {
    size_t length;
    char* data;
} VoltaString;

VoltaString* volta_string_new(const char* data, size_t length);
int64_t volta_string_length(VoltaString* str);

// Array functions
typedef struct {
    size_t length;
    size_t capacity;
    void** data;
} VoltaArray;

VoltaArray* volta_array_new(size_t capacity);
void volta_array_push(VoltaArray* arr, void* elem);
int64_t volta_array_length(VoltaArray* arr);

#ifdef __cplusplus
}
#endif

#endif // VOLTA_RUNTIME_H
```

**File: `src/runtime/volta_runtime.c`**
```c
#include "runtime/volta_runtime.h"
#include <gc.h>  // Boehm GC
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Initialize GC on first use
static void init_gc(void) {
    static int initialized = 0;
    if (!initialized) {
        GC_INIT();
        initialized = 1;
    }
}

void* volta_gc_alloc(size_t size) {
    init_gc();
    void* ptr = GC_MALLOC(size);
    if (!ptr) {
        fprintf(stderr, "GC allocation failed\n");
        exit(1);
    }
    return ptr;
}

void volta_gc_collect(void) {
    GC_gcollect();
}

void volta_print_int(int64_t value) {
    printf("%ld\n", value);
}

void volta_print_float(double value) {
    printf("%f\n", value);
}

void volta_print_bool(int8_t value) {
    printf("%s\n", value ? "true" : "false");
}

void volta_print_string(const char* str) {
    printf("%s\n", str);
}

VoltaString* volta_string_new(const char* data, size_t length) {
    VoltaString* str = volta_gc_alloc(sizeof(VoltaString));
    str->length = length;
    str->data = volta_gc_alloc(length + 1);
    memcpy(str->data, data, length);
    str->data[length] = '\0';
    return str;
}

int64_t volta_string_length(VoltaString* str) {
    return str->length;
}

VoltaArray* volta_array_new(size_t capacity) {
    VoltaArray* arr = volta_gc_alloc(sizeof(VoltaArray));
    arr->length = 0;
    arr->capacity = capacity;
    arr->data = volta_gc_alloc(capacity * sizeof(void*));
    return arr;
}

void volta_array_push(VoltaArray* arr, void* elem) {
    if (arr->length >= arr->capacity) {
        // Grow array
        size_t new_capacity = arr->capacity * 2;
        void** new_data = volta_gc_alloc(new_capacity * sizeof(void*));
        memcpy(new_data, arr->data, arr->length * sizeof(void*));
        arr->data = new_data;
        arr->capacity = new_capacity;
    }
    arr->data[arr->length++] = elem;
}

int64_t volta_array_length(VoltaArray* arr) {
    return arr->length;
}
```

### Step 1.5: Generate Call to Print

**Update `generateExpression()` for CallExpression:**
```cpp
Value* LLVMCodegen::generateExpression(const volta::ast::Expression* expr) {
    if (auto* call = dynamic_cast<const volta::ast::CallExpression*>(expr)) {
        return generateCallExpression(call);
    }

    if (auto* strLit = dynamic_cast<const volta::ast::StringLiteral*>(expr)) {
        return generateStringLiteral(strLit);
    }

    if (auto* intLit = dynamic_cast<const volta::ast::IntegerLiteral*>(expr)) {
        return ConstantInt::get(Type::getInt64Ty(*context_), intLit->value);
    }

    // TODO: More expression types
    reportError("Unsupported expression type");
    return nullptr;
}

Value* LLVMCodegen::generateCallExpression(const volta::ast::CallExpression* call) {
    auto* callee = dynamic_cast<const volta::ast::IdentifierExpression*>(call->callee.get());
    if (!callee) {
        reportError("Complex callee not supported yet");
        return nullptr;
    }

    std::string funcName = callee->name;

    // Check if it's a builtin function
    if (funcName == "print") {
        return generatePrintCall(call);
    }

    // Look up user-defined function
    Function* function = module_->getFunction(funcName);
    if (!function) {
        reportError("Unknown function: " + funcName);
        return nullptr;
    }

    // Generate arguments
    std::vector<Value*> args;
    for (const auto& arg : call->arguments) {
        Value* argValue = generateExpression(arg.get());
        if (!argValue) return nullptr;
        args.push_back(argValue);
    }

    // Create call
    return builder_->CreateCall(function, args);
}

Value* LLVMCodegen::generatePrintCall(const volta::ast::CallExpression* call) {
    if (call->arguments.size() != 1) {
        reportError("print() expects exactly 1 argument");
        return nullptr;
    }

    Value* arg = generateExpression(call->arguments[0].get());
    if (!arg) return nullptr;

    // Determine which print function to call based on type
    Type* argType = arg->getType();

    Function* printFunc = nullptr;
    if (argType->isIntegerTy(64)) {
        printFunc = getPrintIntFunction();
    } else if (argType->isDoubleTy()) {
        printFunc = getPrintFloatFunction();
    } else if (argType->isPointerTy()) {
        // Assume string for now
        printFunc = getPrintStringFunction();
    } else {
        reportError("Unsupported type for print()");
        return nullptr;
    }

    return builder_->CreateCall(printFunc, {arg});
}

Function* LLVMCodegen::getPrintIntFunction() {
    Function* f = module_->getFunction("volta_print_int");
    if (!f) {
        // Declare: void volta_print_int(i64)
        FunctionType* ft = FunctionType::get(
            Type::getVoidTy(*context_),
            {Type::getInt64Ty(*context_)},
            false
        );
        f = Function::Create(ft, Function::ExternalLinkage, "volta_print_int", module_.get());
    }
    return f;
}

Function* LLVMCodegen::getPrintStringFunction() {
    Function* f = module_->getFunction("volta_print_string");
    if (!f) {
        // Declare: void volta_print_string(ptr)
        FunctionType* ft = FunctionType::get(
            Type::getVoidTy(*context_),
            {PointerType::get(*context_, 0)},
            false
        );
        f = Function::Create(ft, Function::ExternalLinkage, "volta_print_string", module_.get());
    }
    return f;
}

Value* LLVMCodegen::generateStringLiteral(const volta::ast::StringLiteral* lit) {
    // Create global string constant
    Constant* strConstant = ConstantDataArray::getString(*context_, lit->value, true);

    GlobalVariable* globalStr = new GlobalVariable(
        *module_,
        strConstant->getType(),
        true,  // isConstant
        GlobalValue::PrivateLinkage,
        strConstant,
        ".str"
    );

    // Get pointer to first element (char*)
    Value* zero = ConstantInt::get(Type::getInt32Ty(*context_), 0);
    return builder_->CreateInBoundsGEP(
        strConstant->getType(),
        globalStr,
        {zero, zero}
    );
}
```

### Step 1.6: Update main.cpp

**File: `src/main.cpp`**
```cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer/lexer.hpp"
#include "parser/Parser.hpp"
#include "semantic/SemanticAnalyzer.hpp"
#include "llvm/LLVMCodegen.hpp"
#include "error/ErrorReporter.hpp"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: volta <command> <file>\n";
        std::cerr << "Commands:\n";
        std::cerr << "  compile <file.vlt>  - Compile to executable\n";
        std::cerr << "  emit-llvm <file.vlt> - Emit LLVM IR\n";
        return 1;
    }

    std::string command = argv[1];
    std::string filename;

    if (argc >= 3) {
        filename = argv[2];
    }

    // Read source file
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: Could not open file " << filename << "\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    // Frontend
    volta::errors::ErrorReporter errorReporter;

    // Lex
    volta::lexer::Lexer lexer(source, filename);
    auto tokens = lexer.tokenize();

    // Parse
    volta::parser::Parser parser(tokens, errorReporter);
    auto ast = parser.parse();
    if (!ast) {
        std::cerr << "Parsing failed\n";
        return 1;
    }

    // Semantic analysis
    volta::semantic::SemanticAnalyzer analyzer(errorReporter);
    if (!analyzer.analyze(*ast)) {
        std::cerr << "Semantic analysis failed\n";
        return 1;
    }

    // LLVM codegen
    volta::llvm_codegen::LLVMCodegen codegen("volta_program");
    if (!codegen.generate(ast.get(), &analyzer)) {
        std::cerr << "Code generation failed\n";
        return 1;
    }

    if (command == "emit-llvm") {
        // Emit LLVM IR
        std::string outputFile = filename.substr(0, filename.find_last_of('.')) + ".ll";
        if (codegen.emitLLVMIR(outputFile)) {
            std::cout << "LLVM IR written to " << outputFile << "\n";
            return 0;
        } else {
            std::cerr << "Failed to emit LLVM IR\n";
            return 1;
        }
    } else if (command == "compile") {
        // Emit LLVM IR to temp file
        std::string llFile = "/tmp/volta_temp.ll";
        if (!codegen.emitLLVMIR(llFile)) {
            std::cerr << "Failed to emit LLVM IR\n";
            return 1;
        }

        // Compile with clang
        std::string exeName = filename.substr(0, filename.find_last_of('.'));
        std::string compileCmd = "clang-18 " + llFile +
                                " bin/libvolta.a -lgc -o " + exeName;

        std::cout << "Compiling: " << compileCmd << "\n";
        int result = system(compileCmd.c_str());

        if (result == 0) {
            std::cout << "Executable created: " << exeName << "\n";
            return 0;
        } else {
            std::cerr << "Compilation failed\n";
            return 1;
        }
    }

    return 0;
}
```

### Step 1.7: Test Hello World

**Create test file: `examples/hello_llvm.vlt`**
```volta
fn main() -> int {
    print("Hello, LLVM!")
    return 0
}
```

**Build and run:**
```bash
# Build compiler
make -f Makefile.llvm

# Compile test program
./bin/volta compile examples/hello_llvm.vlt

# Run
./examples/hello_llvm
# Output: Hello, LLVM!
```

**Success criteria for Week 1:**
- ✅ Volta compiler with LLVM backend compiles
- ✅ Can generate LLVM IR for simple function
- ✅ Can compile to native executable
- ✅ Hello World runs and prints correctly

---

## Phase 2: Core Codegen (Weeks 2-3)

**Goal:** Generate LLVM IR for all basic language features.

### Week 2 Targets

#### 2.1: Arithmetic & Variables

**Target code:**
```volta
fn factorial(n: int) -> int {
    if n <= 1 {
        return 1
    }
    return n * factorial(n - 1)
}

fn main() -> int {
    result := factorial(5)
    print(result)  # Should print 120
    return 0
}
```

**Implement:**
- Binary expressions (+, -, *, /, %, <, >, <=, >=, ==, !=)
- Variable declarations with `:=`
- Return statements
- If/else statements
- Function calls (including recursion)

**Key code additions:**

```cpp
// Binary expressions
Value* LLVMCodegen::generateBinaryExpression(const ast::BinaryExpression* expr) {
    Value* lhs = generateExpression(expr->left.get());
    Value* rhs = generateExpression(expr->right.get());

    if (!lhs || !rhs) return nullptr;

    switch (expr->op) {
        case ast::BinaryExpression::Operator::Add:
            if (lhs->getType()->isIntegerTy()) {
                return builder_->CreateAdd(lhs, rhs, "addtmp");
            } else if (lhs->getType()->isDoubleTy()) {
                return builder_->CreateFAdd(lhs, rhs, "addtmp");
            }
            break;

        case ast::BinaryExpression::Operator::Multiply:
            if (lhs->getType()->isIntegerTy()) {
                return builder_->CreateMul(lhs, rhs, "multmp");
            } else {
                return builder_->CreateFMul(lhs, rhs, "multmp");
            }
            break;

        case ast::BinaryExpression::Operator::Less:
            if (lhs->getType()->isIntegerTy()) {
                return builder_->CreateICmpSLT(lhs, rhs, "cmptmp");
            } else {
                return builder_->CreateFCmpOLT(lhs, rhs, "cmptmp");
            }
            break;

        // ... other operators
    }

    reportError("Unsupported binary operator");
    return nullptr;
}

// If statement
void LLVMCodegen::generateIfStatement(const ast::IfStatement* stmt) {
    Value* cond = generateExpression(stmt->condition.get());
    if (!cond) return;

    Function* function = currentFunction_;

    BasicBlock* thenBB = BasicBlock::Create(*context_, "then", function);
    BasicBlock* elseBB = BasicBlock::Create(*context_, "else");
    BasicBlock* mergeBB = BasicBlock::Create(*context_, "merge");

    // Branch based on condition
    builder_->CreateCondBr(cond, thenBB, elseBB);

    // Then block
    builder_->SetInsertPoint(thenBB);
    generateBlock(stmt->thenBlock.get());
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateBr(mergeBB);
    }

    // Else block
    function->insert(function->end(), elseBB);
    builder_->SetInsertPoint(elseBB);
    if (stmt->elseBlock) {
        generateBlock(stmt->elseBlock.get());
    }
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateBr(mergeBB);
    }

    // Merge block
    function->insert(function->end(), mergeBB);
    builder_->SetInsertPoint(mergeBB);
}
```

#### 2.2: Loops

**Target code:**
```volta
fn sum_to_n(n: int) -> int {
    total := 0
    i := 1
    while i <= n {
        total = total + i
        i = i + 1
    }
    return total
}
```

**Implement:**
- While loops
- Variable assignment
- Mutable variables

#### 2.3: For Loops & Ranges

**Target code:**
```volta
fn sum_range(n: int) -> int {
    total := 0
    for i in 0..n {
        total = total + i
    }
    return total
}
```

**Implement:**
- For loops over ranges
- Range expressions (`0..n`, `0..=n`)

### Week 3 Targets

#### 2.4: Arrays

**Target code:**
```volta
fn array_sum() -> int {
    arr := [1, 2, 3, 4, 5]
    total := 0
    for i in 0..5 {
        total = total + arr[i]
    }
    return total
}
```

**Implement:**
- Array literals
- Array indexing
- For loops over arrays

**Runtime functions needed:**
```c
// In volta_array.c
VoltaArray* volta_array_from_values(void** values, size_t count);
void* volta_array_get(VoltaArray* arr, int64_t index);
void volta_array_set(VoltaArray* arr, int64_t index, void* value);
```

**LLVM codegen for arrays:**
```cpp
Value* LLVMCodegen::generateArrayLiteral(const ast::ArrayLiteral* lit) {
    // Generate array elements
    std::vector<Value*> elements;
    for (const auto& elem : lit->elements) {
        Value* val = generateExpression(elem.get());
        if (!val) return nullptr;

        // Box primitives to void*
        if (val->getType()->isIntegerTy() || val->getType()->isDoubleTy()) {
            val = boxValue(val);
        }

        elements.push_back(val);
    }

    // Allocate array of pointers on stack
    ArrayType* arrayType = ArrayType::get(PointerType::get(*context_, 0), elements.size());
    Value* arrayAlloca = builder_->CreateAlloca(arrayType);

    // Store elements
    for (size_t i = 0; i < elements.size(); i++) {
        Value* idx = ConstantInt::get(Type::getInt64Ty(*context_), i);
        Value* elemPtr = builder_->CreateInBoundsGEP(arrayType, arrayAlloca, {idx});
        builder_->CreateStore(elements[i], elemPtr);
    }

    // Call volta_array_from_values(void** values, size_t count)
    Function* arrayNew = getArrayFromValuesFunction();
    Value* arrayPtr = builder_->CreateBitCast(arrayAlloca, PointerType::get(*context_, 0));
    Value* count = ConstantInt::get(Type::getInt64Ty(*context_), elements.size());

    return builder_->CreateCall(arrayNew, {arrayPtr, count});
}
```

#### 2.5: Strings

**Target code:**
```volta
fn greet(name: str) -> int {
    print("Hello, " + name)
    return 0
}
```

**Implement:**
- String concatenation
- String methods (`.length()`)

---

## Phase 3: Runtime & GC (Week 4)

**Goal:** Complete runtime library with Boehm GC integration.

### 3.1: GC Integration

**Boehm GC is already working** from Phase 1, but add conveniences:

```c
// volta_runtime.c additions
void* volta_gc_alloc_atomic(size_t size) {
    // For data with no pointers (like primitive arrays)
    return GC_MALLOC_ATOMIC(size);
}

void* volta_gc_realloc(void* ptr, size_t new_size) {
    return GC_REALLOC(ptr, new_size);
}

void volta_gc_register_finalizer(void* obj, void (*finalizer)(void*, void*)) {
    GC_register_finalizer(obj, finalizer, NULL, NULL, NULL);
}

// Statistics
size_t volta_gc_get_heap_size(void) {
    return GC_get_heap_size();
}

size_t volta_gc_get_bytes_since_gc(void) {
    return GC_get_bytes_since_gc();
}
```

### 3.2: Boxing/Unboxing Primitives

For putting primitives in arrays/collections:

```c
typedef struct {
    int64_t value;
} VoltaBoxedInt;

typedef struct {
    double value;
} VoltaBoxedFloat;

void* volta_box_int(int64_t value) {
    VoltaBoxedInt* box = volta_gc_alloc(sizeof(VoltaBoxedInt));
    box->value = value;
    return box;
}

int64_t volta_unbox_int(void* box) {
    return ((VoltaBoxedInt*)box)->value;
}
```

### 3.3: Complete Array Runtime

```c
// volta_array.c - complete implementation
VoltaArray* volta_array_new(size_t capacity) {
    VoltaArray* arr = volta_gc_alloc(sizeof(VoltaArray));
    arr->length = 0;
    arr->capacity = capacity;
    arr->data = volta_gc_alloc(capacity * sizeof(void*));
    return arr;
}

VoltaArray* volta_array_from_values(void** values, size_t count) {
    VoltaArray* arr = volta_array_new(count);
    arr->length = count;
    memcpy(arr->data, values, count * sizeof(void*));
    return arr;
}

void* volta_array_get(VoltaArray* arr, int64_t index) {
    if (index < 0 || (size_t)index >= arr->length) {
        fprintf(stderr, "Array index out of bounds: %ld (length: %zu)\n",
                index, arr->length);
        exit(1);
    }
    return arr->data[index];
}

void volta_array_set(VoltaArray* arr, int64_t index, void* value) {
    if (index < 0 || (size_t)index >= arr->length) {
        fprintf(stderr, "Array index out of bounds: %ld (length: %zu)\n",
                index, arr->length);
        exit(1);
    }
    arr->data[index] = value;
}

void volta_array_push(VoltaArray* arr, void* elem) {
    if (arr->length >= arr->capacity) {
        size_t new_capacity = arr->capacity == 0 ? 8 : arr->capacity * 2;
        void** new_data = volta_gc_alloc(new_capacity * sizeof(void*));
        memcpy(new_data, arr->data, arr->length * sizeof(void*));
        arr->data = new_data;
        arr->capacity = new_capacity;
    }
    arr->data[arr->length++] = elem;
}

void* volta_array_pop(VoltaArray* arr) {
    if (arr->length == 0) {
        fprintf(stderr, "Array pop on empty array\n");
        exit(1);
    }
    return arr->data[--arr->length];
}

int64_t volta_array_length(VoltaArray* arr) {
    return arr->length;
}

// Functional operations
VoltaArray* volta_array_map(VoltaArray* arr, void* (*func)(void*)) {
    VoltaArray* result = volta_array_new(arr->length);
    result->length = arr->length;
    for (size_t i = 0; i < arr->length; i++) {
        result->data[i] = func(arr->data[i]);
    }
    return result;
}

VoltaArray* volta_array_filter(VoltaArray* arr, int (*predicate)(void*)) {
    VoltaArray* result = volta_array_new(arr->length);
    for (size_t i = 0; i < arr->length; i++) {
        if (predicate(arr->data[i])) {
            volta_array_push(result, arr->data[i]);
        }
    }
    return result;
}
```

### 3.4: Complete String Runtime

```c
// volta_string.c - complete implementation
VoltaString* volta_string_new(const char* data, size_t length) {
    VoltaString* str = volta_gc_alloc(sizeof(VoltaString));
    str->length = length;
    str->data = volta_gc_alloc(length + 1);
    memcpy(str->data, data, length);
    str->data[length] = '\0';
    return str;
}

VoltaString* volta_string_concat(VoltaString* a, VoltaString* b) {
    size_t new_length = a->length + b->length;
    VoltaString* result = volta_string_new("", new_length);
    memcpy(result->data, a->data, a->length);
    memcpy(result->data + a->length, b->data, b->length);
    result->data[new_length] = '\0';
    return result;
}

int64_t volta_string_length(VoltaString* str) {
    return str->length;
}

int8_t volta_string_equals(VoltaString* a, VoltaString* b) {
    if (a->length != b->length) return 0;
    return memcmp(a->data, b->data, a->length) == 0;
}

int8_t volta_string_contains(VoltaString* haystack, VoltaString* needle) {
    if (needle->length == 0) return 1;
    if (haystack->length < needle->length) return 0;

    for (size_t i = 0; i <= haystack->length - needle->length; i++) {
        if (memcmp(haystack->data + i, needle->data, needle->length) == 0) {
            return 1;
        }
    }
    return 0;
}

VoltaString* volta_string_substring(VoltaString* str, int64_t start, int64_t end) {
    if (start < 0 || end > (int64_t)str->length || start > end) {
        fprintf(stderr, "Invalid substring indices\n");
        exit(1);
    }
    return volta_string_new(str->data + start, end - start);
}

// Type conversion
VoltaString* volta_int_to_string(int64_t value) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%ld", value);
    return volta_string_new(buffer, strlen(buffer));
}

VoltaString* volta_float_to_string(double value) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%f", value);
    return volta_string_new(buffer, strlen(buffer));
}

VoltaString* volta_bool_to_string(int8_t value) {
    return volta_string_new(value ? "true" : "false", value ? 4 : 5);
}
```

---

## Phase 4: Advanced Features (Weeks 5-8)

### Week 5: Structs

**Target code:**
```volta
struct Point {
    x: float,
    y: float
}

fn Point.distance(self, other: Point) -> float {
    dx := self.x - other.x
    dy := self.y - other.y
    return sqrt(dx * dx + dy * dy)
}

fn main() -> int {
    p1 := Point { x: 0.0, y: 0.0 }
    p2 := Point { x: 3.0, y: 4.0 }
    dist := p1.distance(p2)
    print(dist)  # 5.0
    return 0
}
```

**Runtime structure:**
```c
// Structs are GC-allocated with fixed layout
// Each struct type gets a layout descriptor

typedef struct {
    size_t num_fields;
    size_t* field_offsets;
    size_t total_size;
} VoltaStructLayout;

void* volta_struct_alloc(VoltaStructLayout* layout) {
    return volta_gc_alloc(layout->total_size);
}

void volta_struct_set_field(void* struct_ptr, size_t field_index,
                            VoltaStructLayout* layout, void* value) {
    char* base = (char*)struct_ptr;
    void** field_ptr = (void**)(base + layout->field_offsets[field_index]);
    *field_ptr = value;
}

void* volta_struct_get_field(void* struct_ptr, size_t field_index,
                             VoltaStructLayout* layout) {
    char* base = (char*)struct_ptr;
    return *(void**)(base + layout->field_offsets[field_index]);
}
```

**LLVM codegen for structs:**
```cpp
Value* LLVMCodegen::generateStructLiteral(const ast::StructLiteral* lit) {
    // Get struct type from semantic analyzer
    auto structType = analyzer_->getExpressionType((ast::Expression*)lit);
    auto* semStructType = dynamic_cast<volta::semantic::StructType*>(structType.get());

    // Create layout descriptor (global constant)
    auto layout = createStructLayout(semStructType);

    // Allocate struct
    Function* allocFunc = getStructAllocFunction();
    Value* structPtr = builder_->CreateCall(allocFunc, {layout});

    // Set fields
    for (size_t i = 0; i < lit->fields.size(); i++) {
        Value* fieldValue = generateExpression(lit->fields[i]->value.get());
        setStructField(structPtr, i, layout, fieldValue);
    }

    return structPtr;
}
```

### Week 6: Enums & Pattern Matching (Basic)

**Target code:**
```volta
enum Option[T] {
    Some(T),
    None
}

fn unwrap_or(opt: Option[int], default: int) -> int {
    match opt {
        Option.Some(value) => return value,
        Option.None => return default
    }
}
```

**Runtime structure:**
```c
// Enums are tagged unions
typedef struct {
    uint32_t tag;        // Variant index
    void* data[1];       // Flexible array for variant data
} VoltaEnum;

VoltaEnum* volta_enum_new(uint32_t tag, size_t data_size) {
    size_t total_size = sizeof(VoltaEnum) + data_size;
    VoltaEnum* e = volta_gc_alloc(total_size);
    e->tag = tag;
    return e;
}

uint32_t volta_enum_get_tag(VoltaEnum* e) {
    return e->tag;
}

void* volta_enum_get_data(VoltaEnum* e) {
    return e->data;
}
```

**LLVM codegen for pattern matching:**
```cpp
void LLVMCodegen::generateMatchExpression(const ast::MatchExpression* match) {
    Value* matchValue = generateExpression(match->value.get());

    // For enum, get tag
    Function* getTag = getEnumGetTagFunction();
    Value* tag = builder_->CreateCall(getTag, {matchValue});

    // Create switch
    Function* function = currentFunction_;
    BasicBlock* mergeBB = BasicBlock::Create(*context_, "match_merge");

    // Default case (should not happen if exhaustive)
    BasicBlock* defaultBB = BasicBlock::Create(*context_, "match_default", function);

    SwitchInst* switchInst = builder_->CreateSwitch(tag, defaultBB, match->arms.size());

    // Generate each arm
    for (size_t i = 0; i < match->arms.size(); i++) {
        const auto& arm = match->arms[i];

        BasicBlock* armBB = BasicBlock::Create(*context_, "match_arm", function);

        // Get variant index from pattern
        uint32_t variantIndex = getVariantIndex(arm->pattern.get());
        switchInst->addCase(ConstantInt::get(Type::getInt32Ty(*context_), variantIndex), armBB);

        builder_->SetInsertPoint(armBB);

        // Extract data if needed
        if (auto* ctorPattern = dynamic_cast<ast::ConstructorPattern*>(arm->pattern.get())) {
            // Extract enum data and bind to variables
            Value* data = extractEnumData(matchValue, variantIndex);
            // Bind pattern variables...
        }

        // Generate arm body
        Value* armResult = generateExpression(arm->body.get());

        if (!builder_->GetInsertBlock()->getTerminator()) {
            builder_->CreateBr(mergeBB);
        }
    }

    // Default case
    builder_->SetInsertPoint(defaultBB);
    reportError("Non-exhaustive pattern match");
    builder_->CreateUnreachable();

    // Merge
    function->insert(function->end(), mergeBB);
    builder_->SetInsertPoint(mergeBB);
}
```

### Week 7-8: Tuples, Lambdas, Advanced Features

**Tuples:**
```volta
fn swap(pair: (int, int)) -> (int, int) {
    (a, b) := pair
    return (b, a)
}
```

**Lambdas:**
```volta
numbers := [1, 2, 3, 4, 5]
squared := numbers.map(fn(x) = x * x)
```

---

## Phase 5: Standard Library (Weeks 9-10)

### 5.1: Core Module

**File: `stdlib/core.vlt`** (Volta code)
```volta
# Type conversions
fn str(value: int) -> str {
    # Calls runtime function
    return __volta_int_to_string(value)
}

fn str(value: float) -> str {
    return __volta_float_to_string(value)
}

fn str(value: bool) -> str {
    return __volta_bool_to_string(value)
}

# Assertions
fn assert(condition: bool, message: str) {
    if !condition {
        print("Assertion failed: " + message)
        __volta_exit(1)
    }
}

# Range types
struct Range {
    start: int,
    end: int,
    inclusive: bool
}

fn Range.contains(self, value: int) -> bool {
    if self.inclusive {
        return value >= self.start and value <= self.end
    } else {
        return value >= self.start and value < self.end
    }
}
```

### 5.2: Math Module

**File: `stdlib/math.vlt`**
```volta
# Wrappers around C math library
extern "C" fn sqrt(x: float) -> float
extern "C" fn sin(x: float) -> float
extern "C" fn cos(x: float) -> float
extern "C" fn tan(x: float) -> float
extern "C" fn log(x: float) -> float
extern "C" fn exp(x: float) -> float
extern "C" fn pow(x: float, y: float) -> float
extern "C" fn floor(x: float) -> float
extern "C" fn ceil(x: float) -> float
extern "C" fn abs(x: int) -> int
extern "C" fn fabs(x: float) -> float

# Constants
PI := 3.14159265358979323846
E := 2.71828182845904523536
```

### 5.3: Collections Module

**File: `stdlib/collections.vlt`**
```volta
# Array extensions (already have basic ops)
fn Array.first[T](self: Array[T]) -> Option[T] {
    if self.len() == 0 {
        return Option.None
    }
    return Option.Some(self[0])
}

fn Array.last[T](self: Array[T]) -> Option[T] {
    if self.len() == 0 {
        return Option.None
    }
    return Option.Some(self[self.len() - 1])
}

fn Array.reverse[T](self: Array[T]) -> Array[T] {
    result := []
    for i in (self.len() - 1)..=0 {
        result.push(self[i])
    }
    return result
}
```

### 5.4: IO Module

**File: `stdlib/io.vlt`**
```volta
# File operations via C FFI
extern "C" fn fopen(path: *i8, mode: *i8) -> *FILE
extern "C" fn fclose(file: *FILE) -> i32
extern "C" fn fread(ptr: *mut i8, size: u64, count: u64, file: *FILE) -> u64
extern "C" fn fwrite(ptr: *i8, size: u64, count: u64, file: *FILE) -> u64

struct File {
    handle: *FILE
}

fn File.open(path: str, mode: str) -> Option[File] {
    handle := fopen(path.as_c_str(), mode.as_c_str())
    if handle == null {
        return Option.None
    }
    return Option.Some(File { handle: handle })
}

fn File.close(self) {
    fclose(self.handle)
}

fn File.read_all(self) -> str {
    # Implementation using fread
    ...
}

fn File.write(self, content: str) -> Result[(), str] {
    # Implementation using fwrite
    ...
}
```

---

## Phase 6: C Interop & FFI (Week 11)

### 6.1: FFI Syntax

**Add to parser:**
```cpp
// Parser.cpp
std::unique_ptr<ast::FnDeclaration> Parser::parseFunctionDeclaration() {
    // Check for extern
    bool isExtern = false;
    std::string linkage;

    if (match(TokenType::EXTERN)) {
        isExtern = true;

        // Expect string literal for linkage
        if (match(TokenType::STRING_LITERAL)) {
            linkage = previous().getLiteral();  // "C" or "C++"
        }
    }

    expect(TokenType::FN, "Expected 'fn'");
    // ... rest of parsing

    auto fn = std::make_unique<ast::FnDeclaration>(...);
    fn->isExtern = isExtern;
    fn->linkage = linkage;
    return fn;
}
```

### 6.2: C-Compatible Types

**Add to lexer (keywords):**
```cpp
keywords_["i8"] = TokenType::I8;
keywords_["i16"] = TokenType::I16;
keywords_["i32"] = TokenType::I32;
keywords_["i64"] = TokenType::I64;
keywords_["u8"] = TokenType::U8;
keywords_["u16"] = TokenType::U16;
keywords_["u32"] = TokenType::U32;
keywords_["u64"] = TokenType::U64;
```

**Add to type system:**
```cpp
// semantic/Type.hpp
class CIntType : public Type {
public:
    enum class CKind {
        I8, I16, I32, I64,
        U8, U16, U32, U64
    };

    CIntType(CKind kind) : kind_(kind) {}

    Kind kind() const override { return Kind::CInt; }
    std::string toString() const override {
        switch (kind_) {
            case CKind::I8: return "i8";
            case CKind::I16: return "i16";
            // ...
        }
    }

    CKind getCKind() const { return kind_; }

private:
    CKind kind_;
};

class RawPointerType : public Type {
public:
    RawPointerType(std::shared_ptr<Type> pointee, bool isMutable)
        : pointee_(pointee), isMutable_(isMutable) {}

    Kind kind() const override { return Kind::RawPointer; }
    std::string toString() const override {
        return (isMutable_ ? "*mut " : "*") + pointee_->toString();
    }

private:
    std::shared_ptr<Type> pointee_;
    bool isMutable_;
};
```

### 6.3: LLVM Codegen for FFI

```cpp
void LLVMCodegen::generateExternFunction(const ast::FnDeclaration* fn) {
    // Lower types
    Type* returnType = lowerType(analyzer_->resolveTypeAnnotation(fn->returnType.get()));

    std::vector<Type*> paramTypes;
    for (const auto& param : fn->parameters) {
        paramTypes.push_back(lowerType(analyzer_->resolveTypeAnnotation(param->type.get())));
    }

    // Create function type
    FunctionType* funcType = FunctionType::get(returnType, paramTypes, false);

    // Create external function declaration
    Function* function = Function::Create(
        funcType,
        Function::ExternalLinkage,
        fn->identifier,
        module_.get()
    );

    // Set calling convention if needed
    if (fn->linkage == "C") {
        function->setCallingConv(CallingConv::C);
    }

    // No body - it's extern!
    functions_[fn->identifier] = function;
}
```

### 6.4: Scientific Computing Example

**Using BLAS/LAPACK:**
```volta
# matrix_ops.vlt
extern "C" fn cblas_dgemm(
    layout: i32,
    transA: i32, transB: i32,
    M: i32, N: i32, K: i32,
    alpha: float,
    A: *float, lda: i32,
    B: *float, ldb: i32,
    beta: float,
    C: *mut float, ldc: i32
)

struct Matrix {
    rows: int,
    cols: int,
    data: *mut float
}

fn Matrix.new(rows: int, cols: int) -> Matrix {
    size := rows * cols
    data := __volta_alloc_floats(size)
    return Matrix { rows: rows, cols: cols, data: data }
}

fn Matrix.multiply(self, other: Matrix) -> Matrix {
    assert(self.cols == other.rows, "Dimension mismatch")

    result := Matrix.new(self.rows, other.cols)

    # Call BLAS
    cblas_dgemm(
        101,  # Row-major
        111, 111,  # No transpose
        self.rows, other.cols, self.cols,
        1.0,
        self.data, self.cols,
        other.data, other.cols,
        0.0,
        result.data, other.cols
    )

    return result
}
```

---

## Phase 7: Error Reporting (Week 12)

### 7.1: Source Location Tracking

**Already have this in AST!** Just need to wire it through LLVM:

```cpp
void LLVMCodegen::attachDebugInfo(Value* value, volta::errors::SourceLocation loc) {
    if (!value || !isa<Instruction>(value)) return;

    // Create debug location
    DILocation* debugLoc = DILocation::get(
        *context_,
        loc.line,
        loc.column,
        getCurrentScope()
    );

    cast<Instruction>(value)->setDebugLoc(debugLoc);
}
```

### 7.2: DWARF Debug Info

**Enable full debugging:**
```cpp
class LLVMCodegen {
private:
    std::unique_ptr<DIBuilder> debugBuilder_;
    DICompileUnit* compileUnit_;
    DIFile* currentFile_;
    std::vector<DIScope*> scopeStack_;

    void initializeDebugInfo(const std::string& filename) {
        debugBuilder_ = std::make_unique<DIBuilder>(*module_);

        // Create compile unit
        currentFile_ = debugBuilder_->createFile(filename, ".");
        compileUnit_ = debugBuilder_->createCompileUnit(
            dwarf::DW_LANG_C,  // Use C as language
            currentFile_,
            "Volta Compiler",
            false,  // isOptimized
            "",     // flags
            0       // runtime version
        );

        scopeStack_.push_back(compileUnit_);
    }

    DISubprogram* createFunctionDebugInfo(
        const ast::FnDeclaration* fn,
        Function* llvmFunc
    ) {
        DIFile* file = currentFile_;
        unsigned lineNo = fn->location.line;

        // Create function type
        SmallVector<Metadata*, 8> paramTypes;
        paramTypes.push_back(nullptr);  // Return type (TODO: fill in)
        for (const auto& param : fn->parameters) {
            paramTypes.push_back(nullptr);  // Param types (TODO: fill in)
        }

        DISubroutineType* fnType = debugBuilder_->createSubroutineType(
            debugBuilder_->getOrCreateTypeArray(paramTypes)
        );

        // Create subprogram
        DISubprogram* sp = debugBuilder_->createFunction(
            file,                  // scope
            fn->identifier,        // name
            llvmFunc->getName(),   // linkage name
            file,                  // file
            lineNo,                // line number
            fnType,                // type
            lineNo,                // scope line
            DINode::FlagPrototyped,// flags
            DISubprogram::SPFlagDefinition
        );

        llvmFunc->setSubprogram(sp);
        scopeStack_.push_back(sp);

        return sp;
    }

    void finalizeDebugInfo() {
        debugBuilder_->finalize();
    }
};
```

**Usage:**
```cpp
void LLVMCodegen::generateFunction(const ast::FnDeclaration* fn) {
    // ... create function ...

    // Add debug info
    DISubprogram* sp = createFunctionDebugInfo(fn, function);

    // Create entry block with debug loc
    BasicBlock* entry = BasicBlock::Create(*context_, "entry", function);
    builder_->SetInsertPoint(entry);

    // Set debug location for first instruction
    builder_->SetCurrentDebugLocation(
        DILocation::get(*context_, fn->location.line, 0, sp)
    );

    // ... generate body ...

    scopeStack_.pop_back();
}
```

### 7.3: Rust-Style Error Handling (NO EXCEPTIONS!)

**IMPORTANT:** Volta uses Result types, NOT exceptions. Runtime errors return Result values that must be checked.

**Error types in stdlib:**
```volta
# Built-in Result type
enum Result[T, E] {
    Ok(T),
    Err(E)
}

# Standard error type
struct Error {
    message: str,
    file: str,
    line: int
}

# Array operations return Result
fn Array.get[T](self: Array[T], index: int) -> Result[T, Error] {
    if index < 0 or index >= self.len() {
        return Result.Err(Error {
            message: "Array index out of bounds",
            file: __FILE__,
            line: __LINE__
        })
    }
    return Result.Ok(__volta_array_get_unchecked(self, index))
}

# File operations return Result
fn File.open(path: str) -> Result[File, Error] {
    handle := __volta_fopen(path.as_c_str(), "r")
    if handle == null {
        return Result.Err(Error {
            message: "Failed to open file: " + path,
            file: __FILE__,
            line: __LINE__
        })
    }
    return Result.Ok(File { handle: handle })
}
```

**Panic for unrecoverable errors:**
```c
// volta_runtime.c
#include <execinfo.h>  // For backtrace() on Unix

void volta_print_stack_trace(void) {
    void* buffer[100];
    int size = backtrace(buffer, 100);
    char** strings = backtrace_symbols(buffer, size);

    fprintf(stderr, "\nStack trace:\n");
    for (int i = 0; i < size; i++) {
        fprintf(stderr, "  %s\n", strings[i]);
    }

    free(strings);
}

void volta_panic(const char* message, const char* file, int line) {
    fprintf(stderr, "\nPANIC at %s:%d\n", file, line);
    fprintf(stderr, "  %s\n", message);
    volta_print_stack_trace();
    exit(1);
}

// Unwrap helper - panics on None
void* volta_option_unwrap(VoltaEnum* option, const char* file, int line) {
    if (volta_enum_get_tag(option) == 1) {  // None variant
        volta_panic("Unwrap called on None value", file, line);
    }
    return volta_enum_get_data(option);  // Some(T)
}

// Assert helper
void volta_assert(int8_t condition, const char* message, const char* file, int line) {
    if (!condition) {
        fprintf(stderr, "\nAssertion failed at %s:%d\n", file, line);
        fprintf(stderr, "  %s\n", message);
        volta_print_stack_trace();
        exit(1);
    }
}
```

**Usage in Volta:**
```volta
fn read_config(path: str) -> Result[Config, Error] {
    # Open file - returns Result
    file := File.open(path)?  # ? propagates errors up

    # Read content - returns Result
    content := file.read_all()?

    # Parse - returns Result
    config := Config.parse(content)?

    return Result.Ok(config)
}

fn main() -> int {
    match read_config("config.toml") {
        Result.Ok(config) => {
            print("Config loaded successfully")
            use_config(config)
        },
        Result.Err(error) => {
            print("Error: " + error.message)
            return 1
        }
    }

    return 0
}

# Or use unwrap (panics on error)
fn main_panic_style() -> int {
    config := read_config("config.toml").unwrap()  # Panics on Err
    use_config(config)
    return 0
}
```

**Error propagation operator (`?`):**
```volta
# Before (verbose):
fn foo() -> Result[int, Error] {
    x := match get_value() {
        Result.Ok(val) => val,
        Result.Err(e) => return Result.Err(e)
    }
    return Result.Ok(x * 2)
}

# After (with ?):
fn foo() -> Result[int, Error] {
    x := get_value()?  # Auto-unwraps Ok, returns Err early
    return Result.Ok(x * 2)
}
```

**LLVM Codegen for Result checking:**
```cpp
// When generating code that might error
Value* LLVMCodegen::generateArrayGet(Value* array, Value* index) {
    // Call runtime function that returns Result
    Function* getFunc = getArrayGetFunction();
    Value* result = builder_->CreateCall(getFunc, {array, index});

    // Extract tag (0 = Ok, 1 = Err)
    Value* tag = extractResultTag(result);

    // Check if error
    Value* isErr = builder_->CreateICmpEQ(
        tag,
        ConstantInt::get(Type::getInt32Ty(*context_), 1)
    );

    BasicBlock* errBB = BasicBlock::Create(*context_, "error", currentFunction_);
    BasicBlock* okBB = BasicBlock::Create(*context_, "ok", currentFunction_);

    builder_->CreateCondBr(isErr, errBB, okBB);

    // Error path
    builder_->SetInsertPoint(errBB);
    // If function returns Result, propagate error
    if (currentFunctionReturnsResult()) {
        builder_->CreateRet(result);  // Return the error
    } else {
        // Panic if not in Result-returning function
        Function* panic = getPanicFunction();
        Value* errMsg = extractErrorMessage(result);
        builder_->CreateCall(panic, {errMsg});
        builder_->CreateUnreachable();
    }

    // Ok path
    builder_->SetInsertPoint(okBB);
    return extractResultValue(result);
}
```

**NO EXCEPTIONS - ONLY RESULTS!** This is:
- ✅ Explicit (can see which functions can fail)
- ✅ Type-safe (compiler enforces error handling)
- ✅ Fast (no exception unwinding overhead)
- ✅ Rust-style (modern, loved by developers)

### 7.4: Better Error Messages

**Compile-time errors** (already good - from semantic analyzer)

**Runtime errors:**
```c
void volta_array_bounds_check(VoltaArray* arr, int64_t index,
                              const char* file, int line) {
    if (index < 0 || (size_t)index >= arr->length) {
        fprintf(stderr, "Array index out of bounds at %s:%d\n", file, line);
        fprintf(stderr, "  Index: %ld, Array length: %zu\n", index, arr->length);
        volta_print_stack_trace();
        exit(1);
    }
}
```

**Generated code:**
```cpp
// In LLVM codegen for array index
Value* index = generateExpression(indexExpr->index.get());
Value* array = generateExpression(indexExpr->object.get());

// Insert bounds check call
Function* boundsCheck = getBoundsCheckFunction();
builder_->CreateCall(boundsCheck, {
    array,
    index,
    builder_->CreateGlobalStringPtr(filename),
    ConstantInt::get(Type::getInt32Ty(*context_), loc.line)
});

// Then do actual array access
...
```

---

## Phase 8: Optimization & Polish (Weeks 13-16)

### 8.1: Optimization Flags

**Update main.cpp:**
```cpp
enum class OptLevel {
    O0,  // No optimization
    O1,  // Basic
    O2,  // Aggressive
    O3,  // Max + vectorization
};

void optimizeModule(Module* module, OptLevel level) {
    // Create pass manager
    legacy::PassManager pm;

    // Add target info
    TargetMachine* targetMachine = getTargetMachine();
    pm.add(createTargetTransformInfoWrapperPass(
        targetMachine->getTargetIRAnalysis()
    ));

    switch (level) {
        case OptLevel::O0:
            // No optimization passes
            break;

        case OptLevel::O1:
            pm.add(createPromoteMemoryToRegisterPass());  // mem2reg
            pm.add(createInstructionCombiningPass());
            pm.add(createCFGSimplificationPass());
            break;

        case OptLevel::O2:
            // Standard optimization pipeline
            PassManagerBuilder builder;
            builder.OptLevel = 2;
            builder.SizeLevel = 0;
            builder.Inliner = createFunctionInliningPass(2, 0, false);
            builder.populateModulePassManager(pm);
            break;

        case OptLevel::O3:
            PassManagerBuilder builder;
            builder.OptLevel = 3;
            builder.SizeLevel = 0;
            builder.Inliner = createFunctionInliningPass(3, 0, false);
            builder.LoopVectorize = true;
            builder.SLPVectorize = true;
            builder.populateModulePassManager(pm);
            break;
    }

    // Run optimization passes
    pm.run(*module);
}
```

### 8.2: Link-Time Optimization (LTO)

```bash
# Compile multiple modules with LTO
volta compile --emit-llvm module1.vlt -o module1.bc
volta compile --emit-llvm module2.vlt -o module2.bc

# Link with LTO
llvm-link-18 module1.bc module2.bc -o combined.bc
opt-18 -O3 combined.bc -o optimized.bc
llc-18 optimized.bc -o optimized.s
clang optimized.s -o program
```

### 8.3: Profile-Guided Optimization (PGO)

```bash
# Compile with instrumentation
volta compile --pgo-instrument program.vlt -o program_instr

# Run to collect profile
./program_instr
# Generates: default.profraw

# Merge profiles
llvm-profdata merge -output=default.profdata default.profraw

# Compile with profile
volta compile --pgo-use=default.profdata program.vlt -o program_opt
```

### 8.4: Benchmarking

**Create benchmarks:**
```volta
# benchmarks/fibonacci.vlt
fn fib(n: int) -> int {
    if n <= 1 {
        return n
    }
    return fib(n - 1) + fib(n - 2)
}

fn main() -> int {
    start := get_time_ms()
    result := fib(40)
    end := get_time_ms()

    print("fib(40) = " + str(result))
    print("Time: " + str(end - start) + " ms")
    return 0
}
```

**Performance targets:**
```
Benchmark         | Volta LLVM | Python 3.11 | Speedup
----------------------------------------------------------
fibonacci(40)     | 800ms      | 35000ms     | 43x
array sum (1M)    | 15ms       | 150ms       | 10x
matrix mult(1000) | 2000ms     | 45000ms     | 22x
string ops        | 50ms       | 500ms       | 10x
```

---

## Testing Strategy

### Unit Tests (LLVM Codegen)

**File: `tests/llvm/test_codegen.cpp`**
```cpp
#include <gtest/gtest.h>
#include "llvm/LLVMCodegen.hpp"

TEST(LLVMCodegen, SimpleFunction) {
    // Parse: fn add(x: int, y: int) -> int { return x + y }
    auto ast = parseString("fn add(x: int, y: int) -> int { return x + y }");

    SemanticAnalyzer analyzer(errorReporter);
    ASSERT_TRUE(analyzer.analyze(*ast));

    LLVMCodegen codegen("test");
    ASSERT_TRUE(codegen.generate(ast.get(), &analyzer));

    // Verify function exists
    Function* f = codegen.getModule()->getFunction("add");
    ASSERT_NE(f, nullptr);
    ASSERT_EQ(f->arg_size(), 2);
}

TEST(LLVMCodegen, ArrayLiteral) {
    auto ast = parseString("fn test() -> int { arr := [1, 2, 3]; return 0 }");
    // ... test array codegen
}
```

### Integration Tests (End-to-End)

**File: `tests/integration/test_llvm_e2e.cpp`**
```cpp
TEST(Integration, HelloWorld) {
    std::string code = R"(
        fn main() -> int {
            print("Hello, World!")
            return 0
        }
    )";

    // Compile to executable
    std::string exe = compileToExecutable(code);

    // Run and capture output
    std::string output = runProgram(exe);

    EXPECT_EQ(output, "Hello, World!\n");
}

TEST(Integration, Fibonacci) {
    std::string code = loadFile("examples/fibonacci.vlt");
    std::string exe = compileToExecutable(code);
    std::string output = runProgram(exe);

    // Verify correct fibonacci output
    EXPECT_THAT(output, HasSubstr("55"));  // fib(10) = 55
}
```

### Runtime Tests

**File: `tests/runtime/test_gc.c`**
```c
#include "runtime/volta_runtime.h"
#include <assert.h>

void test_gc_allocation() {
    void* p1 = volta_gc_alloc(1024);
    assert(p1 != NULL);

    void* p2 = volta_gc_alloc(2048);
    assert(p2 != NULL);

    // Trigger collection
    volta_gc_collect();

    // Should still be valid
    assert(p1 != NULL);
    assert(p2 != NULL);
}

void test_array_operations() {
    VoltaArray* arr = volta_array_new(10);
    assert(volta_array_length(arr) == 0);

    volta_array_push(arr, (void*)42);
    assert(volta_array_length(arr) == 1);

    void* elem = volta_array_get(arr, 0);
    assert(elem == (void*)42);
}
```

---

## Performance Targets

### Compile Time

```
Source Lines  | Compile Time (O0) | Compile Time (O2)
---------------------------------------------------------
100           | 0.1s              | 0.2s
1000          | 0.5s              | 1.5s
10000         | 3s                | 15s
```

### Runtime Performance

**Relative to C (1.0 = same speed):**
```
Feature          | Target | Notes
------------------------------------------------
Integer math     | 0.95   | Nearly same as C
Floating point   | 0.95   | Nearly same as C
Function calls   | 0.90   | Small overhead
Array access     | 0.85   | Bounds checking
String ops       | 0.80   | GC overhead
Struct access    | 0.90   | Pointer indirection
```

**Relative to Python 3.11 (speedup):**
```
Workload         | Target Speedup
----------------------------------
CPU-bound        | 20-50x
Memory-bound     | 5-10x
I/O-bound        | 2-5x
```

---

## Timeline Summary

### Weeks 1-4: Foundation (Critical Path)
- ✅ Week 1: Hello World (LLVM setup, basic codegen)
- ✅ Week 2: Arithmetic & control flow
- ✅ Week 3: Loops & arrays
- ✅ Week 4: Complete runtime & GC

**Milestone:** Can compile and run factorial, fibonacci, array programs

### Weeks 5-8: Language Features
- ✅ Week 5: Structs
- ✅ Week 6: Enums & pattern matching
- ✅ Week 7-8: Tuples, lambdas, polish

**Milestone:** Full language feature parity with 1.0 spec

### Weeks 9-12: Production Ready
- ✅ Week 9-10: Standard library
- ✅ Week 11: C interop & FFI
- ✅ Week 12: Error reporting & debugging

**Milestone:** Production-ready compiler

### Weeks 13-16: Optimization
- ✅ Week 13-14: Optimization passes & flags
- ✅ Week 15: LTO, PGO, benchmarks
- ✅ Week 16: Polish, docs, release prep

**Milestone:** 1.0 Release

---

## Risk Mitigation

### Risk: LLVM API Too Complex

**Mitigation:**
- Start with simple examples
- Reference LLVM tutorial: https://llvm.org/docs/tutorial/
- Use Kaleidoscope tutorial as guide
- If stuck after 1 week, abort and keep VM

**Abort Criteria:**
- Can't get Hello World in 20 hours
- Constant API frustration
- No progress for 3+ days

### Risk: Performance Worse Than Expected

**Mitigation:**
- Profile with perf/Instruments
- Use LLVM optimization viewer
- Compare against Clang-compiled C
- Tune GC if needed

**Abort Criteria:**
- < 5x speedup vs Python (should be 20-50x)
- Slower than VM (shouldn't happen)

### Risk: GC Issues

**Mitigation:**
- Use Boehm GC (conservative, battle-tested)
- Don't write custom GC for 1.0
- Add precise GC later if needed

**Abort Criteria:**
- Memory leaks in simple programs
- Crashes with GC enabled
- Boehm GC incompatible with LLVM

### Risk: Time Overrun

**Mitigation:**
- Time-box each phase
- Cut features if behind schedule
- MVP first, polish later

**Abort Criteria:**
- Not at Week 4 milestone by Week 6
- No working compiler by Week 12

---

## Appendix: Quick Reference

### LLVM IR Types
```
Volta Type     | LLVM Type
--------------------------------
int            | i64
float          | double
bool           | i1
str            | ptr (to VoltaString)
Array[T]       | ptr (to VoltaArray)
Struct         | ptr (to GC object)
Enum           | ptr (to VoltaEnum)
void           | void
```

### Common LLVM Operations
```cpp
// Create integer constant
Value* five = ConstantInt::get(Type::getInt64Ty(context), 5);

// Create float constant
Value* pi = ConstantFP::get(Type::getDoubleTy(context), 3.14159);

// Add two values
Value* sum = builder.CreateAdd(a, b, "sum");

// Call function
Value* result = builder.CreateCall(func, {arg1, arg2});

// Branch
builder.CreateCondBr(condition, thenBB, elseBB);

// Return
builder.CreateRet(value);
```

### Boehm GC Functions
```c
GC_INIT();                    // Initialize GC
GC_MALLOC(size);              // Allocate with GC
GC_MALLOC_ATOMIC(size);       // Allocate no pointers
GC_REALLOC(ptr, new_size);    // Reallocate
GC_gcollect();                // Force collection
GC_get_heap_size();           // Get heap size
```

### Build Commands
```bash
# Compile Volta compiler
make -f Makefile.llvm

# Compile Volta program
./bin/volta compile program.vlt

# Emit LLVM IR (for inspection)
./bin/volta emit-llvm program.vlt

# Compile with optimization
./bin/volta compile -O2 program.vlt

# Run executable
./program
```

---

## Next Steps

**Ready to start? Here's what to do RIGHT NOW:**

1. **Archive the VM** (5 minutes)
   ```bash
   git checkout -b archive/bytecode-vm
   git add -A
   git commit -m "Archive bytecode VM before LLVM migration"
   git push origin archive/bytecode-vm
   git checkout main
   ```

2. **Install LLVM 18** (10 minutes)
   ```bash
   # Ubuntu
   wget https://apt.llvm.org/llvm.sh
   chmod +x llvm.sh
   sudo ./llvm.sh 18
   sudo apt install llvm-18-dev clang-18 libgc-dev
   ```

3. **Create directory structure** (2 minutes)
   ```bash
   mkdir -p src/llvm src/runtime include/llvm include/runtime tests/llvm
   ```

4. **Start Phase 1** (Week 1)
   - Copy the skeleton code from this document
   - Build Hello World
   - Test compilation pipeline

**YOU GOT THIS! 🚀**

This is going to be an amazing journey. LLVM-based Volta will be faster, simpler, and more professional than the VM approach. And you'll learn LLVM, which is an incredibly valuable skill.

Good luck! Let me know when you hit your first milestone!
