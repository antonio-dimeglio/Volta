# Comments

## Single-Line Comments

Use `#` for single-line comments:

```volta
# This is a comment
x := 42  # Comment after code

# Comments can explain your code
fn calculate(a: int, b: int) -> int {
    # Add the two numbers
    return a + b
}
```

## Multi-Line Comments

Use `#[` and `]#` for multi-line comments:

```volta
#[
This is a multi-line comment.
It can span multiple lines.
Useful for longer explanations.
]#

#[
fn old_function() -> int {
    # This entire function is commented out
    return 42
}
]#
```

## Documentation Style

```volta
#[
Function: calculate_average
Purpose: Computes the arithmetic mean of an array
Parameters: numbers - Array of floats
Returns: Option[float] - Average or None if empty
]#
fn calculate_average(numbers: Array[float]) -> Option[float] {
    if numbers.len() == 0 {
        return None
    }
    sum := numbers.reduce(fn(a: float, b: float) -> float = a + b, 0.0)
    return Some(sum / float(numbers.len()))
}
```

## Best Practices

- Use comments to explain *why*, not *what*
- Keep comments up-to-date with code changes
- Prefer clear code over excessive comments
- Use multi-line comments for larger blocks

```volta
# Good: Explains reasoning
# Use binary search since array is sorted
index := binary_search(arr, target)

# Less useful: States the obvious
# Increment counter by 1
counter = counter + 1
```
