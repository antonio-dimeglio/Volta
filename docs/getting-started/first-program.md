# Your First Program

## Hello, World!

Let's write a simple program that prints a greeting:

```volta
print("Hello, World!")
```

Save this as `hello.volta` and run:
```bash
./bin/volta hello.volta
```

## Adding Variables

```volta
name: str = "Alessandro"
print("Hello, " + name + "!")
```

## Using Functions

```volta
fn greet(name: str) -> str {
    return "Hello, " + name + "!"
}

message := greet("Volta")
print(message)
```

## A Simple Calculator

```volta
fn add(a: int, b: int) -> int {
    return a + b
}

fn multiply(a: int, b: int) -> int {
    return a * b
}

x := 5
y := 3

sum := add(x, y)
product := multiply(x, y)

print("Sum: " + str(sum))
print("Product: " + str(product))
```

## Counting with Loops

```volta
# Print numbers 1 to 10
for i in 1..=10 {
    print(i)
}

# Print even numbers
for i in 0..=20 {
    if i % 2 == 0 {
        print(i)
    }
}
```

## What's Next?

Now that you've written your first program, learn more about:
- [Variables and mutability](../guide/basics/variables.md)
- [Data types](../guide/basics/types.md)
- [Control flow](../guide/control-flow/if-statements.md)
- [Loops](../guide/control-flow/loops.md)
