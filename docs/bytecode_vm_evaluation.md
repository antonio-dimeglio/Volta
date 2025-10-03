# Volta Bytecode, IR, and VM Evaluation Report

**Date**: 2025-10-03
**Evaluator**: System Analysis
**Overall Grade**: 6.5/10

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Bytecode Generator Evaluation](#bytecode-generator-evaluation)
3. [IR Generator Evaluation](#ir-generator-evaluation)
4. [VM Evaluation](#vm-evaluation)
5. [Critical Bugs](#critical-bugs)
6. [Improvement Recommendations](#improvement-recommendations)
7. [Action Plan](#action-plan)

---

## Executive Summary

The Volta bytecode, IR, and VM implementation demonstrates **solid foundational architecture** with good design decisions, but contains **critical bugs** and **incomplete features** that prevent successful execution. The codebase shows promise with comprehensive testing (225+ tests), clean separation of concerns, and forward-thinking features like generational garbage collection.

### Overall Scores

| Component | Score | Maintainability | Future-Proofing | Extensibility |
|-----------|-------|-----------------|-----------------|---------------|
| **Bytecode Generator** | 7/10 | 7/10 | 6/10 | 7/10 |
| **IR Generator** | 6/10 | 6/10 | 5/10 | 6/10 |
| **VM** | 7/10 | 7/10 | 6/10 | 7/10 |
| **Overall** | **6.5/10** | **6.5/10** | **5.5/10** | **6.5/10** |

---

## Bytecode Generator Evaluation

### Location
- `include/bytecode/Bytecode.hpp`
- `src/bytecode/Bytecode.cpp`
- `include/bytecode/BytecodeCompiler.hpp`
- `src/bytecode/BytecodeCompiler.cpp`

### ✅ The Good

#### 1. **X-Macro Architecture** (⭐ Best Practice)
**Location**: [Bytecode.hpp:20-96](../include/bytecode/Bytecode.hpp#L20-L96)

The X-macro pattern provides a single source of truth for opcodes:
```cpp
#define OPCODE_LIST(X) \
    X(Pop, 0) \
    X(ConstInt, 8) \
    X(AddInt, 0)
```

**Benefits**:
- Adding a new opcode requires one line change
- Automatically generates enum, name table, and size table
- Eliminates sync issues between definitions

#### 2. **Well-Designed Instruction Set**
- Stack-based architecture with type-specific operations (AddInt/AddFloat)
- Comprehensive coverage: arithmetic, memory, control flow, arrays, structs
- 60+ opcodes with clear naming conventions

#### 3. **Strong Compiler Design**
**Location**: [BytecodeCompiler.hpp:96-253](../include/bytecode/BytecodeCompiler.hpp#L96-L253)

- Clear separation of concerns with helper methods for each IR instruction type
- Proper symbol resolution with multiple lookup maps
- Forward jump patching mechanism

#### 4. **Excellent Serialization**
**Location**: [BytecodeSerializer.hpp](../include/bytecode/BytecodeSerializer.hpp)

- Binary format with magic number and versioning
- Proper validation on deserialization
- Round-trip tested

#### 5. **Comprehensive Test Coverage**
**Location**: [test_bytecode.cpp](../tests/test_bytecode.cpp)

- 70+ tests covering edge cases, serialization, and compilation scenarios

### ⚠️ The Bad

#### 1. **Primitive Allocation Hack**
**Location**: [BytecodeCompiler.cpp:329-357](../src/bytecode/BytecodeCompiler.cpp#L329-L357)

```cpp
void BytecodeCompiler::compileAllocInst(const ir::AllocInst* inst, Chunk& chunk) {
    auto kind = allocType->kind();
    bool isPrimitive = (kind == semantic::Type::Kind::Int || ...);

    if (isPrimitive) {
        chunk.emitOpcode(Opcode::ConstNull);  // Hack: no heap allocation
    } else {
        chunk.emitOpcode(Opcode::Alloc);      // Heap allocation
    }
}
```

**Problem**: Creates inconsistency - primitives use local slots directly while structs use heap.

**Impact**: Leaky abstraction that complicates reasoning about memory model.

#### 2. **Incomplete Type System**
**Location**: [BytecodeCompiler.cpp:350-351](../src/bytecode/BytecodeCompiler.cpp#L350-L351)

```cpp
chunk.emitInt32(0); // type-id (TODO: implement proper type indexing)
chunk.emitInt32(8); // size in bytes (TODO: calculate from type)
```

**Problem**: Critical gap for structs/objects - hardcoded type IDs and sizes.

**Impact**: Cannot support variable-sized structs or arrays.

#### 3. **Inefficient Conditional Branching**
**Location**: [BytecodeCompiler.cpp:496-531](../src/bytecode/BytecodeCompiler.cpp#L496-L531)

```cpp
void BytecodeCompiler::compileBranchIfInst(...) {
    chunk.emitOpcode(Opcode::JumpIfFalse);  // Jump to else
    // ... emit offset ...
    chunk.emitOpcode(Opcode::Jump);         // Jump to then
    // ... emit offset ...
}
```

**Problem**: Emits `JumpIfFalse` + `Jump` instead of `JumpIfTrue` + fallthrough.

**Impact**: Wastes 5 bytes per conditional, slower execution.

#### 4. **Limited Error Context**
Exceptions lack source location info:
```cpp
throw std::runtime_error("Unresolved forward jumps in function " + function->name());
```

No line numbers, no instruction offset for debugging.

### 🔥 The Ugly

#### 1. **SSA Value Storage Overhead**
**Location**: [BytecodeCompiler.cpp:115-123](../src/bytecode/BytecodeCompiler.cpp#L115-L123)

```cpp
for (const auto& block : function->basicBlocks()) {
    for (const auto& inst : block->instructions()) {
        if (inst->type()->kind() != semantic::Type::Kind::Void) {
            localIndexMap_[inst.get()] = localIndex++;  // Every value gets a slot!
        }
    }
}
```

**Problem**: Allocates local slot for every SSA value, even if only used once.

**Example**:
```
%1 = add %a, %b
%2 = load %1      # %1 immediately consumed
```
Still stores `%1` to local then reloads it.

**Impact**: Excessive memory usage and unnecessary store/load pairs.

#### 2. **Hardcoded Entry Point Names**
**Location**: [BytecodeCompiler.cpp:40-48](../src/bytecode/BytecodeCompiler.cpp#L40-L48)

```cpp
const CompiledFunction* CompiledModule::getEntryPoint() const {
    const auto* initFunc = getFunction("__init__");  // Hardcoded
    if (initFunc) return initFunc;
    return getFunction("main");  // Hardcoded
}
```

**Problem**: Not configurable.

**Impact**: Cannot customize entry point for embedded use.

#### 3. **Missing Bytecode Verification**
No validator pass means VM could receive:
- Invalid jump offsets
- Stack underflow sequences
- Type mismatches
- Out-of-bounds local access

**Impact**: Crashes instead of early errors.

#### 4. **Endianness Assumption**
**Location**: [Bytecode.cpp:12-23](../src/bytecode/Bytecode.cpp#L12-L23)

All serialization assumes little-endian:
```cpp
void Chunk::emitInt32(int32_t value) {
    emitBytes(value);  // Platform-dependent
}
```

**Impact**: Bytecode not portable to big-endian systems.

### 🎯 Improvement Tips

#### **Short Term (1-2 weeks)**

1. **Fix Primitive Allocation**
   ```cpp
   // Option 1: Always use heap (consistent but slower)
   chunk.emitOpcode(Opcode::Alloc);

   // Option 2: Add AllocStack opcode for primitives
   if (isPrimitive) {
       chunk.emitOpcode(Opcode::AllocStack);
   }
   ```

2. **Implement Type Indexing**
   ```cpp
   // Build type table in module
   uint32_t typeId = module_->addType(allocType);
   uint32_t size = calculateTypeSize(allocType);
   chunk.emitInt32(typeId);
   chunk.emitInt32(size);
   ```

3. **Optimize Conditional Branches**
   ```cpp
   // Emit JumpIfTrue + fallthrough instead
   chunk.emitOpcode(Opcode::JumpIfTrue);
   // ... emit offset to then block ...
   // Fall through to else block
   ```

#### **Medium Term (1-2 months)**

4. **Add Peephole Optimizer**
   ```cpp
   class PeepholeOptimizer {
       void optimize(Chunk& chunk) {
           // Eliminate store-load pairs
           // Constant folding
           // Dead code elimination
       }
   };
   ```

5. **Implement Register Allocation**
   Track SSA value liveness and reuse local slots:
   ```cpp
   // Instead of: each value gets new slot
   // Do: reuse slots when values are dead
   auto livenessAnalysis = analyzeLiveness(function);
   auto allocation = allocateRegisters(livenessAnalysis);
   ```

6. **Add Bytecode Verifier**
   ```cpp
   class BytecodeVerifier {
       bool verify(const CompiledModule& module, std::ostream& errors) {
           // Check jump targets are valid
           // Verify stack balance
           // Validate type consistency
       }
   };
   ```

#### **Long Term (3+ months)**

7. **Implement NaN-Boxing Compatible Bytecode**
   Prepare for future VM optimization by encoding type info in opcodes:
   ```cpp
   // Instead of: ConstInt, ConstFloat, ConstBool
   // Use: Const <tagged-value>
   ```

8. **Add Debug Info Tables**
   ```cpp
   struct DebugInfo {
       std::vector<LineNumberEntry> lineNumbers;
       std::vector<VariableEntry> locals;
       std::vector<SourceLocation> inlinedFrames;
   };
   ```

---

## IR Generator Evaluation

### Location
- `include/IR/IR.hpp`
- `src/IR/IR.cpp`
- `include/IR/IRBuilder.hpp`
- `src/IR/IRBuilder.cpp`
- `include/IR/IRGenerator.hpp`
- `src/IR/IRGenerator.cpp`

### ✅ The Good

#### 1. **Clean SSA-Form IR Design**
**Location**: [IR.hpp:26-52](../include/IR/IR.hpp#L26-L52)

```cpp
class Value {
    enum class Kind {
        Instruction, Parameter, Constant, GlobalVariable, BasicBlock
    };
    std::shared_ptr<semantic::Type> type_;
    std::string name_;
};
```

**Benefits**:
- Clear value hierarchy
- Type-safe representation
- Easy to extend

#### 2. **Comprehensive Instruction Set**
**Location**: [IR.hpp:112-136](../include/IR/IR.hpp#L112-L136)

Complete coverage of operations:
- Arithmetic, logical, comparison
- Memory operations (alloc, load, store)
- Control flow (branches, calls)
- Arrays and structs
- Even phi nodes for future SSA optimization

#### 3. **Excellent Builder Pattern**
**Location**: [IRBuilder.hpp](../include/IR/IRBuilder.hpp)

```cpp
builder.setInsertPoint(entry.get());
auto sum = builder.createAdd(param0, param1);
builder.createRet(sum);
```

**Benefits**:
- Clean API with automatic temp naming
- Constant caching (deduplication)
- Type inference

#### 4. **Strong AST→IR Translation**
**Location**: [IRGenerator.cpp:391-462](../src/IR/IRGenerator.cpp#L391-L462)

Handles complex constructs:
- For-loops with range operators
- Method calls with qualified names
- Struct literals with field initialization

#### 5. **Multi-Pass Generation**
**Location**: [IRGenerator.cpp:28-62](../src/IR/IRGenerator.cpp#L28-L62)

```cpp
// First pass: collect functions and structs (forward references)
// Second pass: generate global variables
// Third pass: wrap top-level code in __init__
```

**Benefits**: Enables forward references and clean separation.

### ⚠️ The Bad

#### 1. **Incomplete Features Everywhere**

**Location**: [IRGenerator.cpp](../src/IR/IRGenerator.cpp)

| Feature | Status | Line |
|---------|--------|------|
| If-expression | Stub (returns null) | 895-910 |
| Match expression | Throws exception | 912-916 |
| Lambda | Throws exception | 918-922 |
| Slice | Throws exception | 777-781 |
| Tuple literal | Stub (returns null) | 1003-1011 |
| Some literal | Stub (returns null) | 967-975 |

**Impact**: Large swaths of language features unusable.

#### 2. **Pass-by-Value/Reference Confusion**
**Location**: [IRGenerator.cpp:104-123](../src/IR/IRGenerator.cpp#L104-L123)

```cpp
if (isObjectType) {
    // For arrays/structs, register parameter directly (pass by reference)
    declareVariable(funcDecl.parameters[i]->identifier, param);
} else {
    // For scalars, use alloca+store (pass by value)
    auto* alloca = builder_.createAlloc(param->type());
    builder_.createStore(param, alloca);
    declareVariable(funcDecl.parameters[i]->identifier, alloca);
}
```

**Problem**: Inconsistent - arrays/structs are "passed by reference" (really just not copied), scalars use alloca+store.

**Impact**: Will cause bugs with array/struct mutations. Caller might not see changes.

#### 3. **Scope Management is Broken** 🔥
**Location**: [IRGenerator.cpp:1070-1072](../src/IR/IRGenerator.cpp#L1070-L1072)

```cpp
void IRGenerator::declareVariable(const std::string& name, Value* value) {
    // TODO: Handle nested scopes
    variableMap_[name] = value;  // FLAT MAP - NO SCOPING!
}
```

**Problem**: No scope stack. Inner scopes clobber outer variables.

**Example**:
```rust
let x = 1;
if true {
    let x = 2;  // Clobbers outer x
}
print(x);  // Should be 1, but is 2!
```

**Impact**: Shadowing doesn't work. Critical bug.

#### 4. **Array/Struct Literal Special-Casing**
**Location**: [IRGenerator.cpp:293-314](../src/IR/IRGenerator.cpp#L293-L314)

```cpp
bool isArrayOrStructLiteral =
    dynamic_cast<const volta::ast::ArrayLiteral*>(varDecl.initializer.get()) != nullptr ||
    dynamic_cast<const volta::ast::StructLiteral*>(varDecl.initializer.get()) != nullptr;

if (isArrayOrStructLiteral) {
    declareVariable(varDecl.identifier, initValue);  // No alloca
} else {
    Value* alloca = builder_.createAlloc(type);  // With alloca
}
```

**Problem**: Fragile - checks AST type to decide whether to use alloca.

**Impact**: Breaks if you introduce new object types.

#### 5. **Foreign Function Handling**
**Location**: [IRGenerator.cpp:1091-1156](../src/IR/IRGenerator.cpp#L1091-L1156)

Built-ins registered manually with hardcoded signatures:
```cpp
void IRGenerator::registerBuiltinFunctions() {
    // print(string) -> void
    auto funcType = std::make_shared<semantic::FunctionType>(params, voidType);
    auto func = builder_.createFunction("print", funcType);
    func->setForeign(true);
    module_->addFunction(std::move(func));
}
```

**Problem**: No extensibility mechanism.

**Impact**: Can't add custom built-ins without modifying IRGenerator.

### 🔥 The Ugly

#### 1. **Assignment L-Value Handling is a Mess** 🔥
**Location**: [IRGenerator.cpp:540-654](../src/IR/IRGenerator.cpp#L540-L654)

350+ lines of tangled code handling:
- Simple variables
- Struct member access
- Array indexing
- Compound assignments (+=, -=, etc.)

**Example snippet**:
```cpp
if (auto* ident = dynamic_cast<volta::ast::IdentifierExpression*>(expr.left.get())) {
    leftAddr = lookupVariable(ident->name);
} else if (auto* member = dynamic_cast<volta::ast::MemberExpression*>(expr.left.get())) {
    // 30+ lines of member handling
} else if (auto* index = dynamic_cast<volta::ast::IndexExpression*>(expr.left.get())) {
    // 20+ lines of index handling
}
```

**Problem**: Should use unified GetElementPtr-style addressing.

**Impact**: Hard to maintain, easy to introduce bugs.

#### 2. **No Memory Management for Constants** 🔥
**Location**: [IRBuilder.hpp:226-231](../include/IR/IRBuilder.hpp#L226-L231)

```cpp
class IRBuilder {
    std::unordered_map<int64_t, std::unique_ptr<Constant>> intConstants_;

    Constant* getInt(int64_t value) {
        // ...
        return ptr;  // Return raw pointer to unique_ptr-owned object
    }
};
```

**Problem**: Constants owned by IRBuilder via unique_ptr, but IR holds raw pointers.

**Impact**: If builder dies, constants are freed but IR still has pointers → use-after-free!

#### 3. **Control Flow Block Ordering Fragility**
**Location**: [IRGenerator.cpp:380-381](../src/IR/IRGenerator.cpp#L380-L381)

```cpp
BasicBlock* condBlock = currentFunction_->basicBlocks()[
    currentFunction_->basicBlocks().size() - 2  // Hardcoded index arithmetic!
].get();
```

**Problem**: Relies on blocks being added in specific order.

**Impact**: Will break if block order changes.

#### 4. **Type System Mismatch**
IR uses `shared_ptr<Type>` but AST uses raw pointers. Constant conversion:
```cpp
std::shared_ptr<semantic::Type> resolveType(const volta::ast::Type* astType) {
    return analyzer_->resolveTypeAnnotation(astType);  // Conversion
}
```

**Impact**: Inefficient, error-prone.

#### 5. **Phi Nodes Defined But Never Generated**
**Location**: [IR.hpp:462-483](../include/IR/IR.hpp#L462-L483)

```cpp
class PhiInst : public Instruction {
    // Fully defined with incoming values, etc.
};
```

But IRGenerator never creates PhiInst. Dead code or future tech debt?

### 🎯 Improvement Tips

#### **Critical Fixes (Immediately)**

1. **Fix Scope Management** 🔥
   ```cpp
   class ScopeManager {
       std::vector<std::unordered_map<std::string, Value*>> scopes_;

       void pushScope() { scopes_.emplace_back(); }
       void popScope() { scopes_.pop_back(); }

       void declare(const std::string& name, Value* value) {
           scopes_.back()[name] = value;
       }

       Value* lookup(const std::string& name) {
           for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
               auto found = it->find(name);
               if (found != it->end()) return found->second;
           }
           return nullptr;
       }
   };
   ```

2. **Fix Constant Ownership**
   ```cpp
   // Option 1: Module owns constants
   class IRModule {
       std::vector<std::unique_ptr<Constant>> constants_;
       Constant* addConstant(std::unique_ptr<Constant> c);
   };

   // Option 2: Use shared_ptr
   std::shared_ptr<Constant> getInt(int64_t value);
   ```

#### **Short Term (1-2 weeks)**

3. **Unify Assignment L-Value Handling**
   ```cpp
   struct LValue {
       enum class Kind { Variable, Field, Index };
       Kind kind;
       Value* base;
       union {
           size_t fieldIndex;
           Value* arrayIndex;
       };
   };

   LValue generateLValue(const ast::Expression* expr);
   void emitStore(const LValue& lvalue, Value* value);
   ```

4. **Complete Incomplete Features**
   Priority order:
   - If-expression (needed for ternary)
   - Tuple literals (common pattern)
   - Slice expressions (arrays)
   - Match/Lambda (later)

5. **Make Foreign Functions Extensible**
   ```cpp
   class ForeignFunctionRegistry {
       void registerFunction(const std::string& name, FunctionSignature sig);
       const FunctionSignature* lookup(const std::string& name) const;
   };
   ```

#### **Medium Term (1-2 months)**

6. **Implement Proper SSA Construction**
   ```cpp
   // Add phi nodes at block merge points
   class SSABuilder {
       void insertPhiNodes(Function* func);
       void renameVariables(Function* func);
   };
   ```

7. **Add Optimization Passes**
   ```cpp
   class IRPass {
       virtual bool run(Function* func) = 0;
   };

   class DeadCodeElimination : public IRPass { };
   class ConstantFolding : public IRPass { };
   class CommonSubexpressionElimination : public IRPass { };
   ```

8. **Fix Parameter Passing Semantics**
   ```cpp
   // Make consistent: all by-value with explicit copy
   // Or: all by-reference with move semantics
   // Don't mix!
   ```

#### **Long Term (3+ months)**

9. **Add IR Verification**
   ```cpp
   class IRVerifier {
       bool verify(const IRModule& module, std::ostream& errors) {
           // Check all uses have definitions
           // Verify types match
           // Ensure all blocks end with terminator
           // Check SSA form
       }
   };
   ```

10. **Build IR Serialization**
    For debugging and testing:
    ```cpp
    void IRModule::serialize(std::ostream& out) const;
    std::unique_ptr<IRModule> IRModule::deserialize(std::istream& in);
    ```

---

## VM Evaluation

### Location
- `include/vm/VM.hpp`
- `src/vm/VM.cpp`
- `include/vm/GC.hpp`

### ✅ The Good

#### 1. **Solid Tagged Union Value System**
**Location**: [VM.hpp:42-108](../include/vm/VM.hpp#L42-L108)

```cpp
struct Value {
    ValueType type;
    union {
        int64_t asInt;
        double asFloat;
        bool asBool;
        uint32_t asStringIndex;
        void* asObject;
    };

    bool isTruthy() const { /* ... */ }
};
```

**Benefits**:
- 16-byte value fits in registers (though could be optimized to 8 bytes)
- Type-safe access
- Static factory methods prevent mistakes

#### 2. **Complete Instruction Set Implementation**
**Location**: [VM.cpp:268-368](../src/vm/VM.cpp#L268-L368)

All 60+ opcodes properly implemented:
```cpp
bool VM::executeInstruction() {
    uint8_t opcode = readByte();
    switch (static_cast<bytecode::Opcode>(opcode)) {
        case Opcode::AddInt: execAddInt(); break;
        // ... all opcodes ...
    }
}
```

With proper type checking:
```cpp
void VM::execAddInt() {
    Value right = pop();
    Value left = pop();
    expectType(left, ValueType::Int, "AddInt left operand");
    expectType(right, ValueType::Int, "AddInt right operand");
    push(Value::makeInt(left.asInt + right.asInt));
}
```

#### 3. **Generational GC Architecture**
**Location**: [VM.cpp:18-22](../src/vm/VM.cpp#L18-L22)

```cpp
GCConfig gcConfig;
gcConfig.youngGenMaxSize = 2 * 1024 * 1024;  // 2MB young gen
gcConfig.oldGenMaxSize = 16 * 1024 * 1024;   // 16MB old gen
gcConfig.promotionAge = 3;
gc_ = std::make_unique<GarbageCollector>(this, gcConfig);
```

With write barriers:
```cpp
if (value.type == ValueType::Object && value.asObject != nullptr) {
    gc_->writeBarrier(object.asObject, value.asObject);
}
```

#### 4. **Proper Call Frame Management**
**Location**: [VM.hpp:166-171](../include/vm/VM.hpp#L166-L171)

```cpp
struct CallFrame {
    uint32_t functionIndex;
    size_t returnAddress;
    size_t framePointer;
    uint32_t localCount;
};
```

Separate eval stack and local stack prevents corruption.

#### 5. **Excellent Error Handling**
**Location**: [VM.cpp:1236-1258](../src/vm/VM.cpp#L1236-L1258)

```cpp
void VM::expectType(const Value& value, ValueType expected, const std::string& operation) {
    if (value.type != expected) {
        std::string msg = operation + ": expected " + /* type names */;
        runtimeError(msg);
    }
}
```

Runtime errors with stack traces:
```cpp
void VM::runtimeError(const std::string& message) {
    std::cerr << "Runtime Error: " << message << "\n";
    if (callStackTop_ > 0) {
        std::cerr << "  at " << func->name << ":" << ip_ << "\n";
    }
    throw RuntimeError(message, funcIndex, ip_, lineNumber);
}
```

#### 6. **Strong FFI Design**
**Location**: [VM.hpp:251](../include/vm/VM.hpp#L251)

```cpp
using NativeFunction = std::function<int(VM&)>;

void registerNativeFunction(const std::string& name, NativeFunction func);
```

Native functions access VM directly:
```cpp
vm.registerNativeFunction("add_one", [](VM& vm) {
    Value arg = vm.pop();
    vm.push(Value::makeInt(arg.asInt + 1));
    return 1;  // One return value
});
```

#### 7. **Comprehensive Testing**
**Location**: [test_vm.cpp](../tests/test_vm.cpp)

75+ tests including edge cases, GC, FFI, and stress tests.

### ⚠️ The Bad

#### 1. **Load/Store Instructions Are Broken** 🔥🔥🔥
**Location**: [VM.cpp:759-831](../src/vm/VM.cpp#L759-L831)

```cpp
void VM::execLoad() {
    int32_t offset = readInt32();  // ← Reads offset from bytecode
    Value address = pop();
    // ...
}
```

**But bytecode compiler NEVER emits the offset:**
**Location**: [BytecodeCompiler.cpp:359-384](../src/bytecode/BytecodeCompiler.cpp#L359-L384)

```cpp
void BytecodeCompiler::compileLoadInst(const ir::LoadInst* inst, Chunk& chunk) {
    emitLoadValue(inst->address(), chunk);  // Load address onto stack
    chunk.emitOpcode(Opcode::Load);         // ← NO OFFSET EMITTED!
    chunk.emitOpcode(Opcode::StoreLocal);
}
```

**Impact**: VM will read garbage as offset. **CRITICAL BUG - WILL CRASH.**

#### 2. **Jump Offset Calculation**
**Location**: [VM.cpp:947-973](../src/vm/VM.cpp#L947-L973)

```cpp
void VM::execJump() {
    int32_t offset = readInt32();   // After reading, IP advanced
    size_t currentPos = ip_;         // Position AFTER reading operand
    ip_ = currentPos + offset;       // Relative jump
}
```

Compiler calculates offset relative to position after operand:
**Location**: [BytecodeCompiler.cpp:484-487](../src/bytecode/BytecodeCompiler.cpp#L484-L487)

```cpp
int32_t currentOffset = chunk.currentOffset() + 4; // +4 for operand
int32_t relativeOffset = targetOffset - currentOffset;
```

**Problem**: This works but is fragile. One mistake breaks all control flow.

#### 3. **No Debug Trace Implementation**
**Location**: [VM.cpp:258-260](../src/vm/VM.cpp#L258-L260)

```cpp
void VM::run() {
    while (true) {
        if (debugTrace_) {
            // Print current instruction  ← EMPTY!
        }
        if (!executeInstruction()) break;
    }
}
```

Debug tracing incomplete. Feature doesn't work.

#### 4. **Missing Bounds Checks**

Global variable access:
**Location**: [VM.cpp:740-748](../src/vm/VM.cpp#L740-L748)

```cpp
void VM::execLoadGlobal() {
    int32_t globalIndex = readInt32();
    push(globals_[globalIndex]);  // ← NO BOUNDS CHECK!
}
```

Foreign function table:
**Location**: [VM.cpp:167-178](../src/vm/VM.cpp#L167-L178)

```cpp
void VM::callNativeFunction(uint32_t foreignIndex, uint32_t argCount) {
    if (foreignIndex >= nativeFunctionTable_.size()) {  // ✓ Good
        runtimeError("Foreign function index out of bounds");
    }
    NativeFunction& nativeFunc = nativeFunctionTable_[foreignIndex];
    nativeFunc(*this);  // ← argCount not validated!
}
```

#### 5. **Type Confusion in GetField/SetField**
**Location**: [VM.cpp:833-853](../src/vm/VM.cpp#L833-L853)

```cpp
void VM::execGetField() {
    int32_t fieldIndex = readInt32();
    Value object = pop();

    // NO NULL CHECK!
    // NO TYPE CHECK!
    StructObject* obj = static_cast<StructObject*>(object.asObject);
    push(obj->fields[fieldIndex]);
}
```

**Impact**: Will segfault on:
- Null object
- Non-struct object
- Out-of-bounds field index

### 🔥 The Ugly

#### 1. **Argument Transfer Bug** 🔥🔥🔥
**Location**: [VM.cpp:992-994](../src/vm/VM.cpp#L992-L994)

```cpp
void VM::execCall() {
    // ...
    for (int i = argCount - 1; i >= 0; i--) {
        localStack_[localStackTop_ + i] = pop();  // BUG!
    }
}
```

**Problem**: Pops args in reverse order but stores at increasing indices.

**Example**:
```
Stack before: [... arg0, arg1, arg2] (top = arg2)
Loop iteration 0: pop() → arg2, store at localStackTop_ + 2
Loop iteration 1: pop() → arg1, store at localStackTop_ + 1
Loop iteration 2: pop() → arg0, store at localStackTop_ + 0

Result: [arg2, arg1, arg0]  ← BACKWARDS!
```

**Impact**: **ALL function calls get arguments in wrong order!**

**Fix**:
```cpp
// Option 1: Reverse the store index
for (int i = argCount - 1; i >= 0; i--) {
    localStack_[localStackTop_ + (argCount - 1 - i)] = pop();
}

// Option 2: Pop forward, store forward
for (int i = 0; i < argCount; i++) {
    localStack_[localStackTop_ + i] = stack_[stackTop_ - argCount + i];
}
stackTop_ -= argCount;
```

#### 2. **execPrint Hardcodes Console Output**
**Location**: [VM.cpp:1075-1127](../src/vm/VM.cpp#L1075-L1127)

```cpp
void VM::execPrint() {
    Value val = pop();
    std::cout << /* format value */;  // Hardcoded stdout!
    std::cout << "\n";
}
```

**Problem**: Can't redirect output for testing or embedding.

**Impact**: Untestable, inflexible.

**Fix**:
```cpp
// Add output stream to VM
class VM {
    std::ostream& output_;
public:
    VM(Module, std::ostream& out = std::cout);
};

void VM::execPrint() {
    output_ << /* format value */ << "\n";
}
```

#### 3. **Static Empty Chunk on Error**
**Location**: [VM.cpp:1206-1210](../src/vm/VM.cpp#L1206-L1210)

```cpp
const bytecode::Chunk& VM::currentChunk() const {
    if (callStackTop_ == 0) {
        static bytecode::Chunk empty;  // ← Thread-unsafe!
        return empty;
    }
    // ...
}
```

**Problem**: Returns reference to static local.

**Impact**:
- Thread-unsafe (multiple VMs share same empty chunk)
- Misleading (caller thinks they have valid chunk)

**Fix**:
```cpp
const bytecode::Chunk* VM::currentChunk() const {
    if (callStackTop_ == 0) return nullptr;  // Explicit failure
    // ...
}
```

#### 4. **No NaN-Boxing or Pointer Tagging**

Current: 16-byte values
```cpp
struct Value {
    ValueType type;  // 1 byte + 7 bytes padding
    union { ... };   // 8 bytes
};  // Total: 16 bytes
```

Modern VMs: 8-byte NaN-boxed values
```cpp
// All values fit in 64 bits:
// - Doubles: valid IEEE 754 (non-NaN)
// - Integers: NaN + payload
// - Pointers: NaN + pointer (low 48 bits)
// - Booleans: NaN + bool
```

**Impact**: 2x memory waste, poor cache utilization.

#### 5. **GC Write Barriers Only on Some Paths**

Has barriers:
**Location**: [VM.cpp:824-827](../src/vm/VM.cpp#L824-L827)
```cpp
void VM::execStore() {
    // ...
    if (value.type == ValueType::Object) {
        gc_->writeBarrier(address.asObject, value.asObject);
    }
}
```

Missing barriers:
**Location**: [VM.cpp:833-853](../src/vm/VM.cpp#L833-L853)
```cpp
void VM::execSetField() {
    // ...
    obj->fields[fieldIndex] = value;

    if (value.type == ValueType::Object) {
        gc_->writeBarrier(object.asObject, value.asObject);  // ✓ Has it
    }
}
```

But what about arrays initialized in NewArray?

**Impact**: Inconsistent = potential GC bugs.

#### 6. **Foreign Function Table Building Missing** 🔥
**Location**: [VM.cpp:24](../src/vm/VM.cpp#L24)

```cpp
VM::VM(std::shared_ptr<bytecode::CompiledModule> module)
    : module_(module) {
    // ...

    // Build foreign function and build native function table.
    // ↑ COMMENT SAYS THIS BUT CODE DOESN'T EXIST!
}
```

**Problem**: Comment says to build table but implementation missing.

Compiler expects foreign functions indexed from module:
**Location**: [BytecodeCompiler.cpp:72-79](../src/bytecode/BytecodeCompiler.cpp#L72-L79)

```cpp
uint32_t foreignIndex = 0;
for (const auto& function : module.functions()) {
    if (function->isForeign()) {
        foreignFunctionMap_[function->name()] = foreignIndex++;
        compiledModule->foreignFunctions().push_back(function->name());
    }
}
```

But VM requires manual registration:
```cpp
vm.registerNativeFunction("print", printImpl);
```

**Impact**: No automatic mapping. **Foreign functions won't work!**

**Fix**:
```cpp
VM::VM(std::shared_ptr<bytecode::CompiledModule> module) {
    // ...

    // Map module's foreign functions to native implementations
    for (const auto& name : module->foreignFunctions()) {
        auto it = globalRegistry.find(name);
        if (it != globalRegistry.end()) {
            nativeFunctionTable_.push_back(it->second);
        } else {
            throw std::runtime_error("Undefined foreign function: " + name);
        }
    }
}
```

### 🎯 Improvement Tips

#### **Critical Fixes (Immediately)** 🔥

1. **Fix Load/Store Instructions**

   **Option A**: Remove offset from VM (simpler)
   ```cpp
   void VM::execLoad() {
       // No offset - just load from object directly
       Value address = pop();
       expectType(address, ValueType::Object, "Load");
       // For struct: compiler must emit GetField instead
       // For array: compiler must emit GetElement instead
   }
   ```

   **Option B**: Add offset to compiler (more flexible)
   ```cpp
   void BytecodeCompiler::compileLoadInst(...) {
       emitLoadValue(inst->address(), chunk);
       chunk.emitOpcode(Opcode::Load);
       chunk.emitInt32(0);  // offset (0 for now)
   }
   ```

2. **Fix Argument Transfer** 🔥
   ```cpp
   // Correct version:
   for (int i = argCount - 1; i >= 0; i--) {
       localStack_[localStackTop_ + (argCount - 1 - i)] = pop();
   }

   // Or simpler:
   for (int i = 0; i < argCount; i++) {
       localStack_[localStackTop_ + i] = stack_[stackTop_ - argCount + i];
   }
   stackTop_ -= argCount;
   ```

3. **Build Foreign Function Table**
   ```cpp
   // Add global registry
   class ForeignFunctionRegistry {
   public:
       static ForeignFunctionRegistry& instance();
       void registerFunction(const std::string& name, VM::NativeFunction func);
       VM::NativeFunction lookup(const std::string& name);
   };

   // In VM constructor:
   for (const auto& name : module->foreignFunctions()) {
       nativeFunctionTable_.push_back(
           ForeignFunctionRegistry::instance().lookup(name)
       );
   }
   ```

4. **Add Bounds Checks**
   ```cpp
   void VM::execLoadGlobal() {
       int32_t globalIndex = readInt32();
       if (globalIndex < 0 || globalIndex >= globals_.size()) {
           runtimeError("Global index out of bounds");
       }
       push(globals_[globalIndex]);
   }

   void VM::execGetField() {
       int32_t fieldIndex = readInt32();
       Value object = pop();
       expectType(object, ValueType::Object, "GetField");

       if (!object.asObject) {
           runtimeError("Null pointer in GetField");
       }

       StructObject* obj = static_cast<StructObject*>(object.asObject);
       if (obj->header.kind != ObjectHeader::ObjectKind::Struct) {
           runtimeError("GetField on non-struct");
       }

       // Field count check needed - add to StructObject header?
       push(obj->fields[fieldIndex]);
   }
   ```

#### **Short Term (1-2 weeks)**

5. **Implement Debug Trace**
   ```cpp
   void VM::run() {
       while (true) {
           if (debugTrace_) {
               auto opcode = static_cast<bytecode::Opcode>(currentChunk().code_[ip_]);
               std::cerr << "[" << ip_ << "] "
                         << bytecode::getOpcodeName(opcode) << "\n";
               dumpStack(std::cerr);
           }
           if (!executeInstruction()) break;
       }
   }
   ```

6. **Make Output Configurable**
   ```cpp
   class VM {
       std::ostream& output_;
   public:
       VM(Module module, std::ostream& out = std::cout)
           : module_(module), output_(out) {}
   };
   ```

7. **Add Bytecode Verification**
   ```cpp
   class BytecodeVerifier {
   public:
       bool verify(const CompiledModule& module) {
           for (const auto& func : module.functions()) {
               if (!verifyFunction(func)) return false;
           }
           return true;
       }
   private:
       bool verifyFunction(const CompiledFunction& func) {
           // Check all jumps are in bounds
           // Verify stack depth at all points
           // Check type consistency
       }
   };
   ```

#### **Medium Term (1-2 months)**

8. **Implement NaN-Boxing**
   ```cpp
   // Use IEEE 754 NaN payload for tagging
   class Value {
       uint64_t bits_;

       static constexpr uint64_t NAN_MASK = 0x7FF8000000000000ULL;
       static constexpr uint64_t TAG_INT   = 0x0001000000000000ULL;
       static constexpr uint64_t TAG_BOOL  = 0x0002000000000000ULL;
       // ...
   public:
       static Value makeInt(int64_t v) {
           Value val;
           val.bits_ = NAN_MASK | TAG_INT | (v & 0xFFFFFFFFULL);
           return val;
       }

       bool isInt() const {
           return (bits_ & NAN_MASK) == (NAN_MASK | TAG_INT);
       }

       int64_t asInt() const {
           return static_cast<int32_t>(bits_ & 0xFFFFFFFFULL);
       }
   };
   ```

9. **Add Stack Maps for GC**
   ```cpp
   struct StackMapEntry {
       size_t offset;  // Bytecode offset
       std::vector<uint32_t> liveObjectSlots;  // Which locals are objects
   };

   // Generated by compiler, used by GC for precise scanning
   ```

10. **Implement Inline Caching**
    ```cpp
    // Cache field offsets at call sites
    struct InlineCache {
        uint32_t lastTypeId;
        uint32_t fieldOffset;
    };

    void VM::execGetFieldCached() {
        int32_t cacheIndex = readInt32();
        InlineCache& cache = inlineCaches_[cacheIndex];

        if (obj->header.typeId == cache.lastTypeId) {
            // Fast path: use cached offset
            push(obj->fields[cache.fieldOffset]);
        } else {
            // Slow path: lookup and update cache
            cache.lastTypeId = obj->header.typeId;
            cache.fieldOffset = lookupField(...);
            push(obj->fields[cache.fieldOffset]);
        }
    }
    ```

#### **Long Term (3+ months)**

11. **JIT Compilation Infrastructure**
    ```cpp
    class JITCompiler {
        // Detect hot functions
        // Compile to native code
        // Tier up from interpreter to JIT
    };
    ```

12. **Concurrent GC**
    ```cpp
    // Separate GC thread
    // Read/write barriers for concurrent marking
    // Pause-less collection
    ```

---

## Critical Bugs

### 🔥🔥🔥 **Severity: Critical - Fix Immediately**

#### 1. Load/Store Opcode Operand Mismatch
- **Component**: VM + Bytecode Compiler
- **Files**:
  - `src/vm/VM.cpp:759-831`
  - `src/bytecode/BytecodeCompiler.cpp:359-409`
- **Problem**: VM reads offset operand that compiler never emits
- **Impact**: Memory operations will crash or read garbage
- **Fix**: See [VM Improvement Tips #1](#critical-fixes-immediately-)

#### 2. Function Call Argument Order Reversed
- **Component**: VM
- **File**: `src/vm/VM.cpp:992-994`
- **Problem**: Arguments stored in reverse order
- **Impact**: All function calls get wrong arguments
- **Fix**: See [VM Improvement Tips #2](#critical-fixes-immediately-)

#### 3. Scope Management Broken
- **Component**: IR Generator
- **File**: `src/IR/IRGenerator.cpp:1070-1072`
- **Problem**: Flat map, no scope stack
- **Impact**: Variable shadowing doesn't work, inner scopes clobber outer
- **Fix**: See [IR Generator Improvement Tips #1](#critical-fixes-immediately)

#### 4. Foreign Function Table Never Built
- **Component**: VM
- **File**: `src/vm/VM.cpp:24`
- **Problem**: Comment says build table but code missing
- **Impact**: Cannot call foreign functions
- **Fix**: See [VM Improvement Tips #3](#critical-fixes-immediately-)

---

### ⚠️ **Severity: High - Fix Soon**

#### 5. Missing Bounds Checks
- **Component**: VM
- **Locations**:
  - Global access: `src/vm/VM.cpp:740-748`
  - GetField: `src/vm/VM.cpp:833-853`
- **Impact**: Out-of-bounds access crashes
- **Fix**: See [VM Improvement Tips #4](#critical-fixes-immediately-)

#### 6. Constant Memory Management
- **Component**: IR Generator
- **File**: `include/IR/IRBuilder.hpp:226-231`
- **Problem**: Constants owned by builder but IR holds raw pointers
- **Impact**: Use-after-free when builder destroyed
- **Fix**: See [IR Generator Improvement Tips #2](#critical-fixes-immediately)

---

### ⚠️ **Severity: Medium - Address in Next Sprint**

#### 7. Primitive Allocation Inconsistency
- **Component**: Bytecode Compiler
- **File**: `src/bytecode/BytecodeCompiler.cpp:329-357`
- **Impact**: Leaky abstraction, confusing memory model
- **Fix**: See [Bytecode Generator Improvement Tips #1](#short-term-1-2-weeks)

#### 8. 350-Line Assignment Handler
- **Component**: IR Generator
- **File**: `src/IR/IRGenerator.cpp:540-654`
- **Impact**: Hard to maintain, bug-prone
- **Fix**: See [IR Generator Improvement Tips #3](#short-term-1-2-weeks)

---

## Improvement Recommendations

### Component Health Matrix

| Component | Code Quality | Test Coverage | Documentation | Architecture |
|-----------|--------------|---------------|---------------|--------------|
| Bytecode | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ |
| IR Generator | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ |
| VM | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ |

### Priority Fixes by Component

#### Bytecode Generator
1. **P0**: Add bytecode verification pass
2. **P1**: Fix primitive allocation hack
3. **P1**: Implement type indexing
4. **P2**: Optimize conditional branches
5. **P2**: Add peephole optimizer

#### IR Generator
1. **P0**: Fix scope management 🔥
2. **P0**: Fix constant ownership 🔥
3. **P1**: Complete if-expression
4. **P1**: Unify assignment L-value handling
5. **P2**: Complete remaining features (match, lambda, etc.)

#### VM
1. **P0**: Fix Load/Store instructions 🔥
2. **P0**: Fix argument transfer order 🔥
3. **P0**: Build foreign function table 🔥
4. **P1**: Add bounds checks everywhere
5. **P1**: Implement debug trace
6. **P2**: NaN-boxing for value representation
7. **P2**: Add inline caching

### Testing Recommendations

#### 1. Add Integration Tests
Current tests are mostly unit tests. Need end-to-end tests:

```cpp
TEST(IntegrationTest, CompileAndRunFibonacci) {
    // Parse source
    auto ast = parser.parse("fn fib(n) { ... }");

    // Generate IR
    auto ir = irGenerator.generate(ast, analyzer);

    // Compile to bytecode
    auto bytecode = compiler.compile(*ir);

    // Execute
    VM vm(bytecode);
    Value result = vm.executeFunction("fib");

    EXPECT_EQ(result.asInt, 55);
}
```

#### 2. Add Fuzzing
```cpp
// Fuzz bytecode verifier
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    CompiledModule module("fuzz");
    module.functions()[0].chunk.code_.assign(data, data + size);

    BytecodeVerifier verifier;
    verifier.verify(module);  // Should not crash
    return 0;
}
```

#### 3. Add Property-Based Tests
```cpp
// Test bytecode serialization round-trip
TEST(PropertyTest, SerializationRoundTrip) {
    for (int i = 0; i < 1000; i++) {
        auto module = generateRandomModule();

        std::ostringstream out;
        serializer.serialize(*module, out);

        std::istringstream in(out.str());
        auto deserialized = serializer.deserialize(in);

        EXPECT_TRUE(modulesEqual(*module, *deserialized));
    }
}
```

### Documentation Recommendations

#### 1. Add Architecture Documentation
Create `docs/architecture.md`:
- Overall system design
- Data flow: Source → AST → IR → Bytecode → VM
- Memory model
- Calling conventions
- GC design

#### 2. Add Developer Guide
Create `docs/developer_guide.md`:
- How to add a new opcode
- How to add a new IR instruction
- How to add a foreign function
- How to run tests
- How to debug

#### 3. Add Bytecode Specification
Create `docs/bytecode_spec.md`:
- Formal specification of each opcode
- Operand formats
- Stack effects
- Type requirements
- Examples

---

## Action Plan

### Phase 1: Critical Stabilization (Week 1-2)

**Goal**: Fix critical bugs preventing execution

#### Week 1
- [ ] Fix Load/Store instructions
  - Day 1-2: Design fix (Option A or B)
  - Day 3: Implement fix
  - Day 4-5: Test and verify

- [ ] Fix function call argument order
  - Day 1: Implement fix
  - Day 2-3: Write regression tests

- [ ] Fix scope management
  - Day 1-2: Implement ScopeManager
  - Day 3-4: Integrate into IRGenerator
  - Day 5: Test nested scopes

#### Week 2
- [ ] Build foreign function table
  - Day 1-2: Implement ForeignFunctionRegistry
  - Day 3: Integrate into VM constructor
  - Day 4-5: Test foreign function calls

- [ ] Add bounds checks
  - Day 1-2: Add to all memory access operations
  - Day 3-5: Write edge case tests

**Deliverable**: Working system with basic programs executing correctly

---

### Phase 2: Feature Completion (Week 3-6)

**Goal**: Complete incomplete features

#### Week 3-4
- [ ] Complete if-expression in IR generator
- [ ] Implement tuple literals
- [ ] Add bytecode verification pass
- [ ] Implement debug trace in VM

#### Week 5-6
- [ ] Fix constant ownership in IRBuilder
- [ ] Unify assignment L-value handling
- [ ] Implement type indexing in bytecode
- [ ] Add integration tests

**Deliverable**: Feature-complete system ready for optimization

---

### Phase 3: Optimization (Week 7-14)

**Goal**: Improve performance and memory usage

#### Week 7-9
- [ ] Implement peephole optimizer
- [ ] Add register allocation to reduce local slots
- [ ] Optimize conditional branches

#### Week 10-12
- [ ] Implement NaN-boxing for values (8-byte values)
- [ ] Add inline caching for field access
- [ ] Implement simple JIT tier-up detection

#### Week 13-14
- [ ] Add IR optimization passes (DCE, constant folding, CSE)
- [ ] Benchmark and profile
- [ ] Optimize hot paths

**Deliverable**: Performant system competitive with other bytecode VMs

---

### Phase 4: Production Hardening (Week 15+)

**Goal**: Make production-ready

#### Ongoing
- [ ] Fuzzing infrastructure
- [ ] Security audit
- [ ] Memory leak detection
- [ ] Stress testing
- [ ] Performance benchmarks
- [ ] Complete documentation

**Deliverable**: Production-ready VM

---

## Conclusion

### Strengths to Leverage

1. **X-Macro Pattern**: Use this pattern elsewhere (IR opcodes, GC object types)
2. **Generational GC**: Solid foundation - build on it
3. **Clean Separation**: Maintain clear boundaries between components
4. **Strong Testing Culture**: Continue comprehensive test coverage

### Weaknesses to Address

1. **Critical Bugs**: Fix immediately - system won't work otherwise
2. **Incomplete Features**: Complete before adding new features
3. **Memory Model**: Clarify and unify (value semantics, reference semantics)
4. **Documentation**: Add architecture docs and developer guides

### Overall Assessment

You have built a **solid foundation** with **good architectural decisions**. The X-macro bytecode design, generational GC, and comprehensive testing show strong engineering. However, **critical implementation bugs** (Load/Store mismatch, argument order, scope management) prevent the system from working correctly.

**Fix the 4 critical bugs in Phase 1**, complete the **incomplete features in Phase 2**, and you'll have a **functional, well-architected VM** ready for optimization. The design is sound enough to support future enhancements like JIT compilation and advanced GC.

### Final Grade: 6.5/10

**Breakdown**:
- Architecture: 8/10 (excellent design)
- Implementation: 5/10 (critical bugs)
- Testing: 8/10 (comprehensive)
- Documentation: 5/10 (needs improvement)

**With critical fixes**: Projected 8/10

---

## Appendix: Quick Reference

### File Locations

```
Bytecode Generator:
  include/bytecode/Bytecode.hpp
  src/bytecode/Bytecode.cpp
  include/bytecode/BytecodeCompiler.hpp
  src/bytecode/BytecodeCompiler.cpp
  include/bytecode/BytecodeSerializer.hpp
  src/bytecode/BytecodeSerializer.cpp
  tests/test_bytecode.cpp

IR Generator:
  include/IR/IR.hpp
  src/IR/IR.cpp
  include/IR/IRBuilder.hpp
  src/IR/IRBuilder.cpp
  include/IR/IRGenerator.hpp
  src/IR/IRGenerator.cpp
  include/IR/IRModule.hpp
  src/IR/IRModule.cpp
  tests/test_ir.cpp

VM:
  include/vm/VM.hpp
  src/vm/VM.cpp
  include/vm/GC.hpp
  src/vm/GC.cpp
  tests/test_vm.cpp
```

### Key Metrics

- **Total Lines of Code**: ~15,000
- **Test Count**: 225+
- **Test Coverage**: ~75% (estimated)
- **Opcode Count**: 60+
- **IR Instruction Types**: 25+
- **Critical Bugs**: 4
- **High-Priority Issues**: 4
- **Medium-Priority Issues**: 10+

### Contact

For questions or clarifications, refer to:
- Architecture decisions: `docs/architecture.md` (to be created)
- Implementation details: Inline code comments
- Testing approach: Test files in `tests/`

---

**End of Report**
