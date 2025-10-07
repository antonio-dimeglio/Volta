# Volta Test Suite

This directory contains all C++ unit tests for the Volta compiler, organized by component.

## Directory Structure

```
tests/
├── lexer/           # Lexer/tokenizer tests
├── parser/          # Parser tests
├── semantic/        # Semantic analyzer tests
├── ir/              # IR generation and manipulation tests
├── bytecode/        # Bytecode compiler tests
├── vm/              # Virtual machine tests
├── integration/     # End-to-end integration tests
└── volta_programs/  # Volta language test programs
```

## Running Tests

### Individual Tests

Each test file can be run individually:

```bash
# Lexer tests
make test-lexer
make test-lexer-missing

# Parser tests
make test-parser

# Semantic tests
make test-semantic

# IR tests
make test-ir-basicblock
make test-ir-builder
make test-ir-function
make test-ir-generator
make test-ir-instruction
make test-ir-integration
make test-ir-module
make test-ir-optimization
make test-ir-printer
make test-ir-value
make test-ir-verifier

# Bytecode tests
make test-bytecode-compiler
make test-bytecode-module

# VM tests
make test-vm-arrays
make test-vm-end-to-end

# Integration tests
make test-end-to-end-ir
```

### Test Categories

Run all tests in a category:

```bash
make test-lexer-all       # All lexer tests
make test-parser-all      # All parser tests
make test-semantic-all    # All semantic tests
make test-ir-all          # All IR tests
make test-bytecode-all    # All bytecode tests
make test-vm-all          # All VM tests
make test-integration-all # All integration tests
```

### All Tests

```bash
# Run all C++ tests individually
make test-all-individual

# Run all C++ tests in one executable (faster)
make test

# Build all test executables
make build-tests
```

## Test Organization

### lexer/
Tests for the lexical analyzer (tokenizer):
- Token recognition
- Error handling
- Edge cases (unterminated strings, etc.)
- Keywords and operators

### parser/
Tests for the syntax parser:
- Statement parsing
- Expression parsing
- AST generation
- Error recovery

### semantic/
Tests for semantic analysis:
- Type checking
- Variable scoping
- Function resolution
- Error detection

### ir/
Tests for intermediate representation:
- **test_ir_basicblock.cpp** - Basic block operations
- **test_ir_builder.cpp** - IR builder functionality
- **test_ir_function.cpp** - Function IR generation
- **test_ir_generator.cpp** - General IR generation
- **test_ir_instruction.cpp** - Individual instructions
- **test_ir_integration.cpp** - IR integration tests
- **test_ir_module.cpp** - Module-level IR
- **test_ir_optimization.cpp** - Optimization passes
- **test_ir_printer.cpp** - IR pretty printing
- **test_ir_value.cpp** - IR value types
- **test_ir_verifier.cpp** - IR verification

### bytecode/
Tests for bytecode generation:
- Bytecode compilation
- Register allocation
- Instruction encoding

### vm/
Tests for virtual machine:
- Instruction execution
- Array operations
- GC integration
- End-to-end execution

### integration/
End-to-end tests that exercise multiple components:
- Full compilation pipeline
- Complex programs
- Performance tests

## Test Executables

Each test file compiles to its own executable in `bin/tests/`:

```
bin/tests/
├── lexer/
│   ├── test_lexer
│   └── test_lexer_missing_features
├── parser/
│   └── test_parser
├── semantic/
│   └── test_semantic
├── ir/
│   ├── test_ir_basicblock
│   ├── test_ir_builder
│   └── ...
├── bytecode/
│   ├── test_bytecode_compiler
│   └── test_bytecode_module
├── vm/
│   ├── test_vm_arrays
│   └── test_vm_end_to_end
└── integration/
    └── test_end_to_end_ir
```

## Adding New Tests

1. **Create test file** in appropriate category:
   ```bash
   vim tests/<category>/test_<component>.cpp
   ```

2. **Write Google Test cases**:
   ```cpp
   #include <gtest/gtest.h>
   #include "<component>.hpp"
   
   TEST(ComponentTest, BasicFunctionality) {
       // Test code here
       EXPECT_EQ(result, expected);
   }
   ```

3. **Add Makefile target** (optional - auto-generated):
   ```makefile
   test-<name>: $(BIN_DIR)/tests/<category>/test_<name>
       @echo "Running <name> tests..."
       @$<
   ```

4. **Run the test**:
   ```bash
   make test-<name>
   ```

## Test Framework

All C++ tests use **Google Test** (gtest):

```cpp
#include <gtest/gtest.h>

TEST(TestSuiteName, TestName) {
    EXPECT_EQ(actual, expected);
    ASSERT_TRUE(condition);
}
```

Common assertions:
- `EXPECT_EQ(a, b)` - Expect equal
- `EXPECT_NE(a, b)` - Expect not equal
- `EXPECT_TRUE(x)` - Expect true
- `EXPECT_FALSE(x)` - Expect false
- `ASSERT_*` - Same as EXPECT but fatal

## Debugging Tests

### Run with verbose output
```bash
./bin/tests/lexer/test_lexer --gtest_filter=LexerTest.BasicTokens
```

### Run specific test
```bash
./bin/tests/ir/test_ir_optimization --gtest_filter=OptimizationTest.ConstantFolding
```

### List all tests
```bash
./bin/tests/lexer/test_lexer --gtest_list_tests
```

## CI/CD Integration

```yaml
# Example GitHub Actions
- name: Build
  run: make

- name: Run All C++ Tests
  run: make test-all-individual

- name: Run Specific Category
  run: make test-ir-all
```

## Quick Reference

```bash
# Build & run all tests
make test

# Run tests by category
make test-lexer-all
make test-ir-all
make test-vm-all

# Run specific test
make test-lexer
make test-vm-arrays

# Build all test executables
make build-tests

# Clean and rebuild
make clean && make test-ir-all
```

## Test Coverage

Current test count by category:
- Lexer: 20+ tests
- Parser: 10+ tests
- Semantic: 15+ tests
- IR: 100+ tests across 11 files
- Bytecode: 20+ tests
- VM: 10+ tests
- Integration: 5+ tests

**Total: 180+ C++ unit tests**
