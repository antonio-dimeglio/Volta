# Arrays

Volta provides two types of arrays for different use cases:

- **Fixed-size arrays `[T; N]`**: Compile-time sized, high-performance arrays for fixed-size data
- **Dynamic arrays `Array[T]`**: Growable collections for data of unknown size

## Fixed-Size Arrays

Fixed-size arrays have their size as part of the type and cannot be resized.

### Creating Fixed Arrays

```volta
# Fixed array with explicit values
coords: [f64; 3] = [1.0, 2.0, 3.0]

# Fixed array with repeated value
buffer: [u8; 256] = [0; 256]

# Type inference (size must match)
small: [i32; 5] = [10, 20, 30, 40, 50]
```

### When to Use Fixed Arrays

- Fixed-size buffers (e.g., network packets, file chunks)
- Coordinates and vectors (e.g., 3D positions, RGB colors)
- Small collections with known size
- Performance-critical code (zero overhead)

### Fixed Array Indexing

```volta
coords: [f64; 3] = [1.0, 2.0, 3.0]
y := coords[1]        # 2.0
coords[2] = 5.0       # Mutation allowed if array is mutable
```

## Dynamic Arrays

Dynamic arrays can grow and shrink at runtime.

### Creating Dynamic Arrays

```volta
# Empty dynamic array
numbers: Array[i32] = Array.new()

# From literal (with type annotation)
scores: Array[i32] = [85, 90, 78, 92]

# Type inference requires explicit type annotation
empty: Array[f64] = []
```

### Accessing Elements

Both array types support indexing with `[]`:

```volta
# Fixed arrays
coords: [i32; 5] = [10, 20, 30, 40, 50]
first := coords[0]      # 10
third := coords[2]      # 30

# Dynamic arrays
numbers: Array[i32] = [10, 20, 30, 40, 50]
first := numbers[0]     # 10
last := numbers[4]      # 50
```

### Array Length

```volta
# Fixed arrays - size is compile-time constant
coords: [i32; 5] = [10, 20, 30, 40, 50]
# Size is 5 (part of the type)

# Dynamic arrays - runtime length
numbers: Array[i32] = [1, 2, 3, 4, 5]
length := numbers.length()  # 5
```

## Modifying Dynamic Arrays

Dynamic arrays support growth and shrinkage:

```volta
numbers: mut Array[i32] = Array.new()

# Add elements
numbers.push(1)          # [1]
numbers.push(2)          # [1, 2]
numbers.push(3)          # [1, 2, 3]

# Remove last element
last := numbers.pop()    # 3, numbers is now [1, 2]

# Update element
numbers[0] = 10          # [10, 2]
```

**Note:** Fixed arrays cannot grow or shrink. Calling `push()` or `pop()` on a fixed array is a compile error.

## Dynamic Array Methods

Dynamic arrays provide rich functional programming methods:

### Map

Transform each element:

```volta
numbers: Array[i32] = [1, 2, 3, 4, 5]
doubled := numbers.map(fn(x: i32) -> i32 = x * 2)
# [2, 4, 6, 8, 10]
```

### Filter

Select elements matching a condition:

```volta
numbers: Array[i32] = [1, 2, 3, 4, 5, 6]
evens := numbers.filter(fn(x: i32) -> bool = x % 2 == 0)
# [2, 4, 6]
```

### Reduce

Combine elements into a single value:

```volta
numbers: Array[i32] = [1, 2, 3, 4, 5]
sum := numbers.reduce(0, fn(a: i32, b: i32) -> i32 = a + b)
# 15

product := numbers.reduce(1, fn(a: i32, b: i32) -> i32 = a * b)
# 120
```

**Note:** Fixed arrays don't have these methods. Convert to dynamic arrays first if needed.

## Iterating Over Arrays

Both array types support iteration:

```volta
# Fixed arrays
coords: [i32; 5] = [1, 2, 3, 4, 5]
for coord in coords {
    print(coord)
}

# Dynamic arrays
numbers: Array[i32] = [1, 2, 3, 4, 5]
for num in numbers {
    print(num)
}

# With index (using ranges)
for i in 0..5 {
    print("Index " + str(i) + ": " + str(coords[i]))
}
```

## Common Patterns

### Sum of dynamic array

```volta
numbers: Array[i32] = [1, 2, 3, 4, 5]
sum := numbers.reduce(0, fn(a: i32, b: i32) -> i32 = a + b)
# 15
```

### Find maximum

```volta
numbers: Array[i32] = [3, 7, 2, 9, 1]
max := numbers.reduce(numbers[0], fn(a: i32, b: i32) -> i32 = {
    if a > b { a } else { b }
})
# 9
```

### Count occurrences

```volta
numbers: Array[i32] = [1, 2, 2, 3, 2, 4, 2]
target := 2

count := numbers.filter(fn(x: i32) -> bool = x == target).length()
# 4
```

## Multidimensional Arrays

Both array types support nesting:

```volta
# Fixed 2D array (3x3 matrix)
identity: [[i32; 3]; 3] = [
    [1, 0, 0],
    [0, 1, 0],
    [0, 0, 1]
]

# Dynamic 2D array (jagged array)
grid: Array[Array[i32]] = [
    [1, 2, 3],
    [4, 5, 6],
    [7, 8, 9]
]

element := grid[1][2]  # 6
```

## Converting Between Array Types

```volta
# Fixed to dynamic (via stdlib function)
fixed: [i32; 5] = [1, 2, 3, 4, 5]
dynamic: Array[i32] = Array.from_fixed(fixed)

# Dynamic to fixed (manual copy, size must match)
dynamic: Array[i32] = [10, 20, 30]
fixed: [i32; 3] = [dynamic[0], dynamic[1], dynamic[2]]
```

## Choosing the Right Array Type

Use **fixed arrays `[T; N]`** when:
- ✅ Size is known at compile time
- ✅ Performance is critical (zero overhead)
- ✅ Working with fixed-size data (coordinates, buffers, etc.)
- ✅ Interfacing with low-level code

Use **dynamic arrays `Array[T]`** when:
- ✅ Size is unknown or varies at runtime
- ✅ Need to grow/shrink the collection
- ✅ Using functional methods (map, filter, reduce)
- ✅ Building general-purpose collections

**Rule of thumb:** If you know the size upfront and it won't change, use fixed arrays. Otherwise, use dynamic arrays.
