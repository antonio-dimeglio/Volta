# Variables

## Immutability by Default

In Volta, variables are immutable by default. This helps prevent bugs and makes code easier to reason about.

```volta
x: int = 42
y: float = 3.14
name: str = "Alessandro"

# This would cause an error:
# x = 50  # Error: cannot assign to immutable variable
```

## Mutable Variables

Use the `mut` keyword to declare mutable variables:

```volta
counter: mut int = 0
counter = counter + 1  # OK
counter = 10          # OK
```

## Type Inference

Volta supports type inference for local variables using the `:=` operator:

```volta
x := 42        # inferred as int
y := 3.14      # inferred as float
name := "Volta" # inferred as str
flag := true   # inferred as bool
```

## Declaration Syntax

```volta
# Explicit type annotation
variable_name: type = value

# Mutable with explicit type
variable_name: mut type = value

# Type inference
variable_name := value
```

## Examples

```volta
# Immutable variables
age: int = 25
price: float = 19.99
message: str = "Hello"

# Mutable counter
count: mut int = 0
for i in range(10) {
    count = count + i
}

# Type inference
result := 42 * 2
greeting := "Welcome to Volta"
is_active := true
```
