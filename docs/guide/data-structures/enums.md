# Enums

Enums (enumerated types) allow you to define a type by enumerating its possible variants. Volta supports Rust-style enums with data.

## Simple Enums

Enums without associated data:

```volta
enum Direction {
    North,
    South,
    East,
    West
}

# Usage
dir := Direction.North

# Pattern matching
match dir {
    Direction.North => print("Going north"),
    Direction.South => print("Going south"),
    Direction.East => print("Going east"),
    Direction.West => print("Going west")
}
```

## Enums with Data

Variants can hold data:

```volta
enum Message {
    Quit,                          # No data
    Move(int, int),                # Tuple-like variant
    Write(str),                    # Single value
    ChangeColor(int, int, int)     # Multiple values
}

# Create enum values
msg1 := Message.Quit
msg2 := Message.Move(10, 20)
msg3 := Message.Write("Hello")
msg4 := Message.ChangeColor(255, 0, 128)
```

## Pattern Matching Enums

Extract data from enum variants:

```volta
fn process_message(msg: Message) {
    match msg {
        Message.Quit => {
            print("Quit message")
        },
        Message.Move(x, y) => {
            print("Move to: " + str(x) + ", " + str(y))
        },
        Message.Write(text) => {
            print("Write: " + text)
        },
        Message.ChangeColor(r, g, b) => {
            print("Color: rgb(" + str(r) + ", " + str(g) + ", " + str(b) + ")")
        }
    }
}
```

## Option Type

`Option[T]` is a built-in enum for representing optional values:

```volta
# Definition (built-in)
enum Option[T] {
    Some(T),
    None
}

# Usage
fn find(arr: Array[int], target: int) -> Option[int] {
    for i in 0..arr.len() {
        if arr[i] == target {
            return Some(i)
        }
    }
    return None
}

# Pattern matching
result := find([1, 2, 3], 2)
match result {
    Some(index) => print("Found at: " + str(index)),
    None => print("Not found")
}
```

## Result Type

`Result[T, E]` is a built-in enum for error handling:

```volta
# Definition (built-in)
enum Result[T, E] {
    Ok(T),
    Err(E)
}

# Usage
fn divide(a: float, b: float) -> Result[float, str] {
    if b == 0.0 {
        return Err("Division by zero")
    }
    return Ok(a / b)
}

# Pattern matching
result := divide(10.0, 2.0)
match result {
    Ok(value) => print("Result: " + str(value)),
    Err(msg) => print("Error: " + msg)
}
```

## Generic Enums

Enums can have type parameters:

```volta
enum Container[T] {
    Empty,
    Single(T),
    Multiple(Array[T])
}

# Create generic enum values
empty: Container[int] = Container.Empty
single := Container.Single(42)
multiple := Container.Multiple([1, 2, 3])

# Pattern matching with generics
fn count[T](container: Container[T]) -> int {
    return match container {
        Container.Empty => 0,
        Container.Single(_) => 1,
        Container.Multiple(arr) => arr.len()
    }
}
```

## Enum Methods

Define methods on enums:

```volta
enum Option[T] {
    Some(T),
    None
}

fn Option[T].is_some(self: Option[T]) -> bool {
    return match self {
        Some(_) => true,
        None => false
    }
}

fn Option[T].is_none(self: Option[T]) -> bool {
    return match self {
        Some(_) => false,
        None => true
    }
}

fn Option[T].unwrap_or(self: Option[T], default: T) -> T {
    return match self {
        Some(value) => value,
        None => default
    }
}

# Usage
maybe := Some(42)
print(maybe.is_some())  # true

empty: Option[int] = None
value := empty.unwrap_or(0)  # 0
```

## Practical Examples

### State Machine

```volta
enum State {
    Idle,
    Running(int),      # Running with progress
    Paused(int),       # Paused with progress
    Finished
}

fn update_state(state: State, input: str) -> State {
    return match state {
        State.Idle => {
            if input == "start" {
                State.Running(0)
            } else {
                State.Idle
            }
        },
        State.Running(progress) => {
            if input == "pause" {
                State.Paused(progress)
            } else if progress >= 100 {
                State.Finished
            } else {
                State.Running(progress + 10)
            }
        },
        State.Paused(progress) => {
            if input == "resume" {
                State.Running(progress)
            } else {
                State.Paused(progress)
            }
        },
        State.Finished => State.Finished
    }
}
```

### JSON-like Values

```volta
enum JsonValue {
    Null,
    Bool(bool),
    Number(float),
    String(str),
    Array(Array[JsonValue]),
    Object(Array[(str, JsonValue)])
}

fn print_json(value: JsonValue) {
    match value {
        JsonValue.Null => print("null"),
        JsonValue.Bool(b) => print(str(b)),
        JsonValue.Number(n) => print(str(n)),
        JsonValue.String(s) => print("\"" + s + "\""),
        JsonValue.Array(items) => {
            # Print array...
        },
        JsonValue.Object(pairs) => {
            # Print object...
        }
    }
}
```

### Error Types

```volta
enum CompileError {
    SyntaxError(int, int, str),      # line, column, message
    TypeError(str, str),              # expected, actual
    UndefinedVariable(str),           # name
    FileNotFound(str)                 # path
}

fn compile(path: str) -> Result[Bytecode, CompileError] {
    content := read_file(path)
    match content {
        None => return Err(CompileError.FileNotFound(path)),
        Some(text) => {
            # Continue compilation...
        }
    }
}

# Handle errors
result := compile("test.volta")
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
    Err(CompileError.FileNotFound(path)) => {
        print("File not found: " + path)
    }
}
```

## Enum Variants as Values

Enum variants can be used as constructor functions:

```volta
enum Optional[T] {
    Some(T),
    None
}

# Use variant as a function
numbers := [1, 2, 3, 4, 5]
optionals := numbers.map(Optional.Some)  # [Some(1), Some(2), ...]
```

## Comparison with Structs

**Use structs when:**
- All fields are always present
- You need named fields
- Example: `struct Point { x: float, y: float }`

**Use enums when:**
- Value can be one of several variants
- Different variants have different data
- Example: `enum Shape { Circle(float) | Rectangle(float, float) }`

## Best Practices

1. **Use descriptive variant names:**
```volta
# Good
enum Status { Success, Failure }

# Avoid
enum Status { S, F }
```

2. **Keep variants focused:**
```volta
# Good - each variant has a clear purpose
enum Command {
    Quit,
    Move(int, int),
    Attack(str)
}

# Avoid - too many similar variants
enum Command {
    MoveUp, MoveDown, MoveLeft, MoveRight  # Use Move(Direction) instead
}
```

3. **Use pattern matching exhaustively:**
```volta
# Good - handles all cases
match status {
    Status.Success => # ...,
    Status.Failure => # ...
}

# Avoid - missing cases
match status {
    Status.Success => # ...,
    _ => # ... catches too much
}
```

4. **Combine with Result for error handling:**
```volta
fn parse_int(s: str) -> Result[int, str] {
    # Try to parse...
    if invalid {
        return Err("Invalid number format")
    }
    return Ok(parsed_value)
}
```

## Memory Representation

Enums are stored as:
- A tag (discriminant) indicating which variant
- The data for that variant

```volta
enum Message {
    Quit,              # Tag = 0, no data
    Move(int, int)     # Tag = 1, two ints
}

# In memory:
# Quit:        [tag: 0]
# Move(10, 20): [tag: 1, x: 10, y: 20]
```

The compiler optimizes enum size to fit the largest variant.
