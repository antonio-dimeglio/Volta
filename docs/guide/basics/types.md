# Data Types

Volta is a statically-typed language with type inference support.

## Basic Types

### Integer

64-bit signed integer:

```volta
x: int = 42
y: int = -100
large := 1000000
```

### Float

64-bit floating point:

```volta
pi: float = 3.14159
temp := -40.5
ratio := 0.75
```

### Boolean

Boolean values (`true` or `false`):

```volta
is_active: bool = true
has_error := false
```

### String

UTF-8 strings:

```volta
name: str = "Volta"
message := "Hello, World!"
```

## Collection Types

### Arrays

Dynamic arrays of a single type:

```volta
numbers: Array[int] = [1, 2, 3, 4, 5]
empty: Array[float] = []
names := ["Alice", "Bob", "Charlie"]
```

### Tuples

Fixed-size collections of mixed types:

```volta
point: (int, int) = (3, 4)
person := ("Alice", 30, true)
```

## Option Types

Represents values that may or may not exist:

```volta
fn find(arr: Array[int], target: int) -> Option[int] {
    # Returns Some(value) or None
}

result: Option[int] = Some(42)
empty: Option[str] = None
```

## Type Annotations

Function parameters and return types require explicit annotations:

```volta
fn process(data: Array[float], threshold: float) -> Option[float] {
    # Function body
}
```

Local variables can use type inference:

```volta
x := 42              # int
y := 3.14            # float
items := [1, 2, 3]   # Array[int]
```
