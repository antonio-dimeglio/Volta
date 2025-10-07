# Language Specification

Complete reference for the Volta programming language.

## Overview

Volta is a statically-typed, interpreted language designed for scientific computing with Python-like syntax.

**Version:** 0.1.0  
**Named after:** Alessandro Volta, Italian physicist and pioneer of electricity

## Design Principles

- **Immutability by default**: Variables are immutable unless marked `mut`
- **Static typing with inference**: Explicit types for function signatures, inference for locals
- **Python-familiar syntax**: Easy transition for Python developers
- **Safe error handling**: Option types instead of null/None
- **Scientific computing focus**: First-class support for arrays and mathematical operations

## Type System

### Primitive Types

- `int` - 64-bit signed integer
- `float` - 64-bit floating point
- `bool` - Boolean (true/false)
- `str` - UTF-8 string

### Collection Types

- `Array[T]` - Dynamic array of type T
- Tuple types: `(T1, T2, ...)` - Fixed-size heterogeneous collections

### Option Types

- `Option[T]` - Represents `Some(value)` or `None`
- No null/None values in the type system

### User-Defined Types

- `struct` - Custom data types with named fields

## Variables

### Immutable (default)

```volta
x: int = 42
name: str = "Volta"
```

### Mutable

```volta
counter: mut int = 0
counter = counter + 1
```

### Type Inference

```volta
x := 42        # int
y := 3.14      # float
arr := [1, 2]  # Array[int]
```

## Functions

### Declaration

```volta
fn function_name(param: type) -> return_type {
    # body
    return value
}
```

### Expression Bodies

```volta
fn square(x: int) -> int = x * x
```

### Higher-Order Functions

```volta
fn apply(f: fn(int) -> int, x: int) -> int {
    return f(x)
}
```

## Control Flow

### If Statements

```volta
if condition {
    # then branch
} else {
    # else branch
}
```

If expressions:
```volta
result := if x > 0 { "positive" } else { "negative" }
```

### Loops

While:
```volta
while condition {
    # body
}
```

For with range:
```volta
for i in 0..10 {
    # body
}

# Or inclusive
for i in 0..=10 {
    # body
}
```

For with arrays:
```volta
for item in array {
    # body
}
```

Break and continue:
```volta
for i in range(100) {
    if i == 50 { break }
    if i % 2 == 0 { continue }
}
```

### Match Expressions

```volta
match value {
    pattern1 => expression1,
    pattern2 if guard => expression2,
    _ => default_expression
}
```

Patterns:
- Literals: `0`, `"text"`, `true`
- Identifiers: `x` (binds value)
- Wildcards: `_`
- Tuples: `(x, y)`
- Guards: `n if n > 0`

## Structs

### Definition

```volta
struct TypeName {
    field1: type1,
    field2: type2
}
```

### Instantiation

```volta
instance := TypeName {
    field1: value1,
    field2: value2
}
```

### Methods

```volta
fn TypeName.method_name(self, param: type) -> return_type {
    # body
}
```

## Arrays

### Creation

```volta
arr := [1, 2, 3, 4, 5]
empty: Array[int] = []
```

### Operations

- `arr[i]` - Index access
- `arr[i:j]` - Slicing
- `arr.len()` - Length
- `arr.push(x)` - Append (mutable)
- `arr.pop()` - Remove last (mutable)

### Methods

- `map(f)` - Transform elements
- `filter(pred)` - Select elements
- `reduce(f, init)` - Combine elements

## Option Types

### Creation

```volta
some: Option[int] = Some(42)
none: Option[int] = None
```

### Pattern Matching

```volta
match option {
    Some(value) => # use value,
    None => # handle absence
}
```

### Methods

- `unwrap()` - Get value or panic
- `unwrap_or(default)` - Get value or default
- `map(f)` - Transform if Some
- `and_then(f)` - Chain operations

## Error Handling

Volta uses Option types for error handling:

```volta
fn divide(a: float, b: float) -> Option[float] {
    if b == 0.0 {
        return None
    }
    return Some(a / b)
}
```

## Operators

### Arithmetic

`+`, `-`, `*`, `/`, `%`, `**`

### Comparison

`==`, `!=`, `<`, `>`, `<=`, `>=`

### Logical

`and`, `or`, `not`

### Assignment

`=`, `+=`, `-=`, `*=`, `/=`

## Type Conversion

Explicit casting required:

```volta
x: int = 42
y: float = float(x)

text := str(123)

num := parse_int("123")  # Option[int]
```

## Comments

Single-line:
```volta
# This is a comment
```

Multi-line:
```volta
#[
Multi-line
comment
]#
```

## Modules

Import syntax:
```volta
import math
import stats

result := math.sqrt(16.0)
```

## Standard Library

Core modules:
- `math` - Mathematical functions
- `array` - Array utilities
- `stats` - Statistical functions
- `io` - Input/output
- `random` - Random number generation

## Memory Management

- Garbage collected
- Automatic memory management
- Immutable data can be safely shared

## Keywords

```
fn, return, if, else, while, for, in, match, struct, import, mut,
true, false, Some, None, and, or, not, break, continue
```

## Operator Precedence

1. `**`
2. unary `-`, `not`
3. `*`, `/`, `%`
4. `+`, `-`
5. `<`, `>`, `<=`, `>=`
6. `==`, `!=`
7. `and`
8. `or`

## Best Practices

1. **Prefer immutability** - Use `mut` only when necessary
2. **Use Option types** - Avoid null/None-like values
3. **Pattern match Options** - Safer than unwrap
4. **Explicit types in signatures** - Required and improves clarity
5. **Type inference locally** - Use `:=` for local variables

## Future Features

- Generics and type parameters
- Advanced pattern matching
- Module system enhancements
- Traits/interfaces
- Async/await
- JIT compilation

---

**Motto**: *"Charge forward with clarity"* ⚡
