# Variables

## Variable Declaration

In Volta, all variables must be declared using the `let` keyword. This clearly distinguishes variable declarations from assignments.

```volta
let x: i32 = 42
let y: f64 = 3.14
let name: str = "Alessandro"
```

## Immutability by Default

Variables are immutable by default. This helps prevent bugs and makes code easier to reason about.

```volta
let x: i32 = 42

# This would cause an error:
# x = 50  # Error: cannot assign to immutable variable
```

## Mutable Variables

Use the `mut` keyword after `let` to declare mutable variables:

```volta
let mut counter: i32 = 0
counter = counter + 1  # OK
counter = 10          # OK
```

## Type Inference

Volta supports type inference for local variables using the `:=` operator:

```volta
let x := 42        # inferred as i32
let y := 3.14      # inferred as f64
let name := "Volta" # inferred as str
let flag := true   # inferred as bool
```

You can also use type inference with mutable variables:

```volta
let mut count := 0  # inferred as i32, mutable
count = count + 1
```

## Declaration Syntax

```volta
# Immutable with explicit type annotation
let variable_name: type = value

# Mutable with explicit type
let mut variable_name: type = value

# Type inference (immutable)
let variable_name := value

# Type inference (mutable)
let mut variable_name := value
```

## Variable Assignment vs Declaration

Note the distinction between variable **declaration** (creating a new variable) and **assignment** (updating an existing mutable variable):

```volta
# Declaration - creates a new variable
let x: i32 = 42

# Assignment - updates an existing mutable variable
let mut y: i32 = 10
y = 20  # OK, y is mutable

# This is an error - cannot reassign immutable variable
let z: i32 = 30
z = 40  # Error: cannot assign to immutable variable
```

## Examples

```volta
# Immutable variables
let age: i32 = 25
let price: f64 = 19.99
let message: str = "Hello"

# Mutable counter
let mut count: i32 = 0
for i in range(10) {
    count = count + i
}

# Type inference
let result := 42 * 2
let greeting := "Welcome to Volta"
let is_active := true

# Mutable with type inference
let mut total := 0.0
total = total + 10.5
```
