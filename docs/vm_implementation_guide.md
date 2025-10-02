# VM Implementation Guide

## Overview

The complete Volta VM architecture and API have been created! Here's what you have:

## Documentation

✅ **`docs/vm_architecture.md`** - Complete VM architecture design
  - Value representation
  - Object system
  - Bytecode instruction set
  - Execution model
  - Garbage collection algorithm
  - FFI system
  - Step-by-step implementation guide

## API Headers (`include/vm/`)

✅ **`Value.hpp`** - Tagged union value representation
  - Factory functions: `Value::int_value()`, `Value::float_value()`, etc.
  - Type checking: `is_int()`, `is_float()`, etc.
  - Type conversions: `to_number()`, `to_bool()`, `to_string()`
  - Equality comparison

✅ **`Object.hpp`** - Heap-allocated objects
  - Base `Object` header (type, mark bit, next pointer)
  - `StringObject` - Immutable strings with cached hash
  - `ArrayObject` - Dynamic arrays
  - `StructObject` - User-defined structs
  - `ClosureObject` - Function closures
  - `NativeFunctionObject` - Built-in functions

✅ **`Opcode.hpp`** - Complete bytecode instruction set
  - 40+ opcodes organized by category
  - Load/Store/Move (LOAD_CONST, GET_GLOBAL, etc.)
  - Arithmetic (ADD, SUB, MUL, DIV, etc.)
  - Comparison (EQ, LT, GT, etc.)
  - Logical (AND, OR, NOT)
  - Control flow (JMP, CALL, RETURN)
  - Objects (NEW_ARRAY, GET_INDEX, GET_FIELD, etc.)
  - FFI (CALL_FFI)
  - Debug (PRINT, HALT)

✅ **`CallFrame.hpp`** - Function call frames
  - 256 register file
  - Return address and register
  - Function metadata
  - Debug info (function name, line number)

✅ **`GC.hpp`** - Mark-and-sweep garbage collector
  - `collect()` - Run GC cycle
  - `pause()` / `resume()` - For FFI safety
  - Mark phase: Trace from roots
  - Sweep phase: Free unmarked objects
  - Statistics tracking

✅ **`FFI.hpp`** - Foreign Function Interface
  - Library loading (dlopen/LoadLibrary)
  - Function registration with type signatures
  - Type marshaling (Volta ↔ C)
  - Built-in support for math, BLAS, LAPACK
  - GC coordination during C calls

✅ **`VM.hpp`** - Main virtual machine
  - Execution: `execute()`, `run()`, `step()`
  - Bytecode fetching: `fetch_byte()`, `fetch_short()`
  - Register access
  - Call stack management
  - Global variables
  - Constant pool
  - Object allocation
  - Built-in functions
  - Error handling
  - Debugging support

## Skeleton Implementation

✅ **`src/vm/Value.cpp`** - Started with TODOs
  - Every function has detailed implementation comments
  - Step-by-step instructions
  - Examples where helpful

## Still To Create

The following skeleton files need to be created in `src/vm/`:

📝 **`Object.cpp`** - Object implementations
📝 **`Opcode.cpp`** - Opcode utilities
📝 **`CallFrame.cpp`** - CallFrame constructor
📝 **`GC.cpp`** - Garbage collector implementation
📝 **`FFI.cpp`** - FFI manager implementation
📝 **`VM.cpp`** - Main VM implementation
📝 **`Builtins.cpp`** - Built-in function implementations

## Implementation Order (Recommended)

Follow this order for systematic implementation:

### Phase 1: Foundation (Week 1)
1. **Value.cpp** - Complete all TODOs in Value.cpp
2. **Object.cpp** - Implement object creation and field access
3. **Test**: Create values and objects, verify memory layout

### Phase 2: Basic VM (Week 2)
4. **VM.cpp** - Basic structure (constructor, fetch functions)
5. **Opcode.cpp** - Opcode name/operand count helpers
6. **Implement opcodes**: LOAD_*, MOVE, arithmetic, comparison
7. **Test**: Run simple arithmetic programs

### Phase 3: Control Flow (Week 2-3)
8. **CallFrame.cpp** - Frame constructor
9. **VM.cpp** - Call stack (call_function, return_from_function)
10. **Implement opcodes**: JMP, JMP_IF_*, CALL, RETURN
11. **Test**: Factorial, fibonacci

### Phase 4: Objects (Week 3-4)
12. **Implement opcodes**: NEW_ARRAY, NEW_STRUCT, GET/SET_INDEX, GET/SET_FIELD
13. **Test**: Array operations, struct field access

### Phase 5: GC (Week 4-5)
14. **GC.cpp** - Mark phase (trace from roots)
15. **GC.cpp** - Sweep phase (free unmarked)
16. **Test**: Memory doesn't grow unbounded

### Phase 6: Built-ins (Week 5)
17. **Builtins.cpp** - print, println, len, type, etc.
18. **VM.cpp** - register_builtins()
19. **Test**: I/O and utility functions

### Phase 7: FFI (Week 6-7)
20. **FFI.cpp** - Library loading (dlopen)
21. **FFI.cpp** - Type marshaling
22. **FFI.cpp** - Function calling (libffi integration)
23. **Test**: Call sin(), cos() from libm
24. **Test**: Matrix multiply with BLAS

## Key Implementation Tips

### For Each Function:
1. **Read the TODO comment carefully** - It tells you exactly what to do
2. **Follow the step-by-step instructions** - They're in order for a reason
3. **Use the examples** - They show the expected pattern
4. **Test incrementally** - Don't implement everything before testing
5. **Add assertions** - Catch bugs early with runtime checks

### Common Patterns:

**Type checking:**
```cpp
if (!value.is_int()) {
    runtime_error("Expected integer, got " + value.type_name());
}
```

**Bounds checking:**
```cpp
if (index >= array->length) {
    runtime_error("Index out of bounds");
}
```

**GC-safe allocation:**
```cpp
// GC might run during allocation
Object* obj = vm->allocate_object(size, type);
// obj is now tracked by GC
```

### Debugging:
- Use `vm->set_debug_mode(true)` to print each instruction
- Use `vm->print_state()` to inspect registers and stack
- Use `vm->get_stack_trace()` for error reporting
- Add `assert()` liberally to catch invariant violations

## Testing Strategy

Create test programs for each phase:

**Phase 1:**
```volta
let x = 42
let y = 3.14
let s = "hello"
```

**Phase 2:**
```volta
let x = 2 + 3 * 4
let y = x < 10
```

**Phase 3:**
```volta
fn factorial(n: int) -> int {
    if n <= 1 { return 1 }
    return n * factorial(n - 1)
}
```

**Phase 4:**
```volta
let arr = [1, 2, 3, 4, 5]
let sum = 0
for x in arr { sum = sum + x }

struct Point { x: float, y: float }
let p = Point { x: 3.0, y: 4.0 }
```

**Phase 5:**
```volta
# Allocate until GC runs
for i in 1..1000000 {
    let arr = [1, 2, 3, 4, 5]
}
```

**Phase 7:**
```volta
# Call C math functions
let x = sin(3.14159)
let y = sqrt(16.0)

# Call BLAS
let A = [[1.0, 2.0], [3.0, 4.0]]
let B = [[5.0, 6.0], [7.0, 8.0]]
let C = matmul(A, B)  # Uses dgemm internally
```

## Next Steps

1. **Read `docs/vm_architecture.md`** - Understand the design
2. **Complete `src/vm/Value.cpp`** - Follow the TODOs
3. **Create remaining skeleton files** - Copy the pattern from Value.cpp
4. **Implement incrementally** - One phase at a time
5. **Test continuously** - Don't move on until current phase works

## Need Help?

Each function has detailed comments explaining:
- **What** it does (purpose)
- **How** to implement it (step-by-step)
- **Why** it's done this way (design rationale)

Follow the comments and you'll build a working VM! 🚀

---

**You now have a complete, production-quality VM architecture with detailed implementation guidance. Happy coding!**
