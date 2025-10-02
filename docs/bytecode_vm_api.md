# Volta Bytecode & VM API Documentation

This document provides an overview of the complete API design for Volta's bytecode compiler, virtual machine, and generational garbage collector.

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                         IR Module                           │
│  (High-level intermediate representation)                  │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                   Bytecode Compiler                         │
│  - Translates IR to stack-based bytecode                   │
│  - Resolves jumps and labels                               │
│  - Builds constant pools                                    │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                   Compiled Module                           │
│  - Bytecode chunks (one per function)                      │
│  - String/constant pools                                    │
│  - Function metadata                                        │
└─────────────────────┬───────────────────────────────────────┘
                      │
          ┌───────────┴───────────┐
          ▼                       ▼
┌─────────────────────┐  ┌─────────────────────┐
│   Disassembler      │  │  Serializer         │
│  (Debug/inspect)    │  │  (Save/load .vltb)  │
└─────────────────────┘  └─────────────────────┘
          │
          ▼
┌─────────────────────────────────────────────────────────────┐
│                    Virtual Machine                          │
│  - Stack-based interpreter                                  │
│  - Evaluation stack                                         │
│  - Call stack (frames)                                      │
│  - Local/global variables                                   │
│  - Foreign function interface (FFI)                         │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                  Memory Manager                             │
│  - Coordinates allocation strategies                        │
│  - Triggers GC based on thresholds                          │
│  - Manages object headers and metadata                      │
└─────────────────────┬───────────────────────────────────────┘
                      │
          ┌───────────┼───────────┬───────────────┐
          ▼           ▼           ▼               ▼
┌─────────────┐ ┌──────────┐ ┌─────────┐ ┌──────────────┐
│   Arena     │ │ FreeList │ │  Slab   │ │     Page     │
│  (Young)    │ │  (Old)   │ │ (Fixed) │ │   (Large)    │
└─────────────┘ └──────────┘ └─────────┘ └──────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│            Generational Garbage Collector                   │
│  - Two generations (young/old)                              │
│  - Mark and sweep algorithm                                 │
│  - Write barrier for old→young refs                         │
│  - Automatic promotion based on age                         │
└─────────────────────────────────────────────────────────────┘
```

## Module Structure

### Bytecode Module (`include/bytecode/`)

#### 1. **Bytecode.hpp**
- **Purpose**: Core bytecode definitions
- **Key Components**:
  - `Opcode` enum: All VM instructions (arithmetic, memory, control flow, etc.)
  - `Chunk` class: Container for bytecode with debug info
  - Utility functions: `getOpcodeName()`, `getOpcodeOperandSize()`

**Instruction Set Highlights**:
- Stack operations: `Pop`, `Dup`, `Swap`
- Constants: `ConstInt`, `ConstFloat`, `ConstBool`, `ConstString`, `ConstNull`
- Arithmetic: Type-specific ops (`AddInt`, `AddFloat`, `DivInt`, etc.)
- Memory: `Alloc`, `Load`, `Store`, `GetField`, `SetField`, `GetElement`, `SetElement`
- Control flow: `Jump`, `JumpIfTrue`, `JumpIfFalse`, `Call`, `Return`
- FFI: `CallForeign`

#### 2. **BytecodeCompiler.hpp**
- **Purpose**: IR → Bytecode translation
- **Key Components**:
  - `BytecodeCompiler` class: Main compilation engine
  - `CompiledFunction` struct: Per-function bytecode + metadata
  - `CompiledModule` class: Complete program bytecode

**Compilation Pipeline**:
1. Build function/global indices
2. Compile each function's basic blocks
3. Translate IR instructions to bytecode
4. Resolve labels and patch jumps
5. Build constant pools

#### 3. **Disassembler.hpp**
- **Purpose**: Bytecode → Human-readable text
- **Key Components**:
  - `Disassembler` class: Converts bytecode to assembly-like format
  - Configuration options: show offsets, line numbers, raw bytes
  - Utility functions: `dumpModule()`, `dumpFunction()`, `dumpChunk()`

**Output Format**:
```
Function: main (params=0, locals=2)
  0000: ConstInt 42
  0009: StoreLocal 0
  0014: LoadLocal 0
  0019: Print
  0020: ReturnVoid
```

#### 4. **BytecodeSerializer.hpp**
- **Purpose**: Save/load compiled bytecode to disk
- **Key Components**:
  - `BytecodeSerializer` class: Binary serialization/deserialization
  - File format: `.vltb` (Volta Bytecode)
  - Utilities: `saveCompiledModule()`, `loadCompiledModule()`

**Binary Format**:
- Header: Magic number "VLTB", version, module name
- String pool, foreign functions, globals
- Functions: metadata + bytecode + debug info

### VM Module (`include/vm/`)

#### 5. **VM.hpp**
- **Purpose**: Stack-based bytecode interpreter
- **Key Components**:
  - `Value` struct: Tagged union for runtime values
  - `CallFrame` struct: Function call stack frame
  - `VM` class: Main execution engine

**VM Architecture**:
- **Evaluation stack**: Operand stack for expressions
- **Call stack**: Function call frames
- **Local stack**: Local variables for all frames
- **Global array**: Global variables
- **Heap**: Dynamically allocated objects

**Value Types**:
- `Null`, `Int` (64-bit), `Float` (64-bit), `Bool`, `String` (pool index), `Object` (heap pointer)

**Heap Objects**:
- `StructObject`: User-defined structs with fields
- `ArrayObject`: Dynamic arrays with length

**Public API**:
- `execute()`: Run entry point (main)
- `executeFunction(name/index)`: Call specific function
- `push()/pop()/peek()`: Stack manipulation
- `registerNativeFunction()`: FFI support
- `setDebugTrace()`: Enable instruction tracing
- `dumpStack()/dumpCallStack()`: Debugging

#### 6. **Value.hpp**
- **Purpose**: Value manipulation utilities
- **Key Functions**:
  - `valueToString()`: Convert value to string
  - `valuesEqual()`: Value equality comparison
  - `getStructField()`, `setStructField()`: Struct access
  - `getArrayElement()`, `setArrayElement()`: Array access
  - `printStruct()`, `printArray()`: Debug printing

#### 7. **MemoryManager.hpp**
- **Purpose**: Unified memory allocation interface
- **Key Components**:
  - `MemoryManager` class: Coordinates allocators and GC
  - Automatic GC triggering based on thresholds
  - Write barrier integration

**Allocation Strategy**:
- Small objects (<4KB): Arena (young gen) or FreeList (old gen)
- Large objects (≥4KB): PageAllocator
- Fixed-size objects: Optional SlabAllocator

**Public API**:
- `allocateStruct()`, `allocateArray()`: Object allocation
- `collect()`, `collectMinor()`, `collectMajor()`: Manual GC
- `setAutoGC()`: Enable/disable automatic GC
- `totalAllocated()`, `memoryUsagePercent()`: Statistics
- `writeBarrier()`: Notify GC of old→young references
- `verifyHeap()`, `dumpHeap()`: Debugging

#### 8. **GC.hpp**
- **Purpose**: Generational garbage collector
- **Key Components**:
  - `GarbageCollector` class: Two-generation mark-and-sweep GC
  - `GCConfig` struct: Configuration parameters
  - `GCStats` struct: Collection statistics

**Generational Design**:
- **Young generation**: Newly allocated objects, collected frequently
- **Old generation**: Long-lived objects, collected infrequently
- **Promotion**: Objects surviving N minor GCs move to old gen
- **Remembered set**: Tracks old→young references for efficient minor GC

**GC Algorithm**:
1. **Mark phase**: Traverse from roots, mark reachable objects
2. **Sweep phase**: Free unmarked objects
3. **Promotion**: Move survivors to old generation

**Configuration**:
- Heap size limits (young/old/total)
- Collection thresholds (trigger at X% full)
- Promotion parameters (age threshold, survival ratio)

**Public API**:
- `allocateStruct()`, `allocateArray()`: Allocation with GC integration
- `collectMinor()`: Young generation only (fast)
- `collectMajor()`: Full heap (slow)
- `writeBarrier()`: Record old→young references
- `addRoot()`, `removeRoot()`: Manage GC roots
- `stats()`, `printStats()`: Statistics and profiling

#### 9. **Allocator.hpp**
- **Purpose**: Low-level memory allocation strategies
- **Key Components**:
  - `Arena`: Bump-pointer allocation (young gen)
  - `FreeListAllocator`: Size-segregated free lists (old gen)
  - `SlabAllocator`: Fixed-size object pools
  - `PageAllocator`: Large objects via OS pages

**Arena**:
- Very fast: O(1) bump pointer
- No individual deallocation (bulk reset after GC)
- Good cache locality

**FreeListAllocator**:
- Size classes: 16, 32, 64, 128, 256, 512, 1024, 2048, 4096+ bytes
- First-fit allocation
- Coalescing to reduce fragmentation

**SlabAllocator**:
- Pre-allocated slabs of fixed-size objects
- O(1) allocation/deallocation
- Ideal for common struct sizes

**PageAllocator**:
- Direct OS allocation (mmap/VirtualAlloc)
- No fragmentation
- Can return pages immediately

## Usage Examples

### Compiling IR to Bytecode

```cpp
#include "bytecode/BytecodeCompiler.hpp"

// Given an IR module
ir::IRModule irModule("my_program");
// ... populate with IR ...

// Compile to bytecode
BytecodeCompiler compiler;
auto compiled = compiler.compile(irModule);

// Disassemble for debugging
Disassembler disasm;
std::cout << disasm.disassembleModule(*compiled) << std::endl;

// Save to disk
BytecodeSerializer serializer;
serializer.serializeToFile(*compiled, "program.vltb");
```

### Running Bytecode

```cpp
#include "vm/VM.hpp"
#include "vm/MemoryManager.hpp"

// Load compiled module
auto module = loadCompiledModule("program.vltb");

// Create VM with GC
VM vm(module);

// Register native functions (FFI)
vm.registerNativeFunction("print", [](VM& vm) {
    Value val = vm.pop();
    std::cout << valueToString(val) << std::endl;
    return 0;  // No return value
});

// Execute entry point
int exitCode = vm.execute();
```

### Memory Management

```cpp
#include "vm/MemoryManager.hpp"
#include "vm/GC.hpp"

// Configure GC
GCConfig config;
config.youngGenMaxSize = 4 * 1024 * 1024;  // 4MB
config.oldGenMaxSize = 32 * 1024 * 1024;   // 32MB
config.promotionAge = 3;  // Promote after 3 minor GCs

// Create memory manager
MemoryManager memMgr(&vm, config);

// Allocate objects (automatic GC when needed)
auto myStruct = memMgr.allocateStruct(typeId, 5);  // 5 fields
auto myArray = memMgr.allocateArray(100);          // 100 elements

// Manual GC control
memMgr.setAutoGC(false);
// ... allocate lots of objects ...
memMgr.collectMinor();  // Collect young generation

// Statistics
memMgr.printStats(std::cout);
```

## Implementation Status

✅ **Complete**: API design and header files
⏳ **TODO**: Implementation (.cpp files)
⏳ **TODO**: Unit tests
⏳ **TODO**: Integration with IR generator

## Future Enhancements

1. **JIT Compilation**: Hot code detection and native code generation
2. **Concurrent GC**: Background collection threads
3. **Incremental GC**: Pause-less collection
4. **Profile-Guided Optimization**: Inline caching, type feedback
5. **SIMD Support**: Vectorized operations for arrays/matrices
6. **Parallel Execution**: Multi-threaded VM with work stealing

## Notes

- **No GC initially**: For getting started, the VM can simply allocate without collecting. GC implementation can come later.
- **Stack-based**: Simpler than register-based, easier to implement and reason about.
- **Tagged values**: Runtime type checking for safety (can optimize later with type inference).
- **FFI support**: Native function calls for I/O, system integration.
- **Debug-friendly**: Disassembler, stack dumps, line number tracking.

---

**Author**: Claude Code
**Date**: 2025-10-02
**Version**: 0.1
