# Type Casting

Type casting allows you to convert between different types using the `as` keyword.

## Numeric Type Casting

Volta supports explicit casting between all numeric types using the `as` operator:

```volta
# Signed integer conversions
x: i32 = 42
y: i64 = x as i64    # Widen to 64-bit
z: i16 = x as i16    # Narrow to 16-bit (may truncate)

# Float conversions
a: f32 = 3.14
b: f64 = a as f64    # Widen precision
c: f16 = a as f16    # Reduce precision

# Integer to float
num: i32 = 100
flt: f32 = num as f32  # 100.0

# Float to integer (truncates)
pi: f32 = 3.14
int_pi: i32 = pi as i32  # 3
```

## Implicit Numeric Conversions

Volta allows implicit conversions between numeric types for convenience:

```volta
x: i32 = 42
y: f64 = x     # Implicit i32 -> f64
z: i64 = y     # Implicit f64 -> i64

# In function calls
fn calculate(value: f64) -> f64 {
    return value * 2.0
}

result := calculate(42)  # i32 literal implicitly converts to f64
```

## Type-Specific Conversions

### Widening Conversions (Safe)

```volta
# Signed integers: i8 -> i16 -> i32 -> i64
byte: i8 = 100
word: i16 = byte as i16  # Safe, no data loss
dword: i32 = word as i32
qword: i64 = dword as i64

# Unsigned integers: u8 -> u16 -> u32 -> u64
ubyte: u8 = 200
uword: u16 = ubyte as u16  # Safe
udword: u32 = uword as u32
uqword: u64 = udword as u64

# Floats: f16 -> f32 -> f64
half: f16 = 1.5
single: f32 = half as f32   # Safe
double: f64 = single as f64  # Safe
```

### Narrowing Conversions (May Truncate)

```volta
# Integer narrowing
big: i64 = 1000000
small: i16 = big as i16  # May overflow/truncate

# Float narrowing
precise: f64 = 3.141592653589793
less: f32 = precise as f32  # Loss of precision

# Float to integer (truncates decimal)
pi: f32 = 3.99
int_pi: i32 = pi as i32  # 3 (truncates)
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
num := parse_int(text)  # Option[i32]

match num {
    Some(n) => print("Parsed: " + str(n)),
    None => print("Parse error")
}

# Parse float
text2 := "3.14"
pi := parse_float(text2)  # Option[f32]
```

## Safe Parsing

Parsing returns `Option` types for safety:

```volta
fn parse_int(s: str) -> Option[i32] {
    # Implementation
}

fn parse_float(s: str) -> Option[f32] {
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
float_array := int_array.map(fn(x: i32) -> f32 = float(x))
# [1.0, 2.0, 3.0, 4.0, 5.0]

string_array := int_array.map(fn(x: i32) -> str = str(x))
# ["1", "2", "3", "4", "5"]
```

## Common Patterns

### Parse with default

```volta
fn parse_with_default(s: str, default: i32) -> i32 {
    result := parse_int(s)
    return result.unwrap_or(default)
}

value := parse_with_default("123", 0)  # 123
invalid := parse_with_default("bad", 0)  # 0
```

### Safe numeric operations

```volta
# Ensure float division
a: i32 = 5
b: i32 = 2

result := float(a) / float(b)  # 2.5

# Without casting
bad_result := a / b  # 2 (integer division)
```

### Format numbers

```volta
fn format_price(amount: f32) -> str {
    return "$" + str(amount)
}

price := format_price(19.99)  # "$19.99"
```

## Type Checking

Volta is statically typed, so types are checked at compile time:

```volta
x: i32 = 42
y: f32 = x  # Error! Type mismatch

# Must explicitly cast
y: f32 = float(x)  # OK
```

## Practical Examples

### User input processing

```volta
fn get_age_from_input(input: str) -> Option[i32] {
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
fn celsius_to_fahrenheit(celsius: f32) -> f32 {
    return celsius * 9.0 / 5.0 + 32.0
}

fn fahrenheit_to_celsius(fahrenheit: f32) -> f32 {
    return (fahrenheit - 32.0) * 5.0 / 9.0
}

# With integer input
temp_c: i32 = 25
temp_f := celsius_to_fahrenheit(float(temp_c))
```

### Data formatting

```volta
fn format_user_info(name: str, age: i32, score: f32) -> str {
    return name + " (age " + str(age) + ") - Score: " + str(score)
}

info := format_user_info("Alice", 30, 95.5)
# "Alice (age 30) - Score: 95.5"
```

### Array statistics

```volta
fn average(numbers: Array[i32]) -> Option[f32] {
    if numbers.len() == 0 {
        return None
    }
    
    sum := numbers.reduce(fn(a: i32, b: i32) -> i32 = a + b, 0)
    
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
x: i32 = 42
y: f32 = float(x)  # Must cast explicitly
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
