# Volta Test Programs

This directory contains Volta language test programs organized by category.

## Directory Structure

```
volta_programs/
├── basics/          # Basic language features
│   └── variables.volta
├── arrays/          # Array operations and iteration
│   └── comprehensive.volta
├── loops/           # Loop constructs (for, while)
│   ├── for_ranges.volta
│   └── nested.volta
└── edge_cases/      # Edge cases and stress tests
```

## Running Tests

### Run all tests
```bash
./tests/run_tests.sh all
```

### Run specific category
```bash
./tests/run_tests.sh arrays
./tests/run_tests.sh loops
./tests/run_tests.sh basics
./tests/run_tests.sh edge_cases
```

### Run single test manually
```bash
./bin/volta tests/volta_programs/arrays/comprehensive.volta
```

## Test Categories

### basics/
Tests for fundamental language features:
- Variable declarations (immutable and mutable)
- Basic arithmetic and operations
- Simple control flow

### arrays/
Tests for array operations:
- Array literals and initialization
- Array indexing and iteration
- For-in loops with arrays
- Break and continue in array loops

### loops/
Tests for loop constructs:
- For loops with ranges (exclusive and inclusive)
- Nested loops (array and range)
- Break and continue statements
- Loop variable scoping

### edge_cases/
Stress tests and edge cases:
- Empty arrays
- Deeply nested loops
- Complex break/continue patterns
- Mutable variable updates in loops
- Extreme nesting levels

## Adding New Tests

1. Create a new `.volta` file in the appropriate category directory
2. Add descriptive comments explaining what the test covers
3. Run the test manually first to verify it works
4. Run the test suite to ensure no regressions

## Test Output

Tests are considered passing if:
- They compile without errors
- Semantic analysis passes
- They execute without runtime errors
- They complete within the timeout (5 seconds)

Failed tests will show the compiler/runtime output for debugging.
