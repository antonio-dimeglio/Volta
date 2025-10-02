# Bytecode Generator Design

## Overview

The Bytecode Generator is the final compilation stage that transforms IR (Intermediate Representation) into compact bytecode that can be executed by the VM. It sits between the IR generator and the VM executor.

**Pipeline Position:**
```
Source Code → Lexer → Parser → Semantic Analyzer → IR Generator → Bytecode Generator → VM
```

## Design Goals

1. **Compact Encoding**: Generate space-efficient bytecode
2. **Register Allocation**: Map IR virtual registers to VM's 256 registers
3. **Jump Resolution**: Convert IR labels to bytecode offsets
4. **Constant Pool**: Build constant pool for literals
5. **Function Layout**: Organize functions and their metadata

## Bytecode Format

### Instruction Encoding

Each instruction consists of an opcode byte followed by operands:

```
┌─────────────────────────────────────────────┐
│ Opcode (1 byte) │ Operands (0-3 bytes)      │
└─────────────────────────────────────────────┘
```

**Instruction Formats:**

1. **No operands (0 bytes)**: `HALT`, `RETURN_NONE`
   ```
   [opcode]
   ```

2. **A format (1 byte)**: Single register operand
   ```
   [opcode][ra]
   ```
   Examples: `LOAD_TRUE ra`, `RETURN ra`, `PRINT ra`

3. **AB format (2 bytes)**: Two register operands
   ```
   [opcode][ra][rb]
   ```
   Examples: `MOVE ra, rb`, `NEG ra, rb`, `NOT ra, rb`

4. **ABC format (3 bytes)**: Three register operands
   ```
   [opcode][ra][rb][rc]
   ```
   Examples: `ADD ra, rb, rc`, `SUB ra, rb, rc`, `CALL ra, rb, rc`

5. **ABx format (3 bytes)**: Register + 16-bit immediate
   ```
   [opcode][ra][bx_hi][bx_lo]
   ```
   Examples: `LOAD_CONST ra, kx`, `GET_GLOBAL ra, kx`
   - `bx` is a 16-bit value (0-65535)
   - Stored in big-endian: high byte first, low byte second

6. **Ax format (3 bytes)**: 24-bit immediate (for jumps)
   ```
   [opcode][ax_hi][ax_mid][ax_lo]
   ```
   Examples: `JMP offset`, `JMP_IF_FALSE offset`
   - `ax` is a 24-bit signed offset (-8388608 to 8388607)
   - Stored in big-endian: high byte, middle byte, low byte

## Module Structure

A compiled module contains:

```
┌────────────────────────────────────────┐
│ Module Header                          │
├────────────────────────────────────────┤
│ Constant Pool                          │
│  - Integers                            │
│  - Floats                              │
│  - Strings                             │
├────────────────────────────────────────┤
│ Function Table                         │
│  - Function Metadata                   │
│  - Function Bytecode                   │
├────────────────────────────────────────┤
│ Global Variables Table                 │
└────────────────────────────────────────┘
```

### Module Header

```cpp
struct ModuleHeader {
    uint32_t magic;           // 0x564F4C54 ("VOLT")
    uint16_t version_major;   // 0
    uint16_t version_minor;   // 1
    uint32_t constant_count;  // Number of constants
    uint32_t function_count;  // Number of functions
    uint32_t global_count;    // Number of globals
};
```

### Constant Pool

Constants are stored in a contiguous pool, indexed by their position:

```cpp
struct Constant {
    uint8_t type;  // 0=none, 1=bool, 2=int, 3=float, 4=string
    union {
        bool b;
        int64_t i;
        double f;
        uint32_t str_offset;  // Offset into string table
    } value;
};
```

String table format:
```
[length:4][bytes...][length:4][bytes...]...
```

### Function Metadata

```cpp
struct FunctionMetadata {
    uint32_t name_offset;      // Offset into string table
    uint32_t bytecode_offset;  // Offset into bytecode section
    uint32_t bytecode_length;  // Length of bytecode
    uint8_t arity;            // Number of parameters
    uint8_t register_count;   // Total registers needed
    uint8_t is_variadic;      // 0 or 1
    uint8_t reserved;         // Padding
};
```

## Register Allocation

The VM has 256 registers per stack frame (r0-r255). The bytecode generator must:

1. **Map IR virtual registers to physical registers**
   - IR may have unlimited virtual registers
   - Must fit into 256 physical registers
   - Use linear scan or graph coloring

2. **Reserve special registers**
   - r0-r7: Function parameters (up to 8 direct params)
   - r8-r255: General purpose

3. **Handle register spilling**
   - If >256 live values, spill to stack (future work)
   - For now, error if >256 registers needed

### Simple Linear Scan Algorithm

```
1. Analyze IR to compute live ranges for each virtual register
2. Sort virtual registers by starting point
3. Maintain active register list
4. For each virtual register:
   - Expire old intervals no longer live
   - If free physical register available:
     - Assign it
   - Else:
     - Error: too many live registers (for now)
     - Future: spill to memory
```

## IR to Bytecode Mapping

### IR Instructions → VM Opcodes

| IR Instruction | VM Opcode(s) | Notes |
|----------------|--------------|-------|
| `%0 = add %1, %2` | `ADD r0, r1, r2` | Direct mapping |
| `%0 = sub %1, %2` | `SUB r0, r1, r2` | Direct mapping |
| `%0 = mul %1, %2` | `MUL r0, r1, r2` | Direct mapping |
| `%0 = div %1, %2` | `DIV r0, r1, r2` | Direct mapping |
| `%0 = mod %1, %2` | `MOD r0, r1, r2` | Direct mapping |
| `%0 = neg %1` | `NEG r0, r1` | Unary negation |
| `%0 = eq %1, %2` | `EQ r0, r1, r2` | Equality |
| `%0 = ne %1, %2` | `NE r0, r1, r2` | Not equal |
| `%0 = lt %1, %2` | `LT r0, r1, r2` | Less than |
| `%0 = le %1, %2` | `LE r0, r1, r2` | Less or equal |
| `%0 = gt %1, %2` | `GT r0, r1, r2` | Greater than |
| `%0 = ge %1, %2` | `GE r0, r1, r2` | Greater or equal |
| `%0 = and %1, %2` | `AND r0, r1, r2` | Logical AND |
| `%0 = or %1, %2` | `OR r0, r1, r2` | Logical OR |
| `%0 = not %1` | `NOT r0, r1` | Logical NOT |
| `%0 = load_const 42` | `LOAD_CONST r0, k0` | k0 = constant pool index |
| `%0 = load_const true` | `LOAD_TRUE r0` | Optimized for bool |
| `%0 = load_const false` | `LOAD_FALSE r0` | Optimized for bool |
| `%0 = load_const none` | `LOAD_NONE r0` | Optimized for none |
| `%0 = alloca` | `NEW_STRUCT r0, k0` | k0 = type info |
| `%0 = get_field %1, 2` | `GET_FIELD r0, r1, 2` | Field index |
| `set_field %0, 2, %1` | `SET_FIELD r0, 2, r1` | Field index |
| `%0 = get_index %1, %2` | `GET_INDEX r0, r1, r2` | Array access |
| `set_index %0, %1, %2` | `SET_INDEX r0, r1, r2` | Array store |
| `%0 = call @func, %1, %2` | `LOAD_CONST r_tmp, k_func`<br>`CALL r0, r_tmp, 2` | Multi-step |
| `ret %0` | `RETURN r0` | Return value |
| `ret void` | `RETURN_NONE` | No return value |
| `br label %bb1` | `JMP offset` | Unconditional |
| `br_if %0, %bb1, %bb2` | `JMP_IF_FALSE r0, offset` | Conditional |

### Control Flow

**Basic Blocks:**
- Each IR basic block becomes a contiguous sequence of bytecode
- No explicit labels in bytecode - use offsets

**Jump Target Resolution:**
```
1. First pass: Generate bytecode, record label positions
   - Build map: label_name -> bytecode_offset

2. Second pass: Patch jump offsets
   - For each jump instruction:
     - Calculate offset = target_position - current_position
     - Write offset into bytecode
```

**Example:**
```
IR:
  bb0:
    %0 = load_const 10
    %1 = lt %0, 5
    br_if %1, bb1, bb2
  bb1:
    ret %0
  bb2:
    %2 = add %0, 1
    ret %2

Bytecode (first pass):
  0: LOAD_CONST r0, k0      ; %0 = 10
  3: LOAD_CONST r1, k1      ; load 5
  6: LT r1, r0, r1          ; %1 = %0 < 5
  9: JMP_IF_FALSE r1, ???   ; to bb2
  13: RETURN r0             ; bb1
  14: LOAD_CONST r2, k2     ; bb2: load 1
  17: ADD r2, r0, r2
  20: RETURN r2

Label map:
  bb0: 0
  bb1: 13
  bb2: 14

Bytecode (second pass):
  9: JMP_IF_FALSE r1, 5     ; offset = 14 - 9 = 5
```

## Implementation Strategy

### Phase 1: Constant Pool Builder

```cpp
class ConstantPoolBuilder {
public:
    // Add constant, return index (deduplicate)
    uint32_t add_int(int64_t value);
    uint32_t add_float(double value);
    uint32_t add_string(const std::string& value);
    uint32_t add_bool(bool value);
    uint32_t add_none();

    // Serialize to byte array
    std::vector<uint8_t> serialize() const;

private:
    std::vector<Constant> constants_;
    std::unordered_map<std::string, uint32_t> string_pool_;
    // Deduplication maps...
};
```

### Phase 2: Register Allocator

```cpp
class RegisterAllocator {
public:
    explicit RegisterAllocator(const ir::Function& func);

    // Get physical register for IR virtual register
    uint8_t get_register(const ir::Value* value);

    // Get total register count needed
    uint8_t register_count() const;

private:
    void analyze_live_ranges();
    void allocate_registers();

    std::unordered_map<const ir::Value*, uint8_t> allocation_;
    uint8_t next_register_ = 0;
};
```

### Phase 3: Bytecode Emitter

```cpp
class BytecodeEmitter {
public:
    explicit BytecodeEmitter(ConstantPoolBuilder& constants);

    // Emit instructions
    void emit_opcode(Opcode op);
    void emit_byte(uint8_t byte);
    void emit_short(uint16_t value);  // Big-endian
    void emit_int24(int32_t value);   // 24-bit signed

    // High-level emission
    void emit_a(Opcode op, uint8_t ra);
    void emit_ab(Opcode op, uint8_t ra, uint8_t rb);
    void emit_abc(Opcode op, uint8_t ra, uint8_t rb, uint8_t rc);
    void emit_abx(Opcode op, uint8_t ra, uint16_t bx);
    void emit_ax(Opcode op, int32_t offset);

    // Label management
    uint32_t create_label();
    void mark_label(uint32_t label);
    void emit_jump(uint32_t label);
    void emit_jump_if_false(uint8_t reg, uint32_t label);

    // Get final bytecode
    std::vector<uint8_t> bytecode() const;

    // Resolve jumps (call after all code emitted)
    void resolve_jumps();

private:
    ConstantPoolBuilder& constants_;
    std::vector<uint8_t> code_;
    std::unordered_map<uint32_t, uint32_t> label_positions_;
    std::vector<std::pair<uint32_t, uint32_t>> jump_patches_; // (code_offset, label_id)
};
```

### Phase 4: Function Compiler

```cpp
class FunctionCompiler {
public:
    FunctionCompiler(
        const ir::Function& func,
        ConstantPoolBuilder& constants
    );

    // Compile function to bytecode
    FunctionMetadata compile();

private:
    void compile_basic_block(const ir::BasicBlock& bb);
    void compile_instruction(const ir::Instruction& inst);

    // Instruction compilation
    void compile_binary_op(const ir::BinaryInst& inst);
    void compile_load_const(const ir::Constant& constant);
    void compile_call(const ir::CallInst& inst);
    void compile_return(const ir::ReturnInst& inst);
    // ... more instruction types

    const ir::Function& func_;
    RegisterAllocator allocator_;
    BytecodeEmitter emitter_;
    std::unordered_map<const ir::BasicBlock*, uint32_t> block_labels_;
};
```

### Phase 5: Module Compiler

```cpp
class BytecodeGenerator {
public:
    explicit BytecodeGenerator(const ir::IRModule& module);

    // Generate complete bytecode module
    std::vector<uint8_t> generate();

    // Generate and write to file
    void generate_to_file(const std::string& filename);

private:
    void compile_module_header();
    void compile_constant_pool();
    void compile_functions();
    void compile_globals();

    const ir::IRModule& module_;
    ConstantPoolBuilder constants_;
    std::vector<FunctionMetadata> functions_;
    std::vector<uint8_t> output_;
};
```

## Usage Example

```cpp
// After IR generation
IRModule* ir_module = ir_generator.generate(ast);

// Generate bytecode
BytecodeGenerator bytecode_gen(*ir_module);
std::vector<uint8_t> bytecode = bytecode_gen.generate();

// Execute in VM
VM vm;
Value result = vm.execute(bytecode.data(), bytecode.size());
```

## Optimization Opportunities

### Peephole Optimizations

```
Before:                    After:
LOAD_CONST r0, k0         LOAD_CONST r0, k0
MOVE r1, r0       →       (eliminated - use r0 directly)

Before:                    After:
ADD r0, r1, r1    →       LSH r0, r1, 1  (shift left = x*2)

Before:                    After:
LOAD_CONST r0, k0
LOAD_CONST r1, k1
ADD r2, r0, r1    →       LOAD_CONST r2, k2  (constant folding)
```

### Dead Code Elimination

```
Before:                    After:
%0 = add %1, %2           (eliminate unused %0)
%3 = mul %4, %5   →       %3 = mul %4, %5
ret %3                    ret %3
```

### Instruction Selection

- Use specialized opcodes when available
- `LOAD_CONST r0, k_true` → `LOAD_TRUE r0` (1 byte savings)
- `ADD r0, r1, r1` → left shift (faster on some architectures)

## Testing Strategy

### Unit Tests

1. **Constant Pool Tests**
   - Test deduplication
   - Test serialization
   - Test all constant types

2. **Register Allocation Tests**
   - Test simple cases (< 256 registers)
   - Test parameter allocation
   - Test live range analysis

3. **Bytecode Emission Tests**
   - Test each instruction format
   - Test jump resolution
   - Test label marking

4. **Function Compilation Tests**
   - Test each IR instruction type
   - Test control flow
   - Test function calls

### Integration Tests

```cpp
TEST(BytecodeGeneratorTest, SimpleArithmetic) {
    // fn add(a: int, b: int) -> int { return a + b; }
    IRModule module = create_test_ir();
    BytecodeGenerator gen(module);

    auto bytecode = gen.generate();

    // Verify bytecode structure
    ASSERT_GT(bytecode.size(), 0);

    // Execute and verify
    VM vm;
    vm.set_global("a", Value::int_value(10));
    vm.set_global("b", Value::int_value(20));
    Value result = vm.execute(bytecode.data(), bytecode.size());

    EXPECT_EQ(result.as_int(), 30);
}
```

## Error Handling

```cpp
class BytecodeGenerationError : public std::runtime_error {
public:
    BytecodeGenerationError(const std::string& msg, SourceLocation loc)
        : std::runtime_error(msg), location(loc) {}

    SourceLocation location;
};

// Common errors:
// - Too many registers needed (>256)
// - Jump offset too large (>16MB)
// - Invalid IR structure
// - Constant pool overflow (>65535 constants)
```

## File Format Specification

### Binary Layout

```
Offset | Size | Description
-------|------|-------------
0      | 4    | Magic number (0x564F4C54)
4      | 2    | Major version
6      | 2    | Minor version
8      | 4    | Constant pool size (bytes)
12     | 4    | Function table size (bytes)
16     | 4    | Global table size (bytes)
20     | 4    | Entry point function index
24     | N    | Constant pool data
24+N   | M    | Function table data
24+N+M | K    | Global table data
```

### Disassembly Format

For debugging, provide human-readable disassembly:

```
Function: add (arity=2, registers=3)
  0: LOAD_CONST r0, k0    ; 10
  3: LOAD_CONST r1, k1    ; 20
  6: ADD r2, r0, r1       ; r2 = r0 + r1
  9: RETURN r2            ; return r2
```

## Implementation Checklist

- [ ] Create ConstantPoolBuilder class
- [ ] Implement constant deduplication
- [ ] Create RegisterAllocator class
- [ ] Implement live range analysis
- [ ] Create BytecodeEmitter class
- [ ] Implement all instruction encoding methods
- [ ] Implement label and jump resolution
- [ ] Create FunctionCompiler class
- [ ] Implement IR instruction compilation
- [ ] Create BytecodeGenerator class
- [ ] Implement module serialization
- [ ] Add disassembler for debugging
- [ ] Write comprehensive tests
- [ ] Integrate with VM executor
- [ ] Add optimization passes

## References

- Lua VM design (register-based inspiration)
- Java bytecode format
- WebAssembly binary format
- "Crafting Interpreters" by Robert Nystrom
- Dragon Book (Compiler Design)
