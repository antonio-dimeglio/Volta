# Option Types

Option types represent values that may or may not exist, eliminating null/None errors.

## The Option Type

```volta
# Option can be Some(value) or None
result: Option[int] = Some(42)
empty: Option[str] = None
```

## Why Options?

Options provide safe handling of missing values:

```volta
# Instead of returning null/None
fn find_index(arr: Array[int], target: int) -> Option[int] {
    for i in range(arr.len()) {
        if arr[i] == target {
            return Some(i)
        }
    }
    return None
}
```

## Pattern Matching with Options

The safest way to handle Options:

```volta
result := find_index([1, 2, 3], 2)

match result {
    Some(index) => print("Found at: " + str(index)),
    None => print("Not found")
}
```

## Unwrapping Options

### unwrap_or

Provide a default value:

```volta
result := find_index([1, 2, 3], 5)
index := result.unwrap_or(0)  # Returns 0 if None
```

### unwrap

Get the value or panic (use carefully):

```volta
result := Some(42)
value := result.unwrap()  # 42

# WARNING: This will panic!
# empty := None
# value := empty.unwrap()  # Runtime error!
```

## Option Methods

### map

Transform the value if it exists:

```volta
result: Option[int] = Some(5)
doubled := result.map(fn(x: int) -> int = x * 2)
# Some(10)

empty: Option[int] = None
doubled_empty := empty.map(fn(x: int) -> int = x * 2)
# None
```

### and_then (flatMap)

Chain operations that return Options:

```volta
fn divide(a: float, b: float) -> Option[float] {
    if b == 0.0 {
        return None
    }
    return Some(a / b)
}

fn sqrt(x: float) -> Option[float] {
    if x < 0.0 {
        return None
    }
    return Some(math.sqrt(x))
}

result := divide(16.0, 4.0)
    .and_then(fn(x: float) -> Option[float] = sqrt(x))
# Some(2.0)
```

## Common Patterns

### Safe array access

```volta
fn get(arr: Array[int], index: int) -> Option[int] {
    if index >= 0 and index < arr.len() {
        return Some(arr[index])
    }
    return None
}

numbers := [1, 2, 3]
value := get(numbers, 1)  # Some(2)
invalid := get(numbers, 10)  # None
```

### Safe division

```volta
fn safe_divide(a: float, b: float) -> Option[float] {
    if b == 0.0 {
        return None
    }
    return Some(a / b)
}

result := safe_divide(10.0, 2.0)  # Some(5.0)
error := safe_divide(10.0, 0.0)   # None
```

### Dictionary lookup

```volta
fn lookup(dict: Array[(str, int)], key: str) -> Option[int] {
    for (k, v) in dict {
        if k == key {
            return Some(v)
        }
    }
    return None
}

ages := [("Alice", 30), ("Bob", 25)]
alice_age := lookup(ages, "Alice")  # Some(30)
charlie_age := lookup(ages, "Charlie")  # None
```

## Chaining Options

```volta
# Multiple operations that might fail
result := parse_int("42")
    .map(fn(x: int) -> int = x * 2)
    .map(fn(x: int) -> int = x + 10)
    .unwrap_or(0)
```

## Options in Structs

```volta
struct User {
    name: str,
    email: Option[str],
    phone: Option[str]
}

user := User {
    name: "Alice",
    email: Some("alice@example.com"),
    phone: None
}

match user.email {
    Some(email) => print("Email: " + email),
    None => print("No email")
}
```

## Best Practices

1. **Prefer pattern matching** for clarity:
```volta
match result {
    Some(value) => # handle value,
    None => # handle missing
}
```

2. **Use unwrap_or** for defaults:
```volta
value := option.unwrap_or(default_value)
```

3. **Chain with map** for transformations:
```volta
result := option.map(fn(x: int) -> int = x * 2)
```

4. **Avoid unwrap** unless certain:
```volta
# Risky
value := option.unwrap()

# Better
value := option.unwrap_or(0)
```

## Practical Examples

### Configuration with defaults

```volta
struct Config {
    port: Option[int],
    host: Option[str]
}

fn get_port(config: Config) -> int {
    return config.port.unwrap_or(8080)
}

fn get_host(config: Config) -> str {
    return config.host.unwrap_or("localhost")
}
```

### Search with result

```volta
fn search(items: Array[str], query: str) -> Option[str] {
    for item in items {
        if item == query {
            return Some(item)
        }
    }
    return None
}

items := ["apple", "banana", "cherry"]
found := search(items, "banana")

match found {
    Some(fruit) => print("Found: " + fruit),
    None => print("Not in list")
}
```
