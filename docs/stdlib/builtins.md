# Built-in Functions

Functions available without importing any modules.

## Type Conversion

### int()

Convert to integer:

```volta
x := int(3.14)     # 3
y := int(5.99)     # 5
z := int(-2.7)     # -2

# From string (use parse_int instead)
```

### float()

Convert to float:

```volta
x := float(42)     # 42.0
y := float(5)      # 5.0

# For division
result := float(5) / float(2)  # 2.5
```

### str()

Convert to string:

```volta
num := str(42)         # "42"
pi := str(3.14)        # "3.14"
flag := str(true)      # "true"
flag2 := str(false)    # "false"

# In concatenation
age := 25
message := "Age: " + str(age)
```

## Parsing

### parse_int()

Parse string to integer:

```volta
fn parse_int(s: str) -> Option[int]

# Usage
result := parse_int("123")
match result {
    Some(n) => print("Number: " + str(n)),
    None => print("Parse error")
}

# With default
value := parse_int("42").unwrap_or(0)
```

### parse_float()

Parse string to float:

```volta
fn parse_float(s: str) -> Option[float]

# Usage
result := parse_float("3.14")
match result {
    Some(f) => print("Float: " + str(f)),
    None => print("Parse error")
}
```

## Output

### print()

Print without newline:

```volta
print("Hello")
print(" ")
print("World")
# Output: Hello World
```

### println()

Print with newline:

```volta
println("Hello")
println("World")
# Output:
# Hello
# World
```

## Array Functions

### len()

Get array length:

```volta
arr := [1, 2, 3, 4, 5]
length := arr.len()  # 5

empty: Array[int] = []
size := empty.len()  # 0
```

### push()

Add element to array:

```volta
arr: mut Array[int] = [1, 2, 3]
arr.push(4)
# arr is now [1, 2, 3, 4]
```

### pop()

Remove and return last element:

```volta
arr: mut Array[int] = [1, 2, 3]
last := arr.pop()  # 3
# arr is now [1, 2]
```

## Range Operators

### `..` (Exclusive Range)

Create a range that doesn't include the end value:

```volta
for i in 0..5 {
    print(i)  # 0, 1, 2, 3, 4
}

for i in 1..10 {
    print(i)  # 1, 2, 3, 4, 5, 6, 7, 8, 9
}
```

### `..=` (Inclusive Range)

Create a range that includes the end value:

```volta
for i in 0..=5 {
    print(i)  # 0, 1, 2, 3, 4, 5
}

for i in 1..=10 {
    print(i)  # 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
}
```

**Note:** Volta uses range operators (`..` and `..=`), not a `range()` function like Python.

## Array Methods

### map()

Transform each element:

```volta
fn map(f: fn(T) -> U) -> Array[U]

numbers := [1, 2, 3, 4, 5]
doubled := numbers.map(fn(x: int) -> int = x * 2)
# [2, 4, 6, 8, 10]
```

### filter()

Select elements:

```volta
fn filter(predicate: fn(T) -> bool) -> Array[T]

numbers := [1, 2, 3, 4, 5, 6]
evens := numbers.filter(fn(x: int) -> bool = x % 2 == 0)
# [2, 4, 6]
```

### reduce()

Combine elements:

```volta
fn reduce(f: fn(T, T) -> T, initial: T) -> T

numbers := [1, 2, 3, 4, 5]
sum := numbers.reduce(fn(a: int, b: int) -> int = a + b, 0)
# 15
```

## String Methods

### len()

String length:

```volta
text := "Hello"
length := text.len()  # 5
```

### contains()

Check substring:

```volta
text := "Hello, World"
has_world := text.contains("World")  # true
has_foo := text.contains("foo")      # false
```

### split()

Split string:

```volta
text := "a,b,c"
parts := text.split(",")  # ["a", "b", "c"]
```

### to_upper()

Convert to uppercase:

```volta
text := "hello"
upper := text.to_upper()  # "HELLO"
```

### to_lower()

Convert to lowercase:

```volta
text := "HELLO"
lower := text.to_lower()  # "hello"
```

## Option Methods

### unwrap()

Get value or panic:

```volta
opt: Option[int] = Some(42)
value := opt.unwrap()  # 42

# WARNING: Panics if None
# bad: Option[int] = None
# value := bad.unwrap()  # Runtime error!
```

### unwrap_or()

Get value with default:

```volta
opt: Option[int] = None
value := opt.unwrap_or(0)  # 0

some: Option[int] = Some(42)
value2 := some.unwrap_or(0)  # 42
```

### map()

Transform option value:

```volta
opt: Option[int] = Some(5)
doubled := opt.map(fn(x: int) -> int = x * 2)
# Some(10)
```

### and_then()

Chain options:

```volta
result := Some(10)
    .and_then(fn(x: int) -> Option[int] = 
        if x > 5 { Some(x * 2) } else { None }
    )
# Some(20)
```

## Practical Examples

### Type conversion pipeline

```volta
fn process_input(input: str) -> Option[float] {
    return parse_int(input)
        .map(fn(x: int) -> float = float(x))
        .map(fn(x: float) -> float = x * 2.0)
}

result := process_input("21")  # Some(42.0)
```

### Array statistics

```volta
fn sum(arr: Array[int]) -> int {
    return arr.reduce(fn(a: int, b: int) -> int = a + b, 0)
}

fn average(arr: Array[int]) -> Option[float] {
    if arr.len() == 0 {
        return None
    }
    total := sum(arr)
    return Some(float(total) / float(arr.len()))
}
```

### String processing

```volta
fn count_words(text: str) -> int {
    words := text.split(" ")
    return words.len()
}

fn to_title_case(text: str) -> str {
    words := text.split(" ")
    # Process each word...
    return # joined result
}
```
