# Function Declarations

## Basic Function Syntax

```volta
fn function_name(parameter: type) -> return_type {
    # Function body
    return value
}
```

## Simple Function

```volta
fn greet(name: str) -> str {
    return "Hello, " + name + "!"
}

message := greet("Volta")
print(message)  # "Hello, Volta!"
```

## Multiple Parameters

```volta
fn add(a: int, b: int) -> int {
    return a + b
}

fn calculate(x: float, y: float, z: float) -> float {
    return (x + y) * z
}

result := add(5, 3)
value := calculate(2.0, 3.0, 4.0)
```

## Functions Without Return Values

Functions that don't return a value have `-> void` or omit the return type:

```volta
fn print_message(msg: str) {
    print(msg)
}

fn log_error(code: int, message: str) {
    print("Error " + str(code) + ": " + message)
}
```

## Function with Multiple Statements

```volta
fn factorial(n: int) -> int {
    if n <= 1 {
        return 1
    }
    return n * factorial(n - 1)
}
```

## Type Annotations Required

Function signatures require explicit type annotations:

```volta
# Correct
fn process(data: Array[int]) -> int {
    return data.len()
}

# Error: missing type annotations
# fn process(data) -> int { ... }
```

## Examples

### Area calculator

```volta
fn circle_area(radius: float) -> float {
    pi := 3.14159
    return pi * radius * radius
}

area := circle_area(5.0)
```

### String utilities

```volta
fn repeat(text: str, times: int) -> str {
    result: mut str = ""
    for i in range(times) {
        result = result + text
    }
    return result
}

stars := repeat("*", 10)
```

### Max of three numbers

```volta
fn max_of_three(a: int, b: int, c: int) -> int {
    max_ab := if a > b { a } else { b }
    return if max_ab > c { max_ab } else { c }
}

maximum := max_of_three(10, 25, 15)
```
