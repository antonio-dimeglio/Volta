# Volta Backend Architecture Design

## Table of Contents
1. [Overview](#overview)
2. [The Three-Stage Backend Pipeline](#the-three-stage-backend-pipeline)
3. [Stage 1: Intermediate Representation (IR)](#stage-1-intermediate-representation-ir)
4. [Stage 2: Bytecode Generation](#stage-2-bytecode-generation)
5. [Stage 3: Virtual Machine](#stage-3-virtual-machine)
6. [Memory Management & Garbage Collection](#memory-management--garbage-collection)
7. [C Interoperability for LAPACK/BLAS](#c-interoperability-for-lapackblas)
8. [Implementation Roadmap](#implementation-roadmap)
9. [Performance Considerations](#performance-considerations)
10. [Further Reading](#further-reading)

---

## Overview

The Volta backend is responsible for taking the semantically-analyzed Abstract Syntax Tree (AST) and executing it. We use a **three-stage pipeline** approach that balances simplicity, performance, and educational value:

```
AST → IR (SSA-like) → Bytecode (Register-based) → VM Execution
```

### Why This Approach?

**For a beginner:**
- Each stage is conceptually isolated and testable
- You can see the transformation at each level (printable IR, disassemblable bytecode)
- Mirrors how real-world compilers work (LLVM, V8, Java HotSpot)

**For performance:**
- IR enables optimizations (constant folding, dead code elimination)
- Register-based bytecode reduces instruction dispatch overhead
- Clean separation allows future JIT compilation

**For LAPACK/BLAS integration:**
- IR can identify array-heavy code for C dispatch
- Bytecode can have specialized instructions for foreign calls
- VM can manage memory layout compatible with C libraries

---

## The Three-Stage Backend Pipeline

### High-Level Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                    Frontend (Already Done!)                     │
│  Source Code → Tokens → AST → Semantic Analysis                │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│ Stage 1: IR Generation                                          │
│  • Convert AST to SSA-like 3-address code                      │
│  • Lower high-level constructs (for loops → while, etc.)       │
│  • Type information flows through (from semantic analysis)     │
│  Output: List of IR instructions (human-readable)             │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│ Stage 2: Bytecode Generation                                    │
│  • Allocate virtual registers for IR temporaries              │
│  • Convert IR instructions to bytecode instructions           │
│  • Build constant pool (strings, floats, function metadata)   │
│  • Encode instructions compactly                              │
│  Output: Bytecode array + metadata                           │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│ Stage 3: Virtual Machine Execution                              │
│  • Fetch-decode-execute loop                                   │
│  • Register-based execution (256 virtual registers)           │
│  • Call stack management                                       │
│  • Garbage collection (mark-and-sweep)                        │
│  • Foreign function interface (C interop)                     │
│  Output: Program results!                                     │
└─────────────────────────────────────────────────────────────────┘
```

---

## Stage 1: Intermediate Representation (IR)

### What is IR?

Think of IR as a "simplified programming language" that sits between your high-level AST and low-level bytecode. It's designed to be:
- **Simple**: Fewer constructs than the full language
- **Explicit**: No hidden operations
- **Analyzable**: Easy to optimize and reason about

### Example Transformation

**Volta Source:**
```volta
fn factorial(n: int) -> int {
    if n <= 1 {
        return 1
    }
    return n * factorial(n - 1)
}
```

**Generated IR (SSA-like 3-address code):**
```
function @factorial(%n: int) -> int:
  bb0:                              // Basic block 0 (entry)
    %0 = le %n, 1                   // %0 = (n <= 1)
    br_if %0, bb1, bb2              // if %0 goto bb1 else bb2

  bb1:                              // Then branch
    ret 1                           // return 1

  bb2:                              // Else branch
    %1 = sub %n, 1                  // %1 = n - 1
    %2 = call @factorial, %1        // %2 = factorial(%1)
    %3 = mul %n, %2                 // %3 = n * %2
    ret %3                          // return %3
```

### Key IR Concepts

#### 1. **Three-Address Code**
Each instruction has at most three operands:
```
%result = operation %operand1, %operand2
```

Examples:
- `%1 = add %2, %3`  (addition)
- `%4 = load [%5]`    (memory load)
- `store %6, [%7]`    (memory store)

**Why?** It's close to how CPUs work (and our register-based VM), making translation easier.

#### 2. **Basic Blocks**
A basic block is a sequence of instructions with:
- **One entry point** (only the first instruction can be jumped to)
- **One exit point** (only the last instruction is a branch/return)
- **No internal branches**

```
bb0:
    %1 = add %a, %b      // Can't jump here
    %2 = mul %1, 2       // Can't jump here
    br_if %2, bb1, bb2   // Exit: conditional branch
```

**Why?** Basic blocks are the unit of optimization. Within a block, you can reorder instructions freely.

#### 3. **SSA Form (Static Single Assignment)**
Each variable is assigned exactly once:

**Non-SSA (bad):**
```
x = 1
x = x + 2
x = x * 3
```

**SSA (good):**
```
x1 = 1
x2 = add x1, 2
x3 = mul x2, 3
```

**Why?** Makes data flow explicit. You can see exactly where each value comes from. Enables powerful optimizations.

**The Phi Node Problem:**
At control flow merge points, we need to know which version of a variable to use:

```volta
let x = 1
if condition {
    x = 2      // Creates x2 in SSA
} else {
    x = 3      // Creates x3 in SSA
}
print(x)       // Which x? x2 or x3?
```

**Full SSA solution** uses "phi nodes":
```
bb_merge:
    x4 = phi [x2, bb_then], [x3, bb_else]
    print(x4)
```

**Our simplified approach** for initial implementation:
- Use fresh variables at each assignment
- Track variable versions in symbol table
- For now, avoid complex control flow merging
- Add phi nodes later if we want advanced optimizations

### IR Instruction Set

Here's the instruction set for Volta IR:

#### Arithmetic & Logic
```
%r = add %a, %b         // Integer/float addition
%r = sub %a, %b         // Subtraction
%r = mul %a, %b         // Multiplication
%r = div %a, %b         // Division
%r = mod %a, %b         // Modulo
%r = neg %a             // Unary negation
%r = and %a, %b         // Logical AND
%r = or %a, %b          // Logical OR
%r = not %a             // Logical NOT
```

#### Comparison
```
%r = eq %a, %b          // Equal
%r = ne %a, %b          // Not equal
%r = lt %a, %b          // Less than
%r = le %a, %b          // Less than or equal
%r = gt %a, %b          // Greater than
%r = ge %a, %b          // Greater than or equal
```

#### Memory Operations
```
%r = load [%addr]       // Load from memory
store %val, [%addr]     // Store to memory
%r = alloc <type>       // Allocate object
%r = get_field %obj, field_idx   // Get struct field
set_field %obj, field_idx, %val  // Set struct field
%r = get_elem %arr, %idx         // Get array element
set_elem %arr, %idx, %val        // Set array element
```

#### Control Flow
```
br label                // Unconditional branch
br_if %cond, then_label, else_label  // Conditional branch
ret                     // Return void
ret %val                // Return value
```

#### Function Calls
```
%r = call @func, %arg1, %arg2, ...  // Direct call
%r = call_foreign @c_func, ...      // C function call
```

#### Constants
```
%r = const_int 42
%r = const_float 3.14
%r = const_bool true
%r = const_string "hello"
%r = const_none
```

### IR Generation Strategy

When converting from AST to IR:

1. **Expressions**: Generate instructions bottom-up, each expression produces a temporary
2. **Statements**: Generate instructions in sequence
3. **Control flow**: Create basic blocks and branches
4. **Functions**: Create new IR function with parameters

**Example: Variable Declaration**
```volta
let x: int = 5 + 3
```
→
```
%0 = const_int 5
%1 = const_int 3
%2 = add %0, %1
store %2, [x]
```

**Example: If Statement**
```volta
if x > 0 {
    y = 1
} else {
    y = 2
}
```
→
```
bb0:
    %0 = load [x]
    %1 = const_int 0
    %2 = gt %0, %1
    br_if %2, bb_then, bb_else

bb_then:
    %3 = const_int 1
    store %3, [y]
    br bb_end

bb_else:
    %4 = const_int 2
    store %4, [y]
    br bb_end

bb_end:
    // Continue...
```

---

## Stage 2: Bytecode Generation

### What is Bytecode?

Bytecode is the "machine code" for our virtual machine. It's:
- **Compact**: Encoded in bytes (hence "bytecode")
- **Fast to decode**: Simple instruction format
- **Platform-independent**: Same bytecode runs on any platform

### Register-Based vs Stack-Based

**Stack-Based (Python, Java pre-JIT):**
```
PUSH 5
PUSH 3
ADD          // Pops 5 and 3, pushes 8
STORE x
```
- Simpler to generate
- More instructions needed
- More memory traffic (stack push/pop)

**Register-Based (Lua 5.0+, our choice):**
```
LOAD_INT R0, 5
LOAD_INT R1, 3
ADD R2, R0, R1
STORE x, R2
```
- Fewer instructions (30-50% reduction)
- Direct addressing
- Faster execution
- More complex compiler

### Bytecode Instruction Format

We use a **variable-length encoding**:

```
┌──────────┬──────────┬──────────┬──────────┬──────────┐
│  Opcode  │   Arg1   │   Arg2   │   Arg3   │  Extra   │
│  (1 byte)│  (varies)│  (varies)│  (varies)│  (varies)│
└──────────┴──────────┴──────────┴──────────┴──────────┘
```

**Registers**: 256 virtual registers (R0-R255), addressed with 1 byte

**Common instruction formats:**

1. **ABC format** (3 registers): `OP RA, RB, RC`
   ```
   ADD R5, R3, R4    // R5 = R3 + R4
   [opcode=ADD][A=5][B=3][C=4]
   ```

2. **ABx format** (2 registers + constant): `OP RA, RB, Kx`
   ```
   LOAD_CONST R2, K10   // R2 = constants[10]
   [opcode=LOAD_CONST][A=2][Bx=10 (2 bytes)]
   ```

3. **Ax format** (large immediate): `OP Ax`
   ```
   JMP +1000
   [opcode=JMP][Ax=1000 (3 bytes)]
   ```

### Bytecode Instruction Set

Here's the core bytecode instruction set (opcodes):

#### Constants & Moves
```
LOAD_CONST   RA, Kx       // RA = constants[Kx]
LOAD_INT     RA, imm      // RA = small integer (-128 to 127)
LOAD_TRUE    RA           // RA = true
LOAD_FALSE   RA           // RA = false
LOAD_NONE    RA           // RA = none
MOVE         RA, RB       // RA = RB
```

#### Arithmetic
```
ADD     RA, RB, RC        // RA = RB + RC
SUB     RA, RB, RC        // RA = RB - RC
MUL     RA, RB, RC        // RA = RB * RC
DIV     RA, RB, RC        // RA = RB / RC
MOD     RA, RB, RC        // RA = RB % RC
NEG     RA, RB            // RA = -RB
```

#### Comparison
```
EQ      RA, RB, RC        // RA = (RB == RC)
NE      RA, RB, RC        // RA = (RB != RC)
LT      RA, RB, RC        // RA = (RB < RC)
LE      RA, RB, RC        // RA = (RB <= RC)
GT      RA, RB, RC        // RA = (RB > RC)
GE      RA, RB, RC        // RA = (RB >= RC)
```

#### Logical
```
AND     RA, RB, RC        // RA = RB && RC
OR      RA, RB, RC        // RA = RB || RC
NOT     RA, RB            // RA = !RB
```

#### Control Flow
```
JMP     offset            // PC += offset
JMP_IF_FALSE RA, offset   // if !RA then PC += offset
JMP_IF_TRUE  RA, offset   // if RA then PC += offset
CALL    RA, func, nargs   // RA = call func with nargs from stack
RETURN  RA                // Return RA
RETURN_NONE               // Return none
```

#### Memory & Data Structures
```
NEW_ARRAY    RA, size     // RA = new array[size]
NEW_STRUCT   RA, type     // RA = new struct(type)
GET_GLOBAL   RA, name     // RA = globals[name]
SET_GLOBAL   name, RA     // globals[name] = RA
GET_LOCAL    RA, idx      // RA = locals[idx]
SET_LOCAL    idx, RA      // locals[idx] = RA
GET_INDEX    RA, RB, RC   // RA = RB[RC]
SET_INDEX    RA, RB, RC   // RA[RB] = RC
GET_FIELD    RA, RB, idx  // RA = RB.field[idx]
SET_FIELD    RA, idx, RB  // RA.field[idx] = RB
```

#### Foreign Function Interface
```
CALL_FFI     RA, ffi_id, nargs  // RA = call C function
```

### Constant Pool

The bytecode file includes a **constant pool** that stores:
- String literals
- Float literals (integers fit in instructions)
- Function metadata (name, arity, local count)
- Type information
- C function signatures

```
Constant Pool:
  K0: "Hello, World!"
  K1: 3.14159
  K2: Function(@factorial, arity=1, locals=3)
  K3: FFI(@dgemm, signature="...")
```

Instructions reference these by index: `LOAD_CONST R0, K1` loads 3.14159 into R0.

### Register Allocation

**Simple approach** (good for start):
- Each IR temporary gets a register
- Function parameters → R0, R1, R2, ...
- Locals → next available registers
- Temporaries → remaining registers
- If > 256 values, spill to memory (later optimization)

**Example:**
```
IR:
  %0 = add %a, %b
  %1 = mul %0, %c
  ret %1

Register mapping:
  %a → R0 (parameter)
  %b → R1 (parameter)
  %c → R2 (parameter)
  %0 → R3 (temporary)
  %1 → R4 (temporary)

Bytecode:
  ADD R3, R0, R1
  MUL R4, R3, R2
  RETURN R4
```

### Bytecode File Format

```
┌─────────────────────────────────────┐
│ Header                              │
│  - Magic number (0x564F4C54 "VOLT") │
│  - Version                          │
│  - Constant pool size               │
├─────────────────────────────────────┤
│ Constant Pool                       │
│  - Strings                          │
│  - Floats                           │
│  - Function metadata                │
│  - FFI signatures                   │
├─────────────────────────────────────┤
│ Code Section                        │
│  - Bytecode instructions            │
│  - Function boundaries              │
├─────────────────────────────────────┤
│ Debug Info (optional)               │
│  - Source line mapping              │
│  - Variable names                   │
└─────────────────────────────────────┘
```

---

## Stage 3: Virtual Machine

### What is a VM?

A Virtual Machine is a program that executes bytecode. It simulates a computer with:
- **Registers**: Fast temporary storage
- **Memory**: Heap for objects, stack for function calls
- **Program Counter (PC)**: Points to current instruction
- **Instruction Decoder**: Reads and executes bytecode

### VM Architecture

```
┌────────────────────────────────────────────────────┐
│                   Volta VM                         │
│                                                    │
│  ┌──────────────┐  ┌──────────────┐              │
│  │   Registers  │  │   Call Stack │              │
│  │  R0 - R255   │  │   Frame 0    │              │
│  │              │  │   Frame 1    │              │
│  │  PC (Program │  │   Frame 2    │              │
│  │   Counter)   │  │     ...      │              │
│  └──────────────┘  └──────────────┘              │
│                                                    │
│  ┌──────────────────────────────────────────────┐ │
│  │              Heap Memory                     │ │
│  │  ┌──────┐ ┌──────┐ ┌──────┐                 │ │
│  │  │Object│ │Array │ │String│  ...            │ │
│  │  └──────┘ └──────┘ └──────┘                 │ │
│  └──────────────────────────────────────────────┘ │
│                                                    │
│  ┌──────────────────────────────────────────────┐ │
│  │        Garbage Collector                     │ │
│  │        (Mark & Sweep)                        │ │
│  └──────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────┘
```

### Execution Loop

The VM's core is a simple loop:

```cpp
void VM::execute() {
    while (true) {
        uint8_t opcode = fetch();      // Read next instruction

        switch (opcode) {
            case OP_ADD: {
                uint8_t ra = fetch();
                uint8_t rb = fetch();
                uint8_t rc = fetch();
                registers[ra] = registers[rb] + registers[rc];
                break;
            }
            case OP_JMP: {
                int16_t offset = fetch_short();
                pc += offset;
                break;
            }
            case OP_RETURN: {
                uint8_t ra = fetch();
                return registers[ra];
            }
            // ... more cases ...
        }
    }
}
```

**This is called a "switch-based dispatch" or "indirect threaded interpreter".**

### Value Representation

How do we store values in registers? We need to handle different types:

**Option 1: Tagged Unions** (simpler, what we'll use)
```cpp
struct Value {
    enum Type { INT, FLOAT, BOOL, OBJECT } type;
    union {
        int64_t i;
        double f;
        bool b;
        Object* obj;  // Pointer to heap object
    } as;
};
```

Each register holds a `Value`. Type checks happen at runtime.

**Option 2: NaN Boxing** (advanced, faster)
Store type in unused bits of float64 (all values are 64-bit):
- Integers: tag + 48-bit int
- Floats: normal IEEE 754
- Objects: pointer with tag bits
- Booleans: special NaN patterns

We'll start with tagged unions for clarity.

### Call Stack

Each function call creates a **stack frame**:

```cpp
struct CallFrame {
    uint8_t* return_address;     // Where to return to
    Value* registers;            // This frame's registers
    int register_count;          // How many registers
    Closure* closure;            // Function being executed
};
```

Stack example:
```
main() calls factorial(5) calls factorial(4)

Stack:
┌──────────────────┐  ← Top
│ factorial(4)     │
│  - return_addr   │
│  - R0 = 4        │
│  - R1, R2, ...   │
├──────────────────┤
│ factorial(5)     │
│  - return_addr   │
│  - R0 = 5        │
│  - R1, R2, ...   │
├──────────────────┤
│ main()           │
│  - return_addr   │
│  - R0, R1, ...   │
└──────────────────┘
```

### VM State

```cpp
class VM {
private:
    // Execution state
    uint8_t* bytecode_;           // The bytecode being executed
    uint8_t* pc_;                 // Program counter
    Value registers_[256];        // Register file

    // Call stack
    std::vector<CallFrame> call_stack_;

    // Global state
    std::unordered_map<std::string, Value> globals_;

    // Memory management
    Heap heap_;
    GarbageCollector gc_;

    // Constant pool
    std::vector<Value> constants_;

    // FFI
    FFIManager ffi_;

public:
    void execute(uint8_t* bytecode);
    Value call_function(Function* func, std::vector<Value> args);
    // ... more methods ...
};
```

---

## Memory Management & Garbage Collection

### Why Garbage Collection?

Manual memory management (malloc/free) is error-prone:
- **Memory leaks**: Forget to free → memory fills up
- **Use-after-free**: Free too early → crashes
- **Double free**: Free twice → corruption

GC automatically reclaims unused memory.

### Mark-and-Sweep Algorithm

This is one of the simplest GC algorithms:

**Phase 1: Mark** (Find reachable objects)
```
1. Start from "roots" (globals, registers, call stack)
2. Mark each reachable object
3. For each marked object, mark objects it references
4. Repeat recursively
```

**Phase 2: Sweep** (Free unreachable objects)
```
1. Walk through all allocated objects
2. If unmarked → free it
3. If marked → unmark for next GC cycle
```

### Example

```
Heap before GC:
  A (used) → B (used) → C (not referenced)
  D (used)
  E (not referenced) → F (not referenced)

Roots: A, D

After Mark phase:
  A (marked) → B (marked) → C (unmarked)
  D (marked)
  E (unmarked) → F (unmarked)

After Sweep phase:
  A → B
  D
  (C, E, F freed)
```

### Object Layout

Every heap object has a header:

```cpp
struct ObjectHeader {
    uint8_t type;        // Array, Struct, String, etc.
    uint8_t marked;      // GC mark bit
    uint16_t size;       // Size in bytes
    Object* next;        // For GC's object list
};

struct Object {
    ObjectHeader header;
    // Type-specific data follows...
};
```

**Array:**
```cpp
struct ArrayObject : Object {
    size_t length;
    Value elements[];  // Flexible array member
};
```

**Struct:**
```cpp
struct StructObject : Object {
    StructType* type;
    Value fields[];
};
```

### GC Triggering

When to run GC?
1. **Allocation threshold**: After allocating N bytes
2. **Out of memory**: When allocation fails
3. **Explicit**: User calls gc.collect()

```cpp
Value* VM::allocate_object(size_t size) {
    if (heap_.bytes_allocated > heap_.next_gc_threshold) {
        collect_garbage();
    }
    return heap_.allocate(size);
}
```

### GC Roots

Where do we start marking?

1. **Registers**: All 256 VM registers
2. **Call stack**: All registers in all frames
3. **Globals**: Global variable table
4. **Upvalues**: Closed-over variables (for closures)

```cpp
void GC::mark_roots() {
    // Mark registers
    for (int i = 0; i < 256; i++) {
        mark_value(&vm.registers_[i]);
    }

    // Mark call stack
    for (auto& frame : vm.call_stack_) {
        for (int i = 0; i < frame.register_count; i++) {
            mark_value(&frame.registers[i]);
        }
    }

    // Mark globals
    for (auto& [name, value] : vm.globals_) {
        mark_value(&value);
    }
}
```

### Memory for BLAS/LAPACK

For C interop, we need **contiguous, packed arrays**:

```cpp
struct PackedFloatArray : Object {
    size_t length;
    float data[];  // C-compatible layout
};
```

When calling BLAS:
```cpp
// Volta: let mat = [1.0, 2.0, 3.0, 4.0]
// C call: dgemm(..., mat.data, ...)

Value result = ffi_call("dgemm", {
    /* Pass mat.data pointer directly */
});
```

---

## C Interoperability for LAPACK/BLAS

### Why FFI?

LAPACK/BLAS are highly optimized C/Fortran libraries for numerical computing. Rewriting them in Volta would be:
- **Slower**: Decades of optimization
- **Buggy**: Numerical stability is tricky
- **Wasteful**: They already exist

FFI (Foreign Function Interface) lets Volta call C functions directly.

### FFI Architecture

```
┌──────────────────────────────────────────────────┐
│              Volta Code                          │
│  let result = blas.dgemm(A, B)                  │
└──────────────────┬───────────────────────────────┘
                   │
                   ▼
┌──────────────────────────────────────────────────┐
│           FFI Manager (Volta VM)                 │
│  • Marshal Volta arrays → C pointers            │
│  • Convert types (Volta float → C double)       │
│  • Handle calling convention                    │
└──────────────────┬───────────────────────────────┘
                   │
                   ▼
┌──────────────────────────────────────────────────┐
│         C Library (liblapack.so)                 │
│  void dgemm_(char*, char*, int*, int*, ...);    │
│  • Execute optimized linear algebra             │
└──────────────────┬───────────────────────────────┘
                   │
                   ▼ (return)
┌──────────────────────────────────────────────────┐
│           FFI Manager                            │
│  • Convert C return → Volta value               │
│  • Update Volta arrays with results             │
└──────────────────┬───────────────────────────────┘
                   │
                   ▼
┌──────────────────────────────────────────────────┐
│              Volta Code                          │
│  // result now contains C return value          │
└──────────────────────────────────────────────────┘
```

### Type Mapping

| Volta Type | C Type | Notes |
|------------|--------|-------|
| `int` | `int64_t` | Always 64-bit |
| `float` | `double` | Always 64-bit float |
| `bool` | `bool` | C99 bool |
| `string` | `char*` | Null-terminated |
| `Array[int]` | `int64_t*` | Pointer to first element |
| `Array[float]` | `double*` | Pointer to first element |
| `none` | `void` | No return value |

### FFI Declaration Syntax (Proposal)

```volta
// Declare external C function
extern "C" fn dgemm(
    transa: string,
    transb: string,
    m: int,
    n: int,
    k: int,
    alpha: float,
    a: Array[float],
    lda: int,
    b: Array[float],
    ldb: int,
    beta: float,
    c: Array[float],
    ldc: int
) -> none;

// Use it like a normal function
let A = [[1.0, 2.0], [3.0, 4.0]]
let B = [[5.0, 6.0], [7.0, 8.0]]
let C = [[0.0, 0.0], [0.0, 0.0]]

dgemm("N", "N", 2, 2, 2, 1.0, A, 2, B, 2, 0.0, C, 2)
// C now contains A * B
```

### FFI Implementation

**1. Registration**
```cpp
class FFIManager {
    struct FFIFunction {
        void* function_ptr;           // dlsym result
        std::vector<Type> param_types;
        Type return_type;
        std::string signature;
    };

    std::unordered_map<std::string, FFIFunction> functions_;

public:
    void register_function(const std::string& name,
                          void* ptr,
                          const std::vector<Type>& params,
                          Type ret_type);
};
```

**2. Loading Shared Libraries**
```cpp
// Use dlopen/dlsym (POSIX) or LoadLibrary (Windows)
void* handle = dlopen("liblapack.so", RTLD_LAZY);
if (!handle) {
    error("Cannot load LAPACK");
}

void* dgemm_ptr = dlsym(handle, "dgemm_");  // Note: Fortran adds _
if (!dgemm_ptr) {
    error("Cannot find dgemm");
}

ffi.register_function("dgemm", dgemm_ptr,
    {Type::String, Type::String, Type::Int, /* ... */},
    Type::Void);
```

**3. Marshaling (Converting Volta → C)**
```cpp
Value FFIManager::call(const std::string& name,
                       const std::vector<Value>& args) {
    FFIFunction& func = functions_[name];

    // Prepare C arguments
    std::vector<void*> c_args;
    for (size_t i = 0; i < args.size(); i++) {
        void* c_arg = marshal_to_c(args[i], func.param_types[i]);
        c_args.push_back(c_arg);
    }

    // Call using libffi or manual assembly
    void* result = invoke_c_function(func.function_ptr, c_args);

    // Convert back to Volta
    return marshal_from_c(result, func.return_type);
}
```

**4. Array Marshaling (Critical for BLAS)**
```cpp
void* marshal_array_to_c(ArrayObject* volta_array) {
    // Volta arrays are already contiguous in memory
    // Just return pointer to first element
    return &volta_array->elements[0];
}
```

**Important**: GC must not move or collect arrays during C call!
```cpp
Value FFIManager::call(const std::string& name,
                       const std::vector<Value>& args) {
    gc_.pause();  // Disable GC during C call

    Value result = do_call(name, args);

    gc_.resume();  // Re-enable GC
    return result;
}
```

### Using libffi Library

Instead of manually handling calling conventions, use `libffi`:

```cpp
#include <ffi.h>

Value call_c_function(void* func_ptr,
                     const std::vector<Value>& args,
                     const FFISignature& sig) {
    ffi_cif cif;
    ffi_type* arg_types[args.size()];
    void* arg_values[args.size()];

    // Setup argument types
    for (size_t i = 0; i < args.size(); i++) {
        arg_types[i] = volta_type_to_ffi_type(sig.param_types[i]);
        arg_values[i] = marshal_to_c(args[i]);
    }

    // Prepare call interface
    ffi_prep_cif(&cif, FFI_DEFAULT_ABI, args.size(),
                 volta_type_to_ffi_type(sig.return_type),
                 arg_types);

    // Call!
    void* result;
    ffi_call(&cif, FFI_FN(func_ptr), &result, arg_values);

    return marshal_from_c(result, sig.return_type);
}
```

### Safety Considerations

FFI is inherently unsafe because C doesn't have:
- Bounds checking
- Type safety
- Memory safety

**Mitigations:**
1. **Type checking**: Semantic analyzer verifies FFI calls match declarations
2. **Array bounds**: Document that C functions must not exceed array bounds
3. **GC pause**: Don't collect during C calls
4. **Documentation**: Clearly mark FFI functions as unsafe

---

## Implementation Roadmap

### Phase 1: IR Generation (Week 1-2)
**Goal**: Convert AST to printable IR

- [ ] Define IR instruction types (enum)
- [ ] Create `IRBuilder` class
- [ ] Implement `IRGenerator` to walk AST
- [ ] Generate IR for expressions (arithmetic, calls, etc.)
- [ ] Generate IR for statements (if, while, return, etc.)
- [ ] Generate IR for functions
- [ ] Basic block creation
- [ ] Print IR in human-readable format
- [ ] Write tests for each AST node type

**Success**: Can print IR for all Volta programs

### Phase 2: Bytecode Generation (Week 2-3)
**Goal**: Convert IR to bytecode

- [ ] Define bytecode opcodes (enum)
- [ ] Implement instruction encoding
- [ ] Create constant pool
- [ ] Implement simple register allocator
- [ ] Generate bytecode for each IR instruction
- [ ] Write bytecode to file
- [ ] Write bytecode disassembler (for debugging!)
- [ ] Write tests comparing IR → bytecode

**Success**: Can generate and disassemble bytecode

### Phase 3: Basic VM (Week 3-5)
**Goal**: Execute simple programs

- [ ] Define `Value` type (tagged union)
- [ ] Implement register file
- [ ] Implement fetch-decode-execute loop
- [ ] Implement arithmetic instructions
- [ ] Implement comparison instructions
- [ ] Implement jump instructions
- [ ] Implement function calls and returns
- [ ] Implement global variables
- [ ] Write VM tests for each instruction

**Success**: Can run factorial, fibonacci, simple arithmetic

### Phase 4: Memory & Objects (Week 5-6)
**Goal**: Support arrays, structs, strings

- [ ] Implement heap allocator
- [ ] Implement array objects
- [ ] Implement struct objects
- [ ] Implement string objects
- [ ] Implement array indexing
- [ ] Implement struct field access
- [ ] Write tests for data structures

**Success**: Can create and manipulate arrays/structs

### Phase 5: Garbage Collection (Week 6-7)
**Goal**: Automatic memory management

- [ ] Implement object header with mark bit
- [ ] Track all allocated objects
- [ ] Implement mark phase (trace from roots)
- [ ] Implement sweep phase (free unmarked)
- [ ] Trigger GC on allocation threshold
- [ ] Write GC stress tests
- [ ] Verify no memory leaks

**Success**: Programs don't leak memory

### Phase 6: C Interop / FFI (Week 7-8)
**Goal**: Call LAPACK/BLAS

- [ ] Integrate libffi
- [ ] Implement FFI function registration
- [ ] Implement argument marshaling
- [ ] Implement return value marshaling
- [ ] Add `extern "C"` syntax to parser
- [ ] Load shared libraries (.so/.dylib/.dll)
- [ ] Test with simple C functions
- [ ] Test with BLAS functions (dgemm, daxpy, etc.)
- [ ] Write LAPACK/BLAS wrapper library

**Success**: Can multiply matrices using BLAS

### Phase 7: Optimization (Week 9-10+)
**Goal**: Make it faster (optional, but fun!)

- [ ] Constant folding in IR
- [ ] Dead code elimination
- [ ] Common subexpression elimination
- [ ] Register allocation optimization
- [ ] Direct threaded dispatch (computed goto)
- [ ] Inline small functions
- [ ] Specialize for numeric types
- [ ] Benchmark and profile

**Success**: 2-5x speedup on numerical code

---

## Performance Considerations

### Expected Performance

**Realistic targets** for this VM architecture:

| Benchmark | vs C | vs Python | Notes |
|-----------|------|-----------|-------|
| Integer arithmetic | 10-20x slower | 1-2x faster | Switch dispatch overhead |
| Float arithmetic | 5-10x slower | 1-2x faster | Similar reasons |
| BLAS operations | ~1x | ~1x | Both call same C library! |
| Array operations | 20-50x slower | 1-3x faster | No SIMD, bounds checks |

**With optimizations:**
- Direct threading: 2x faster
- Type specialization: 1.5x faster
- JIT compilation: 5-10x faster (but complex!)

### Why Not Just Interpret AST?

Direct AST interpretation is 5-10x slower than bytecode because:
1. **More pointer chasing**: AST nodes are scattered in memory
2. **More abstraction overhead**: Virtual dispatch on node types
3. **Harder to optimize**: Can't easily add threaded dispatch
4. **Larger memory footprint**: AST is bigger than bytecode

### Optimization Opportunities

**Easy wins:**
1. **Specialized instructions**: `ADD_INT`, `ADD_FLOAT` instead of generic `ADD`
2. **Inline caching**: Cache method/field lookups
3. **Constant folding**: `2 + 3` → `5` at compile time

**Medium difficulty:**
1. **Direct threading**: Replace switch with computed gotos
2. **Register coalescing**: Reuse registers more efficiently
3. **Peephole optimization**: `LOAD R0, K1; LOAD R0, K2` → `LOAD R0, K2`

**Hard (don't do initially):**
1. **JIT compilation**: Generate native x86/ARM code
2. **Type specialization**: Compile hot paths with known types
3. **Escape analysis**: Allocate on stack instead of heap

---

## Further Reading

### Papers & Articles

**Virtual Machines:**
- "Virtual Machine Showdown: Stack vs Registers" (2001) - Shows register VMs are faster
- "The Implementation of Lua 5.0" (2003) - Excellent register VM design
- "Crafting Interpreters" by Robert Nystrom - Best beginner book on VMs

**Garbage Collection:**
- "The Garbage Collection Handbook" - Comprehensive GC resource
- "Baby's First Garbage Collector" - Simple tutorial implementation

**SSA & IR:**
- "Efficiently Computing Static Single Assignment Form" (1991) - The SSA paper
- "LLVM: A Compilation Framework for Lifelong Program Analysis" - Modern IR design

**FFI:**
- libffi documentation - How to call C from any language
- "Calling Hell: Portable Calling Conventions" - Deep dive on calling conventions

### Open Source Examples

**Similar VMs to study:**
- **Lua 5.0+**: Register-based, clean C code, excellent architecture
- **CPython**: Stack-based, good for comparison
- **Wren**: Small, elegant, modern design
- **Gravity**: C-based, similar goals to Volta

**Tools:**
- **rr (record-replay debugger)**: Essential for debugging GC
- **Valgrind**: Catch memory errors
- **perf**: Profile VM performance

---

## Summary

The Volta backend uses a **three-stage pipeline**:

1. **IR Generation**: AST → SSA-like 3-address code
   - Easy to analyze and optimize
   - Human-readable for debugging

2. **Bytecode Generation**: IR → Register-based bytecode
   - Compact encoding
   - Fast to execute
   - 256 virtual registers

3. **Virtual Machine**: Execute bytecode
   - Switch-based dispatch loop
   - Mark-and-sweep GC
   - FFI for C interop (LAPACK/BLAS)

This approach balances:
- **Simplicity**: Each stage is straightforward
- **Performance**: Register VM + BLAS integration
- **Education**: Learn IR, bytecode, GC, FFI
- **Future-proofing**: Easy to add optimizations

**Start simple**: Get basic execution working first, optimize later!

**Next steps**: Begin with Phase 1 (IR generation) and work through incrementally. Each phase produces testable, debuggable output.

Good luck! 🚀
