# Data Types

Volta is a statically-typed language with type inference support.

## Numeric Types

Volta provides fine-grained numeric types for precise control over data representation, C FFI compatibility, and GPU computing.

### Signed Integers

```volta
tiny: i8 = 127          # 8-bit signed (-128 to 127)
small: i16 = 32000      # 16-bit signed (-32,768 to 32,767)
normal: i32 = 2000000   # 32-bit signed (default for integer literals)
big: i64 = 9000000000   # 64-bit signed
```

### Unsigned Integers

```volta
byte: u8 = 255          # 8-bit unsigned (0 to 255)
word: u16 = 65000       # 16-bit unsigned (0 to 65,535)
dword: u32 = 4000000000 # 32-bit unsigned
qword: u64 = 18000000000000000000  # 64-bit unsigned
```

### Floating Point

```volta
half: f16 = 3.14        # 16-bit float (GPU/ML)
single: f32 = 3.14159   # 32-bit float (default for float literals)
double: f64 = 3.141592653589793  # 64-bit float (high precision)
mini: f8 = 1.5          # 8-bit float (experimental, GPU/ML)
```

### Type Inference

Integer and float literals use default types:

```volta
x := 42        # i32 (default integer type)
y := 3.14      # f32 (default float type)
```

### Implicit Conversions

Volta allows implicit conversions between all numeric types:

```volta
x: i32 = 42
y: f64 = x     # i32 -> f64 conversion
z: i64 = y     # f64 -> i64 conversion
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
numbers: Array[i32] = [1, 2, 3, 4, 5]
empty: Array[f32] = []
names := ["Alice", "Bob", "Charlie"]
```

### Tuples

Fixed-size collections of mixed types:

```volta
point: (i32, i32) = (3, 4)
person := ("Alice", 30, true)
```

## Option Types

Represents values that may or may not exist:

```volta
fn find(arr: Array[i32], target: i32) -> Option[i32] {
    # Returns Some(value) or None
}

result: Option[i32] = Some(42)
empty: Option[str] = None
```

## Type Annotations

Function parameters and return types require explicit annotations:

```volta
fn process(data: Array[f32], threshold: f32) -> Option[f32] {
    # Function body
}
```

Local variables can use type inference:

```volta
x := 42              # i32
y := 3.14            # f32
items := [1, 2, 3]   # Array[i32]
```
