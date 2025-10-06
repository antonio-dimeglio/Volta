# Volta Language Specification

**Version:** 0.1.0  
**Named after:** Alessandro Volta, Italian physicist and pioneer of electricity

## Overview

Volta is a statically-typed, interpreted language designed for scientific computing with Python-like syntax. It prioritizes performance through immutability by default, static typing, and efficient array operations, while maintaining the readability and ease of use that makes Python popular.

## Design Philosophy

- **Immutability by default**: Variables are immutable unless explicitly marked as mutable
- **Static typing with inference**: Type annotations are required for function signatures but can be inferred in local contexts
- **Python-familiar syntax**: Easy transition for Python developers
- **Scientific computing focus**: First-class support for arrays, matrices, and mathematical operations
- **Safe error handling**: Option types instead of null/None

## Core Syntax

### Variables and Mutability

```volta
# Immutable by default
x: int = 42
y: float = 3.14
name: str = "Alessandro"

# Mutable variables
counter: mut int = 0
counter = counter + 1

# Type inference in local scope
z := 100  # inferred as int
ratio := 2.5  # inferred as float
```

### Basic Types

- `int` - 64-bit signed integer
- `float` - 64-bit floating point
- `bool` - Boolean (true/false)
- `str` - UTF-8 string
- `Array[T]` - Dynamic array of type T
- `Matrix[T]` - 2D matrix of type T
- `Option[T]` - Optional value (Some(value) or None)

### Functions

```volta
# Function with explicit return type
fn add(a: int, b: int) -> int {
    return a + b
}

# Single-expression functions (implicit return)
fn square(x: float) -> float = x * x

# Functions are first-class values
fn apply(f: fn(int) -> int, value: int) -> int {
    return f(value)
}

# Higher-order function example
increment := fn(x: int) -> int = x + 1
result := apply(increment, 5)  # result = 6
```

### Option Types (No Null)

```volta
# Functions that might not return a value use Option
fn find_index(arr: Array[int], target: int) -> Option[int] {
    for i in range(arr.len()) {
        if arr[i] == target {
            return Some(i)
        }
    }
    return None
}

# Pattern matching on Options
result := find_index([1, 2, 3], 2)
match result {
    Some(index) => print("Found at index: " + str(index)),
    None => print("Not found")
}

# Unwrap with default
index := result.unwrap_or(0)
```

### Control Flow

```volta
# If expressions (return values)
max_val := if x > y { x } else { y }

# Traditional if statements
if score >= 90 {
    print("A")
} else if score >= 80 {
    print("B")
} else {
    print("C")
}

# While loops
counter: mut int = 0
while counter < 10 {
    print(counter)
    counter = counter + 1
}

# For loops with ranges
for i in range(10) {
    print(i)
}

# For loops with arrays
numbers := [1, 2, 3, 4, 5]
for num in numbers {
    print(num)
}
```

### Pattern Matching

```volta
# Match expressions
fn classify(x: int) -> str {
    return match x {
        0 => "zero",
        1 => "one",
        n if n < 0 => "negative",
        n if n > 100 => "large",
        _ => "other"
    }
}

# Destructuring in match
point := (3, 4)
match point {
    (0, 0) => print("Origin"),
    (x, 0) => print("On x-axis"),
    (0, y) => print("On y-axis"),
    (x, y) => print("Point at " + str(x) + ", " + str(y))
}
```

### Arrays and Matrices

```volta
# Array creation
numbers := [1, 2, 3, 4, 5]
empty: Array[float] = []

# Array operations
first := numbers[0]
length := numbers.len()
sliced := numbers[1:3]  # [2, 3]

# Array methods
doubled := numbers.map(fn(x: int) -> int = x * 2)
sum := numbers.reduce(fn(a: int, b: int) -> int = a + b, 0)
evens := numbers.filter(fn(x: int) -> bool = x % 2 == 0)

# Matrix creation
mat := Matrix.zeros(3, 3)
identity := Matrix.identity(4)
data := Matrix.from_array([[1, 2], [3, 4]])

# Matrix operations
result := data * identity
transposed := data.transpose()
det := data.determinant()
```

### Scientific Computing Features

```volta
# Built-in mathematical functions
import math

x := math.sin(3.14)
y := math.sqrt(16.0)
z := math.exp(2.0)

# Array broadcasting
arr := [1.0, 2.0, 3.0]
scaled := arr * 2.0  # [2.0, 4.0, 6.0]

# Element-wise operations
a := [1, 2, 3]
b := [4, 5, 6]
c := a + b  # [5, 7, 9]

# Dot product and linear algebra
vec1 := [1.0, 2.0, 3.0]
vec2 := [4.0, 5.0, 6.0]
dot_prod := vec1.dot(vec2)
```

### Type System Details

```volta
# Explicit type annotations
fn process(data: Array[float]) -> Option[float] {
    if data.len() == 0 {
        return None
    }
    sum := data.reduce(fn(a: float, b: float) -> float = a + b, 0.0)
    return Some(sum / float(data.len()))
}

# Generic functions
fn first[T](arr: Array[T]) -> Option[T] {
    if arr.len() > 0 {
        return Some(arr[0])
    }
    return None
}

# Type aliases
type Vector = Array[float]
type Point = (float, float)
```

### Structs (Future Extension)

```volta
# Struct definition
struct Point {
    x: float,
    y: float
}

# Struct instantiation
p := Point { x: 3.0, y: 4.0 }

# Method syntax
fn Point.distance(self) -> float {
    return math.sqrt(self.x * self.x + self.y * self.y)
}

dist := p.distance()
```

## Standard Library Modules

### Core Modules

- `math` - Mathematical functions (sin, cos, sqrt, exp, log, etc.)
- `array` - Array utilities and operations
- `matrix` - Matrix operations and linear algebra
- `io` - Input/output operations
- `random` - Random number generation
- `stats` - Statistical functions (mean, median, std, etc.)

### Example Usage

```volta
import math
import stats

data := [1.0, 2.0, 3.0, 4.0, 5.0]
mean := stats.mean(data)
std := stats.std(data)

print("Mean: " + str(mean))
print("Std Dev: " + str(std))
```

## Memory Management

- **Garbage Collection**: Automatic memory management using a generational garbage collector
- **Immutability benefits**: Immutable data structures can be safely shared without copying

## Implementation Notes

- **Interpreter written in C++**: Loosely following Crafting Interpreters architecture
- **Bytecode VM**: Compile to bytecode for better performance than tree-walk interpreter
- **Static typing**: Type checking happens before bytecode generation
- **Optimizations**: Constant folding, dead code elimination, tail call optimization

## Error Handling

```volta
# Errors use Option types instead of exceptions
fn divide(a: float, b: float) -> Option[float] {
    if b == 0.0 {
        return None
    }
    return Some(a / b)
}

# Chaining operations with Option
result := divide(10.0, 2.0)
    .map(fn(x: float) -> float = x * 2.0)
    .unwrap_or(0.0)
```

## Comments

```volta
# Single-line comment

#[
Multi-line
comment
]#
```

## Keywords

`fn`, `return`, `if`, `else`, `while`, `for`, `in`, `match`, `struct`, `import`, `mut`, `true`, `false`, `Some`, `None`, `and`, `or`, `not`

## Operators

- Arithmetic: `+`, `-`, `*`, `/`, `%`, `**` (power)
- Comparison: `==`, `!=`, `<`, `>`, `<=`, `>=`
- Logical: `and`, `or`, `not`
- Assignment: `=`, `+=`, `-=`, `*=`, `/=`
- Range: `..`, `..=` (inclusive)

## Future Roadmap

1. **Phase 1** (MVP): Variables, functions, control flow, basic types
2. **Phase 2**: Arrays, Option types, pattern matching
3. **Phase 3**: Structs, methods, modules
4. **Phase 4**: Matrix operations, scientific computing library
5. **Phase 5**: Optimizations, JIT compilation, parallel operations

---

**Motto**: *"Charge forward with clarity"* ⚡

Built with 🇮🇹 inspiration and ☕ determination.