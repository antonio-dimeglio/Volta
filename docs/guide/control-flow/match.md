# Match Expressions

Match expressions provide powerful pattern matching capabilities.

## Basic Match

```volta
x := 2

result := match x {
    0 => "zero",
    1 => "one",
    2 => "two",
    _ => "other"
}

print(result)  # "two"
```

## Match with Guards

Use `if` guards for conditional matching:

```volta
fn classify(x: int) -> str {
    return match x {
        0 => "zero",
        1 => "one",
        n if n < 0 => "negative",
        n if n > 100 => "large",
        _ => "other"
    }
}
```

## Matching Tuples

Destructure tuples in match patterns:

```volta
point := (3, 4)

match point {
    (0, 0) => print("Origin"),
    (x, 0) => print("On x-axis at " + str(x)),
    (0, y) => print("On y-axis at " + str(y)),
    (x, y) => print("Point at (" + str(x) + ", " + str(y) + ")")
}
```

## Matching Options

Match is perfect for handling `Option` types:

```volta
fn find_user(id: int) -> Option[str] {
    # ...
}

result := find_user(42)

match result {
    Some(name) => print("Found: " + name),
    None => print("User not found")
}
```

## Match with Multiple Patterns

```volta
day := 3

day_type := match day {
    1 | 2 | 3 | 4 | 5 => "weekday",
    6 | 7 => "weekend",
    _ => "invalid"
}
```

## Match with Blocks

```volta
status := 200

match status {
    200 => {
        print("Success")
        print("Request completed")
    },
    404 => {
        print("Not found")
        print("Check the URL")
    },
    _ => {
        print("Unknown status")
    }
}
```

## Practical Examples

```volta
# HTTP status code handler
fn handle_response(code: int) -> str {
    return match code {
        200 => "OK",
        201 => "Created",
        400 => "Bad Request",
        404 => "Not Found",
        500 => "Server Error",
        c if c >= 200 and c < 300 => "Success",
        c if c >= 400 and c < 500 => "Client Error",
        c if c >= 500 => "Server Error",
        _ => "Unknown"
    }
}

# Processing optional values
result := find_index([1, 2, 3], 2)
message := match result {
    Some(i) => "Found at index " + str(i),
    None => "Not found"
}
```
