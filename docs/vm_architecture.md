# Volta Virtual Machine Architecture

## Table of Contents
1. [Overview](#overview)
2. [Design Philosophy](#design-philosophy)
3. [VM Components](#vm-components)
4. [Value Representation](#value-representation)
5. [Register File & Execution State](#register-file--execution-state)
6. [Memory Layout](#memory-layout)
7. [Bytecode Instruction Set](#bytecode-instruction-set)
8. [Execution Model](#execution-model)
9. [Function Calls & Stack Frames](#function-calls--stack-frames)
10. [Garbage Collection](#garbage-collection)
11. [FFI System](#ffi-system)
12. [Implementation Guide](#implementation-guide)

---

## Overview

The Volta VM is a **register-based virtual machine** designed for executing Volta bytecode efficiently. It features:

- **256 virtual registers** per stack frame
- **Mark-and-sweep garbage collection**
- **Foreign Function Interface (FFI)** for C interop
- **Tagged union value representation** (simple and debuggable)
- **Switch-based dispatch** (upgradeable to direct threading)

### Key Design Goals

1. **Performance**: Register-based design reduces instruction count by 30-50% vs stack-based
2. **Simplicity**: Clean separation of concerns, easy to debug and extend
3. **Memory Safety**: Automatic GC prevents leaks and use-after-free
4. **Interoperability**: Native FFI support for LAPACK/BLAS integration
5. **Debuggability**: Clear instruction formats, inspectable state

---

## Design Philosophy

### Why Register-Based?

**Stack-Based VM** (Python, early Java):
```
PUSH 5
PUSH 3
ADD
STORE x
```
- Simple to generate
- Many instructions
- Stack thrashing

**Register-Based VM** (Lua, Volta):
```
LOAD_INT R0, 5
LOAD_INT R1, 3
ADD R2, R0, R1
STORE_GLOBAL x, R2
```
- Fewer instructions (30-50% reduction)
- Direct data flow
- Better cache locality
- Slightly more complex codegen

### Why Mark-and-Sweep GC?

**Simplicity**: Easiest GC algorithm to implement correctly
**Sufficient**: Works well for our use case (not real-time)
**Educational**: Clear phases, easy to understand and debug
**Future**: Can upgrade to generational/incremental later

**Alternatives considered**:
- Reference counting: Cyclic reference issues, overhead on every assignment
- Copying GC: Requires compaction, moves objects (breaks FFI pointers)
- Generational: More complex, better for later optimization

---

## VM Components

```
┌─────────────────────────────────────────────────────────────┐
│                        Volta VM                             │
│                                                             │
│  ┌──────────────────────────┐  ┌──────────────────────┐   │
│  │   Execution Engine       │  │   Memory Manager     │   │
│  │  ┌────────────────────┐  │  │  ┌────────────────┐ │   │
│  │  │ Registers (R0-R255)│  │  │  │     Heap       │ │   │
│  │  └────────────────────┘  │  │  │  ┌──────────┐  │ │   │
│  │  ┌────────────────────┐  │  │  │  │ Objects  │  │ │   │
│  │  │  Program Counter   │  │  │  │  │ Arrays   │  │ │   │
│  │  │       (PC)         │  │  │  │  │ Structs  │  │ │   │
│  │  └────────────────────┘  │  │  │  │ Strings  │  │ │   │
│  │  ┌────────────────────┐  │  │  │  └──────────┘  │ │   │
│  │  │   Call Stack       │  │  │  └────────────────┘ │   │
│  │  │   Frame 0 (main)   │  │  │  ┌────────────────┐ │   │
│  │  │   Frame 1 (fn1)    │  │  │  │ GC (Mark/Sweep)│ │   │
│  │  │   Frame 2 (fn2)    │  │  │  └────────────────┘ │   │
│  │  └────────────────────┘  │  └──────────────────────┘   │
│  └──────────────────────────┘                             │
│                                                             │
│  ┌──────────────────────────┐  ┌──────────────────────┐   │
│  │   Constant Pool          │  │   FFI Manager        │   │
│  │  • Strings               │  │  • C Function Table  │   │
│  │  • Float Constants       │  │  • Type Marshaling   │   │
│  │  • Function Metadata     │  │  • dlopen/dlsym      │   │
│  └──────────────────────────┘  └──────────────────────┘   │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │              Bytecode (Instruction Stream)           │  │
│  │  [OP][ARGS...] [OP][ARGS...] [OP][ARGS...] ...     │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

---

## Value Representation

Every value in the VM is represented by a **tagged union**:

```cpp
enum class ValueType : uint8_t {
    NONE,       // None/null value
    BOOL,       // Boolean
    INT,        // 64-bit integer
    FLOAT,      // 64-bit double
    OBJECT      // Heap-allocated object (pointer)
};

struct Value {
    ValueType type;
    union {
        bool b;
        int64_t i;
        double f;
        Object* obj;
    } as;
};
```

### Type Checking

Operations check types at runtime:
```cpp
Value add(Value a, Value b) {
    if (a.type == ValueType::INT && b.type == ValueType::INT) {
        return Value::int_value(a.as.i + b.as.i);
    } else if (a.type == ValueType::FLOAT || b.type == ValueType::FLOAT) {
        return Value::float_value(to_float(a) + to_float(b));
    } else {
        runtime_error("Invalid types for addition");
    }
}
```

### Object Layout

All heap-allocated objects share a common header:

```cpp
enum class ObjectType : uint8_t {
    STRING,
    ARRAY,
    STRUCT,
    CLOSURE,
    NATIVE_FUNCTION
};

struct Object {
    ObjectType type;        // What kind of object
    bool marked;            // GC mark bit
    Object* next;           // For GC linked list

    // Type-specific data follows...
};
```

**String Object:**
```cpp
struct StringObject : Object {
    size_t length;
    size_t hash;           // Cached for fast comparison
    char data[];           // Flexible array member
};
```

**Array Object:**
```cpp
struct ArrayObject : Object {
    size_t length;
    size_t capacity;       // For dynamic growth
    Value elements[];      // Flexible array member
};
```

**Struct Object:**
```cpp
struct StructObject : Object {
    size_t field_count;
    Value fields[];        // Fields stored inline
};
```

**Closure Object (for functions):**
```cpp
struct ClosureObject : Object {
    Function* function;    // Function metadata
    size_t upvalue_count;  // For closures (future)
    Value upvalues[];      // Captured variables
};
```

---

## Register File & Execution State

### Register File

Each stack frame has **256 virtual registers** (R0-R255):

```cpp
struct CallFrame {
    Value registers[256];           // Register file
    uint8_t* return_address;        // Where to return
    ClosureObject* closure;         // Current function
    Value* stack_start;             // For varargs (future)
};
```

**Register Allocation Convention:**
- **R0-R(n-1)**: Function parameters (n = parameter count)
- **R(n)-R(m)**: Local variables (m = n + local_count)
- **R(m+1)-R255**: Temporary values for expressions

**Example:**
```volta
fn add_and_mul(a: int, b: int, c: int) -> int {
    let sum = a + b      // Local variable
    return sum * c
}
```
```
R0 = a (parameter 0)
R1 = b (parameter 1)
R2 = c (parameter 2)
R3 = sum (local 0)
R4 = temp for sum * c
```

### VM Execution State

```cpp
class VM {
private:
    // Current execution
    CallFrame* current_frame;           // Active stack frame
    std::vector<CallFrame> call_stack;  // Function call stack
    uint8_t* ip;                        // Instruction pointer

    // Global state
    std::unordered_map<std::string, Value> globals;

    // Memory management
    Object* objects;                    // Linked list of all objects
    size_t bytes_allocated;
    size_t next_gc_threshold;

    // Constants
    std::vector<Value> constants;

    // FFI
    FFIManager ffi;
};
```

---

## Memory Layout

### Heap Organization

The heap is a simple **freelist allocator** with objects linked for GC:

```
Heap Memory:
┌─────────────────────────────────────────────────┐
│  Object 1 (String)                              │
│  [Header: type=STRING, marked=0, next=→]       │
│  [Length: 5] ["hello"]                         │
├─────────────────────────────────────────────────┤
│  Object 2 (Array)                               │
│  [Header: type=ARRAY, marked=0, next=→]        │
│  [Length: 3] [V1][V2][V3]                      │
├─────────────────────────────────────────────────┤
│  Object 3 (Struct)                              │
│  [Header: type=STRUCT, marked=0, next=nullptr] │
│  [Field count: 2] [V1][V2]                     │
└─────────────────────────────────────────────────┘
```

### Global Variables

Stored in a hash table for O(1) lookup:
```cpp
std::unordered_map<std::string, Value> globals;

// Access
Value get_global(const std::string& name) {
    auto it = globals.find(name);
    if (it == globals.end()) {
        runtime_error("Undefined variable: " + name);
    }
    return it->second;
}
```

### Constant Pool

Stores compile-time constants:
```cpp
std::vector<Value> constants;

constants[0] = String("Hello")
constants[1] = Float(3.14159)
constants[2] = Int(1000000)
constants[3] = FunctionMetadata(...)
```

---

## Bytecode Instruction Set

### Instruction Format

Variable-length encoding for compactness:

```
Format ABC: [OPCODE:8] [A:8] [B:8] [C:8]           (4 bytes)
Format ABx: [OPCODE:8] [A:8] [Bx:16]               (4 bytes)
Format Ax:  [OPCODE:8] [Ax:24]                     (4 bytes)
```

### Complete Instruction Set

#### Load/Store/Move (0x00-0x0F)
```
0x00  LOAD_CONST    RA, Kx         RA = constants[Kx]
0x01  LOAD_INT      RA, imm8       RA = sign_extend(imm8)
0x02  LOAD_TRUE     RA              RA = true
0x03  LOAD_FALSE    RA              RA = false
0x04  LOAD_NONE     RA              RA = none
0x05  MOVE          RA, RB          RA = RB
0x06  GET_GLOBAL    RA, Kx          RA = globals[constants[Kx]]
0x07  SET_GLOBAL    Kx, RA          globals[constants[Kx]] = RA
```

#### Arithmetic (0x10-0x1F)
```
0x10  ADD           RA, RB, RC      RA = RB + RC
0x11  SUB           RA, RB, RC      RA = RB - RC
0x12  MUL           RA, RB, RC      RA = RB * RC
0x13  DIV           RA, RB, RC      RA = RB / RC
0x14  MOD           RA, RB, RC      RA = RB % RC
0x15  NEG           RA, RB          RA = -RB
```

#### Comparison (0x20-0x2F)
```
0x20  EQ            RA, RB, RC      RA = (RB == RC)
0x21  NE            RA, RB, RC      RA = (RB != RC)
0x22  LT            RA, RB, RC      RA = (RB < RC)
0x23  LE            RA, RB, RC      RA = (RB <= RC)
0x24  GT            RA, RB, RC      RA = (RB > RC)
0x25  GE            RA, RB, RC      RA = (RB >= RC)
```

#### Logical (0x30-0x3F)
```
0x30  AND           RA, RB, RC      RA = RB && RC
0x31  OR            RA, RB, RC      RA = RB || RC
0x32  NOT           RA, RB          RA = !RB
```

#### Control Flow (0x40-0x4F)
```
0x40  JMP           offset16        ip += sign_extend(offset16)
0x41  JMP_IF_FALSE  RA, offset16    if (!RA) ip += offset16
0x42  JMP_IF_TRUE   RA, offset16    if (RA) ip += offset16
0x43  CALL          RA, RB, nargs   RA = call(closure@RB, nargs)
0x44  RETURN        RA              return RA
0x45  RETURN_NONE                   return none
```

#### Object Operations (0x50-0x6F)
```
0x50  NEW_ARRAY     RA, size16      RA = new Array(size16)
0x51  NEW_STRUCT    RA, Kx          RA = new Struct(type@Kx)
0x52  GET_INDEX     RA, RB, RC      RA = RB[RC]
0x53  SET_INDEX     RA, RB, RC      RA[RB] = RC
0x54  GET_FIELD     RA, RB, idx8    RA = RB.field[idx8]
0x55  SET_FIELD     RA, idx8, RB    RA.field[idx8] = RB
0x56  GET_LEN       RA, RB          RA = length(RB)
```

#### FFI (0x70-0x7F)
```
0x70  CALL_FFI      RA, Kx, nargs   RA = call_c_func(ffi@Kx, nargs)
```

#### Debug/Meta (0xF0-0xFF)
```
0xF0  PRINT         RA              print(RA) to stdout
0xFF  HALT                          stop execution
```

---

## Execution Model

### Fetch-Decode-Execute Loop

```cpp
void VM::run() {
    while (true) {
        uint8_t opcode = fetch_byte();

        switch (opcode) {
            case OP_ADD: {
                uint8_t ra = fetch_byte();
                uint8_t rb = fetch_byte();
                uint8_t rc = fetch_byte();
                registers[ra] = add(registers[rb], registers[rc]);
                break;
            }

            case OP_JMP: {
                int16_t offset = fetch_short();
                ip += offset;
                break;
            }

            case OP_CALL: {
                uint8_t ra = fetch_byte();
                uint8_t rb = fetch_byte();
                uint8_t nargs = fetch_byte();
                call_function(ra, rb, nargs);
                break;
            }

            case OP_RETURN: {
                uint8_t ra = fetch_byte();
                Value result = registers[ra];
                return_from_function(result);
                break;
            }

            case OP_HALT:
                return;

            default:
                runtime_error("Unknown opcode: " + std::to_string(opcode));
        }
    }
}
```

### Instruction Decoding Helpers

```cpp
uint8_t VM::fetch_byte() {
    return *ip++;
}

uint16_t VM::fetch_short() {
    uint16_t value = (ip[0] << 8) | ip[1];
    ip += 2;
    return value;
}

uint32_t VM::fetch_int() {
    uint32_t value = (ip[0] << 24) | (ip[1] << 16) | (ip[2] << 8) | ip[3];
    ip += 4;
    return value;
}
```

---

## Function Calls & Stack Frames

### Call Convention

When calling `fn foo(a, b, c) -> int`:

1. **Caller prepares arguments** in registers (or pushes if >256 args)
2. **Execute CALL instruction**: `CALL RA, RB, nargs`
   - RA = destination register for return value
   - RB = register containing closure object
   - nargs = number of arguments
3. **VM creates new frame**:
   - Allocate registers[256]
   - Copy arguments: frame.registers[0..nargs-1] = args
   - Save return address
   - Set instruction pointer to function start
4. **Function executes** with its own registers
5. **Execute RETURN**: `RETURN RC`
   - Save return value: result = frame.registers[RC]
   - Pop frame from call stack
   - Restore previous frame
   - Store result: caller_frame.registers[RA] = result
   - Continue execution

### Stack Frame Structure

```cpp
struct CallFrame {
    // Registers for this function
    Value registers[256];

    // Return information
    uint8_t* return_address;      // Where to jump back
    uint8_t return_register;      // Where to store result

    // Function metadata
    ClosureObject* closure;       // Function being executed

    // For debugging
    std::string function_name;
    int line_number;
};
```

### Example Call Sequence

```volta
fn add(a: int, b: int) -> int {
    return a + b
}

fn main() -> int {
    let x = add(3, 5)
    return x
}
```

**Bytecode:**
```
main:
  0: LOAD_INT R0, 3
  1: LOAD_INT R1, 5
  2: LOAD_CONST R2, K0         // K0 = @add closure
  3: CALL R3, R2, 2             // R3 = add(R0, R1)
  4: RETURN R3

add:
  0: ADD R2, R0, R1             // R2 = R0 + R1
  1: RETURN R2
```

**Execution trace:**
```
1. main frame: Execute LOAD_INT R0, 3        → R0 = 3
2. main frame: Execute LOAD_INT R1, 5        → R1 = 5
3. main frame: Execute LOAD_CONST R2, K0     → R2 = @add
4. main frame: Execute CALL R3, R2, 2        → Push new frame
5. add frame:  R0 = 3, R1 = 5 (args copied)
6. add frame:  Execute ADD R2, R0, R1        → R2 = 8
7. add frame:  Execute RETURN R2             → Pop frame
8. main frame: R3 = 8 (return value stored)
9. main frame: Execute RETURN R3             → Done
```

---

## Garbage Collection

### Mark-and-Sweep Algorithm

**Phase 1: Mark Reachable Objects**

Start from GC roots and mark all reachable objects:

```cpp
void GC::collect_garbage() {
    // Phase 1: Mark
    mark_roots();

    // Phase 2: Sweep
    sweep();

    // Update threshold
    vm->next_gc_threshold = vm->bytes_allocated * GC_HEAP_GROW_FACTOR;
}

void GC::mark_roots() {
    // Mark current frame registers
    for (size_t i = 0; i < 256; i++) {
        mark_value(&vm->current_frame->registers[i]);
    }

    // Mark all call stack frames
    for (auto& frame : vm->call_stack) {
        for (size_t i = 0; i < 256; i++) {
            mark_value(&frame.registers[i]);
        }
        mark_object((Object*)frame.closure);
    }

    // Mark globals
    for (auto& [name, value] : vm->globals) {
        mark_value(&value);
    }

    // Mark constants (may contain closures/strings)
    for (auto& constant : vm->constants) {
        mark_value(&constant);
    }
}

void GC::mark_value(Value* value) {
    if (value->type != ValueType::OBJECT) return;
    mark_object(value->as.obj);
}

void GC::mark_object(Object* obj) {
    if (obj == nullptr || obj->marked) return;

    obj->marked = true;

    // Recursively mark referenced objects
    switch (obj->type) {
        case ObjectType::ARRAY: {
            ArrayObject* array = (ArrayObject*)obj;
            for (size_t i = 0; i < array->length; i++) {
                mark_value(&array->elements[i]);
            }
            break;
        }
        case ObjectType::STRUCT: {
            StructObject* struct_obj = (StructObject*)obj;
            for (size_t i = 0; i < struct_obj->field_count; i++) {
                mark_value(&struct_obj->fields[i]);
            }
            break;
        }
        case ObjectType::CLOSURE: {
            ClosureObject* closure = (ClosureObject*)obj;
            for (size_t i = 0; i < closure->upvalue_count; i++) {
                mark_value(&closure->upvalues[i]);
            }
            break;
        }
        case ObjectType::STRING:
        case ObjectType::NATIVE_FUNCTION:
            // No references to mark
            break;
    }
}
```

**Phase 2: Sweep Unreachable Objects**

```cpp
void GC::sweep() {
    Object** obj = &vm->objects;

    while (*obj != nullptr) {
        if ((*obj)->marked) {
            // Object is reachable, unmark for next GC
            (*obj)->marked = false;
            obj = &(*obj)->next;
        } else {
            // Object is unreachable, free it
            Object* unreached = *obj;
            *obj = unreached->next;
            free_object(unreached);
        }
    }
}

void GC::free_object(Object* obj) {
    vm->bytes_allocated -= object_size(obj);

    switch (obj->type) {
        case ObjectType::STRING:
            delete (StringObject*)obj;
            break;
        case ObjectType::ARRAY:
            delete (ArrayObject*)obj;
            break;
        case ObjectType::STRUCT:
            delete (StructObject*)obj;
            break;
        case ObjectType::CLOSURE:
            delete (ClosureObject*)obj;
            break;
        case ObjectType::NATIVE_FUNCTION:
            delete (NativeFunctionObject*)obj;
            break;
    }
}
```

### GC Triggering

```cpp
Object* VM::allocate_object(size_t size, ObjectType type) {
    // Trigger GC if threshold exceeded
    if (bytes_allocated + size > next_gc_threshold) {
        gc.collect_garbage();
    }

    // Allocate new object
    Object* obj = (Object*)malloc(size);
    obj->type = type;
    obj->marked = false;
    obj->next = objects;
    objects = obj;

    bytes_allocated += size;
    return obj;
}
```

---

## FFI System

### FFI Architecture

```cpp
class FFIManager {
public:
    // Register a C function
    void register_function(
        const std::string& name,
        void* function_ptr,
        const std::vector<Type>& param_types,
        Type return_type
    );

    // Call a registered C function
    Value call(const std::string& name, const std::vector<Value>& args);

    // Load a shared library
    void load_library(const std::string& path);

private:
    struct FFIFunction {
        void* function_ptr;
        std::vector<Type> param_types;
        Type return_type;
    };

    std::unordered_map<std::string, FFIFunction> functions;
    std::vector<void*> library_handles;  // dlopen handles
};
```

### Type Marshaling

```cpp
// Convert Volta Value to C argument
void* FFIManager::marshal_to_c(const Value& volta_value, Type c_type) {
    switch (c_type) {
        case Type::INT:
            return new int64_t(volta_value.as.i);
        case Type::FLOAT:
            return new double(volta_value.as.f);
        case Type::STRING: {
            StringObject* str = (StringObject*)volta_value.as.obj;
            return (void*)str->data;  // Null-terminated
        }
        case Type::ARRAY_INT: {
            ArrayObject* arr = (ArrayObject*)volta_value.as.obj;
            int64_t* c_array = new int64_t[arr->length];
            for (size_t i = 0; i < arr->length; i++) {
                c_array[i] = arr->elements[i].as.i;
            }
            return c_array;
        }
        case Type::ARRAY_FLOAT: {
            ArrayObject* arr = (ArrayObject*)volta_value.as.obj;
            double* c_array = new double[arr->length];
            for (size_t i = 0; i < arr->length; i++) {
                c_array[i] = arr->elements[i].as.f;
            }
            return c_array;
        }
    }
}

// Convert C return value to Volta Value
Value FFIManager::marshal_from_c(void* c_value, Type c_type) {
    switch (c_type) {
        case Type::VOID:
            return Value::none();
        case Type::INT:
            return Value::int_value(*(int64_t*)c_value);
        case Type::FLOAT:
            return Value::float_value(*(double*)c_value);
        case Type::STRING:
            return Value::string((char*)c_value);
    }
}
```

### BLAS/LAPACK Integration

```cpp
// Example: Matrix multiplication using BLAS dgemm
void FFIManager::init_blas() {
    // Load BLAS library
    void* blas_handle = dlopen("libopenblas.so", RTLD_LAZY);
    if (!blas_handle) {
        throw std::runtime_error("Cannot load BLAS library");
    }

    // Get dgemm function pointer
    void* dgemm_ptr = dlsym(blas_handle, "dgemm_");
    if (!dgemm_ptr) {
        throw std::runtime_error("Cannot find dgemm in BLAS library");
    }

    // Register with type signature
    register_function("dgemm", dgemm_ptr,
        {Type::STRING,      // transa
         Type::STRING,      // transb
         Type::INT,         // m
         Type::INT,         // n
         Type::INT,         // k
         Type::FLOAT,       // alpha
         Type::ARRAY_FLOAT, // A
         Type::INT,         // lda
         Type::ARRAY_FLOAT, // B
         Type::INT,         // ldb
         Type::FLOAT,       // beta
         Type::ARRAY_FLOAT, // C
         Type::INT},        // ldc
        Type::VOID
    );
}
```

---

## Implementation Guide

### Step 1: Value System

**What to implement:**
- `Value` struct with tagged union
- Helper functions: `is_int()`, `is_float()`, `as_int()`, etc.
- Value creation: `Value::int_value()`, `Value::float_value()`, etc.
- Type checking and conversions

**Start here because:** Everything else depends on values

### Step 2: Object System

**What to implement:**
- `Object` base struct with header
- `StringObject`, `ArrayObject`, `StructObject`
- Allocation functions
- Object linked list for GC tracking

**Why now:** Need objects for strings, arrays in later steps

### Step 3: Basic VM Structure

**What to implement:**
- `VM` class with state (registers, IP, call stack)
- `fetch_byte()`, `fetch_short()` helpers
- Empty `run()` loop with switch statement
- Simple main that creates VM and runs bytecode

**Why now:** Framework for adding instructions incrementally

### Step 4: Load/Move Instructions

**What to implement:**
- `LOAD_INT`, `LOAD_TRUE`, `LOAD_FALSE`, `LOAD_NONE`
- `MOVE`
- `LOAD_CONST` with constant pool
- Test: Load values into registers

**Why now:** Need to get data into registers before operating on it

### Step 5: Arithmetic Instructions

**What to implement:**
- `ADD`, `SUB`, `MUL`, `DIV`, `MOD`, `NEG`
- Type checking (int + int, float + float, int + float promotion)
- Test: Arithmetic expressions

**Why now:** Core computation, test with simple math programs

### Step 6: Comparison & Logical

**What to implement:**
- `EQ`, `NE`, `LT`, `LE`, `GT`, `GE`
- `AND`, `OR`, `NOT`
- Test: Boolean expressions

**Why now:** Needed for control flow

### Step 7: Control Flow

**What to implement:**
- `JMP`, `JMP_IF_FALSE`, `JMP_IF_TRUE`
- Test: if/else, loops

**Why now:** Makes programs interesting, can write conditional logic

### Step 8: Global Variables

**What to implement:**
- Global variable hash table
- `GET_GLOBAL`, `SET_GLOBAL`
- Test: Global state

**Why now:** Programs need persistent state

### Step 9: Function Calls

**What to implement:**
- `CallFrame` struct
- Call stack
- `CALL`, `RETURN`, `RETURN_NONE`
- Test: Factorial, fibonacci

**Why now:** Functions are essential for real programs

### Step 10: Arrays

**What to implement:**
- `ArrayObject` creation
- `NEW_ARRAY`, `GET_INDEX`, `SET_INDEX`, `GET_LEN`
- Test: Array operations

**Why now:** Data structures unlock many algorithms

### Step 11: Structs

**What to implement:**
- `StructObject` creation
- `NEW_STRUCT`, `GET_FIELD`, `SET_FIELD`
- Test: Struct creation and field access

**Why now:** User-defined types are powerful

### Step 12: Garbage Collection

**What to implement:**
- Mark phase (trace from roots)
- Sweep phase (free unmarked)
- GC triggering on allocation
- Test: Memory doesn't grow unbounded

**Why now:** Memory management is now critical

### Step 13: Built-in Functions

**What to implement:**
- `PRINT` instruction or native print function
- String concatenation
- Type conversion functions
- Test: I/O and utility functions

**Why now:** User-facing functionality

### Step 14: FFI Basic

**What to implement:**
- `dlopen`/`dlsym` for loading C libraries
- Type marshaling for basic types (int, float, string)
- `CALL_FFI` instruction
- Test: Call simple C function (e.g., `sin` from libm)

**Why now:** Validation before complex BLAS integration

### Step 15: BLAS/LAPACK Integration

**What to implement:**
- Array marshaling (convert to C arrays)
- Register BLAS functions
- GC pause during C calls
- Test: Matrix multiplication, linear algebra

**Why now:** Final goal achieved!

---

## Performance Tips

### Optimization Opportunities

**Later** (after basic VM works):

1. **Direct Threading**: Replace switch with computed goto (2x faster)
2. **Type-Specialized Instructions**: `ADD_INT`, `ADD_FLOAT` (1.5x faster)
3. **Inline Caching**: Cache field offsets (2x faster for field access)
4. **Register Coalescing**: Reuse registers (reduces memory)
5. **Superinstructions**: Combine common patterns (e.g., `LOAD_ADD`)

### Memory Optimization

1. **NaN Boxing**: Use unused float bits for types (8 bytes vs 16 bytes per value)
2. **String Interning**: Deduplicate strings
3. **Generational GC**: Most objects die young, optimize for that

### Don't Optimize Yet!

**Get it working first**, then profile to find bottlenecks. Premature optimization wastes time.

---

## Testing Strategy

### Unit Tests

For each component:
- Value system: Type checking, conversions
- Instructions: Each opcode independently
- GC: Mark phase, sweep phase, object lifetime
- FFI: Marshaling each type

### Integration Tests

- Arithmetic: `2 + 3 * 4`
- Conditionals: `if x > 0 then ... else ...`
- Loops: `for i in 1..10`
- Functions: Factorial, fibonacci
- Arrays: Sort, search
- Structs: Geometric calculations
- GC: Allocate until GC triggers
- FFI: Call C math functions

### Regression Tests

Save each bug fix as a test case!

---

## Summary

The Volta VM is a **register-based virtual machine** featuring:

✅ **256 registers per frame** - Efficient computation
✅ **Tagged union values** - Simple and debuggable
✅ **Mark-and-sweep GC** - Automatic memory management
✅ **FFI system** - Native C library integration
✅ **Switch dispatch** - Easy to implement and debug

**Implementation order**: Start simple, add complexity incrementally, test continuously.

**Next step**: Begin implementing the VM API headers following this design!

🚀 **Let's build it!**
