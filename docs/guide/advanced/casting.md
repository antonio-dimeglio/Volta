# Type Casting

Type casting allows you to convert between different types.

## Basic Casting

Volta provides built-in functions for type conversion:

```volta
# Integer to float
x: int = 42
y: float = float(x)  # 42.0

# Float to integer (truncates)
a: float = 3.14
b: int = int(a)  # 3

# To string
num := 42
text := str(num)  # "42"

pi := 3.14159
pi_str := str(pi)  # "3.14159"

flag := true
flag_str := str(flag)  # "true"
```

## Numeric Conversions

### Integer to Float

```volta
x: int = 10
y: float = float(x)  # 10.0

# In expressions
result := float(5) / float(2)  # 2.5 (not 2)
```

### Float to Integer

```volta
# Truncates decimal part
x: float = 3.99
y: int = int(x)  # 3

x2: float = -2.7
y2: int = int(x2)  # -2
```

## String Conversions

### To String

```volta
# Numbers to string
num := str(42)        # "42"
pi := str(3.14)       # "3.14"

# Boolean to string
flag := str(true)     # "true"

# In concatenation
age := 25
message := "Age: " + str(age)
```

### From String

```volta
# Parse integer
text := "42"
num := parse_int(text)  # Option[int]

match num {
    Some(n) => print("Parsed: " + str(n)),
    None => print("Parse error")
}

# Parse float
text2 := "3.14"
pi := parse_float(text2)  # Option[float]
```

## Safe Parsing

Parsing returns `Option` types for safety:

```volta
fn parse_int(s: str) -> Option[int] {
    # Implementation
}

fn parse_float(s: str) -> Option[float] {
    # Implementation
}

# Usage
result := parse_int("123")
value := result.unwrap_or(0)

# Invalid parse
bad := parse_int("not a number")  # None
```

## Boolean Conversions

```volta
# To string
flag := true
text := str(flag)  # "true"

# Truthiness (not implicit)
x := 5
# if x {  # Error! Must use explicit comparison
if x > 0 {
    # Correct
}
```

## Array Element Conversion

```volta
# Convert all elements
int_array := [1, 2, 3, 4, 5]
float_array := int_array.map(fn(x: int) -> float = float(x))
# [1.0, 2.0, 3.0, 4.0, 5.0]

string_array := int_array.map(fn(x: int) -> str = str(x))
# ["1", "2", "3", "4", "5"]
```

## Common Patterns

### Parse with default

```volta
fn parse_with_default(s: str, default: int) -> int {
    result := parse_int(s)
    return result.unwrap_or(default)
}

value := parse_with_default("123", 0)  # 123
invalid := parse_with_default("bad", 0)  # 0
```

### Safe numeric operations

```volta
# Ensure float division
a: int = 5
b: int = 2

result := float(a) / float(b)  # 2.5

# Without casting
bad_result := a / b  # 2 (integer division)
```

### Format numbers

```volta
fn format_price(amount: float) -> str {
    return "$" + str(amount)
}

price := format_price(19.99)  # "$19.99"
```

## Type Checking

Volta is statically typed, so types are checked at compile time:

```volta
x: int = 42
y: float = x  # Error! Type mismatch

# Must explicitly cast
y: float = float(x)  # OK
```

## Practical Examples

### User input processing

```volta
fn get_age_from_input(input: str) -> Option[int] {
    age := parse_int(input)
    
    return match age {
        None => None,
        Some(n) => {
            if n > 0 and n < 150 {
                Some(n)
            } else {
                None
            }
        }
    }
}
```

### Temperature conversion

```volta
fn celsius_to_fahrenheit(celsius: float) -> float {
    return celsius * 9.0 / 5.0 + 32.0
}

fn fahrenheit_to_celsius(fahrenheit: float) -> float {
    return (fahrenheit - 32.0) * 5.0 / 9.0
}

# With integer input
temp_c: int = 25
temp_f := celsius_to_fahrenheit(float(temp_c))
```

### Data formatting

```volta
fn format_user_info(name: str, age: int, score: float) -> str {
    return name + " (age " + str(age) + ") - Score: " + str(score)
}

info := format_user_info("Alice", 30, 95.5)
# "Alice (age 30) - Score: 95.5"
```

### Array statistics

```volta
fn average(numbers: Array[int]) -> Option[float] {
    if numbers.len() == 0 {
        return None
    }
    
    sum := numbers.reduce(fn(a: int, b: int) -> int = a + b, 0)
    
    # Cast to float for division
    return Some(float(sum) / float(numbers.len()))
}

nums := [1, 2, 3, 4, 5]
avg := average(nums)  # Some(3.0)
```

## Best Practices

1. **Explicit casting required**:
```volta
# No implicit conversion
x: int = 42
y: float = float(x)  # Must cast explicitly
```

2. **Use parse functions** for strings:
```volta
num := parse_int(text).unwrap_or(0)
```

3. **Handle parse errors**:
```volta
match parse_int(input) {
    Some(n) => # use n,
    None => # handle error
}
```

4. **Cast before division** when precision matters:
```volta
result := float(a) / float(b)  # Preserves decimal
```
