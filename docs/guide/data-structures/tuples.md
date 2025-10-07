# Tuples

Tuples are fixed-size collections of mixed types.

## Creating Tuples

```volta
# Basic tuple
point := (3, 4)

# With explicit type
coordinates: (int, int) = (10, 20)

# Mixed types
person := ("Alice", 30, true)
```

## Tuple Types

Tuples can contain any combination of types:

```volta
# (int, int)
point: (int, int) = (3, 4)

# (str, int, bool)
user: (str, int, bool) = ("Bob", 25, true)

# (float, float, float)
rgb: (float, float, float) = (0.5, 0.8, 0.2)
```

## Accessing Tuple Elements

Use destructuring or indexing:

```volta
point := (3, 4)

# Destructuring
(x, y) := point

# Indexing
x := point.0
y := point.1
```

## Destructuring

```volta
person := ("Alice", 30, true)

# Destructure all elements
(name, age, active) := person

print(name)   # "Alice"
print(age)    # 30
print(active) # true
```

## Tuples as Return Values

Functions can return multiple values using tuples:

```volta
fn divide_with_remainder(a: int, b: int) -> (int, int) {
    quotient := a / b
    remainder := a % b
    return (quotient, remainder)
}

(q, r) := divide_with_remainder(17, 5)
# q = 3, r = 2
```

## Pattern Matching with Tuples

```volta
point := (3, 4)

match point {
    (0, 0) => print("Origin"),
    (x, 0) => print("On x-axis: " + str(x)),
    (0, y) => print("On y-axis: " + str(y)),
    (x, y) => print("Point: (" + str(x) + ", " + str(y) + ")")
}
```

## Common Use Cases

### Coordinates

```volta
fn distance(p1: (float, float), p2: (float, float)) -> float {
    (x1, y1) := p1
    (x2, y2) := p2
    
    dx := x2 - x1
    dy := y2 - y1
    
    return math.sqrt(dx * dx + dy * dy)
}

p1 := (0.0, 0.0)
p2 := (3.0, 4.0)
dist := distance(p1, p2)  # 5.0
```

### Min and Max

```volta
fn min_max(arr: Array[int]) -> (int, int) {
    min := arr[0]
    max := arr[0]
    
    for num in arr {
        if num < min { min = num }
        if num > max { max = num }
    }
    
    return (min, max)
}

(minimum, maximum) := min_max([3, 1, 4, 1, 5, 9])
```

### Swap values

```volta
a := 10
b := 20

# Swap using tuple
(a, b) := (b, a)
# Now a = 20, b = 10
```

## Nested Tuples

```volta
# Tuple containing tuples
line := ((0, 0), (10, 10))

((x1, y1), (x2, y2)) := line

# Triangle as three points
triangle := ((0, 0), (5, 0), (2, 4))
```

## Tuples in Arrays

```volta
# Array of tuples
points: Array[(int, int)] = [(1, 2), (3, 4), (5, 6)]

for (x, y) in points {
    print("(" + str(x) + ", " + str(y) + ")")
}
```

## Immutability

Tuples are immutable:

```volta
point := (3, 4)

# Cannot modify
# point.0 = 5  # Error!

# Create new tuple instead
new_point := (5, 4)
```

## Practical Examples

### RGB Color

```volta
type Color = (float, float, float)

fn create_color(r: float, g: float, b: float) -> Color {
    return (r, g, b)
}

red := create_color(1.0, 0.0, 0.0)
(r, g, b) := red
```

### Statistics

```volta
fn stats(numbers: Array[float]) -> (float, float, float) {
    sum := numbers.reduce(fn(a: float, b: float) -> float = a + b, 0.0)
    mean := sum / float(numbers.len())
    
    min := numbers[0]
    max := numbers[0]
    
    for num in numbers {
        if num < min { min = num }
        if num > max { max = num }
    }
    
    return (mean, min, max)
}

(avg, minimum, maximum) := stats([1.5, 2.7, 3.2, 1.8])
```
