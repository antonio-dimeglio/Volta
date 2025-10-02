# Bytecode Generator Implementation Summary

## Files Created

### Header Files (include/bytecode/)

1. **BytecodeGenerator.hpp** - Main generator class
   - `BytecodeGenerator` - Orchestrates full module compilation
   - `ModuleHeader` - Binary format header structure
   - `FunctionMetadata` - Function metadata structure
   - `BytecodeDisassembler` - Debugging utility

2. **ConstantPool.hpp** - Constant pool builder
   - `Constant` - Value wrapper with type tag
   - `ConstantPoolBuilder` - Manages all constants with deduplication
   - Supports: int, float, bool, string, none

3. **RegisterAllocator.hpp** - Register allocation
   - `LiveRange` - Tracks value lifetime
   - `RegisterAllocator` - Linear scan algorithm (256 registers)

4. **BytecodeEmitter.hpp** - Instruction encoding
   - `BytecodeEmitter` - Low-level bytecode emission
   - Supports 6 instruction formats: A, AB, ABC, ABx, Ax, No-operands
   - Label management and jump resolution

5. **FunctionCompiler.hpp** - Per-function compilation
   - `FunctionCompiler` - Compiles single IR function
   - Handles all IR instruction types
   - Manages control flow and basic blocks

### Implementation Files (src/bytecode/)

All implementation files created with detailed TODO comments explaining:
- What each function does
- Step-by-step implementation instructions
- Expected behavior and edge cases

1. **ConstantPool.cpp**
   - `add_int/float/string/bool/none()` - Add with deduplication
   - `serialize()` - Write to binary format

2. **RegisterAllocator.cpp**
   - `analyze_live_ranges()` - Compute value lifetimes
   - `allocate_registers()` - Linear scan algorithm
   - `assign_parameter_registers()` - Reserve r0-rN for params

3. **BytecodeEmitter.cpp**
   - `emit_a/ab/abc/abx/ax()` - Instruction encoders
   - `emit_jump()` - Forward/backward jump handling
   - `resolve_jumps()` - Two-pass assembly

4. **FunctionCompiler.cpp**
   - `compile()` - Main compilation entry point
   - `compile_binary_op()` - Arithmetic/logic operations
   - `compile_call()` - Function calls
   - `compile_branch()` - Control flow
   - IR instruction → VM opcode mapping

5. **BytecodeGenerator.cpp**
   - `generate()` - Full module compilation
   - `compile_module_header()` - Binary header
   - `compile_functions()` - All functions
   - `write_xxx()` - Binary serialization helpers

## Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│                   BytecodeGenerator                      │
│  - Orchestrates full pipeline                           │
│  - Manages constant pool                                │
│  - Writes binary format                                 │
└────────────────────┬────────────────────────────────────┘
                     │
        ┌────────────┴────────────┐
        │                         │
        ▼                         ▼
┌───────────────────┐    ┌──────────────────┐
│ FunctionCompiler  │    │  ConstantPool    │
│ - Per-function    │◄───┤  - Deduplication │
│ - Instruction     │    │  - Serialization │
│   compilation     │    └──────────────────┘
└────────┬──────────┘
         │
    ┌────┴─────┐
    │          │
    ▼          ▼
┌────────────┐ ┌──────────────┐
│ Register   │ │  Bytecode    │
│ Allocator  │ │  Emitter     │
│ - Linear   │ │ - Encoding   │
│   scan     │ │ - Labels     │
│ - 256 regs │ │ - Jumps      │
└────────────┘ └──────────────┘
```

## Implementation Order

Recommended order to implement the TODOs:

### Phase 1: Foundation (No dependencies)
1. **ConstantPool.cpp** - Start here, simple logic
   - `add_int/float/string/bool/none()`
   - `get()`
   - `serialize()`

2. **BytecodeEmitter.cpp** - Low-level encoding
   - `emit_byte/opcode/short/int24()`
   - `emit_a/ab/abc/abx/ax()`
   - `create_label()` and `mark_label()`

### Phase 2: Register Allocation (Requires IR understanding)
3. **RegisterAllocator.cpp**
   - `number_instructions()`
   - `assign_parameter_registers()`
   - `analyze_live_ranges()`
   - `allocate_registers()`
   - `expire_old_intervals()`

### Phase 3: Jump Resolution (Requires emitter)
4. **BytecodeEmitter.cpp** (continued)
   - `emit_jump/jump_if_false/jump_if_true()`
   - `resolve_jumps()`

### Phase 4: Function Compilation (Requires all above)
5. **FunctionCompiler.cpp**
   - `compile()` - Entry point
   - `compile_basic_block()`
   - `compile_instruction()` - Dispatch
   - Individual instruction compilers:
     - `compile_binary_op()`
     - `compile_load_const()`
     - `compile_return()`
     - `compile_call()`
     - `compile_branch/cond_branch()`
     - `compile_get_field/set_field()`
     - `compile_get_index/set_index()`
     - `compile_alloca()`
   - Helper functions:
     - `get_block_label()`
     - `get_value_register()`
     - `map_opcode()`

### Phase 5: Module Generation (Requires all above)
6. **BytecodeGenerator.cpp**
   - `compile_module_header()`
   - `compile_functions()`
   - `compile_globals()`
   - `write_uint32/16/8()`
   - `write_header()`
   - `generate()`
   - `generate_to_file()`

### Phase 6: Debugging (Optional but useful)
7. **BytecodeDisassembler** (in BytecodeGenerator.cpp)
   - `disassemble_instruction()`
   - `disassemble()`

## Testing Strategy

### Unit Tests (Create test_bytecode.cpp)

1. **ConstantPool Tests**
   ```cpp
   TEST(ConstantPoolTest, IntDeduplication) {
       ConstantPoolBuilder pool;
       uint32_t idx1 = pool.add_int(42);
       uint32_t idx2 = pool.add_int(42);
       EXPECT_EQ(idx1, idx2);  // Same constant = same index
   }
   ```

2. **BytecodeEmitter Tests**
   ```cpp
   TEST(BytecodeEmitterTest, EmitABC) {
       ConstantPoolBuilder pool;
       BytecodeEmitter emitter(pool);
       emitter.emit_abc(Opcode::ADD, 0, 1, 2);
       auto code = emitter.bytecode();
       EXPECT_EQ(code.size(), 4);  // opcode + 3 operands
   }
   ```

3. **RegisterAllocator Tests**
   ```cpp
   TEST(RegisterAllocatorTest, SimpleFunction) {
       // Create simple IR function
       // Allocate registers
       // Verify parameters in r0-rN
   }
   ```

4. **FunctionCompiler Tests**
   ```cpp
   TEST(FunctionCompilerTest, CompileAdd) {
       // fn add(a: int, b: int) -> int { return a + b }
       // Compile to bytecode
       // Verify bytecode structure
   }
   ```

### Integration Tests

```cpp
TEST(BytecodeGeneratorTest, FullPipeline) {
    // Source → Lexer → Parser → Semantic → IR → Bytecode
    const char* source = R"(
        fn main() -> int {
            return 42
        }
    )";

    // Parse and analyze
    auto ast = parse(source);
    auto ir = generate_ir(ast);

    // Generate bytecode
    BytecodeGenerator gen(*ir);
    auto bytecode = gen.generate();

    // Verify structure
    ASSERT_GT(bytecode.size(), 0);

    // Execute in VM
    VM vm;
    Value result = vm.execute(bytecode.data(), bytecode.size());
    EXPECT_EQ(result.as_int(), 42);
}
```

## Binary Format

```
Offset | Size | Description
-------|------|-------------
0      | 4    | Magic (0x564F4C54 = "VOLT")
4      | 2    | Version major
6      | 2    | Version minor
8      | 4    | Constant count
12     | 4    | Function count
16     | 4    | Global count
20     | 4    | Entry point index
24     | N    | Constant pool
24+N   | M    | Function metadata
24+N+M | K    | Function bytecode
...    | L    | Global table
```

## Next Steps

1. **Implement skeletons** - Follow TODOs in order above
2. **Write unit tests** - Test each component independently
3. **Integration test** - Full pipeline test
4. **Debug** - Use disassembler to inspect bytecode
5. **Optimize** - Add peephole optimizations

## Common Pitfalls

1. **Endianness** - Always use big-endian for multi-byte values
2. **Jump offsets** - Calculate relative to NEXT instruction
3. **Register spilling** - Error if >256 live registers (for now)
4. **Constant deduplication** - Must work correctly for correctness
5. **Label resolution** - Two-pass: generate then patch
6. **Parameter registers** - Reserve r0-r{arity-1}

## Completion Criteria

✅ Bytecode generator compiles simple functions
✅ Constant pool works with deduplication
✅ Register allocation succeeds for simple cases
✅ Jump resolution works for control flow
✅ Generated bytecode is valid and executable
✅ All unit tests pass
✅ Integration test: full pipeline works end-to-end

Once complete, the Volta interpreter will be fully functional!
