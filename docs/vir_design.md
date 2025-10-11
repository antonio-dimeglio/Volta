# Volta Intermediate Representation (VIR) Design

**Version:** 1.0
**Date:** October 2025
**Status:** Design Specification

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Motivation & Goals](#motivation--goals)
3. [Design Principles](#design-principles)
4. [Architecture Overview](#architecture-overview)
5. [Type System](#type-system)
6. [VIR Node Hierarchy](#vir-node-hierarchy)
7. [Expression Nodes](#expression-nodes)
8. [Statement Nodes](#statement-nodes)
9. [Lowering Rules (AST → VIR)](#lowering-rules-ast--vir)
10. [Codegen Rules (VIR → LLVM)](#codegen-rules-vir--llvm)
11. [Edge Cases & Validation](#edge-cases--validation)
12. [Implementation Plan](#implementation-plan)
13. [API Reference](#api-reference)

---

## Executive Summary

This document specifies the **Volta Intermediate Representation (VIR)**, a typed intermediate language that sits between the Volta AST and LLVM IR. VIR makes implicit operations explicit, simplifies LLVM code generation, and provides a clean layer for future optimizations.

### Key Decisions

| Question | Decision | Rationale |
|----------|----------|-----------|
| **Generic instantiation** | Monomorphization during AST→VIR | LLVM optimizes away unused instantiations |
| **Method dispatch** | Desugar to function calls | Cleaner, easier for overloading/traits |
| **Pattern matching** | Keep high-level in VIR | Better error messages, cleaner codegen |
| **Memory model** | Heap-allocated structs, copy semantics | Simple GC integration |
| **Array types** | Two types: `[T; N]` (fixed) and `Array[T]` (dynamic) | Zero-cost fixed arrays, stdlib-implemented dynamic arrays |
| **String literals** | Interned global constants | Performance optimization |
| **Bounds checks** | Explicit `VIRBoundsCheck` nodes | Safety + debuggability |
| **Optimizations** | Trust LLVM | Focus on correctness, not optimization |

---

## Motivation & Goals

### Problems with Direct AST → LLVM Codegen

1. **Ad-hoc wrapper generation:** Each higher-order function needs custom wrapper logic
2. **Implicit operations:** Boxing, type conversions hidden in codegen
3. **Duplication:** Method calls, static methods, functions all handled separately
4. **Scalability:** Adding structs, enums, generics requires patching existing code
5. **Testing:** Hard to test codegen without running LLVM

### What VIR Solves

1. ✅ **Explicit operations:** Boxing, unboxing, casts, method dispatch all visible
2. ✅ **Unified wrapper generation:** One system handles all function adaptations
3. ✅ **Testable:** Can inspect VIR without LLVM
4. ✅ **Incremental migration:** Can mix AST and VIR codegen during transition
5. ✅ **Future-proof:** Easy to add closures, traits, async later

### Non-Goals

- ❌ VIR-level optimizations (trust LLVM)
- ❌ SSA form (too complex initially)
- ❌ Control flow graph representation (tree-shaped is fine)

---

## Design Principles

### 1. **Separation of Concerns**

```
Semantic Analyzer: Type checking, inference, validation
        ↓
VIR Lowering:      Desugaring, monomorphization, explicit operations
        ↓
LLVM Codegen:      Mechanical translation to LLVM IR
```

**Contract:** VIR lowering assumes semantic analyzer has resolved all types.

### 2. **Explicitness Over Cleverness**

Every operation is visible in VIR:

```volta
// Volta source
let arr: Array[i32] = [1, 2, 3]
let doubled := arr.map(double)

// VIR (simplified)
VIRLet("arr", VIRCallRuntime("volta_array_new", ...))
VIRLet("doubled",
    VIRCall("Array_map_i32_i32", [
        VIRLocalRef("arr"),
        VIRWrapFunction("double", UnboxCallBox)
    ])
)
```

**No magic:** If it happens, it's in the VIR.

### 3. **Type Safety**

Every VIR node has a concrete type. No inference needed during codegen.

```cpp
class VIRExpr {
    virtual std::shared_ptr<semantic::Type> getType() const = 0;
};
```

### 4. **Immutability**

VIR nodes are immutable once created. This makes transformations safe and predictable.

### 5. **Source Location Tracking**

Every VIR node tracks its source location for error messages:

```cpp
class VIRNode {
    SourceLocation location;
    const ast::ASTNode* originatingAST;  // For debugging
};
```

---

## Architecture Overview

### Compilation Pipeline

```
┌─────────────────────────────────────────────────────────────┐
│  Volta Source Code                                          │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│  Lexer & Parser  →  AST                                     │
│  - High-level, user-facing representation                   │
│  - Generic types, method calls, implicit operations         │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│  Semantic Analyzer                                          │
│  - Type inference & checking                                │
│  - Symbol resolution                                        │
│  - Generic instantiation tracking                           │
│  OUTPUT: Fully-typed AST + type information                 │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│  VIR Lowering (NEW!)                                        │
│  - Monomorphize generics                                    │
│  - Desugar method calls → function calls                    │
│  - Make boxing/unboxing explicit                            │
│  - Compile pattern matching                                 │
│  - Insert bounds checks                                     │
│  OUTPUT: VIR (explicit, typed, tree-shaped)                 │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│  VIR Validation (NEW!)                                      │
│  - Type checking VIR nodes                                  │
│  - Verify all operations are valid                          │
│  - Check for undefined behavior                             │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│  LLVM Codegen                                               │
│  - Mechanical translation VIR → LLVM IR                     │
│  - No decisions, just instruction emission                  │
│  OUTPUT: LLVM IR Module                                     │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│  LLVM Optimizer & Backend                                   │
│  - Optimizations                                            │
│  - Machine code generation                                  │
│  OUTPUT: Native executable                                  │
└─────────────────────────────────────────────────────────────┘
```

---

## Type System

### VIR Type Representation

VIR uses the **same type system** as the semantic analyzer (`volta::semantic::Type`). No separate type representation.

**Why?**
- Avoids duplicate type logic
- Type information flows seamlessly from semantic analysis
- VIR is a lowering pass, not a separate type system

### Numeric Types (All 12)

```cpp
// VIR must handle all numeric types for FFI and scientific computing
enum class NumericType {
    // Signed integers
    I8, I16, I32, I64,
    // Unsigned integers
    U8, U16, U32, U64,
    // Floating point
    F8, F16, F32, F64
};
```

### Implicit Numeric Conversions

**Rule:** Implicit conversions allowed for simple numeric types in assignments and function calls.

```volta
let x: i32 = 42
let y: f64 = x     // Implicit i32 → f64 (OK)
```

**VIR representation:**
```cpp
VIRLet("y", VIRImplicitCast(VIRLocalRef("x"), i32, f64))
```

**But NOT for generics:**
```volta
let arr: Array[i32] = [1, 2, 3]
fn double(x: f32) -> f32 { return x * 2.0 }
arr.map(double)  // ERROR: Cannot map f32 function over Array[i32]
```

**VIR lowering:** Semantic analyzer catches this, VIR never sees invalid code.

### Monomorphization

**Generic code:**
```volta
struct Box[T] {
    value: T
}

fn Box.get[T](self: Box[T]) -> T {
    return self.value
}

let intBox: Box[i32] = Box { value: 42 }
let floatBox: Box[f64] = Box { value: 3.14 }
```

**VIR lowering creates:**
```
Struct: Box_i32 { value: i32 }
Struct: Box_f64 { value: f64 }
Function: Box_i32_get(self: Box_i32) -> i32
Function: Box_f64_get(self: Box_f64) -> f64
```

**Naming convention:**
- `Box[T]` → `Box_{T}`
- `Box.get[T]` → `Box_{T}_get`
- Multiple params: `Pair[T, U]` → `Pair_{T}_{U}`

### Array Types: Fixed vs Dynamic

Volta provides **two distinct array types** that serve different purposes:

#### 1. Fixed-Size Arrays: `[T; N]`

**Purpose:** Stack-friendly, compile-time sized collections with zero overhead.

**Syntax:**
```volta
let buffer: [i32; 10] = [0; 10]      // Array of 10 i32s initialized to 0
let coords: [f64; 3] = [1.0, 2.0, 3.0]  // Array of 3 f64s with values
```

**Key characteristics:**
- Size `N` is **part of the type**: `[i32; 10]` and `[i32; 20]` are different types
- Size must be a **compile-time constant**
- **Not resizable** - attempting to grow/shrink is a compile error
- **Allocation strategy** (implementation detail, invisible to user):
  - Stack-allocated if: variable is local, size is reasonable, doesn't escape scope
  - Heap-allocated (GC) if: size is large, escapes scope (returned/captured), or user-requested
- Elements stored **inline** (no boxing) for maximum performance
- Ideal for: Fixed buffers, coordinates, small collections, FFI boundaries

**Example usage:**
```volta
fn process_vector() {
    let vec3: [f64; 3] = [x, y, z]      // Stack-allocated (local, small)
    let result = normalize(vec3)         // Pass by reference
    return result                        // Escapes → heap allocation
}

fn read_file_chunk(file: File) -> [u8; 4096] {
    let buffer: [u8; 4096] = [0; 4096]  // Heap! (escapes as return value)
    file.read(buffer)
    return buffer
}
```

#### 2. Dynamic Arrays: `Array[T]`

**Purpose:** Growable, heap-allocated collections implemented in Volta's standard library.

**Syntax:**
```volta
let nums: Array[i32] = Array.new()   // Empty dynamic array
let items: Array[i32] = [1, 2, 3]   // Inferred as Array[i32] from context
```

**Key characteristics:**
- **Always heap-allocated** (GC-managed)
- **Resizable** - can grow and shrink dynamically
- Implemented as a **generic struct in Volta stdlib** (not compiler primitive!)
- Uses fixed arrays `[T]` as backing storage internally
- **Monomorphized** per element type - no boxing overhead
- Rich API: `push()`, `pop()`, `map()`, `filter()`, `reduce()`, etc.
- Ideal for: Collections of unknown size, data that grows, functional programming patterns

**Stdlib implementation (conceptual):**
```volta
// Array[T] is implemented in Volta itself!
struct Array[T] {
    data: [T],        // Reference to heap-allocated fixed array (backing storage)
    length: i32,      // Current number of elements
    capacity: i32     // Total capacity of backing array
}

fn Array.new[T]() -> Array[T] {
    let data = allocate_fixed_array[T](16)  // Runtime helper
    return Array { data: data, length: 0, capacity: 16 }
}

fn Array.push[T](mut self: Array[T], value: T) {
    if self.length == self.capacity {
        self.resize_internal()  // Allocate bigger backing array, copy data
    }
    self.data[self.length] = value
    self.length = self.length + 1
}
```

**How resizing works:**
When `Array[T]` needs to grow:
1. Allocate a new, larger fixed array `[T]` (typically 2x capacity)
2. Copy elements from old backing array to new one
3. Update `data` reference to point to new backing array
4. GC automatically collects the old backing array

This is exactly how `Vec<T>` (Rust) and `std::vector<T>` (C++) work!

#### Comparison Table

| Feature | `[T; N]` Fixed Array | `Array[T]` Dynamic Array |
|---------|---------------------|-------------------------|
| **Size** | Compile-time constant | Runtime dynamic |
| **Resizable** | No | Yes |
| **Allocation** | Stack or heap (compiler decides) | Always heap |
| **Performance** | Zero overhead | Slight overhead (length/capacity tracking) |
| **Implementation** | Compiler built-in | Stdlib generic struct |
| **Monomorphization** | Per type and size | Per element type |
| **Use case** | Fixed buffers, coordinates | Collections, growable lists |

#### Type Inference and Literals

Array literals `[...]` are inferred based on context:

```volta
// Context determines which array type
let fixed: [i32; 3] = [1, 2, 3]        // Fixed array (size in type)
let dynamic: Array[i32] = [1, 2, 3]    // Dynamic array (calls Array.from())

// Error: ambiguous without type annotation
let ambiguous = [1, 2, 3]              // ERROR: Cannot infer array type
```

#### Conversion Between Types

Fixed and dynamic arrays are **distinct types** with no implicit conversion:

```volta
let fixed: [i32; 3] = [1, 2, 3]
let dynamic: Array[i32] = fixed        // ERROR: Type mismatch

// Explicit conversion via stdlib function
let dynamic: Array[i32] = Array.from_fixed(fixed)  // OK
```

#### Nested and Generic Arrays

Both array types support nesting and work with generics:

```volta
// Matrix using fixed arrays (stack-friendly for small matrices)
let matrix: [[f64; 3]; 3] = [
    [1.0, 0.0, 0.0],
    [0.0, 1.0, 0.0],
    [0.0, 0.0, 1.0]
]

// Dynamic array of dynamic arrays
let nested: Array[Array[i32]] = [[1, 2], [3, 4, 5]]

// Generic struct containing both
struct Buffer[T] {
    fixed_part: [T; 16],      // First 16 elements on fast path
    overflow: Array[T]         // Remaining elements if any
}
```

#### VIR Representation

**Fixed arrays** are represented with dedicated VIR nodes:
```cpp
VIRFixedArrayType(elementType=i32, size=10)
VIRFixedArrayLiteral(elements=[...], type=[i32; 10])
VIRFixedArrayGet(array, index, elementType=i32)
```

**Dynamic arrays** use regular struct operations (since they're stdlib structs):
```cpp
// Array.new() → monomorphized function call
VIRCall("Array_i32_new", [], returnType=Array_i32)

// arr.push(x) → method call desugaring
VIRCall("Array_i32_push", [VIRLocalRef("arr"), VIRLocalRef("x")], returnType=unit)
```

This design achieves:
- ✅ Zero-cost fixed arrays for performance-critical code
- ✅ Convenient dynamic arrays implemented in Volta itself
- ✅ No boxing overhead due to monomorphization
- ✅ Clear distinction in the type system
- ✅ Simple implementation strategy (fixed arrays first, then stdlib Array[T])

---

## VIR Node Hierarchy

### Base Classes

```cpp
namespace volta::vir {

// Base class for all VIR nodes
class VIRNode {
public:
    virtual ~VIRNode() = default;

    // Source location for error messages
    SourceLocation getLocation() const { return location_; }

    // Original AST node (for debugging)
    const ast::ASTNode* getOriginatingAST() const { return originatingAST_; }

protected:
    SourceLocation location_;
    const ast::ASTNode* originatingAST_;

    VIRNode(SourceLocation loc, const ast::ASTNode* ast)
        : location_(loc), originatingAST_(ast) {}
};

// Base class for expressions (produce values)
class VIRExpr : public VIRNode {
public:
    // Every expression has a concrete type
    virtual std::shared_ptr<semantic::Type> getType() const = 0;
};

// Base class for statements (produce side effects)
class VIRStmt : public VIRNode {
public:
    // Statements have no value type (return void/unit)
};

} // namespace volta::vir
```

### Node Categories

```
VIRNode
├── VIRExpr (produces value)
│   ├── Literals (VIRIntLiteral, VIRFloatLiteral, ...)
│   ├── References (VIRLocalRef, VIRParamRef, VIRGlobalRef)
│   ├── Operators (VIRBinaryOp, VIRUnaryOp)
│   ├── Calls (VIRCall, VIRCallRuntime, VIRCallIndirect)
│   ├── Memory (VIRAlloca, VIRLoad, VIRStore, VIRGEP)
│   ├── Type Operations (VIRBox, VIRUnbox, VIRCast, VIRImplicitCast)
│   ├── Aggregates (VIRStructNew, VIRStructGet, VIRArrayNew, VIRArrayGet)
│   ├── Control Flow (VIRIf, VIRMatch)
│   └── Advanced (VIRWrapFunction, VIRClosure)
│
└── VIRStmt (produces side effect)
    ├── VIRBlock
    ├── VIRReturn
    ├── VIRVarDecl
    ├── VIRIf (statement form)
    ├── VIRWhile
    ├── VIRFor
    ├── VIRBreak
    ├── VIRContinue
    └── VIRExprStmt
```

---

## Expression Nodes

### Literals

#### VIRIntLiteral

Represents integer literals with explicit type.

```cpp
class VIRIntLiteral : public VIRExpr {
    int64_t value_;
    std::shared_ptr<semantic::Type> type_;  // i8, i16, i32, i64, u8, etc.

public:
    VIRIntLiteral(int64_t value, std::shared_ptr<semantic::Type> type,
                  SourceLocation loc, const ast::ASTNode* ast)
        : VIRExpr(loc, ast), value_(value), type_(type) {}

    int64_t getValue() const { return value_; }
    std::shared_ptr<semantic::Type> getType() const override { return type_; }
};
```

**Lowering:**
```volta
42      // AST: IntegerLiteral(42)
↓
VIRIntLiteral(42, i32)  // Default type from semantic analyzer
```

**Codegen:**
```cpp
llvm::Value* codegen(const VIRIntLiteral* lit) {
    llvm::Type* llvmType = lowerType(lit->getType());
    return llvm::ConstantInt::get(llvmType, lit->getValue());
}
```

#### VIRFloatLiteral

```cpp
class VIRFloatLiteral : public VIRExpr {
    double value_;
    std::shared_ptr<semantic::Type> type_;  // f8, f16, f32, f64

public:
    double getValue() const { return value_; }
    std::shared_ptr<semantic::Type> getType() const override { return type_; }
};
```

#### VIRBoolLiteral

```cpp
class VIRBoolLiteral : public VIRExpr {
    bool value_;

public:
    bool getValue() const { return value_; }
    std::shared_ptr<semantic::Type> getType() const override {
        return semantic::Type::bool_();
    }
};
```

#### VIRStringLiteral

String literals are **interned as global constants** for efficiency.

```cpp
class VIRStringLiteral : public VIRExpr {
    std::string value_;
    std::string internedName_;  // e.g., ".str.0"

public:
    const std::string& getValue() const { return value_; }
    const std::string& getInternedName() const { return internedName_; }

    std::shared_ptr<semantic::Type> getType() const override {
        return semantic::Type::string();
    }
};
```

**Lowering:**
```volta
"Hello"
↓
VIRStringLiteral("Hello", internedName=".str.0")
```

**Codegen:**
```cpp
// Create global constant
GlobalVariable* str0 = builder_->CreateGlobalString("Hello", ".str.0");
// Return pointer to string
return str0;
```

---

### References

#### VIRLocalRef

References a local variable.

```cpp
class VIRLocalRef : public VIRExpr {
    std::string name_;
    std::shared_ptr<semantic::Type> type_;
    bool isMutable_;

public:
    const std::string& getName() const { return name_; }
    bool isMutable() const { return isMutable_; }
    std::shared_ptr<semantic::Type> getType() const override { return type_; }
};
```

#### VIRParamRef

References a function parameter.

```cpp
class VIRParamRef : public VIRExpr {
    std::string name_;
    size_t paramIndex_;  // Position in parameter list
    std::shared_ptr<semantic::Type> type_;

public:
    const std::string& getName() const { return name_; }
    size_t getParamIndex() const { return paramIndex_; }
    std::shared_ptr<semantic::Type> getType() const override { return type_; }
};
```

#### VIRGlobalRef

References a global function or constant.

```cpp
class VIRGlobalRef : public VIRExpr {
    std::string name_;
    std::shared_ptr<semantic::Type> type_;

public:
    const std::string& getName() const { return name_; }
    std::shared_ptr<semantic::Type> getType() const override { return type_; }
};
```

---

### Binary Operations

#### VIRBinaryOp

```cpp
enum class VIRBinaryOpKind {
    // Arithmetic (LLVM-level)
    Add, Sub, Mul, Div, Mod,

    // Comparisons (LLVM-level)
    Eq, Ne, Lt, Le, Gt, Ge,

    // Logical (LLVM-level)
    And, Or,

    // NOTE: No assignment operators here - those become VIRStore
};

class VIRBinaryOp : public VIRExpr {
    VIRBinaryOpKind op_;
    std::unique_ptr<VIRExpr> left_;
    std::unique_ptr<VIRExpr> right_;
    std::shared_ptr<semantic::Type> resultType_;

public:
    VIRBinaryOpKind getOp() const { return op_; }
    const VIRExpr* getLeft() const { return left_.get(); }
    const VIRExpr* getRight() const { return right_.get(); }
    std::shared_ptr<semantic::Type> getType() const override { return resultType_; }
};
```

**Lowering:**
```volta
x + y
↓
// Semantic analyzer ensures types are compatible
// If needed, inserts implicit casts
VIRBinaryOp(Add,
    VIRLocalRef("x", i32),
    VIRLocalRef("y", i32),
    resultType=i32
)
```

**For string concatenation:**
```volta
"Hello" + " World"
↓
VIRCallRuntime("volta_string_concat",
    [VIRStringLiteral("Hello"), VIRStringLiteral(" World")],
    returnType=str
)
```

**Codegen:**
```cpp
llvm::Value* codegen(const VIRBinaryOp* op) {
    llvm::Value* left = codegen(op->getLeft());
    llvm::Value* right = codegen(op->getRight());

    switch (op->getOp()) {
        case Add: return builder_->CreateAdd(left, right);
        case Sub: return builder_->CreateSub(left, right);
        // ... etc
    }
}
```

---

### Unary Operations

```cpp
enum class VIRUnaryOpKind {
    Neg,   // Arithmetic negation: -x
    Not    // Logical not: !x
};

class VIRUnaryOp : public VIRExpr {
    VIRUnaryOpKind op_;
    std::unique_ptr<VIRExpr> operand_;
    std::shared_ptr<semantic::Type> resultType_;

public:
    VIRUnaryOpKind getOp() const { return op_; }
    const VIRExpr* getOperand() const { return operand_.get(); }
    std::shared_ptr<semantic::Type> getType() const override { return resultType_; }
};
```

---

### Type Operations

#### VIRBox

**Explicitly** boxes a value into `void*` for runtime calls.

```cpp
class VIRBox : public VIRExpr {
    std::unique_ptr<VIRExpr> value_;
    std::shared_ptr<semantic::Type> sourceType_;

public:
    const VIRExpr* getValue() const { return value_.get(); }
    std::shared_ptr<semantic::Type> getSourceType() const { return sourceType_; }

    // Box always produces ptr
    std::shared_ptr<semantic::Type> getType() const override {
        return semantic::Type::pointer();
    }
};
```

**Lowering:**
```volta
// When passing to map
arr.map(double)
↓
// Need to box double's argument for runtime
VIRCall("Array_map_i32_i32", [
    VIRLocalRef("arr"),
    VIRWrapFunction(...)  // Wrapper handles boxing internally
])
```

**Codegen:**
```cpp
llvm::Value* codegen(const VIRBox* box) {
    llvm::Value* value = codegen(box->getValue());
    llvm::Type* valueType = value->getType();

    // Allocate memory
    size_t size = dataLayout_->getTypeAllocSize(valueType);
    llvm::Value* sizeVal = llvm::ConstantInt::get(i64Type, size);
    llvm::Value* boxed = builder_->CreateCall(runtime_->getGCAlloc(), {sizeVal});

    // Cast to typed pointer and store
    llvm::Value* typedPtr = builder_->CreateBitCast(boxed,
                                                      llvm::PointerType::get(valueType, 0));
    builder_->CreateStore(value, typedPtr);

    return boxed;
}
```

#### VIRUnbox

**Explicitly** unboxes `void*` into a typed value.

```cpp
class VIRUnbox : public VIRExpr {
    std::unique_ptr<VIRExpr> boxed_;
    std::shared_ptr<semantic::Type> targetType_;

public:
    const VIRExpr* getBoxed() const { return boxed_.get(); }
    std::shared_ptr<semantic::Type> getType() const override { return targetType_; }
};
```

**Codegen:**
```cpp
llvm::Value* codegen(const VIRUnbox* unbox) {
    llvm::Value* boxed = codegen(unbox->getBoxed());
    llvm::Type* targetType = lowerType(unbox->getType());

    // Cast from void* to typed pointer
    llvm::Value* typedPtr = builder_->CreateBitCast(boxed,
                                                      llvm::PointerType::get(targetType, 0));
    // Load the value
    return builder_->CreateLoad(targetType, typedPtr);
}
```

#### VIRCast

**Explicit** type cast with validation (using `as` operator).

```cpp
class VIRCast : public VIRExpr {
    std::unique_ptr<VIRExpr> value_;
    std::shared_ptr<semantic::Type> sourceType_;
    std::shared_ptr<semantic::Type> targetType_;
    CastKind kind_;  // See below

public:
    enum class CastKind {
        IntToInt,        // i32 -> i64 (widening), i64 -> i32 (narrowing)
        IntToFloat,      // i32 -> f32
        FloatToInt,      // f32 -> i32 (truncates)
        FloatToFloat,    // f32 -> f64 (widening), f64 -> f32 (narrowing)
        NoOp             // Same type (optimizer can remove)
    };

    const VIRExpr* getValue() const { return value_.get(); }
    CastKind getCastKind() const { return kind_; }
    std::shared_ptr<semantic::Type> getType() const override { return targetType_; }
};
```

**Lowering:**
```volta
let x: i32 = 42
let y: f64 = x as f64
↓
VIRCast(VIRLocalRef("x"), i32, f64, IntToFloat)
```

**Codegen:**
```cpp
llvm::Value* codegen(const VIRCast* cast) {
    llvm::Value* value = codegen(cast->getValue());
    llvm::Type* targetType = lowerType(cast->getType());

    switch (cast->getCastKind()) {
        case IntToInt:
            return builder_->CreateIntCast(value, targetType, isSigned);
        case IntToFloat:
            return builder_->CreateSIToFP(value, targetType);
        case FloatToInt:
            return builder_->CreateFPToSI(value, targetType);
        case FloatToFloat:
            return builder_->CreateFPCast(value, targetType);
        case NoOp:
            return value;
    }
}
```

#### VIRImplicitCast

Implicit numeric conversions (inserted by semantic analyzer).

```cpp
class VIRImplicitCast : public VIRExpr {
    std::unique_ptr<VIRExpr> value_;
    std::shared_ptr<semantic::Type> sourceType_;
    std::shared_ptr<semantic::Type> targetType_;

    // Same CastKind as VIRCast
    VIRCast::CastKind kind_;

public:
    // Same interface as VIRCast
};
```

**Note:** Codegen is identical to `VIRCast`. The distinction is for clarity in VIR.

---

### Function Calls

#### VIRCall

Direct function call (monomorphized, fully resolved).

```cpp
class VIRCall : public VIRExpr {
    std::string functionName_;  // Monomorphized name: "double" or "Box_i32_get"
    std::vector<std::unique_ptr<VIRExpr>> args_;
    std::shared_ptr<semantic::Type> returnType_;

public:
    const std::string& getFunctionName() const { return functionName_; }
    const std::vector<std::unique_ptr<VIRExpr>>& getArgs() const { return args_; }
    std::shared_ptr<semantic::Type> getType() const override { return returnType_; }
};
```

**Lowering:**
```volta
// Method call (desugared)
p.distance()
↓
VIRCall("Point_distance", [VIRLocalRef("p")], returnType=f64)

// Static method call
Array.new(5)
↓
VIRCall("Array_new_i32", [VIRIntLiteral(5)], returnType=Array[i32])

// Generic function (monomorphized)
unwrap[i32](box)
↓
VIRCall("unwrap_i32", [VIRLocalRef("box")], returnType=i32)
```

**Codegen:**
```cpp
llvm::Value* codegen(const VIRCall* call) {
    // Look up function in module
    llvm::Function* fn = module_->getFunction(call->getFunctionName());

    // Generate arguments
    std::vector<llvm::Value*> args;
    for (const auto& arg : call->getArgs()) {
        args.push_back(codegen(arg.get()));
    }

    // Create call
    return builder_->CreateCall(fn, args);
}
```

#### VIRCallRuntime

Call to Volta runtime function.

```cpp
class VIRCallRuntime : public VIRExpr {
    std::string runtimeFunc_;  // e.g., "volta_array_new", "volta_gc_alloc"
    std::vector<std::unique_ptr<VIRExpr>> args_;
    std::shared_ptr<semantic::Type> returnType_;

public:
    const std::string& getRuntimeFunc() const { return runtimeFunc_; }
    const std::vector<std::unique_ptr<VIRExpr>>& getArgs() const { return args_; }
    std::shared_ptr<semantic::Type> getType() const override { return returnType_; }
};
```

**Lowering:**
```volta
// Array literal
[1, 2, 3]
↓
// Step 1: Create array
VIRCallRuntime("volta_array_new", [VIRIntLiteral(3)], Array[i32])
// Step 2: Push elements (simplified)
VIRCallRuntime("volta_array_push", [arr, VIRBox(VIRIntLiteral(1))])
VIRCallRuntime("volta_array_push", [arr, VIRBox(VIRIntLiteral(2))])
VIRCallRuntime("volta_array_push", [arr, VIRBox(VIRIntLiteral(3))])
```

**Codegen:**
```cpp
llvm::Value* codegen(const VIRCallRuntime* call) {
    // Look up runtime function
    llvm::Function* fn = runtime_->getFunction(call->getRuntimeFunc());

    // Generate arguments
    std::vector<llvm::Value*> args;
    for (const auto& arg : call->getArgs()) {
        args.push_back(codegen(arg.get()));
    }

    return builder_->CreateCall(fn, args);
}
```

#### VIRCallIndirect

Call through function pointer (for closures, higher-order functions).

```cpp
class VIRCallIndirect : public VIRExpr {
    std::unique_ptr<VIRExpr> functionPtr_;  // Evaluates to function pointer
    std::vector<std::unique_ptr<VIRExpr>> args_;
    std::shared_ptr<semantic::Type> returnType_;

public:
    const VIRExpr* getFunctionPtr() const { return functionPtr_.get(); }
    const std::vector<std::unique_ptr<VIRExpr>>& getArgs() const { return args_; }
    std::shared_ptr<semantic::Type> getType() const override { return returnType_; }
};
```

**Note:** Used for closures (unimplemented initially).

---

### Wrapper Generation (Solves the Core Problem!)

#### VIRWrapFunction

**This is the key innovation:** A single node type that handles ALL function adaptation.

```cpp
class VIRWrapFunction : public VIRExpr {
public:
    enum class Strategy {
        // For map: fn(T) -> U becomes ptr(ptr) -> ptr
        // Unbox input, call function, box output
        UnboxCallBox,

        // For filter: fn(T) -> bool becomes ptr(ptr) -> i32
        // Unbox input, call function, convert bool to i32
        UnboxCallBoolToInt,

        // For reduce: fn(Acc, T) -> Acc becomes ptr(ptr, ptr) -> ptr
        // Unbox both inputs, call function, box output
        UnboxUnboxCallBox,

        // Future: More strategies as needed
        // UnboxUnboxUnboxCallBox for 3-arg functions
        // etc.
    };

private:
    std::string originalFunc_;  // Name of function to wrap
    Strategy strategy_;
    std::shared_ptr<semantic::Type> originalSig_;  // Original function type
    std::shared_ptr<semantic::Type> targetSig_;    // Target signature expected by runtime

public:
    const std::string& getOriginalFunc() const { return originalFunc_; }
    Strategy getStrategy() const { return strategy_; }
    std::shared_ptr<semantic::Type> getOriginalSig() const { return originalSig_; }
    std::shared_ptr<semantic::Type> getTargetSig() const { return targetSig_; }

    // Wrapper produces a function pointer
    std::shared_ptr<semantic::Type> getType() const override { return targetSig_; }
};
```

**Lowering:**
```volta
// map
arr.map(double)  // double: fn(i32) -> i32
↓
VIRCall("Array_map_i32_i32", [
    VIRLocalRef("arr"),
    VIRWrapFunction("double", UnboxCallBox,
                    originalSig=fn(i32)->i32,
                    targetSig=fn(ptr)->ptr)
])

// filter
arr.filter(is_even)  // is_even: fn(i32) -> bool
↓
VIRCall("Array_filter_i32", [
    VIRLocalRef("arr"),
    VIRWrapFunction("is_even", UnboxCallBoolToInt,
                    originalSig=fn(i32)->bool,
                    targetSig=fn(ptr)->i32)
])

// reduce (future)
arr.reduce(add, 0)  // add: fn(i32, i32) -> i32
↓
VIRCall("Array_reduce_i32", [
    VIRLocalRef("arr"),
    VIRWrapFunction("add", UnboxUnboxCallBox,
                    originalSig=fn(i32,i32)->i32,
                    targetSig=fn(ptr,ptr)->ptr),
    VIRBox(VIRIntLiteral(0))
])
```

**Codegen:**
```cpp
llvm::Function* codegen(const VIRWrapFunction* wrap) {
    // Look up original function
    llvm::Function* originalFunc = module_->getFunction(wrap->getOriginalFunc());

    // Get type information
    auto originalSig = wrap->getOriginalSig()->asFunctionType();
    auto targetSig = wrap->getTargetSig()->asFunctionType();

    // Create wrapper function
    std::string wrapperName = wrap->getOriginalFunc() + "_wrapper";
    llvm::FunctionType* wrapperFuncType = lowerFunctionType(targetSig);
    llvm::Function* wrapper = llvm::Function::Create(
        wrapperFuncType,
        llvm::Function::InternalLinkage,
        wrapperName,
        module_
    );

    // Generate wrapper body based on strategy
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context_, "entry", wrapper);
    builder_->SetInsertPoint(entry);

    switch (wrap->getStrategy()) {
        case Strategy::UnboxCallBox: {
            // ptr(ptr) -> ptr
            // 1. Unbox input: ptr -> T
            llvm::Value* boxedInput = wrapper->getArg(0);
            llvm::Type* inputType = lowerType(originalSig->getParamType(0));
            llvm::Value* unboxed = createUnbox(boxedInput, inputType);

            // 2. Call original function: T -> U
            llvm::Value* result = builder_->CreateCall(originalFunc, {unboxed});

            // 3. Box output: U -> ptr
            llvm::Value* boxed = createBox(result);

            builder_->CreateRet(boxed);
            break;
        }

        case Strategy::UnboxCallBoolToInt: {
            // ptr(ptr) -> i32
            // 1. Unbox input
            llvm::Value* boxedInput = wrapper->getArg(0);
            llvm::Type* inputType = lowerType(originalSig->getParamType(0));
            llvm::Value* unboxed = createUnbox(boxedInput, inputType);

            // 2. Call original function (returns bool)
            llvm::Value* boolResult = builder_->CreateCall(originalFunc, {unboxed});

            // 3. Convert bool (i1) to i32
            llvm::Value* i32Result = builder_->CreateZExt(boolResult, i32Type);

            builder_->CreateRet(i32Result);
            break;
        }

        case Strategy::UnboxUnboxCallBox: {
            // ptr(ptr, ptr) -> ptr  (for reduce)
            // 1. Unbox both inputs
            llvm::Value* boxedAcc = wrapper->getArg(0);
            llvm::Value* boxedElem = wrapper->getArg(1);
            llvm::Type* accType = lowerType(originalSig->getParamType(0));
            llvm::Type* elemType = lowerType(originalSig->getParamType(1));
            llvm::Value* unboxedAcc = createUnbox(boxedAcc, accType);
            llvm::Value* unboxedElem = createUnbox(boxedElem, elemType);

            // 2. Call original function
            llvm::Value* result = builder_->CreateCall(originalFunc,
                                                        {unboxedAcc, unboxedElem});

            // 3. Box output
            llvm::Value* boxed = createBox(result);

            builder_->CreateRet(boxed);
            break;
        }
    }

    return wrapper;
}
```

**This replaces ALL the ad-hoc wrapper code!** Adding `zip`, `flatMap`, `fold` just means:
1. Add new strategy to enum
2. Add case to codegen switch

---

### Memory Operations

#### VIRAlloca

Stack allocation (for local variables).

```cpp
class VIRAlloca : public VIRExpr {
    std::shared_ptr<semantic::Type> allocatedType_;

public:
    std::shared_ptr<semantic::Type> getAllocatedType() const { return allocatedType_; }

    // Returns pointer to allocated type
    std::shared_ptr<semantic::Type> getType() const override {
        return semantic::Type::pointer();
    }
};
```

**Lowering:**
```volta
let mut x: i32 = 42
↓
// Step 1: Allocate
VIRVarDecl("x", VIRAlloca(i32), isMutable=true)
// Step 2: Store initial value
VIRStore(VIRLocalRef("x"), VIRIntLiteral(42))
```

**Codegen:**
```cpp
llvm::Value* codegen(const VIRAlloca* alloca) {
    llvm::Type* type = lowerType(alloca->getAllocatedType());
    return builder_->CreateAlloca(type);
}
```

#### VIRLoad

Load value from pointer.

```cpp
class VIRLoad : public VIRExpr {
    std::unique_ptr<VIRExpr> pointer_;
    std::shared_ptr<semantic::Type> loadedType_;

public:
    const VIRExpr* getPointer() const { return pointer_.get(); }
    std::shared_ptr<semantic::Type> getType() const override { return loadedType_; }
};
```

#### VIRStore

Store value to pointer.

```cpp
class VIRStore : public VIRExpr {
    std::unique_ptr<VIRExpr> pointer_;
    std::unique_ptr<VIRExpr> value_;

public:
    const VIRExpr* getPointer() const { return pointer_.get(); }
    const VIRExpr* getValue() const { return value_.get(); }

    // Store returns void
    std::shared_ptr<semantic::Type> getType() const override {
        return semantic::Type::unit();
    }
};
```

**Lowering:**
```volta
x = 42
↓
VIRStore(VIRLocalRef("x"), VIRIntLiteral(42))
```

#### VIRGEPFieldAccess

Get Element Pointer for struct field access.

```cpp
class VIRGEPFieldAccess : public VIRExpr {
    std::unique_ptr<VIRExpr> structPtr_;
    std::string fieldName_;
    size_t fieldIndex_;
    std::shared_ptr<semantic::Type> fieldType_;

public:
    const VIRExpr* getStructPtr() const { return structPtr_.get(); }
    const std::string& getFieldName() const { return fieldName_; }
    size_t getFieldIndex() const { return fieldIndex_; }

    // Returns pointer to field
    std::shared_ptr<semantic::Type> getType() const override {
        return semantic::Type::pointer();
    }
};
```

**Lowering:**
```volta
p.x  // Access field
↓
VIRLoad(VIRGEPFieldAccess(VIRLocalRef("p"), "x", fieldIndex=0, f64))
```

**Codegen:**
```cpp
llvm::Value* codegen(const VIRGEPFieldAccess* gep) {
    llvm::Value* structPtr = codegen(gep->getStructPtr());
    llvm::StructType* structType = /* get from type */;

    return builder_->CreateStructGEP(structType, structPtr, gep->getFieldIndex());
}
```

#### VIRGEPArrayAccess

Get element pointer for array indexing (with bounds check).

```cpp
class VIRGEPArrayAccess : public VIRExpr {
    std::unique_ptr<VIRExpr> arrayPtr_;
    std::unique_ptr<VIRExpr> index_;
    std::shared_ptr<semantic::Type> elementType_;

public:
    const VIRExpr* getArrayPtr() const { return arrayPtr_.get(); }
    const VIRExpr* getIndex() const { return index_.get(); }

    // Returns pointer to element
    std::shared_ptr<semantic::Type> getType() const override {
        return semantic::Type::pointer();
    }
};
```

**Note:** Bounds checking handled separately by `VIRBoundsCheck`.

---

### Safety Operations

#### VIRBoundsCheck

**Explicit** bounds checking for arrays.

```cpp
class VIRBoundsCheck : public VIRExpr {
    std::unique_ptr<VIRExpr> array_;
    std::unique_ptr<VIRExpr> index_;

public:
    const VIRExpr* getArray() const { return array_.get(); }
    const VIRExpr* getIndex() const { return index_.get(); }

    // Returns the index if valid, panics otherwise
    std::shared_ptr<semantic::Type> getType() const override {
        return index_->getType();
    }
};
```

**Lowering:**
```volta
arr[i]
↓
// Step 1: Bounds check
VIRBoundsCheck(VIRLocalRef("arr"), VIRLocalRef("i"))
// Step 2: Access (if check passes)
VIRLoad(VIRGEPArrayAccess(arr, i))
```

**Codegen:**
```cpp
llvm::Value* codegen(const VIRBoundsCheck* check) {
    llvm::Value* array = codegen(check->getArray());
    llvm::Value* index = codegen(check->getIndex());

    // Get array length
    llvm::Value* length = builder_->CreateCall(
        runtime_->getArrayLength(), {array}
    );

    // Check: index >= 0 && index < length
    llvm::Value* isNegative = builder_->CreateICmpSLT(index,
                                                       llvm::ConstantInt::get(i64Type, 0));
    llvm::Value* isTooBig = builder_->CreateICmpSGE(index, length);
    llvm::Value* isOutOfBounds = builder_->CreateOr(isNegative, isTooBig);

    // Create panic block
    llvm::BasicBlock* panicBB = llvm::BasicBlock::Create(*context_, "panic");
    llvm::BasicBlock* okBB = llvm::BasicBlock::Create(*context_, "ok");

    builder_->CreateCondBr(isOutOfBounds, panicBB, okBB);

    // Panic block
    builder_->SetInsertPoint(panicBB);
    builder_->CreateCall(runtime_->getPanic(), {
        builder_->CreateGlobalStringPtr("Array index out of bounds")
    });
    builder_->CreateUnreachable();

    // OK block
    builder_->SetInsertPoint(okBB);
    return index;  // Return validated index
}
```

---

### Struct Operations

#### VIRStructNew

Allocate and initialize a struct.

```cpp
class VIRStructNew : public VIRExpr {
    std::string structName_;  // Monomorphized: "Point" or "Box_i32"
    std::vector<std::unique_ptr<VIRExpr>> fieldValues_;
    std::shared_ptr<semantic::Type> structType_;

public:
    const std::string& getStructName() const { return structName_; }
    const std::vector<std::unique_ptr<VIRExpr>>& getFieldValues() const {
        return fieldValues_;
    }
    std::shared_ptr<semantic::Type> getType() const override { return structType_; }
};
```

**Lowering:**
```volta
Point { x: 3.0, y: 4.0 }
↓
VIRStructNew("Point",
    [VIRFloatLiteral(3.0, f64), VIRFloatLiteral(4.0, f64)],
    type=Point
)
```

**Codegen:**
```cpp
llvm::Value* codegen(const VIRStructNew* structNew) {
    // Get struct type
    llvm::StructType* structType = getStructType(structNew->getStructName());

    // Allocate on heap (GC)
    size_t size = dataLayout_->getTypeAllocSize(structType);
    llvm::Value* sizeVal = llvm::ConstantInt::get(i64Type, size);
    llvm::Value* ptr = builder_->CreateCall(runtime_->getGCAlloc(), {sizeVal});

    // Cast to struct pointer
    llvm::Value* structPtr = builder_->CreateBitCast(ptr,
                                                       llvm::PointerType::get(structType, 0));

    // Store each field
    for (size_t i = 0; i < structNew->getFieldValues().size(); i++) {
        llvm::Value* fieldValue = codegen(structNew->getFieldValues()[i].get());
        llvm::Value* fieldPtr = builder_->CreateStructGEP(structType, structPtr, i);
        builder_->CreateStore(fieldValue, fieldPtr);
    }

    return structPtr;
}
```

#### VIRStructGet

Get field value (loads from pointer).

```cpp
class VIRStructGet : public VIRExpr {
    std::unique_ptr<VIRExpr> structPtr_;
    std::string fieldName_;
    size_t fieldIndex_;
    std::shared_ptr<semantic::Type> fieldType_;

public:
    const VIRExpr* getStructPtr() const { return structPtr_.get(); }
    const std::string& getFieldName() const { return fieldName_; }
    size_t getFieldIndex() const { return fieldIndex_; }
    std::shared_ptr<semantic::Type> getType() const override { return fieldType_; }
};
```

**Lowering:**
```volta
p.x
↓
VIRStructGet(VIRLocalRef("p"), "x", fieldIndex=0, type=f64)
```

**Codegen:**
```cpp
llvm::Value* codegen(const VIRStructGet* get) {
    llvm::Value* structPtr = codegen(get->getStructPtr());
    llvm::StructType* structType = /* ... */;

    // GEP to field
    llvm::Value* fieldPtr = builder_->CreateStructGEP(structType, structPtr,
                                                        get->getFieldIndex());

    // Load value
    llvm::Type* fieldType = lowerType(get->getType());
    return builder_->CreateLoad(fieldType, fieldPtr);
}
```

#### VIRStructSet

Set field value (stores to pointer).

```cpp
class VIRStructSet : public VIRExpr {
    std::unique_ptr<VIRExpr> structPtr_;
    std::string fieldName_;
    size_t fieldIndex_;
    std::unique_ptr<VIRExpr> value_;

public:
    const VIRExpr* getStructPtr() const { return structPtr_.get(); }
    const std::string& getFieldName() const { return fieldName_; }
    size_t getFieldIndex() const { return fieldIndex_; }
    const VIRExpr* getValue() const { return value_.get(); }

    // Returns void
    std::shared_ptr<semantic::Type> getType() const override {
        return semantic::Type::unit();
    }
};
```

**Lowering:**
```volta
p.x = 5.0
↓
VIRStructSet(VIRLocalRef("p"), "x", fieldIndex=0, VIRFloatLiteral(5.0))
```

---

### Array Operations

Volta has two array types, each with different VIR representations:
- **Fixed arrays `[T; N]`**: Compiler built-in with dedicated VIR nodes
- **Dynamic arrays `Array[T]`**: Stdlib structs using regular VIR operations

#### Fixed Array Operations

Fixed arrays are compiler primitives with dedicated VIR nodes.

##### VIRFixedArrayNew

Create new fixed-size array.

```cpp
class VIRFixedArrayNew : public VIRExpr {
    std::vector<std::unique_ptr<VIRExpr>> elements_;  // Initial values
    size_t size_;                                      // Compile-time size
    std::shared_ptr<semantic::Type> elementType_;
    bool isStackAllocated_;  // Determined by escape analysis

public:
    const std::vector<std::unique_ptr<VIRExpr>>& getElements() const { return elements_; }
    size_t getSize() const { return size_; }
    std::shared_ptr<semantic::Type> getElementType() const { return elementType_; }
    bool isStackAllocated() const { return isStackAllocated_; }

    std::shared_ptr<semantic::Type> getType() const override {
        return semantic::Type::fixedArray(elementType_, size_);
    }
};
```

**Lowering:**
```volta
// Array with explicit values
let coords: [f64; 3] = [1.0, 2.0, 3.0]
↓
VIRFixedArrayNew(
    elements=[VIRFloatLiteral(1.0), VIRFloatLiteral(2.0), VIRFloatLiteral(3.0)],
    size=3,
    elementType=f64,
    isStackAllocated=true  // Local, doesn't escape
)

// Array with repeated value
let buffer: [u8; 256] = [0; 256]
↓
VIRFixedArrayNew(
    elements=[VIRIntLiteral(0)],  // Single element, repeated 256 times
    size=256,
    elementType=u8,
    isStackAllocated=false  // Too large, heap-allocated
)
```

**Codegen:**
```cpp
llvm::Value* codegen(const VIRFixedArrayNew* arrayNew) {
    llvm::Type* elemType = lowerType(arrayNew->getElementType());
    llvm::ArrayType* arrayType = llvm::ArrayType::get(elemType, arrayNew->getSize());

    if (arrayNew->isStackAllocated()) {
        // Stack allocation
        llvm::Value* arrayPtr = builder_->CreateAlloca(arrayType);

        // Initialize elements
        for (size_t i = 0; i < arrayNew->getSize(); i++) {
            llvm::Value* elemPtr = builder_->CreateConstGEP2_64(arrayType, arrayPtr, 0, i);
            llvm::Value* elemValue = codegen(arrayNew->getElements()[i % arrayNew->getElements().size()]);
            builder_->CreateStore(elemValue, elemPtr);
        }

        return arrayPtr;
    } else {
        // Heap allocation via GC
        size_t arraySize = dataLayout_->getTypeAllocSize(arrayType);
        llvm::Value* memory = builder_->CreateCall(runtime_->getGCAlloc(), {
            llvm::ConstantInt::get(i64Type, arraySize)
        });
        llvm::Value* arrayPtr = builder_->CreateBitCast(memory, llvm::PointerType::get(arrayType, 0));

        // Initialize elements (same as stack case)
        for (size_t i = 0; i < arrayNew->getSize(); i++) {
            llvm::Value* elemPtr = builder_->CreateConstGEP2_64(arrayType, arrayPtr, 0, i);
            llvm::Value* elemValue = codegen(arrayNew->getElements()[i % arrayNew->getElements().size()]);
            builder_->CreateStore(elemValue, elemPtr);
        }

        return arrayPtr;
    }
}
```

##### VIRFixedArrayGet

Index into fixed-size array.

```cpp
class VIRFixedArrayGet : public VIRExpr {
    std::unique_ptr<VIRExpr> array_;
    std::unique_ptr<VIRExpr> index_;
    std::shared_ptr<semantic::Type> elementType_;

public:
    const VIRExpr* getArray() const { return array_.get(); }
    const VIRExpr* getIndex() const { return index_.get(); }
    std::shared_ptr<semantic::Type> getType() const override { return elementType_; }
};
```

**Lowering:**
```volta
let coords: [f64; 3] = [1.0, 2.0, 3.0]
let y = coords[1]
↓
VIRFixedArrayGet(
    VIRLocalRef("coords"),
    VIRBoundsCheck(VIRLocalRef("coords"), VIRIntLiteral(1)),  // Bounds check inserted
    elementType=f64
)
```

**Codegen:**
```cpp
llvm::Value* codegen(const VIRFixedArrayGet* get) {
    llvm::Value* arrayPtr = codegen(get->getArray());
    llvm::Value* index = codegen(get->getIndex());  // Already bounds-checked

    // GEP to element
    llvm::Type* arrayType = arrayPtr->getType()->getPointerElementType();
    llvm::Value* elemPtr = builder_->CreateGEP(arrayType, arrayPtr, {
        llvm::ConstantInt::get(i64Type, 0),
        index
    });

    // Load element
    return builder_->CreateLoad(lowerType(get->getType()), elemPtr);
}
```

##### VIRFixedArraySet

Update element in fixed-size array.

```cpp
class VIRFixedArraySet : public VIRExpr {
    std::unique_ptr<VIRExpr> array_;
    std::unique_ptr<VIRExpr> index_;
    std::unique_ptr<VIRExpr> value_;

public:
    const VIRExpr* getArray() const { return array_.get(); }
    const VIRExpr* getIndex() const { return index_.get(); }
    const VIRExpr* getValue() const { return value_.get(); }

    std::shared_ptr<semantic::Type> getType() const override {
        return semantic::Type::unit();
    }
};
```

**Lowering:**
```volta
coords[1] = 5.0
↓
VIRFixedArraySet(
    VIRLocalRef("coords"),
    VIRBoundsCheck(VIRLocalRef("coords"), VIRIntLiteral(1)),
    VIRFloatLiteral(5.0)
)
```

**Codegen:**
```cpp
llvm::Value* codegen(const VIRFixedArraySet* set) {
    llvm::Value* arrayPtr = codegen(set->getArray());
    llvm::Value* index = codegen(set->getIndex());  // Already bounds-checked
    llvm::Value* value = codegen(set->getValue());

    // GEP to element
    llvm::Type* arrayType = arrayPtr->getType()->getPointerElementType();
    llvm::Value* elemPtr = builder_->CreateGEP(arrayType, arrayPtr, {
        llvm::ConstantInt::get(i64Type, 0),
        index
    });

    // Store new value
    builder_->CreateStore(value, elemPtr);
    return nullptr;  // Unit type
}
```

#### Dynamic Array Operations

Dynamic arrays (`Array[T]`) are **stdlib generic structs**, not compiler primitives. They use standard VIR struct operations.

**Stdlib definition (conceptual):**
```volta
struct Array[T] {
    data: [T],        // Reference to heap-allocated fixed array
    length: i32,
    capacity: i32
}

fn Array.new[T]() -> Array[T] { /* ... */ }
fn Array.push[T](mut self: Array[T], value: T) { /* ... */ }
fn Array.get[T](self: Array[T], index: i32) -> T { /* ... */ }
```

**VIR representation:**
```volta
// User code
let nums: Array[i32] = Array.new()
nums.push(42)
let x = nums.get(0)

// VIR (after monomorphization and method desugaring)
VIRVarDecl("nums",
    VIRCall("Array_i32_new", [], returnType=Array_i32),
    type=Array_i32
)

VIRExprStmt(
    VIRCall("Array_i32_push", [
        VIRLocalRef("nums"),
        VIRIntLiteral(42)
    ], returnType=unit)
)

VIRVarDecl("x",
    VIRCall("Array_i32_get", [
        VIRLocalRef("nums"),
        VIRIntLiteral(0)
    ], returnType=i32),
    type=i32
)
```

**Key insight:** Dynamic arrays don't need special VIR nodes! They're just monomorphized generic structs with method calls. This is possible because:
1. Fixed arrays `[T]` provide the low-level primitive
2. Monomorphization eliminates generic overhead
3. Method desugaring converts `arr.push(x)` to `Array_T_push(arr, x)`

**Higher-order functions:**
```volta
let doubled = nums.map(double)
↓
VIRCall("Array_i32_map_i32", [
    VIRLocalRef("nums"),
    VIRWrapFunction("double", UnboxCallBox)  // Wrapper generation
], returnType=Array_i32)
```

---

### Enum Operations

#### VIREnumNew

Construct enum variant.

```cpp
class VIREnumNew : public VIRExpr {
    std::string enumName_;  // Monomorphized: "Option_i32", "Result_f64_str"
    std::string variantName_;  // "Some", "Ok", "Err", "None"
    size_t variantTag_;  // Discriminant value
    std::vector<std::unique_ptr<VIRExpr>> variantData_;  // Associated data
    std::shared_ptr<semantic::Type> enumType_;

public:
    const std::string& getEnumName() const { return enumName_; }
    const std::string& getVariantName() const { return variantName_; }
    size_t getVariantTag() const { return variantTag_; }
    const std::vector<std::unique_ptr<VIRExpr>>& getVariantData() const {
        return variantData_;
    }
    std::shared_ptr<semantic::Type> getType() const override { return enumType_; }
};
```

**Lowering:**
```volta
Some(42)
↓
VIREnumNew("Option_i32", "Some", tag=0,
           [VIRIntLiteral(42)],
           type=Option[i32])

None
↓
VIREnumNew("Option_i32", "None", tag=1,
           [],
           type=Option[i32])
```

**Codegen:**
```cpp
llvm::Value* codegen(const VIREnumNew* enumNew) {
    // Enum runtime representation:
    // struct VoltaEnum {
    //     int32_t tag;
    //     void* data;  // Boxed variant data
    // };

    // Allocate enum
    llvm::StructType* enumType = getEnumRuntimeType();
    size_t size = dataLayout_->getTypeAllocSize(enumType);
    llvm::Value* enumPtr = builder_->CreateCall(runtime_->getGCAlloc(), {
        llvm::ConstantInt::get(i64Type, size)
    });

    // Cast to enum pointer
    llvm::Value* typedPtr = builder_->CreateBitCast(enumPtr,
                                                      llvm::PointerType::get(enumType, 0));

    // Set tag
    llvm::Value* tagPtr = builder_->CreateStructGEP(enumType, typedPtr, 0);
    builder_->CreateStore(llvm::ConstantInt::get(i32Type, enumNew->getVariantTag()),
                           tagPtr);

    // Box and store variant data (if any)
    if (!enumNew->getVariantData().empty()) {
        // For now: box all data together
        // Future: optimize for single-value variants
        llvm::Value* dataPtr = builder_->CreateStructGEP(enumType, typedPtr, 1);
        llvm::Value* boxedData = boxVariantData(enumNew->getVariantData());
        builder_->CreateStore(boxedData, dataPtr);
    } else {
        // No data (e.g., None)
        llvm::Value* dataPtr = builder_->CreateStructGEP(enumType, typedPtr, 1);
        builder_->CreateStore(llvm::ConstantPointerNull::get(ptrType), dataPtr);
    }

    return typedPtr;
}
```

#### VIREnumGetTag

Get enum discriminant (for pattern matching).

```cpp
class VIREnumGetTag : public VIRExpr {
    std::unique_ptr<VIRExpr> enumPtr_;

public:
    const VIRExpr* getEnumPtr() const { return enumPtr_.get(); }

    std::shared_ptr<semantic::Type> getType() const override {
        return semantic::Type::i32();
    }
};
```

#### VIREnumGetData

Extract data from enum variant.

```cpp
class VIREnumGetData : public VIRExpr {
    std::unique_ptr<VIRExpr> enumPtr_;
    size_t dataIndex_;  // Which field in variant data
    std::shared_ptr<semantic::Type> dataType_;

public:
    const VIRExpr* getEnumPtr() const { return enumPtr_.get(); }
    size_t getDataIndex() const { return dataIndex_; }
    std::shared_ptr<semantic::Type> getType() const override { return dataType_; }
};
```

---

### Control Flow Expressions

#### VIRIf (Expression Form)

If expression that returns a value.

```cpp
class VIRIf : public VIRExpr {
    std::unique_ptr<VIRExpr> condition_;
    std::unique_ptr<VIRBlock> thenBlock_;
    std::unique_ptr<VIRBlock> elseBlock_;  // Required for expression form
    std::shared_ptr<semantic::Type> resultType_;

public:
    const VIRExpr* getCondition() const { return condition_.get(); }
    const VIRBlock* getThenBlock() const { return thenBlock_.get(); }
    const VIRBlock* getElseBlock() const { return elseBlock_.get(); }
    std::shared_ptr<semantic::Type> getType() const override { return resultType_; }
};
```

**Lowering:**
```volta
let x = if cond { 1 } else { 2 }
↓
VIRVarDecl("x",
    VIRIf(VIRLocalRef("cond"),
          VIRBlock([VIRIntLiteral(1)]),
          VIRBlock([VIRIntLiteral(2)]),
          resultType=i32)
)
```

**Codegen:**
```cpp
llvm::Value* codegen(const VIRIf* ifExpr) {
    llvm::Value* cond = codegen(ifExpr->getCondition());

    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*context_, "then");
    llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(*context_, "else");
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*context_, "merge");

    builder_->CreateCondBr(cond, thenBB, elseBB);

    // Then block
    builder_->SetInsertPoint(thenBB);
    llvm::Value* thenVal = codegen(ifExpr->getThenBlock());
    builder_->CreateBr(mergeBB);
    thenBB = builder_->GetInsertBlock();  // May have changed

    // Else block
    builder_->SetInsertPoint(elseBB);
    llvm::Value* elseVal = codegen(ifExpr->getElseBlock());
    builder_->CreateBr(mergeBB);
    elseBB = builder_->GetInsertBlock();

    // Merge block with PHI
    builder_->SetInsertPoint(mergeBB);
    llvm::PHINode* phi = builder_->CreatePHI(thenVal->getType(), 2);
    phi->addIncoming(thenVal, thenBB);
    phi->addIncoming(elseVal, elseBB);

    return phi;
}
```

#### VIRMatch

Pattern matching expression.

```cpp
class VIRMatch : public VIRExpr {
    std::unique_ptr<VIRExpr> scrutinee_;  // Value being matched
    std::vector<VIRMatchArm> arms_;
    std::shared_ptr<semantic::Type> resultType_;

public:
    struct VIRMatchArm {
        std::unique_ptr<VIRPattern> pattern;
        std::unique_ptr<VIRExpr> guard;  // Optional
        std::unique_ptr<VIRBlock> body;
    };

    const VIRExpr* getScrutinee() const { return scrutinee_.get(); }
    const std::vector<VIRMatchArm>& getArms() const { return arms_; }
    std::shared_ptr<semantic::Type> getType() const override { return resultType_; }
};
```

**Pattern types:**

```cpp
class VIRPattern {
public:
    enum class Kind {
        Wildcard,           // _
        Literal,            // 42, "text", true
        Identifier,         // x (binds value)
        EnumVariant,        // Some(x), Ok(value)
        Struct,             // Point { x, y }
        Tuple               // (x, y, z)
    };

    virtual Kind getKind() const = 0;
};

class VIRPatternWildcard : public VIRPattern {
    // Matches anything, doesn't bind
};

class VIRPatternLiteral : public VIRPattern {
    std::unique_ptr<VIRExpr> literal_;  // Must be constant
};

class VIRPatternIdentifier : public VIRPattern {
    std::string name_;  // Variable to bind
    std::shared_ptr<semantic::Type> type_;
};

class VIRPatternEnumVariant : public VIRPattern {
    std::string enumName_;
    std::string variantName_;
    size_t variantTag_;
    std::vector<std::unique_ptr<VIRPattern>> subPatterns_;  // Nested patterns
};
```

**Lowering:**
```volta
match result {
    Ok(value) => value * 2,
    Err(e) => 0
}
↓
VIRMatch(VIRLocalRef("result"), [
    VIRMatchArm(
        VIRPatternEnumVariant("Result_i32_str", "Ok", tag=0,
                              [VIRPatternIdentifier("value", i32)]),
        guard=nullptr,
        VIRBlock([VIRBinaryOp(Mul, VIRLocalRef("value"), VIRIntLiteral(2))])
    ),
    VIRMatchArm(
        VIRPatternEnumVariant("Result_i32_str", "Err", tag=1,
                              [VIRPatternIdentifier("e", str)]),
        guard=nullptr,
        VIRBlock([VIRIntLiteral(0)])
    )
], resultType=i32)
```

**Codegen strategy:**

```cpp
llvm::Value* codegen(const VIRMatch* match) {
    llvm::Value* scrutinee = codegen(match->getScrutinee());

    // For enums: compile to switch on tag
    // For literals: compile to cascading if-else
    // For complex patterns: decision tree

    // Simplified for enum matching:
    llvm::Value* tag = createEnumGetTag(scrutinee);

    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*context_, "match.merge");
    llvm::SwitchInst* switchInst = builder_->CreateSwitch(tag, mergeBB,
                                                            match->getArms().size());

    std::vector<std::pair<llvm::BasicBlock*, llvm::Value*>> resultPairs;

    for (const auto& arm : match->getArms()) {
        llvm::BasicBlock* armBB = llvm::BasicBlock::Create(*context_, "match.arm");

        // Get tag from pattern
        size_t armTag = getPatternTag(arm.pattern.get());
        switchInst->addCase(llvm::ConstantInt::get(i32Type, armTag), armBB);

        builder_->SetInsertPoint(armBB);

        // Bind pattern variables
        bindPattern(arm.pattern.get(), scrutinee);

        // Evaluate guard (if any)
        if (arm.guard) {
            llvm::Value* guardResult = codegen(arm.guard.get());
            llvm::BasicBlock* guardTrueBB = llvm::BasicBlock::Create(*context_, "guard.true");
            llvm::BasicBlock* guardFalseBB = mergeBB;  // Fall through to default
            builder_->CreateCondBr(guardResult, guardTrueBB, guardFalseBB);
            builder_->SetInsertPoint(guardTrueBB);
        }

        // Generate arm body
        llvm::Value* armResult = codegen(arm.body.get());
        builder_->CreateBr(mergeBB);

        llvm::BasicBlock* armEndBB = builder_->GetInsertBlock();
        resultPairs.push_back({armEndBB, armResult});
    }

    // Merge block with PHI
    builder_->SetInsertPoint(mergeBB);
    llvm::PHINode* phi = builder_->CreatePHI(resultPairs[0].second->getType(),
                                               resultPairs.size());
    for (const auto& [bb, val] : resultPairs) {
        phi->addIncoming(val, bb);
    }

    return phi;
}
```

---

### Closure Operations (Unimplemented, Stubbed)

#### VIRClosure

Represents a closure with captured environment.

```cpp
class VIRClosure : public VIRExpr {
    std::unique_ptr<VIRFunction> function_;  // Closure body
    std::vector<CapturedVar> captures_;
    std::shared_ptr<semantic::Type> closureType_;

public:
    struct CapturedVar {
        std::string name;
        std::shared_ptr<semantic::Type> type;
        bool isMutable;
    };

    const VIRFunction* getFunction() const { return function_.get(); }
    const std::vector<CapturedVar>& getCaptures() const { return captures_; }
    std::shared_ptr<semantic::Type> getType() const override { return closureType_; }
};
```

**Note:** Codegen left unimplemented for Phase 1. Stub returns error.

---

## Statement Nodes

### VIRBlock

Sequence of statements.

```cpp
class VIRBlock : public VIRStmt {
    std::vector<std::unique_ptr<VIRStmt>> statements_;

public:
    void addStatement(std::unique_ptr<VIRStmt> stmt) {
        statements_.push_back(std::move(stmt));
    }

    const std::vector<std::unique_ptr<VIRStmt>>& getStatements() const {
        return statements_;
    }
};
```

### VIRVarDecl

Variable declaration with initialization.

```cpp
class VIRVarDecl : public VIRStmt {
    std::string name_;
    std::unique_ptr<VIRExpr> initializer_;
    bool isMutable_;
    std::shared_ptr<semantic::Type> type_;

public:
    const std::string& getName() const { return name_; }
    const VIRExpr* getInitializer() const { return initializer_.get(); }
    bool isMutable() const { return isMutable_; }
    std::shared_ptr<semantic::Type> getType() const { return type_; }
};
```

**Lowering:**
```volta
let mut x: i32 = 42
↓
VIRVarDecl("x", VIRIntLiteral(42), isMutable=true, type=i32)
```

**Codegen:**
```cpp
void codegen(const VIRVarDecl* decl) {
    // Allocate on stack
    llvm::Type* type = lowerType(decl->getType());
    llvm::Value* alloca = builder_->CreateAlloca(type, nullptr, decl->getName());

    // Store in symbol table
    namedValues_[decl->getName()] = alloca;

    // Initialize
    llvm::Value* initValue = codegen(decl->getInitializer());
    builder_->CreateStore(initValue, alloca);
}
```

### VIRReturn

Return statement.

```cpp
class VIRReturn : public VIRStmt {
    std::unique_ptr<VIRExpr> value_;  // nullptr for void return

public:
    const VIRExpr* getValue() const { return value_.get(); }
};
```

### VIRIf (Statement Form)

If statement without return value.

```cpp
class VIRIfStmt : public VIRStmt {
    std::unique_ptr<VIRExpr> condition_;
    std::unique_ptr<VIRBlock> thenBlock_;
    std::unique_ptr<VIRBlock> elseBlock_;  // Optional

public:
    const VIRExpr* getCondition() const { return condition_.get(); }
    const VIRBlock* getThenBlock() const { return thenBlock_.get(); }
    const VIRBlock* getElseBlock() const { return elseBlock_.get(); }
};
```

### VIRWhile

While loop.

```cpp
class VIRWhile : public VIRStmt {
    std::unique_ptr<VIRExpr> condition_;
    std::unique_ptr<VIRBlock> body_;

public:
    const VIRExpr* getCondition() const { return condition_.get(); }
    const VIRBlock* getBody() const { return body_.get(); }
};
```

### VIRFor

For loop (desugared to while with iterator).

```cpp
class VIRFor : public VIRStmt {
    std::string iteratorVar_;
    std::unique_ptr<VIRExpr> iterable_;  // Array or range
    std::unique_ptr<VIRBlock> body_;

public:
    const std::string& getIteratorVar() const { return iteratorVar_; }
    const VIRExpr* getIterable() const { return iterable_.get(); }
    const VIRBlock* getBody() const { return body_.get(); }
};
```

**Lowering:**
```volta
for i in 0..10 {
    print(i)
}
↓
// Desugar to:
// let mut i = 0
// while i < 10 {
//     print(i)
//     i = i + 1
// }

VIRBlock([
    VIRVarDecl("i", VIRIntLiteral(0), isMutable=true),
    VIRWhile(
        VIRBinaryOp(Lt, VIRLocalRef("i"), VIRIntLiteral(10)),
        VIRBlock([
            VIRExprStmt(VIRCall("print", [VIRLocalRef("i")])),
            VIRStore(VIRLocalRef("i"),
                     VIRBinaryOp(Add, VIRLocalRef("i"), VIRIntLiteral(1)))
        ])
    )
])
```

### VIRBreak, VIRContinue

Loop control flow.

```cpp
class VIRBreak : public VIRStmt {};
class VIRContinue : public VIRStmt {};
```

### VIRExprStmt

Expression evaluated for side effects.

```cpp
class VIRExprStmt : public VIRStmt {
    std::unique_ptr<VIRExpr> expr_;

public:
    const VIRExpr* getExpr() const { return expr_.get(); }
};
```

---

### VIRFunction

Top-level function definition.

```cpp
class VIRFunction {
    std::string name_;  // Monomorphized: "double" or "Box_i32_get"
    std::vector<VIRParam> params_;
    std::shared_ptr<semantic::Type> returnType_;
    std::unique_ptr<VIRBlock> body_;
    bool isPublic_;  // For module system (future)

public:
    struct VIRParam {
        std::string name;
        std::shared_ptr<semantic::Type> type;
        bool isMutable;
    };

    const std::string& getName() const { return name_; }
    const std::vector<VIRParam>& getParams() const { return params_; }
    std::shared_ptr<semantic::Type> getReturnType() const { return returnType_; }
    const VIRBlock* getBody() const { return body_.get(); }
    bool isPublic() const { return isPublic_; }
};
```

### VIRModule

Top-level compilation unit.

```cpp
class VIRModule {
    std::string name_;  // Module name (from file)
    std::vector<std::unique_ptr<VIRFunction>> functions_;
    std::vector<std::unique_ptr<VIRStructDecl>> structs_;
    std::vector<std::unique_ptr<VIREnumDecl>> enums_;
    std::vector<std::string> imports_;  // For future module system

public:
    const std::string& getName() const { return name_; }
    const std::vector<std::unique_ptr<VIRFunction>>& getFunctions() const {
        return functions_;
    }
    const std::vector<std::unique_ptr<VIRStructDecl>>& getStructs() const {
        return structs_;
    }
    const std::vector<std::unique_ptr<VIREnumDecl>>& getEnums() const {
        return enums_;
    }
};
```

---

## Lowering Rules (AST → VIR)

### Lowering Pipeline

```cpp
class VIRLowering {
    const ast::Program* program_;
    const semantic::SemanticAnalyzer* analyzer_;
    std::unique_ptr<VIRModule> module_;

public:
    VIRLowering(const ast::Program* program,
                const semantic::SemanticAnalyzer* analyzer)
        : program_(program), analyzer_(analyzer) {}

    std::unique_ptr<VIRModule> lower() {
        module_ = std::make_unique<VIRModule>(/* module name */);

        // Phase 1: Collect all type declarations (structs, enums)
        for (const auto& stmt : program_->statements) {
            if (auto* structDecl = dynamic_cast<ast::StructDeclaration*>(stmt.get())) {
                lowerStructDeclaration(structDecl);
            } else if (auto* enumDecl = dynamic_cast<ast::EnumDeclaration*>(stmt.get())) {
                lowerEnumDeclaration(enumDecl);
            }
        }

        // Phase 2: Monomorphize generic types
        monomorphizeGenerics();

        // Phase 3: Lower functions
        for (const auto& stmt : program_->statements) {
            if (auto* fnDecl = dynamic_cast<ast::FnDeclaration*>(stmt.get())) {
                lowerFunctionDeclaration(fnDecl);
            }
        }

        return std::move(module_);
    }

private:
    void monomorphizeGenerics();
    std::unique_ptr<VIRFunction> lowerFunctionDeclaration(const ast::FnDeclaration* fn);
    std::unique_ptr<VIRStmt> lowerStatement(const ast::Statement* stmt);
    std::unique_ptr<VIRExpr> lowerExpression(const ast::Expression* expr);
};
```

### Generic Monomorphization

```cpp
void VIRLowering::monomorphizeGenerics() {
    // Get all generic instantiations from semantic analyzer
    auto instantiations = analyzer_->getGenericInstantiations();

    for (const auto& [genericDecl, typeArgs] : instantiations) {
        if (auto* structDecl = dynamic_cast<ast::StructDeclaration*>(genericDecl)) {
            // Create monomorphic struct
            std::string monomorphicName = mangleName(structDecl->name, typeArgs);
            auto virStruct = createMonomorphicStruct(structDecl, typeArgs, monomorphicName);
            module_->addStruct(std::move(virStruct));
        } else if (auto* enumDecl = dynamic_cast<ast::EnumDeclaration*>(genericDecl)) {
            std::string monomorphicName = mangleName(enumDecl->name, typeArgs);
            auto virEnum = createMonomorphicEnum(enumDecl, typeArgs, monomorphicName);
            module_->addEnum(std::move(virEnum));
        } else if (auto* fnDecl = dynamic_cast<ast::FnDeclaration*>(genericDecl)) {
            std::string monomorphicName = mangleName(fnDecl->name, typeArgs);
            auto virFunc = createMonomorphicFunction(fnDecl, typeArgs, monomorphicName);
            module_->addFunction(std::move(virFunc));
        }
    }
}

std::string VIRLowering::mangleName(const std::string& baseName,
                                     const std::vector<Type*>& typeArgs) {
    std::string result = baseName;
    for (const auto* type : typeArgs) {
        result += "_" + type->toString();
    }
    return result;
}
```

**Example:**
```volta
struct Box[T] { value: T }
let intBox: Box[i32] = Box { value: 42 }
let floatBox: Box[f64] = Box { value: 3.14 }

// Monomorphization creates:
// Box_i32 { value: i32 }
// Box_f64 { value: f64 }
```

### Method Call Desugaring

```cpp
std::unique_ptr<VIRExpr> VIRLowering::lowerMethodCall(const ast::MethodCallExpression* call) {
    // Method call: object.method(args)
    // Desugar to: Type_method(object, args)

    auto* object = call->object.get();
    auto objectType = analyzer_->getType(object);

    std::string methodName;
    if (objectType->isGeneric()) {
        // Monomorphized method: Box_i32_get
        auto instantiatedType = analyzer_->getInstantiatedType(objectType);
        methodName = mangleName(objectType->getName(), instantiatedType->getTypeArgs());
        methodName += "_" + call->method->name;
    } else {
        // Regular method: Point_distance
        methodName = objectType->getName() + "_" + call->method->name;
    }

    // Build arguments: [object, ...args]
    std::vector<std::unique_ptr<VIRExpr>> virArgs;
    virArgs.push_back(lowerExpression(object));
    for (const auto& arg : call->arguments) {
        virArgs.push_back(lowerExpression(arg.get()));
    }

    return std::make_unique<VIRCall>(
        methodName,
        std::move(virArgs),
        analyzer_->getType(call)
    );
}
```

**Example:**
```volta
p.distance()
↓
VIRCall("Point_distance", [VIRLocalRef("p")], f64)

intBox.get()  // Box[i32]
↓
VIRCall("Box_i32_get", [VIRLocalRef("intBox")], i32)
```

### Array Literal Lowering

```cpp
std::unique_ptr<VIRExpr> VIRLowering::lowerArrayLiteral(const ast::ArrayLiteral* lit) {
    auto elementType = analyzer_->getType(lit)->asArrayType()->getElementType();

    // Create temporary variable for array
    std::string tempName = generateTempName();

    // Generate:
    // let temp = volta_array_new(capacity)
    // volta_array_push(temp, box(elem1))
    // volta_array_push(temp, box(elem2))
    // ...
    // temp

    std::vector<std::unique_ptr<VIRStmt>> stmts;

    // Create array
    auto arrayNew = std::make_unique<VIRCallRuntime>(
        "volta_array_new",
        std::vector<std::unique_ptr<VIRExpr>>{
            std::make_unique<VIRIntLiteral>(lit->elements.size(), semantic::Type::i64())
        },
        analyzer_->getType(lit)
    );

    stmts.push_back(std::make_unique<VIRVarDecl>(
        tempName,
        std::move(arrayNew),
        /*isMutable=*/true,
        analyzer_->getType(lit)
    ));

    // Push each element
    for (const auto& elem : lit->elements) {
        auto loweredElem = lowerExpression(elem.get());
        auto boxed = std::make_unique<VIRBox>(std::move(loweredElem), elementType);

        auto push = std::make_unique<VIRCallRuntime>(
            "volta_array_push",
            std::vector<std::unique_ptr<VIRExpr>>{
                std::make_unique<VIRLocalRef>(tempName, analyzer_->getType(lit)),
                std::move(boxed)
            },
            semantic::Type::unit()
        );

        stmts.push_back(std::make_unique<VIRExprStmt>(std::move(push)));
    }

    // Return array reference
    // Note: This is simplified. In practice, we'd use a VIRBlock expression
    // or insert statements into containing block.
    return std::make_unique<VIRLocalRef>(tempName, analyzer_->getType(lit));
}
```

### For Loop Lowering

```cpp
std::unique_ptr<VIRStmt> VIRLowering::lowerForStatement(const ast::ForStatement* forStmt) {
    auto iterableType = analyzer_->getType(forStmt->iterable.get());

    if (iterableType->isRange()) {
        // For range: 0..10
        return lowerForRange(forStmt);
    } else if (iterableType->isArray()) {
        // For array: for x in arr
        return lowerForArray(forStmt);
    }

    // Error: semantic analyzer should have caught this
    throw std::runtime_error("Invalid iterable type in for loop");
}

std::unique_ptr<VIRStmt> VIRLowering::lowerForRange(const ast::ForStatement* forStmt) {
    // for i in start..end { body }
    // ↓
    // let mut i = start
    // let end_temp = end
    // while i < end_temp {
    //     body
    //     i = i + 1
    // }

    auto* rangeExpr = dynamic_cast<ast::BinaryExpression*>(forStmt->iterable.get());
    bool isInclusive = (rangeExpr->op == ast::BinaryExpression::Operator::RangeInclusive);

    std::vector<std::unique_ptr<VIRStmt>> stmts;

    // let mut i = start
    stmts.push_back(std::make_unique<VIRVarDecl>(
        forStmt->iterator->name,
        lowerExpression(rangeExpr->left.get()),
        /*isMutable=*/true,
        analyzer_->getType(rangeExpr->left.get())
    ));

    // let end_temp = end
    std::string endTempName = generateTempName();
    stmts.push_back(std::make_unique<VIRVarDecl>(
        endTempName,
        lowerExpression(rangeExpr->right.get()),
        /*isMutable=*/false,
        analyzer_->getType(rangeExpr->right.get())
    ));

    // while i < end_temp (or i <= end_temp for inclusive)
    auto condition = std::make_unique<VIRBinaryOp>(
        isInclusive ? VIRBinaryOpKind::Le : VIRBinaryOpKind::Lt,
        std::make_unique<VIRLocalRef>(forStmt->iterator->name,
                                       analyzer_->getType(rangeExpr->left.get())),
        std::make_unique<VIRLocalRef>(endTempName,
                                       analyzer_->getType(rangeExpr->right.get())),
        semantic::Type::bool_()
    );

    // Body + increment
    auto whileBody = std::make_unique<VIRBlock>();

    // Original loop body
    auto loweredBody = lowerStatement(forStmt->body.get());
    whileBody->addStatement(std::move(loweredBody));

    // i = i + 1
    auto increment = std::make_unique<VIRStore>(
        std::make_unique<VIRLocalRef>(forStmt->iterator->name,
                                       analyzer_->getType(rangeExpr->left.get())),
        std::make_unique<VIRBinaryOp>(
            VIRBinaryOpKind::Add,
            std::make_unique<VIRLocalRef>(forStmt->iterator->name,
                                           analyzer_->getType(rangeExpr->left.get())),
            std::make_unique<VIRIntLiteral>(1, analyzer_->getType(rangeExpr->left.get())),
            analyzer_->getType(rangeExpr->left.get())
        )
    );
    whileBody->addStatement(std::make_unique<VIRExprStmt>(std::move(increment)));

    // Create while loop
    stmts.push_back(std::make_unique<VIRWhile>(
        std::move(condition),
        std::move(whileBody)
    ));

    // Wrap in block
    auto block = std::make_unique<VIRBlock>();
    for (auto& stmt : stmts) {
        block->addStatement(std::move(stmt));
    }

    return block;
}
```

### Pattern Matching Lowering

```cpp
std::unique_ptr<VIRExpr> VIRLowering::lowerMatchExpression(const ast::MatchExpression* match) {
    // Keep high-level match in VIR for cleaner codegen

    auto scrutinee = lowerExpression(match->scrutinee.get());
    std::vector<VIRMatch::VIRMatchArm> virArms;

    for (const auto& arm : match->arms) {
        auto pattern = lowerPattern(arm.pattern.get());
        auto guard = arm.guard ? lowerExpression(arm.guard.get()) : nullptr;
        auto body = lowerBlock(arm.body.get());

        virArms.push_back(VIRMatch::VIRMatchArm{
            std::move(pattern),
            std::move(guard),
            std::move(body)
        });
    }

    return std::make_unique<VIRMatch>(
        std::move(scrutinee),
        std::move(virArms),
        analyzer_->getType(match)
    );
}

std::unique_ptr<VIRPattern> VIRLowering::lowerPattern(const ast::Pattern* pattern) {
    // Convert AST pattern to VIR pattern
    // Details depend on pattern type

    switch (pattern->getKind()) {
        case ast::Pattern::Kind::Wildcard:
            return std::make_unique<VIRPatternWildcard>();

        case ast::Pattern::Kind::Literal:
            // ... lower literal pattern

        case ast::Pattern::Kind::EnumVariant: {
            auto* enumPattern = static_cast<const ast::EnumVariantPattern*>(pattern);

            // Get monomorphized enum name
            auto enumType = analyzer_->getEnumType(enumPattern);
            std::string monomorphicName = getMonomorphicName(enumType);

            // Get variant info
            size_t variantTag = analyzer_->getVariantTag(enumPattern->variantName);

            // Lower sub-patterns recursively
            std::vector<std::unique_ptr<VIRPattern>> subPatterns;
            for (const auto& subPat : enumPattern->subPatterns) {
                subPatterns.push_back(lowerPattern(subPat.get()));
            }

            return std::make_unique<VIRPatternEnumVariant>(
                monomorphicName,
                enumPattern->variantName,
                variantTag,
                std::move(subPatterns)
            );
        }

        // ... other pattern types
    }
}
```

---

## Codegen Rules (VIR → LLVM)

### Codegen Architecture

```cpp
class VIRCodegen {
    llvm::LLVMContext* context_;
    llvm::Module* module_;
    llvm::IRBuilder<>* builder_;
    RuntimeFunctions* runtime_;
    const semantic::SemanticAnalyzer* analyzer_;

    // State
    llvm::Function* currentFunction_;
    std::unordered_map<std::string, llvm::Value*> namedValues_;  // Local variables
    std::vector<LoopState> loopStack_;  // For break/continue

public:
    VIRCodegen(llvm::LLVMContext* context,
               llvm::Module* module,
               const semantic::SemanticAnalyzer* analyzer)
        : context_(context),
          module_(module),
          builder_(new llvm::IRBuilder<>(*context)),
          runtime_(new RuntimeFunctions(module, context)),
          analyzer_(analyzer),
          currentFunction_(nullptr) {}

    bool generate(const VIRModule* virModule) {
        // Phase 1: Declare all structs
        for (const auto& structDecl : virModule->getStructs()) {
            declareStruct(structDecl.get());
        }

        // Phase 2: Declare all functions (prototypes)
        for (const auto& func : virModule->getFunctions()) {
            declareFunction(func.get());
        }

        // Phase 3: Generate function bodies
        for (const auto& func : virModule->getFunctions()) {
            generateFunction(func.get());
        }

        return true;
    }

private:
    void declareStruct(const VIRStructDecl* structDecl);
    void declareFunction(const VIRFunction* func);
    void generateFunction(const VIRFunction* func);

    llvm::Value* codegen(const VIRExpr* expr);
    void codegen(const VIRStmt* stmt);

    // Helpers
    llvm::Type* lowerType(std::shared_ptr<semantic::Type> type);
    llvm::Value* createBox(llvm::Value* value, llvm::Type* valueType);
    llvm::Value* createUnbox(llvm::Value* boxed, llvm::Type* targetType);
};
```

### Type Lowering

```cpp
llvm::Type* VIRCodegen::lowerType(std::shared_ptr<semantic::Type> type) {
    if (type->isInt()) {
        switch (type->getIntWidth()) {
            case 8: return llvm::Type::getInt8Ty(*context_);
            case 16: return llvm::Type::getInt16Ty(*context_);
            case 32: return llvm::Type::getInt32Ty(*context_);
            case 64: return llvm::Type::getInt64Ty(*context_);
        }
    } else if (type->isFloat()) {
        switch (type->getFloatWidth()) {
            case 16: return llvm::Type::getHalfTy(*context_);
            case 32: return llvm::Type::getFloatTy(*context_);
            case 64: return llvm::Type::getDoubleTy(*context_);
        }
    } else if (type->isBool()) {
        return llvm::Type::getInt1Ty(*context_);
    } else if (type->isString() || type->isArray() || type->isStruct() || type->isEnum()) {
        // All reference types are pointers
        return llvm::PointerType::getUnqual(*context_);
    }

    throw std::runtime_error("Cannot lower type: " + type->toString());
}
```

### Struct Declaration

```cpp
void VIRCodegen::declareStruct(const VIRStructDecl* structDecl) {
    std::vector<llvm::Type*> fieldTypes;
    for (const auto& field : structDecl->getFields()) {
        fieldTypes.push_back(lowerType(field.type));
    }

    llvm::StructType* structType = llvm::StructType::create(
        *context_,
        fieldTypes,
        structDecl->getName()
    );

    // Store in registry for later lookup
    structTypes_[structDecl->getName()] = structType;
}
```

### Function Generation

```cpp
void VIRCodegen::generateFunction(const VIRFunction* func) {
    // Get or create function
    llvm::Function* llvmFunc = module_->getFunction(func->getName());
    if (!llvmFunc) {
        // Declare if not already done
        declareFunction(func);
        llvmFunc = module_->getFunction(func->getName());
    }

    // Create entry block
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context_, "entry", llvmFunc);
    builder_->SetInsertPoint(entry);

    // Set current function
    currentFunction_ = llvmFunc;
    namedValues_.clear();

    // Create allocas for parameters
    size_t paramIdx = 0;
    for (auto& arg : llvmFunc->args()) {
        const auto& param = func->getParams()[paramIdx];

        // Create alloca
        llvm::AllocaInst* alloca = builder_->CreateAlloca(
            arg.getType(),
            nullptr,
            param.name
        );

        // Store parameter value
        builder_->CreateStore(&arg, alloca);

        // Remember allocation
        namedValues_[param.name] = alloca;

        paramIdx++;
    }

    // Generate body
    codegen(func->getBody());

    // Add return if missing (for void functions)
    if (!builder_->GetInsertBlock()->getTerminator()) {
        if (func->getReturnType()->isUnit()) {
            builder_->CreateRetVoid();
        } else {
            // Error: non-void function missing return
            throw std::runtime_error("Missing return in non-void function");
        }
    }

    // Verify function
    std::string errorMsg;
    llvm::raw_string_ostream errorStream(errorMsg);
    if (llvm::verifyFunction(*llvmFunc, &errorStream)) {
        throw std::runtime_error("Function verification failed: " + errorMsg);
    }
}
```

---

## Edge Cases & Validation

### VIR Validator

```cpp
class VIRValidator {
    std::vector<std::string> errors_;

public:
    bool validate(const VIRModule* module) {
        errors_.clear();

        // Validate all functions
        for (const auto& func : module->getFunctions()) {
            validateFunction(func.get());
        }

        return errors_.empty();
    }

    const std::vector<std::string>& getErrors() const { return errors_; }

private:
    void validateFunction(const VIRFunction* func) {
        // Check all parameters have valid types
        for (const auto& param : func->getParams()) {
            if (!isValidType(param.type)) {
                reportError("Invalid parameter type: " + param.type->toString());
            }
        }

        // Check return type is valid
        if (!isValidType(func->getReturnType())) {
            reportError("Invalid return type: " + func->getReturnType()->toString());
        }

        // Validate body
        validateBlock(func->getBody(), func->getReturnType());
    }

    void validateBlock(const VIRBlock* block,
                       std::shared_ptr<semantic::Type> expectedReturnType) {
        for (const auto& stmt : block->getStatements()) {
            validateStatement(stmt.get(), expectedReturnType);
        }
    }

    void validateStatement(const VIRStmt* stmt,
                           std::shared_ptr<semantic::Type> expectedReturnType) {
        if (auto* ret = dynamic_cast<const VIRReturn*>(stmt)) {
            if (ret->getValue()) {
                auto actualType = ret->getValue()->getType();
                if (!actualType->equals(expectedReturnType)) {
                    reportError("Return type mismatch: expected " +
                                expectedReturnType->toString() +
                                ", got " + actualType->toString());
                }
            } else {
                if (!expectedReturnType->isUnit()) {
                    reportError("Missing return value in non-void function");
                }
            }
        } else if (auto* varDecl = dynamic_cast<const VIRVarDecl*>(stmt)) {
            validateExpression(varDecl->getInitializer());

            auto initType = varDecl->getInitializer()->getType();
            if (!initType->equals(varDecl->getType())) {
                reportError("Variable initialization type mismatch");
            }
        }
        // ... validate other statement types
    }

    void validateExpression(const VIRExpr* expr) {
        if (!expr->getType()) {
            reportError("Expression has no type");
            return;
        }

        if (auto* call = dynamic_cast<const VIRCall*>(expr)) {
            // Validate function exists (would need symbol table)
            // Validate argument types match parameters
            // ...
        } else if (auto* binOp = dynamic_cast<const VIRBinaryOp*>(expr)) {
            validateExpression(binOp->getLeft());
            validateExpression(binOp->getRight());

            auto leftType = binOp->getLeft()->getType();
            auto rightType = binOp->getRight()->getType();

            // Check operand types are compatible
            if (!areCompatibleForOp(leftType, rightType, binOp->getOp())) {
                reportError("Incompatible operand types for binary operation");
            }
        }
        // ... validate other expression types
    }

    void reportError(const std::string& msg) {
        errors_.push_back(msg);
    }

    bool isValidType(std::shared_ptr<semantic::Type> type) {
        // Check type is not null, not unresolved, etc.
        return type != nullptr && !type->isUnknown();
    }

    bool areCompatibleForOp(std::shared_ptr<semantic::Type> left,
                             std::shared_ptr<semantic::Type> right,
                             VIRBinaryOpKind op) {
        // Check if types can be used with this operator
        // ... implementation details
        return true;  // Simplified
    }
};
```

### Common Edge Cases

#### 1. Empty Array Literals

```volta
let arr: Array[i32] = []
```

**Challenge:** Cannot infer element type from empty literal.

**Solution:** Semantic analyzer provides explicit type from annotation.

**VIR:**
```cpp
VIRVarDecl("arr",
    VIRCallRuntime("volta_array_new", [VIRIntLiteral(0)], Array[i32]),
    type=Array[i32]
)
```

#### 2. Nested Generic Types

```volta
let nested: Array[Box[i32]] = [Box { value: 1 }, Box { value: 2 }]
```

**Challenge:** Multiple levels of monomorphization.

**Solution:** Recursively monomorphize inner types first.

**VIR:**
```cpp
// Monomorphized types: Box_i32, Array_Box_i32
VIRVarDecl("nested",
    VIRArrayLiteral([
        VIRStructNew("Box_i32", [VIRIntLiteral(1)]),
        VIRStructNew("Box_i32", [VIRIntLiteral(2)])
    ]),
    type=Array[Box_i32]
)
```

#### 3. Mutable Methods on Immutable Variables

```volta
let counter = Counter { value: 0 }
counter.increment()  // ERROR: counter is immutable
```

**Challenge:** VIR must preserve mutability information.

**Solution:** VIRCall tracks if method requires `mut self`, validation checks variable is mutable.

#### 4. Division by Zero

```volta
let x = 10 / 0
```

**Challenge:** Runtime error, but constant could be caught at compile time.

**Solution:**
- Semantic analyzer warns for constant divisions by zero
- VIR codegen inserts runtime check (optional in release builds)

#### 5. String Literal Deduplication

```volta
let s1 = "Hello"
let s2 = "Hello"
```

**Challenge:** Should both use same global?

**Solution:** VIRStringLiteral interning ensures same interned name.

---

## Implementation Plan

### Phase 1: Core Infrastructure (Week 1-2)

**Goal:** Get VIR infrastructure compiling and testable.

**Tasks:**
1. Create VIR node base classes (`VIRNode`, `VIRExpr`, `VIRStmt`)
2. Implement literal nodes (`VIRIntLiteral`, `VIRFloatLiteral`, etc.)
3. Implement basic binary/unary operations
4. Implement reference nodes (`VIRLocalRef`, `VIRParamRef`)
5. Create `VIRBuilder` helper class for constructing VIR
6. Write unit tests for each node type

**Success Criteria:**
- Can construct VIR nodes programmatically
- All node types compile
- Basic tests pass

### Phase 2: Simple Lowering (Week 2-3)

**Goal:** Lower simple AST nodes to VIR.

**Tasks:**
1. Implement `VIRLowering` class skeleton
2. Lower function declarations (no generics)
3. Lower variable declarations
4. Lower simple expressions (literals, binary ops, variables)
5. Lower simple statements (return, expression statement)
6. Write integration tests (AST → VIR)

**Success Criteria:**
- Can lower simple functions like `fn add(a: i32, b: i32) -> i32 { return a + b }`
- Generated VIR is valid

### Phase 3: Function Calls & Wrappers (Week 3-4)

**Goal:** Implement the core wrapper generation system.

**Tasks:**
1. Implement `VIRCall`, `VIRCallRuntime`
2. Implement `VIRWrapFunction` with strategies
3. Lower method calls to function calls
4. Lower array operations (`map`, `filter`)
5. Implement boxing/unboxing nodes
6. Write tests for wrapper generation

**Success Criteria:**
- `arr.map(double)` lowers correctly
- `arr.filter(is_even)` lowers correctly
- Wrappers handle boxing/unboxing

### Phase 4: Basic Codegen (Week 4-5)

**Goal:** Generate LLVM IR from simple VIR.

**Tasks:**
1. Implement `VIRCodegen` class skeleton
2. Codegen for literals
3. Codegen for binary/unary operations
4. Codegen for function declarations
5. Codegen for function calls
6. Codegen for variables (alloca, load, store)
7. Write end-to-end tests (Volta → VIR → LLVM → Executable)

**Success Criteria:**
- Simple programs compile and run
- `examples/hello.vlt` works through VIR

### Phase 5: Control Flow (Week 5-6)

**Goal:** Lower and codegen if/while/for.

**Tasks:**
1. Implement `VIRIf` (expression and statement forms)
2. Implement `VIRWhile`
3. Implement `VIRFor` lowering (desugar to while)
4. Implement `VIRBreak`, `VIRContinue`
5. Codegen for control flow (basic blocks, branches, PHI nodes)
6. Write tests for loops

**Success Criteria:**
- All loop examples work through VIR
- `examples/for.vlt`, `examples/while.vlt` compile

### Phase 6: Arrays (Week 6-7)

**Goal:** Full array support through VIR.

**Tasks:**
1. Implement array literal lowering
2. Implement array indexing with bounds checks
3. Implement array method calls (`push`, `pop`, `len`)
4. Codegen for arrays
5. Write array-focused tests

**Success Criteria:**
- `examples/array_complex.vlt` works through VIR

### Phase 7: Structs (Week 7-8)

**Goal:** Add struct support.

**Tasks:**
1. Implement `VIRStructNew`, `VIRStructGet`, `VIRStructSet`
2. Lower struct literals
3. Lower field access
4. Codegen for structs (heap allocation, GEP)
5. Write struct tests

**Success Criteria:**
- Can define and use structs
- Nested structs work

### Phase 8: Enums & Pattern Matching (Week 8-10)

**Goal:** Add enum and match support.

**Tasks:**
1. Implement `VIREnumNew`, `VIREnumGetTag`, `VIREnumGetData`
2. Implement pattern nodes
3. Implement `VIRMatch`
4. Lower match expressions
5. Codegen for enums (tag + data representation)
6. Codegen for match (switch on tag, bind variables)
7. Write enum/match tests

**Success Criteria:**
- `Option[T]` and `Result[T, E]` work
- Pattern matching compiles correctly

### Phase 9: Generics (Week 10-12)

**Goal:** Monomorphization system.

**Tasks:**
1. Implement generic instantiation tracking
2. Implement monomorphization in lowering
3. Handle generic structs
4. Handle generic enums
5. Handle generic functions
6. Write generics tests

**Success Criteria:**
- `Box[T]` works for multiple types
- Generic methods work

### Phase 10: Validation & Testing (Week 12-13)

**Goal:** Ensure VIR is robust.

**Tasks:**
1. Implement `VIRValidator`
2. Add validation to lowering pipeline
3. Write comprehensive test suite
4. Fix all edge cases
5. Performance testing

**Success Criteria:**
- All existing examples work through VIR
- No regressions
- VIR validator catches invalid IR

### Phase 11: Migration (Week 13-14)

**Goal:** Replace old codegen with VIR codegen.

**Tasks:**
1. Update main compiler driver to use VIR pipeline
2. Remove old AST → LLVM codegen
3. Update documentation
4. Final testing

**Success Criteria:**
- All tests pass
- No old codegen remains

---

## API Reference

### VIRBuilder - Helper for Constructing VIR

```cpp
class VIRBuilder {
    const semantic::SemanticAnalyzer* analyzer_;

public:
    VIRBuilder(const semantic::SemanticAnalyzer* analyzer)
        : analyzer_(analyzer) {}

    // Literals
    std::unique_ptr<VIRIntLiteral> createIntLiteral(
        int64_t value,
        std::shared_ptr<semantic::Type> type,
        SourceLocation loc
    ) {
        return std::make_unique<VIRIntLiteral>(value, type, loc, nullptr);
    }

    std::unique_ptr<VIRFloatLiteral> createFloatLiteral(
        double value,
        std::shared_ptr<semantic::Type> type,
        SourceLocation loc
    ) {
        return std::make_unique<VIRFloatLiteral>(value, type, loc, nullptr);
    }

    // Binary operations
    std::unique_ptr<VIRBinaryOp> createAdd(
        std::unique_ptr<VIRExpr> left,
        std::unique_ptr<VIRExpr> right,
        SourceLocation loc
    ) {
        auto resultType = left->getType();  // Assume same type
        return std::make_unique<VIRBinaryOp>(
            VIRBinaryOpKind::Add,
            std::move(left),
            std::move(right),
            resultType,
            loc,
            nullptr
        );
    }

    // Function calls
    std::unique_ptr<VIRCall> createCall(
        const std::string& functionName,
        std::vector<std::unique_ptr<VIRExpr>> args,
        std::shared_ptr<semantic::Type> returnType,
        SourceLocation loc
    ) {
        return std::make_unique<VIRCall>(
            functionName,
            std::move(args),
            returnType,
            loc,
            nullptr
        );
    }

    // Wrapper generation
    std::unique_ptr<VIRWrapFunction> createMapWrapper(
        const std::string& functionName,
        std::shared_ptr<semantic::Type> originalSig,
        SourceLocation loc
    ) {
        // Target signature for map: fn(ptr) -> ptr
        auto targetSig = semantic::Type::function({semantic::Type::pointer()},
                                                   semantic::Type::pointer());

        return std::make_unique<VIRWrapFunction>(
            functionName,
            VIRWrapFunction::Strategy::UnboxCallBox,
            originalSig,
            targetSig,
            loc,
            nullptr
        );
    }

    std::unique_ptr<VIRWrapFunction> createFilterWrapper(
        const std::string& functionName,
        std::shared_ptr<semantic::Type> originalSig,
        SourceLocation loc
    ) {
        // Target signature for filter: fn(ptr) -> i32
        auto targetSig = semantic::Type::function({semantic::Type::pointer()},
                                                   semantic::Type::i32());

        return std::make_unique<VIRWrapFunction>(
            functionName,
            VIRWrapFunction::Strategy::UnboxCallBoolToInt,
            originalSig,
            targetSig,
            loc,
            nullptr
        );
    }

    // Control flow
    std::unique_ptr<VIRIf> createIfExpr(
        std::unique_ptr<VIRExpr> condition,
        std::unique_ptr<VIRBlock> thenBlock,
        std::unique_ptr<VIRBlock> elseBlock,
        std::shared_ptr<semantic::Type> resultType,
        SourceLocation loc
    ) {
        return std::make_unique<VIRIf>(
            std::move(condition),
            std::move(thenBlock),
            std::move(elseBlock),
            resultType,
            loc,
            nullptr
        );
    }

    // Structs
    std::unique_ptr<VIRStructNew> createStructNew(
        const std::string& structName,
        std::vector<std::unique_ptr<VIRExpr>> fieldValues,
        std::shared_ptr<semantic::Type> structType,
        SourceLocation loc
    ) {
        return std::make_unique<VIRStructNew>(
            structName,
            std::move(fieldValues),
            structType,
            loc,
            nullptr
        );
    }

    std::unique_ptr<VIRStructGet> createStructGet(
        std::unique_ptr<VIRExpr> structPtr,
        const std::string& fieldName,
        size_t fieldIndex,
        std::shared_ptr<semantic::Type> fieldType,
        SourceLocation loc
    ) {
        return std::make_unique<VIRStructGet>(
            std::move(structPtr),
            fieldName,
            fieldIndex,
            fieldType,
            loc,
            nullptr
        );
    }

    // Arrays
    std::unique_ptr<VIRArrayNew> createArrayNew(
        std::unique_ptr<VIRExpr> capacity,
        std::shared_ptr<semantic::Type> elementType,
        SourceLocation loc
    ) {
        return std::make_unique<VIRArrayNew>(
            std::move(capacity),
            elementType,
            loc,
            nullptr
        );
    }

    std::unique_ptr<VIRBoundsCheck> createBoundsCheck(
        std::unique_ptr<VIRExpr> array,
        std::unique_ptr<VIRExpr> index,
        SourceLocation loc
    ) {
        return std::make_unique<VIRBoundsCheck>(
            std::move(array),
            std::move(index),
            loc,
            nullptr
        );
    }

    // Type operations
    std::unique_ptr<VIRBox> createBox(
        std::unique_ptr<VIRExpr> value,
        SourceLocation loc
    ) {
        auto sourceType = value->getType();
        return std::make_unique<VIRBox>(
            std::move(value),
            sourceType,
            loc,
            nullptr
        );
    }

    std::unique_ptr<VIRUnbox> createUnbox(
        std::unique_ptr<VIRExpr> boxed,
        std::shared_ptr<semantic::Type> targetType,
        SourceLocation loc
    ) {
        return std::make_unique<VIRUnbox>(
            std::move(boxed),
            targetType,
            loc,
            nullptr
        );
    }
};
```

---

## Conclusion

This VIR design provides:

1. ✅ **Explicit operations** - No hidden magic
2. ✅ **Unified wrapper generation** - Solves the core scalability problem
3. ✅ **Clean separation** - Semantic analysis → VIR lowering → LLVM codegen
4. ✅ **Testability** - Can inspect and validate VIR
5. ✅ **Extensibility** - Easy to add closures, traits, async later
6. ✅ **Type safety** - Every node knows its type
7. ✅ **Validation** - Catches errors early
8. ✅ **Documentation** - Every operation is well-defined

**Next Steps:**
1. Review this design document
2. Clarify any questions
3. Begin Phase 1 implementation
4. Iterate based on real-world usage

---

**Document Prepared By:** Claude
**Review Status:** Awaiting feedback
**Last Updated:** October 10, 2025
