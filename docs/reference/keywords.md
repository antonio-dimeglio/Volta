# Keywords

Reserved words in the Volta programming language.

## Control Flow

### fn
Define a function:
```volta
fn add(a: int, b: int) -> int {
    return a + b
}
```

### return
Return a value from a function:
```volta
fn double(x: int) -> int {
    return x * 2
}
```

### if / else
Conditional execution:
```volta
if x > 0 {
    print("positive")
} else {
    print("non-positive")
}
```

### match
Pattern matching:
```volta
match value {
    0 => print("zero"),
    _ => print("other")
}
```

### for
Iterate over a range or collection:
```volta
for i in range(10) {
    print(i)
}
```

### in
Used with `for` loops:
```volta
for item in array {
    print(item)
}
```

### while
Loop while condition is true:
```volta
while counter < 10 {
    counter = counter + 1
}
```

### break
Exit a loop:
```volta
for i in range(100) {
    if i == 50 {
        break
    }
}
```

### continue
Skip to next iteration:
```volta
for i in range(10) {
    if i % 2 == 0 {
        continue
    }
    print(i)  # Only odd numbers
}
```

## Data Types

### enum
Define an enumerated type with variants:
```volta
enum Direction {
    North,
    South,
    East,
    West
}

enum Option[T] {
    Some(T),
    None
}
```

### struct
Define a custom data type:
```volta
struct Point {
    x: float,
    y: float
}
```

### mut
Declare a mutable variable:
```volta
counter: mut int = 0
counter = counter + 1
```

### import
Import a module:
```volta
import math
import stats
```

## Literals

### true
Boolean true value:
```volta
flag: bool = true
```

### false
Boolean false value:
```volta
flag: bool = false
```

### Some
Option with a value:
```volta
result: Option[int] = Some(42)
```

### None
Option without a value:
```volta
result: Option[int] = None
```

### Ok
Result with a success value:
```volta
result: Result[int, str] = Ok(42)
```

### Err
Result with an error value:
```volta
result: Result[int, str] = Err("failed")
```

## Logical Operators

### and
Logical AND:
```volta
if x > 0 and x < 100 {
    print("in range")
}
```

### or
Logical OR:
```volta
if x < 0 or x > 100 {
    print("out of range")
}
```

### not
Logical NOT:
```volta
if not is_valid {
    print("invalid")
}
```

## Reserved for Future Use

These keywords are reserved but may not be fully implemented:

- `type` - Type aliases
- `let` - Alternative variable declaration
- `const` - Compile-time constants
- `trait` - Trait definitions
- `impl` - Trait implementations
- `pub` - Public visibility
- `priv` - Private visibility
- `mod` - Module definitions
- `use` - Shorter import syntax

## Complete List

```
fn, return, if, else, while, for, in, match, enum, struct, import, mut,
true, false, Some, None, Ok, Err, and, or, not, break, continue
```

## Usage Guidelines

1. **Keywords cannot be used as identifiers**:
```volta
# Invalid
fn := 42  # Error: 'fn' is a keyword

# Valid
function := 42
```

2. **Keywords are case-sensitive**:
```volta
True   # Not a keyword (but invalid identifier)
true   # Keyword
```

3. **Keywords in strings are fine**:
```volta
text := "fn is a keyword"  # OK
```
