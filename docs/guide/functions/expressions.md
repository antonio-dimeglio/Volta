# Expression Bodies

Functions can use expression syntax for single-expression bodies.

## Basic Expression Functions

```volta
# Traditional function
fn square(x: int) -> int {
    return x * x
}

# Expression function (no braces, no return keyword)
fn square(x: int) -> int = x * x
```

## Single-Expression Syntax

Use `=` instead of braces for single expressions:

```volta
fn double(x: int) -> int = x * 2

fn is_positive(n: int) -> bool = n > 0

fn circle_area(r: float) -> float = 3.14159 * r * r
```

## When to Use Expression Bodies

Expression bodies work best for simple, one-line functions:

```volta
# Good candidates for expression syntax
fn abs(x: int) -> int = if x < 0 { -x } else { x }

fn max(a: int, b: int) -> int = if a > b { a } else { b }

fn increment(x: int) -> int = x + 1
```

## Comparison

```volta
# Traditional style
fn add(a: int, b: int) -> int {
    return a + b
}

# Expression style (more concise)
fn add(a: int, b: int) -> int = a + b
```

## With If Expressions

```volta
fn sign(x: int) -> str = 
    if x > 0 { "positive" }
    else if x < 0 { "negative" }
    else { "zero" }

fn clamp(x: int, min: int, max: int) -> int =
    if x < min { min }
    else if x > max { max }
    else { x }
```

## Anonymous Functions

Expression syntax is commonly used with anonymous functions:

```volta
# Anonymous function stored in a variable
square := fn(x: int) -> int = x * x

# Used as a parameter
numbers := [1, 2, 3, 4, 5]
doubled := numbers.map(fn(x: int) -> int = x * 2)
```

## Higher-Order Functions

```volta
fn apply(f: fn(int) -> int, x: int) -> int = f(x)

increment := fn(x: int) -> int = x + 1
result := apply(increment, 5)  # 6
```

## Examples

### Math utilities

```volta
fn celsius_to_fahrenheit(c: float) -> float = c * 9.0 / 5.0 + 32.0

fn fahrenheit_to_celsius(f: float) -> float = (f - 32.0) * 5.0 / 9.0

fn pythagoras(a: float, b: float) -> float = 
    math.sqrt(a * a + b * b)
```

### Boolean logic

```volta
fn is_even(n: int) -> bool = n % 2 == 0

fn is_odd(n: int) -> bool = n % 2 != 0

fn in_range(x: int, min: int, max: int) -> bool = 
    x >= min and x <= max
```

### Array operations

```volta
# Function that creates a filter function
fn greater_than(threshold: int) -> fn(int) -> bool =
    fn(x: int) -> bool = x > threshold

numbers := [1, 5, 10, 15, 20]
filter_fn := greater_than(10)
filtered := numbers.filter(filter_fn)
```

## When to Use Block Syntax Instead

Use traditional block syntax for:
- Multiple statements
- Complex logic
- Better readability with longer expressions

```volta
# Better with block syntax
fn process(data: Array[int]) -> Option[int] {
    if data.len() == 0 {
        return None
    }
    sum: mut int = 0
    for item in data {
        sum = sum + item
    }
    return Some(sum)
}
```
