# Quick Start: Implementing Volta Bytecode & VM

This guide will walk you through implementing your first component and getting tests passing.

## Prerequisites

- C++17 compiler (g++ or clang++)
- Google Test installed
- Basic understanding of compilers and VMs

## Step 1: Start with Bytecode Chunks (Easiest Component)

The `Chunk` class is the simplest component and has no dependencies. Let's start here!

### Create the implementation file:

```bash
mkdir -p src/bytecode
touch src/bytecode/Bytecode.cpp
```

### Open `src/bytecode/Bytecode.cpp` and implement:

```cpp
#include "bytecode/Bytecode.hpp"
#include <cstring>
#include <sstream>
#include <iomanip>

namespace volta::bytecode {

// ============================================================================
// Chunk Implementation
// ============================================================================

void Chunk::emitOpcode(Opcode opcode) {
    code_.push_back(static_cast<uint8_t>(opcode));
}

void Chunk::emitInt32(int32_t value) {
    // Little-endian encoding
    code_.push_back((value >> 0) & 0xFF);
    code_.push_back((value >> 8) & 0xFF);
    code_.push_back((value >> 16) & 0xFF);
    code_.push_back((value >> 24) & 0xFF);
}

void Chunk::emitInt64(int64_t value) {
    // Little-endian encoding
    for (int i = 0; i < 8; i++) {
        code_.push_back((value >> (i * 8)) & 0xFF);
    }
}

void Chunk::emitFloat64(double value) {
    // Reinterpret as bytes
    uint64_t bits;
    std::memcpy(&bits, &value, sizeof(double));
    emitInt64(static_cast<int64_t>(bits));
}

void Chunk::emitBool(bool value) {
    code_.push_back(value ? 1 : 0);
}

void Chunk::patchInt32(size_t offset, int32_t value) {
    code_[offset + 0] = (value >> 0) & 0xFF;
    code_[offset + 1] = (value >> 8) & 0xFF;
    code_[offset + 2] = (value >> 16) & 0xFF;
    code_[offset + 3] = (value >> 24) & 0xFF;
}

void Chunk::addLineNumber(size_t offset, uint32_t lineNumber) {
    lineNumbers_.push_back({offset, lineNumber});
}

uint32_t Chunk::getLineNumber(size_t offset) const {
    // Find the line number for this offset
    // Linear search is fine for now (can optimize later)
    uint32_t currentLine = 0;
    for (const auto& [lineOffset, line] : lineNumbers_) {
        if (lineOffset > offset) break;
        currentLine = line;
    }
    return currentLine;
}

// ============================================================================
// Helper Functions
// ============================================================================

std::string getOpcodeName(Opcode opcode) {
    switch (opcode) {
        case Opcode::Pop: return "Pop";
        case Opcode::Dup: return "Dup";
        case Opcode::Swap: return "Swap";

        case Opcode::ConstInt: return "ConstInt";
        case Opcode::ConstFloat: return "ConstFloat";
        case Opcode::ConstBool: return "ConstBool";
        case Opcode::ConstString: return "ConstString";
        case Opcode::ConstNull: return "ConstNull";

        case Opcode::AddInt: return "AddInt";
        case Opcode::AddFloat: return "AddFloat";
        case Opcode::SubInt: return "SubInt";
        case Opcode::SubFloat: return "SubFloat";
        case Opcode::MulInt: return "MulInt";
        case Opcode::MulFloat: return "MulFloat";
        case Opcode::DivInt: return "DivInt";
        case Opcode::DivFloat: return "DivFloat";
        case Opcode::ModInt: return "ModInt";

        case Opcode::NegInt: return "NegInt";
        case Opcode::NegFloat: return "NegFloat";

        case Opcode::EqInt: return "EqInt";
        case Opcode::EqFloat: return "EqFloat";
        case Opcode::EqBool: return "EqBool";
        case Opcode::NeInt: return "NeInt";
        case Opcode::NeFloat: return "NeFloat";
        case Opcode::NeBool: return "NeBool";
        case Opcode::LtInt: return "LtInt";
        case Opcode::LtFloat: return "LtFloat";
        case Opcode::LeInt: return "LeInt";
        case Opcode::LeFloat: return "LeFloat";
        case Opcode::GtInt: return "GtInt";
        case Opcode::GtFloat: return "GtFloat";
        case Opcode::GeInt: return "GeInt";
        case Opcode::GeFloat: return "GeFloat";

        case Opcode::And: return "And";
        case Opcode::Or: return "Or";
        case Opcode::Not: return "Not";

        case Opcode::LoadLocal: return "LoadLocal";
        case Opcode::StoreLocal: return "StoreLocal";
        case Opcode::LoadGlobal: return "LoadGlobal";
        case Opcode::StoreGlobal: return "StoreGlobal";

        case Opcode::Alloc: return "Alloc";
        case Opcode::Load: return "Load";
        case Opcode::Store: return "Store";

        case Opcode::GetField: return "GetField";
        case Opcode::SetField: return "SetField";

        case Opcode::NewArray: return "NewArray";
        case Opcode::GetElement: return "GetElement";
        case Opcode::SetElement: return "SetElement";
        case Opcode::ArrayLength: return "ArrayLength";

        case Opcode::Jump: return "Jump";
        case Opcode::JumpIfTrue: return "JumpIfTrue";
        case Opcode::JumpIfFalse: return "JumpIfFalse";

        case Opcode::Call: return "Call";
        case Opcode::CallForeign: return "CallForeign";
        case Opcode::Return: return "Return";
        case Opcode::ReturnVoid: return "ReturnVoid";

        case Opcode::IntToFloat: return "IntToFloat";
        case Opcode::FloatToInt: return "FloatToInt";
        case Opcode::IntToBool: return "IntToBool";

        case Opcode::Print: return "Print";
        case Opcode::Halt: return "Halt";

        default: return "Unknown";
    }
}

int getOpcodeOperandSize(Opcode opcode) {
    switch (opcode) {
        // No operands
        case Opcode::Pop:
        case Opcode::Dup:
        case Opcode::Swap:
        case Opcode::AddInt:
        case Opcode::AddFloat:
        case Opcode::SubInt:
        case Opcode::SubFloat:
        case Opcode::MulInt:
        case Opcode::MulFloat:
        case Opcode::DivInt:
        case Opcode::DivFloat:
        case Opcode::ModInt:
        case Opcode::NegInt:
        case Opcode::NegFloat:
        case Opcode::EqInt:
        case Opcode::EqFloat:
        case Opcode::EqBool:
        case Opcode::NeInt:
        case Opcode::NeFloat:
        case Opcode::NeBool:
        case Opcode::LtInt:
        case Opcode::LtFloat:
        case Opcode::LeInt:
        case Opcode::LeFloat:
        case Opcode::GtInt:
        case Opcode::GtFloat:
        case Opcode::GeInt:
        case Opcode::GeFloat:
        case Opcode::And:
        case Opcode::Or:
        case Opcode::Not:
        case Opcode::Load:
        case Opcode::Store:
        case Opcode::GetElement:
        case Opcode::SetElement:
        case Opcode::ArrayLength:
        case Opcode::Return:
        case Opcode::ReturnVoid:
        case Opcode::IntToFloat:
        case Opcode::FloatToInt:
        case Opcode::IntToBool:
        case Opcode::Print:
        case Opcode::Halt:
        case Opcode::ConstNull:
            return 0;

        // 1-byte operands
        case Opcode::ConstBool:
            return 1;

        // 4-byte operands
        case Opcode::LoadLocal:
        case Opcode::StoreLocal:
        case Opcode::LoadGlobal:
        case Opcode::StoreGlobal:
        case Opcode::ConstString:
        case Opcode::GetField:
        case Opcode::SetField:
        case Opcode::NewArray:
        case Opcode::Jump:
        case Opcode::JumpIfTrue:
        case Opcode::JumpIfFalse:
            return 4;

        // 8-byte operands
        case Opcode::ConstInt:
        case Opcode::ConstFloat:
            return 8;

        // Variable-length or multiple operands
        case Opcode::Alloc:
            return 8;  // type-id (4) + size (4)
        case Opcode::Call:
        case Opcode::CallForeign:
            return 8;  // function-index (4) + arg-count (4)

        default:
            return -1;  // Unknown
    }
}

} // namespace volta::bytecode
```

### Build and test:

```bash
make clean
make test
```

You should see the bytecode tests starting to pass!

## Step 2: Run Specific Tests

```bash
# Run just the chunk tests
./bin/test_volta --gtest_filter=BytecodeTest.Chunk_*

# Expected output:
# [==========] Running 11 tests from 1 test suite.
# [----------] 11 tests from BytecodeTest
# [ RUN      ] BytecodeTest.Chunk_EmitOpcode
# [       OK ] BytecodeTest.Chunk_EmitOpcode
# ...
# [  PASSED  ] 11 tests.
```

## Step 3: Implement Value Utilities

Next, implement the simple value utilities in `src/vm/Value.cpp`:

```cpp
#include "vm/Value.hpp"
#include <sstream>
#include <iomanip>

namespace volta::vm {

std::string valueTypeToString(ValueType type) {
    switch (type) {
        case ValueType::Null: return "Null";
        case ValueType::Int: return "Int";
        case ValueType::Float: return "Float";
        case ValueType::Bool: return "Bool";
        case ValueType::String: return "String";
        case ValueType::Object: return "Object";
    }
    return "Unknown";
}

std::string valueToString(const Value& value, const std::vector<std::string>* stringPool) {
    switch (value.type) {
        case ValueType::Null:
            return "null";

        case ValueType::Int:
            return std::to_string(value.asInt);

        case ValueType::Float: {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(6) << value.asFloat;
            return oss.str();
        }

        case ValueType::Bool:
            return value.asBool ? "true" : "false";

        case ValueType::String:
            if (stringPool && value.asStringIndex < stringPool->size()) {
                return "\"" + (*stringPool)[value.asStringIndex] + "\"";
            }
            return "@str" + std::to_string(value.asStringIndex);

        case ValueType::Object:
            return "<object@" + std::to_string(reinterpret_cast<uintptr_t>(value.asObject)) + ">";
    }
    return "?";
}

void printValue(std::ostream& out, const Value& value, const std::vector<std::string>* stringPool) {
    out << valueToString(value, stringPool);
}

bool valuesEqual(const Value& a, const Value& b) {
    if (a.type != b.type) return false;

    switch (a.type) {
        case ValueType::Null: return true;
        case ValueType::Int: return a.asInt == b.asInt;
        case ValueType::Float: return a.asFloat == b.asFloat;
        case ValueType::Bool: return a.asBool == b.asBool;
        case ValueType::String: return a.asStringIndex == b.asStringIndex;
        case ValueType::Object: return a.asObject == b.asObject;
    }
    return false;
}

bool sameType(const Value& a, const Value& b) {
    return a.type == b.type;
}

// ... implement remaining object utilities ...

} // namespace volta::vm
```

Test:
```bash
./bin/test_volta --gtest_filter=ValueUtilityTest.*
```

## Step 4: Continue with VM Stack Operations

Implement basic VM operations in `src/vm/VM.cpp`:

```cpp
#include "vm/VM.hpp"
#include <stdexcept>

namespace volta::vm {

VM::VM(std::shared_ptr<bytecode::CompiledModule> module)
    : module_(std::move(module)) {

    // Pre-allocate stacks
    stack_.resize(MAX_STACK_SIZE);
    callStack_.resize(MAX_CALL_DEPTH);

    // Allocate globals
    globals_.resize(module_->globalCount());
}

VM::~VM() {
    freeHeap();
}

void VM::push(const Value& value) {
    if (stackTop_ >= MAX_STACK_SIZE) {
        throw RuntimeError("Stack overflow", 0, 0, 0);
    }
    stack_[stackTop_++] = value;
}

Value VM::pop() {
    if (stackTop_ == 0) {
        throw RuntimeError("Stack underflow", 0, 0, 0);
    }
    return stack_[--stackTop_];
}

const Value& VM::peek(size_t depth) const {
    if (stackTop_ <= depth) {
        throw RuntimeError("Stack underflow on peek", 0, 0, 0);
    }
    return stack_[stackTop_ - 1 - depth];
}

// ... continue implementing remaining VM methods ...

} // namespace volta::vm
```

## Step 5: Verify Tests Pass

```bash
# Test VM stack operations
./bin/test_volta --gtest_filter=VMTest.PushPop
./bin/test_volta --gtest_filter=VMTest.Peek

# See how many tests pass
make test 2>&1 | grep "PASSED"
```

## Development Tips

### 1. Use Test-Driven Development

Look at a test to understand what to implement:

```cpp
TEST(BytecodeTest, Chunk_EmitOpcode) {
    Chunk chunk;
    chunk.emitOpcode(Opcode::ConstInt);

    EXPECT_EQ(chunk.code().size(), 1);
    EXPECT_EQ(static_cast<Opcode>(chunk.code()[0]), Opcode::ConstInt);
}
```

This tells you exactly what `emitOpcode()` should do!

### 2. Implement in Order

1. ✅ Bytecode.cpp (no dependencies)
2. ✅ Value.cpp (no dependencies)
3. ✅ VM.cpp (depends on Value)
4. ⏳ BytecodeCompiler.cpp (depends on Bytecode)
5. ⏳ Disassembler.cpp (depends on Bytecode)
6. ⏳ ... continue ...

### 3. Use Debug Output

Enable test output to see what's happening:

```bash
./bin/test_volta --gtest_filter=VMTest.* --gtest_verbose
```

### 4. Build with Sanitizers

Catch bugs early:

```bash
make clean
make DEBUG=1 test
```

## Common Pitfalls

### Endianness

Always use little-endian for cross-platform compatibility:

```cpp
// ✅ Correct
void emitInt32(int32_t value) {
    code_.push_back((value >> 0) & 0xFF);   // LSB first
    code_.push_back((value >> 8) & 0xFF);
    code_.push_back((value >> 16) & 0xFF);
    code_.push_back((value >> 24) & 0xFF);  // MSB last
}

// ❌ Wrong (platform-dependent)
void emitInt32(int32_t value) {
    code_.insert(code_.end(),
                 reinterpret_cast<uint8_t*>(&value),
                 reinterpret_cast<uint8_t*>(&value) + 4);
}
```

### Stack Bounds Checking

Always check stack bounds:

```cpp
void push(const Value& value) {
    if (stackTop_ >= MAX_STACK_SIZE) {
        throw RuntimeError("Stack overflow", 0, 0, 0);  // ✅
    }
    stack_[stackTop_++] = value;
}
```

### Memory Management

For now, just allocate without freeing (implement GC later):

```cpp
StructObject* allocateStruct(uint32_t typeId, uint32_t fieldCount) {
    size_t size = sizeof(StructObject) + sizeof(Value) * (fieldCount - 1);
    auto* obj = static_cast<StructObject*>(malloc(size));

    obj->header.kind = ObjectHeader::ObjectKind::Struct;
    obj->header.size = size;
    obj->header.typeId = typeId;

    heapObjects_.push_back(obj);  // Track for cleanup
    return obj;
}
```

## Next Steps

Once you have the basics working:

1. Implement bytecode compiler (IR → Bytecode)
2. Implement VM execution loop (fetch-decode-execute)
3. Wire up end-to-end integration tests
4. Add GC (can be deferred initially)

## Getting Help

- Check the API documentation: [bytecode_vm_api.md](bytecode_vm_api.md)
- Look at existing tests for examples
- Read the implementation status: [implementation_status.md](implementation_status.md)

Good luck! 🚀
