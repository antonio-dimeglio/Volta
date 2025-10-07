# Volta Programming Language

Welcome to the Volta programming language documentation!

## What is Volta?

Volta is a modern, statically-typed programming language designed for clarity, performance, and safety. It combines the best features of contemporary languages while maintaining simplicity and expressiveness.

## Key Features

- **Static Typing**: Catch errors at compile time with a robust type system
- **Type Inference**: Write less code with automatic type inference using `:=`
- **Pattern Matching**: Powerful `match` expressions for elegant control flow
- **Option Types**: Built-in handling for nullable values with `Option[T]`
- **Structs**: Define custom data structures with methods
- **Expression-Based**: Everything is an expression, enabling functional-style programming
- **Clear Syntax**: Readable and intuitive syntax inspired by modern languages

## Quick Example

```volta
# Define a struct
struct Point {
    x: int,
    y: int,
}

# Define a method on the struct
fn Point.distance(other: Point) -> float {
    dx := self.x - other.x
    dy := self.y - other.y
    return sqrt(dx * dx + dy * dy)
}

# Use it
p1 := Point { x: 0, y: 0 }
p2 := Point { x: 3, y: 4 }
dist := p1.distance(p2)
print(dist)  # 5.0
```

## Getting Started

Ready to dive in? Check out our [Installation Guide](getting-started/installation.md) and [Quick Start Tutorial](getting-started/quickstart.md)!

## Community & Support

- **Issues**: Report bugs on [GitHub Issues](https://github.com/your-repo/volta/issues)
- **Discussions**: Join the conversation on [GitHub Discussions](https://github.com/your-repo/volta/discussions)

## License

Volta is open source and available under the MIT License.
