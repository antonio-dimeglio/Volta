# Volta Language Features

This document provides a comprehensive overview of Volta's current feature set.

## Type System

### Primitive Types
- **Integer**: `int` (64-bit signed)
- **Float**: `float` (64-bit floating point)
- **Boolean**: `bool` (`true` / `false`)
- **String**: `str` (immutable UTF-8)
- **Void**: `void` (unit type)

### Compound Types
- **Arrays**: `Array[T]` - Dynamic, growable arrays
- **Tuples**: `(T1, T2, ...)` - Fixed-size heterogeneous collections
- **Structs**: User-defined product types with named fields
- **Enums**: Algebraic data types with variants

## Variables & Mutability

### Variable Declaration

```volta
# Immutable (default with :=)
x := 42
name := "Alice"

# Explicit type
age: int = 25

# Mutable
mut counter: int = 0
counter = counter + 1
```

### Mutability Rules

- Variables are **immutable by default**
- Use `mut` keyword for mutable variables
- Type inference with `:=` creates immutable variables
- Explicit types with `:` can be mutable or immutable

## Functions

### Basic Functions

```volta
fn add(a: int, b: int) -> int {
    return a + b
}

fn greet(name: str) {
    print("Hello, " + name)
}
```

### Mutable Parameters

Functions can accept mutable parameters:

```volta
fn increment(mut n: int) {
    n = n + 1
}

mut x: int = 10
increment(x)  # x can be modified by the function
```

**Rules:**
- Only mutable variables can be passed to `mut` parameters
- Literals and immutable variables cannot be passed to `mut` parameters
- Essential for modifying struct fields through functions

## Structs

### Basic Structs

```volta
struct Point {
    x: float,
    y: float
}

p := Point { x: 3.0, y: 4.0 }
```

### Generic Structs

Structs support type parameters:

```volta
struct Box[T] {
    value: T
}

intBox: Box[int] = Box { value: 42 }
strBox: Box[str] = Box { value: "hello" }

# Multiple type parameters
struct Pair[T, U] {
    first: T,
    second: U
}

p: Pair[int, str] = Pair { first: 1, second: "one" }
```

### Struct Methods

```volta
struct Point {
    x: float,
    y: float
}

# Instance method
fn Point.distance(self) -> float {
    return sqrt(self.x * self.x + self.y * self.y)
}

# Static method
fn Point.origin() -> Point {
    return Point { x: 0.0, y: 0.0 }
}

# Mutable method
fn Point.translate(mut self, dx: float, dy: float) {
    self.x = self.x + dx
    self.y = self.y + dy
}
```

### Generic Methods

Methods on generic structs fully support type parameters:

```volta
struct Box[T] {
    value: T
}

fn Box.new(val: T) -> Box[T] {
    return Box { value: val }
}

fn Box.getValue(self) -> T {
    return self.value
}

# Usage - type inference works!
intBox: Box[int] = Box.new(42)
value: int = intBox.getValue()  # Returns int
```

**Implementation:** Uses monomorphization (like C++ templates/Rust generics) for zero runtime overhead.

### Mutable Self

Methods can modify struct instances using `mut self`:

```volta
struct Counter {
    value: int
}

fn Counter.increment(mut self) {
    self.value = self.value + 1
}

mut counter: Counter = Counter { value: 0 }
counter.increment()  # ✓ OK - counter is mutable

counter2 := Counter { value: 0 }
counter2.increment()  # ✗ Error - counter2 is immutable
```

## Enums (Algebraic Data Types)

### Simple Enums

```volta
enum Direction {
    North,
    South,
    East,
    West
}

dir := Direction.North
```

### Enums with Data

```volta
enum Message {
    Quit,
    Move(int, int),
    Write(str),
    ChangeColor(int, int, int)
}

msg := Message.Move(10, 20)
```

### Generic Enums

```volta
enum Option[T] {
    Some(T),
    None
}

enum Result[T, E] {
    Ok(T),
    Err(E)
}

# Usage
fn divide(a: float, b: float) -> Result[float, str] {
    if b == 0.0 {
        return Err("Division by zero")
    }
    return Ok(a / b)
}
```

## Pattern Matching

### Match Expressions

```volta
# With enums
result := divide(10.0, 2.0)
match result {
    Ok(value) => print(value),
    Err(msg) => print("Error: " + msg)
}

# With literals
match x {
    0 => print("zero"),
    1 => print("one"),
    _ => print("other")
}

# Destructuring
point := Point { x: 0, y: 0 }
match point {
    Point { x: 0, y: 0 } => print("origin"),
    Point { x, y } => print("point at " + str(x) + ", " + str(y))
}
```

## Control Flow

### If/Else

```volta
if condition {
    # ...
} else if other_condition {
    # ...
} else {
    # ...
}
```

### Loops

```volta
# While loop
while condition {
    # ...
}

# For loop with ranges
for i in range(0, 10) {
    print(i)
}

# For loop with arrays
for item in array {
    print(item)
}

# Break and continue
for i in range(0, 100) {
    if i == 50 {
        break
    }
    if i % 2 == 0 {
        continue
    }
    print(i)
}
```

## Operators

### Arithmetic
- `+`, `-`, `*`, `/`, `%` (modulo)
- `**` (power)

### Comparison
- `==`, `!=`, `<`, `<=`, `>`, `>=`

### Logical
- `and`, `or`, `not`

### Assignment
- `=` (assignment)
- `:=` (declaration with type inference)

## Type Inference

Volta features powerful type inference:

```volta
# Type inferred as int
x := 42

# Type inferred from function return
result := divide(10.0, 2.0)  # Result[float, str]

# Generic type inference
box := Box.new(42)  # Box[int] inferred from argument
```

## Memory Management

- **Automatic garbage collection** for heap-allocated objects
- **Structs are heap-allocated** (reference semantics)
- **Arrays are dynamic** and heap-allocated
- **Primitive types are stack-allocated** (value semantics)

## Compilation Model

### Semantic Analysis
- Full static type checking
- Type inference
- Mutability checking
- Control flow validation

### IR Generation
- Intermediate representation (IR) generation
- **Monomorphization** for generic types (zero-cost abstractions)
- Optimization passes:
  - Constant folding
  - Constant propagation
  - Dead code elimination
  - Control flow simplification

### Bytecode Compilation
- Compiled to bytecode
- Executed by Volta VM
- Support for both interpreted and compiled execution

## Current Limitations

- Generic type parameters are erased after compilation (monomorphization only)
- No trait/interface system yet
- No closures or higher-order function types yet
- Limited standard library

## Future Features (Planned)

- Trait system for polymorphism
- Closures and lambda expressions
- Module system
- Async/await
- More comprehensive standard library
- Native code generation

---

**Last Updated:** 2025-10-08
**Language Version:** 0.1.0
