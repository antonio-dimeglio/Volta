# Quick Start

## Running Your First Program

Create a file named `hello.volta`:

```volta
print("Hello, Volta!")
```

Run it with:
```bash
./bin/volta hello.volta
```

## Basic Variables

```volta
# Immutable by default
x: int = 42
name: str = "Alessandro"

# Mutable variables
counter: mut int = 0
counter = counter + 1

# Type inference with :=
y := 100
```

## Simple Function

```volta
fn greet(name: str) -> str {
    return "Hello, " + name + "!"
}

print(greet("World"))
```

## Loops

```volta
# For loop with range
for i in 0..5 {
    print(i)  # Prints 0, 1, 2, 3, 4
}

# Inclusive range
for i in 1..=5 {
    print(i)  # Prints 1, 2, 3, 4, 5
}
```

## Next Steps

- Learn about [variables and types](../guide/basics/variables.md)
- Explore [control flow](../guide/control-flow/if-statements.md)
- Check out [functions](../guide/functions/declarations.md)
