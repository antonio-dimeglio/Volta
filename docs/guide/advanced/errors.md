# Error Handling

Volta uses `Option[T]` and `Result[T, E]` types for error handling instead of exceptions. This makes errors explicit and forces you to handle them.

## Error Handling with Options

Functions that can fail can return `Option` types:

```volta
fn divide(a: float, b: float) -> Option[float] {
    if b == 0.0 {
        return None
    }
    return Some(a / b)
}

result := divide(10.0, 2.0)  # Some(5.0)
error := divide(10.0, 0.0)   # None
```

**Limitation:** Option doesn't tell you *why* something failed.

## Error Handling with Result

`Result[T, E]` provides error context:

```volta
fn divide(a: float, b: float) -> Result[float, str] {
    if b == 0.0 {
        return Err("Division by zero")
    }
    return Ok(a / b)
}

result := divide(10.0, 2.0)   # Ok(5.0)
error := divide(10.0, 0.0)    # Err("Division by zero")
```

### Pattern Matching on Result

```volta
result := divide(10.0, 0.0)

match result {
    Ok(value) => print("Result: " + str(value)),
    Err(msg) => print("Error: " + msg)
}
```

## Handling Errors

### Pattern Matching

The most explicit way:

```volta
result := divide(10.0, 0.0)

match result {
    Some(value) => print("Result: " + str(value)),
    None => print("Error: Division by zero")
}
```

### Default Values

Provide a fallback:

```volta
result := divide(10.0, 0.0).unwrap_or(0.0)
# result = 0.0
```

## Propagating Errors

Return `None` to propagate errors up the call stack:

```volta
fn calculate(x: float, y: float, z: float) -> Option[float] {
    step1 := divide(x, y)
    match step1 {
        None => return None,
        Some(value) => {
            return divide(value, z)
        }
    }
}
```

## Multiple Error Conditions

```volta
fn parse_and_divide(a_str: str, b_str: str) -> Option[float] {
    # Parse first number
    a := parse_float(a_str)
    match a {
        None => return None,
        Some(a_val) => {
            # Parse second number
            b := parse_float(b_str)
            match b {
                None => return None,
                Some(b_val) => {
                    # Divide
                    return divide(a_val, b_val)
                }
            }
        }
    }
}
```

## Error Recovery

```volta
fn safe_operation(x: int) -> int {
    result := risky_operation(x)
    
    return match result {
        Some(value) => value,
        None => {
            # Fallback or recovery
            print("Operation failed, using default")
            0
        }
    }
}
```

## Validation

```volta
fn validate_age(age: int) -> Option[int] {
    if age < 0 or age > 150 {
        return None
    }
    return Some(age)
}

fn validate_email(email: str) -> Option[str] {
    if email.contains("@") {
        return Some(email)
    }
    return None
}
```

## Chaining Operations

```volta
# Chain operations that might fail
result := parse_int("42")
    .and_then(fn(x: int) -> Option[int] = 
        if x > 0 { Some(x) } else { None }
    )
    .map(fn(x: int) -> int = x * 2)
    .unwrap_or(0)
```

## Common Patterns

### Safe array access

```volta
fn safe_get(arr: Array[int], index: int) -> Option[int] {
    if index < 0 or index >= arr.len() {
        return None
    }
    return Some(arr[index])
}

numbers := [1, 2, 3]

value := safe_get(numbers, 1)  # Some(2)
error := safe_get(numbers, 10)  # None
```

### File operations

```volta
fn read_config(path: str) -> Option[str] {
    # Try to read file
    content := file_read(path)
    
    match content {
        Some(text) => return Some(text),
        None => {
            print("Could not read file: " + path)
            return None
        }
    }
}
```

### Safe parsing

```volta
fn parse_positive_int(s: str) -> Option[int] {
    parsed := parse_int(s)
    
    return match parsed {
        None => None,
        Some(n) => {
            if n > 0 {
                Some(n)
            } else {
                None
            }
        }
    }
}
```

## Best Practices

1. **Return None for errors**:
```volta
fn operation() -> Option[T] {
    if error_condition {
        return None
    }
    return Some(value)
}
```

2. **Use pattern matching** for explicit handling:
```volta
match result {
    Some(value) => # success path,
    None => # error path
}
```

3. **Provide context** with print statements:
```volta
match result {
    None => {
        print("Error: Invalid input")
        return None
    },
    Some(value) => # continue
}
```

4. **Chain with map/and_then**:
```volta
result
    .map(transform)
    .and_then(validate)
    .unwrap_or(default)
```

## Structured Error Types

Use enums for rich error information:

```volta
enum FileError {
    NotFound(str),           # path
    PermissionDenied(str),   # path
    InvalidFormat(str)       # reason
}

fn read_config(path: str) -> Result[Config, FileError] {
    content := read_file(path)

    match content {
        None => return Err(FileError.NotFound(path)),
        Some(text) => {
            parsed := parse_config(text)
            match parsed {
                None => return Err(FileError.InvalidFormat("Bad syntax")),
                Some(cfg) => return Ok(cfg)
            }
        }
    }
}

# Handle specific errors
result := read_config("config.volta")
match result {
    Ok(cfg) => print("Config loaded"),
    Err(FileError.NotFound(path)) => {
        print("File not found: " + path)
        # Use defaults
    },
    Err(FileError.PermissionDenied(path)) => {
        print("Cannot read: " + path)
    },
    Err(FileError.InvalidFormat(reason)) => {
        print("Invalid config: " + reason)
    }
}
```

## Compiler Errors Example

```volta
enum CompileError {
    SyntaxError(int, int, str),      # line, col, message
    TypeError(str, str),              # expected, actual
    UndefinedVariable(str),           # name
    DuplicateDefinition(str)          # name
}

fn compile(source: str) -> Result[Bytecode, CompileError] {
    tokens := lex(source)
    ast := parse(tokens)

    match ast {
        Err(e) => return Err(e),
        Ok(tree) => {
            # Type check
            checked := type_check(tree)
            match checked {
                Err(e) => return Err(e),
                Ok(typed_ast) => {
                    return Ok(generate_code(typed_ast))
                }
            }
        }
    }
}

# Handle compilation errors
result := compile(source_code)
match result {
    Ok(bytecode) => {
        print("Compilation successful")
    },
    Err(CompileError.SyntaxError(line, col, msg)) => {
        print("Syntax error at " + str(line) + ":" + str(col) + ": " + msg)
    },
    Err(CompileError.TypeError(expected, actual)) => {
        print("Type error: expected " + expected + ", got " + actual)
    },
    Err(CompileError.UndefinedVariable(name)) => {
        print("Undefined variable: " + name)
    },
    Err(CompileError.DuplicateDefinition(name)) => {
        print("Duplicate definition: " + name)
    }
}
```

## Practical Examples

### Configuration loader

```volta
enum ConfigError {
    FileNotFound,
    ParseError(str)
}

fn load_config(path: str) -> Result[Config, ConfigError] {
    content := read_file(path)

    match content {
        None => return Err(ConfigError.FileNotFound),
        Some(text) => {
            parsed := parse_config(text)
            match parsed {
                None => return Err(ConfigError.ParseError("Invalid syntax")),
                Some(cfg) => return Ok(cfg)
            }
        }
    }
}

config := load_config("config.volta")
match config {
    Ok(cfg) => print("Config loaded"),
    Err(ConfigError.FileNotFound) => print("Using defaults"),
    Err(ConfigError.ParseError(msg)) => print("Config error: " + msg)
}
```

### Input validation

```volta
fn create_user(name: str, age: int, email: str) -> Option[User] {
    # Validate name
    if name.len() == 0 {
        print("Error: Name cannot be empty")
        return None
    }
    
    # Validate age
    validated_age := validate_age(age)
    match validated_age {
        None => {
            print("Error: Invalid age")
            return None
        },
        Some(valid_age) => {
            # Validate email
            validated_email := validate_email(email)
            match validated_email {
                None => {
                    print("Error: Invalid email")
                    return None
                },
                Some(valid_email) => {
                    return Some(User {
                        name: name,
                        age: valid_age,
                        email: Some(valid_email)
                    })
                }
            }
        }
    }
}
```
