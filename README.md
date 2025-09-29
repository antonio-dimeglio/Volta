# Volta

A statically-typed interpreted language for scientific computing, inspired by Python's simplicity and designed for performance.

Named after Alessandro Volta, the Italian physicist who invented the electric battery.

## Overview

Volta combines Python's readable syntax with static typing and immutability by default, making it well-suited for scientific computing tasks where correctness and performance matter.

## Features

- **Immutability by default** - Variables are immutable unless explicitly marked with `mut`
- **Static typing with inference** - Type safety without excessive annotations
- **No null references** - Option types for safe handling of missing values
- **First-class functions** - Functions as values with full closure support
- **Pattern matching** - Elegant branching and destructuring
- **Array and matrix operations** - Built-in support for scientific computing
- **Python-like syntax** - Familiar feel for Python developers

## Quick Example

```volta
# Calculate mean and standard deviation
import stats
import math

fn analyze_data(values: Array[float]) -> Option[float] {
    if values.len() == 0 {
        return None
    }
    
    mean := stats.mean(values)
    variance := values
        .map(fn(x: float) -> float = (x - mean) ** 2.0)
        .reduce(fn(a: float, b: float) -> float = a + b, 0.0) / float(values.len())
    
    return Some(math.sqrt(variance))
}

data := [2.3, 4.5, 1.2, 8.9, 3.4]
result := analyze_data(data)

match result {
    Some(std_dev) => print("Standard deviation: " + str(std_dev)),
    None => print("No data provided")
}
```

## Building from Source

Volta is implemented in C following the architecture described in "Crafting Interpreters" by Robert Nystrom.

### Prerequisites

- GCC or Clang compiler
- Make

### Build Instructions

```bash
cd /Users/quantum/Desktop/Volta
make
```

### Running Volta

```bash
# Run a Volta script
./volta script.vlt

# Start the REPL
./volta
```

## Project Structure

```
/Users/quantum/Desktop/Volta/
├── src/           # Source code for the interpreter
├── docs/          # Documentation and specifications
├── examples/      # Example Volta programs
├── tests/         # Test suite
└── lib/           # Standard library implementation
```

## Language Guide

### Variables

```volta
# Immutable by default
x: int = 42
name: str = "Alessandro"

# Mutable when needed
counter: mut int = 0
counter = counter + 1

# Type inference
value := 3.14  # inferred as float
```

### Functions

```volta
fn add(a: int, b: int) -> int {
    return a + b
}

# Single-expression syntax
fn square(x: float) -> float = x * x

# Lambda functions
double := fn(x: int) -> int = x * 2
```

### Control Flow

```volta
# If expressions
max := if x > y { x } else { y }

# Pattern matching
match value {
    0 => print("zero"),
    n if n < 0 => print("negative"),
    _ => print("positive")
}

# Loops
for i in range(10) {
    print(i)
}
```

### Arrays and Matrices

```volta
numbers := [1, 2, 3, 4, 5]
doubled := numbers.map(fn(x: int) -> int = x * 2)
sum := numbers.reduce(fn(a: int, b: int) -> int = a + b, 0)

matrix := Matrix.from_array([[1, 2], [3, 4]])
result := matrix.transpose()
```

## Documentation

- [Language Specification](docs/specification.md)
- [BNF Grammar](docs/grammar.bnf)
- [Standard Library](docs/stdlib.md)

## Roadmap

**Phase 1: Core Language**
- Lexer and parser
- Basic types and variables
- Functions and control flow

**Phase 2: Type System**
- Static type checker
- Option types
- Pattern matching

**Phase 3: Arrays and Collections**
- Array implementation
- Array operations (map, filter, reduce)
- Matrix support

**Phase 4: Standard Library**
- Math module
- Statistics module
- I/O operations

**Phase 5: Optimization**
- Bytecode compilation
- Garbage collector tuning
- Performance benchmarks

## Contributing

Volta is in early development. Contributions, bug reports, and suggestions are welcome.

## License

MIT License - see LICENSE file for details

## Acknowledgments

This project follows the design and implementation principles from "Crafting Interpreters" by Robert Nystrom. The language design draws inspiration from Python, Rust, and OCaml.