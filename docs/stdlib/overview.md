# Standard Library Overview

Volta provides a standard library with essential functions and modules for common programming tasks.

## Core Modules

### math

Mathematical functions and constants:

```volta
import math

# Trigonometric functions
sin_val := math.sin(3.14)
cos_val := math.cos(0.0)
tan_val := math.tan(1.57)

# Square root and powers
sqrt_val := math.sqrt(16.0)  # 4.0
pow_val := math.pow(2.0, 8.0)  # 256.0

# Exponential and logarithm
exp_val := math.exp(1.0)  # e
log_val := math.log(10.0)  # natural log
```

### array

Array utilities and operations:

```volta
import array

# Array creation helpers
zeros := array.zeros(5)  # [0, 0, 0, 0, 0]
ones := array.ones(3)    # [1, 1, 1]
range := array.range(1, 6)  # [1, 2, 3, 4, 5]

# Array operations
sum := array.sum([1, 2, 3, 4, 5])  # 15
max := array.max([3, 7, 2, 9])     # 9
min := array.min([3, 7, 2, 9])     # 2
```

### stats

Statistical functions:

```volta
import stats

data := [1.0, 2.0, 3.0, 4.0, 5.0]

mean := stats.mean(data)      # 3.0
median := stats.median(data)  # 3.0
std_dev := stats.std(data)    # Standard deviation
variance := stats.var(data)   # Variance
```

### io

Input/output operations:

```volta
import io

# Print functions
io.print("Hello")
io.println("Hello with newline")

# File operations (future)
content := io.read_file("data.txt")
io.write_file("output.txt", "content")
```

### random

Random number generation:

```volta
import random

# Random float between 0 and 1
r := random.random()

# Random integer in range
dice := random.randint(1, 6)

# Random choice from array
items := ["a", "b", "c"]
choice := random.choice(items)
```

## Built-in Functions

Functions available without imports:

```volta
# Type conversions
int_val := int(3.14)      # 3
float_val := float(5)     # 5.0
str_val := str(42)        # "42"

# Parsing
num := parse_int("123")   # Option[int]
pi := parse_float("3.14") # Option[float]

# Output
print("Hello, World!")

# Array length
arr := [1, 2, 3]
length := arr.len()       # 3

# Range operators (not functions!)
for i in 0..10 {
    # 0 to 9 (exclusive)
}

for i in 0..=10 {
    # 0 to 10 (inclusive)
}
```

## Module System

Import modules to use their functions:

```volta
import math
import stats

# Use module functions
result := math.sqrt(16.0)
average := stats.mean([1.0, 2.0, 3.0])
```

## Common Operations

### Math operations

```volta
import math

# Constants
pi := math.PI
e := math.E

# Rounding
ceil_val := math.ceil(3.2)    # 4.0
floor_val := math.floor(3.8)  # 3.0
round_val := math.round(3.5)  # 4.0

# Absolute value
abs_val := math.abs(-5.0)     # 5.0
```

### Array operations

```volta
# Built-in array methods
numbers := [1, 2, 3, 4, 5]

# Transform
doubled := numbers.map(fn(x: int) -> int = x * 2)

# Filter
evens := numbers.filter(fn(x: int) -> bool = x % 2 == 0)

# Reduce
sum := numbers.reduce(fn(a: int, b: int) -> int = a + b, 0)
```

### String operations

```volta
text := "Hello, Volta"

# Length
len := text.len()

# Contains
has_volta := text.contains("Volta")  # true

# Split
parts := text.split(",")  # ["Hello", " Volta"]

# Case conversion
upper := text.to_upper()  # "HELLO, VOLTA"
lower := text.to_lower()  # "hello, volta"
```

## Future Additions

Planned standard library modules:

- **string**: Advanced string manipulation
- **collections**: Additional data structures (HashMap, Set)
- **time**: Date and time operations
- **json**: JSON parsing and serialization
- **http**: HTTP client (future)
- **testing**: Unit testing framework

## Usage Examples

### Scientific computing

```volta
import math
import stats

fn analyze_data(data: Array[float]) -> (float, float, float) {
    mean := stats.mean(data)
    std := stats.std(data)
    
    # Normalize data
    normalized := data.map(fn(x: float) -> float = 
        (x - mean) / std
    )
    
    return (mean, std, stats.mean(normalized))
}
```

### Random sampling

```volta
import random

fn sample(arr: Array[int], n: int) -> Array[int] {
    result: mut Array[int] = []
    for i in range(n) {
        choice := random.choice(arr)
        result.push(choice)
    }
    return result
}
```

### Statistical analysis

```volta
import stats

fn summarize(numbers: Array[float]) {
    print("Mean: " + str(stats.mean(numbers)))
    print("Median: " + str(stats.median(numbers)))
    print("Std Dev: " + str(stats.std(numbers)))
}

data := [1.5, 2.3, 3.7, 2.8, 4.1]
summarize(data)
```
