# Arrays

Arrays are dynamic, homogeneous collections.

## Creating Arrays

```volta
# Array literal
numbers := [1, 2, 3, 4, 5]

# With explicit type
scores: Array[int] = [85, 90, 78, 92]

# Empty array
empty: Array[float] = []
```

## Accessing Elements

```volta
numbers := [10, 20, 30, 40, 50]

first := numbers[0]      # 10
third := numbers[2]      # 30
last := numbers[4]       # 50
```

## Array Length

```volta
numbers := [1, 2, 3, 4, 5]
length := numbers.len()  # 5
```

## Array Slicing

```volta
numbers := [1, 2, 3, 4, 5]

slice := numbers[1:3]    # [2, 3]
from_start := numbers[:3] # [1, 2, 3]
to_end := numbers[2:]    # [3, 4, 5]
```

## Modifying Arrays

Arrays are immutable by default. Use mutable arrays to modify:

```volta
numbers: mut Array[int] = [1, 2, 3]

# Add element
numbers.push(4)          # [1, 2, 3, 4]

# Remove last element
last := numbers.pop()    # 4, numbers is now [1, 2, 3]

# Update element
numbers[0] = 10          # [10, 2, 3]
```

## Array Methods

### Map

Transform each element:

```volta
numbers := [1, 2, 3, 4, 5]
doubled := numbers.map(fn(x: int) -> int = x * 2)
# [2, 4, 6, 8, 10]
```

### Filter

Select elements matching a condition:

```volta
numbers := [1, 2, 3, 4, 5, 6]
evens := numbers.filter(fn(x: int) -> bool = x % 2 == 0)
# [2, 4, 6]
```

### Reduce

Combine elements into a single value:

```volta
numbers := [1, 2, 3, 4, 5]
sum := numbers.reduce(fn(a: int, b: int) -> int = a + b, 0)
# 15

product := numbers.reduce(fn(a: int, b: int) -> int = a * b, 1)
# 120
```

## Iterating Over Arrays

```volta
numbers := [1, 2, 3, 4, 5]

for num in numbers {
    print(num)
}

# With index
for i in range(numbers.len()) {
    print("Index " + str(i) + ": " + str(numbers[i]))
}
```

## Common Patterns

### Sum of array

```volta
numbers := [1, 2, 3, 4, 5]
sum := numbers.reduce(fn(a: int, b: int) -> int = a + b, 0)
```

### Find maximum

```volta
numbers := [3, 7, 2, 9, 1]
max := numbers.reduce(
    fn(a: int, b: int) -> int = if a > b { a } else { b },
    numbers[0]
)
```

### Count occurrences

```volta
numbers := [1, 2, 2, 3, 2, 4, 2]
target := 2

count := numbers.filter(fn(x: int) -> bool = x == target).len()
```

### Create array from range

```volta
fn range_array(start: int, end: int) -> Array[int] {
    result: mut Array[int] = []
    for i in range(start, end) {
        result.push(i)
    }
    return result
}

numbers := range_array(1, 6)  # [1, 2, 3, 4, 5]
```

## Multidimensional Arrays

```volta
# 2D array (array of arrays)
grid: Array[Array[int]] = [
    [1, 2, 3],
    [4, 5, 6],
    [7, 8, 9]
]

element := grid[1][2]  # 6
```

## Array Concatenation

```volta
a := [1, 2, 3]
b := [4, 5, 6]

# Using reduce or custom function
combined: mut Array[int] = []
for item in a {
    combined.push(item)
}
for item in b {
    combined.push(item)
}
# combined is [1, 2, 3, 4, 5, 6]
```
