# Operators

## Arithmetic Operators

```volta
# Basic arithmetic
a := 10 + 5   # Addition: 15
b := 10 - 5   # Subtraction: 5
c := 10 * 5   # Multiplication: 50
d := 10 / 5   # Division: 2
e := 10 % 3   # Modulo: 1
f := 2 ** 8   # Power: 256
```

## Comparison Operators

```volta
x := 10
y := 20

a := x == y   # Equal to: false
b := x != y   # Not equal to: true
c := x < y    # Less than: true
d := x > y    # Greater than: false
e := x <= y   # Less than or equal: true
f := x >= y   # Greater than or equal: false
```

## Logical Operators

```volta
a := true and false  # Logical AND: false
b := true or false   # Logical OR: true
c := not true        # Logical NOT: false

# Short-circuit evaluation
result := x > 0 and y < 100
```

## Assignment Operators

```volta
x: mut int = 10

x = 5      # Assignment
x += 3     # x = x + 3
x -= 2     # x = x - 2
x *= 4     # x = x * 4
x /= 2     # x = x / 2
```

## Range Operators

Range operators create sequences for iteration:

```volta
# Exclusive range (doesn't include end)
for i in 0..5 {      # 0, 1, 2, 3, 4
    print(i)
}

# Inclusive range (includes end)
for i in 0..=5 {     # 0, 1, 2, 3, 4, 5
    print(i)
}

# Ranges with negative numbers
for i in -2..3 {     # -2, -1, 0, 1, 2
    print(i)
}

# Backwards ranges
for i in 5..0 {      # 5, 4, 3, 2, 1
    print(i)
}
```

**Note:** Range operators (`..` and `..=`) are used with `for` loops. There is no separate `range()` function.

## Operator Precedence

From highest to lowest:

1. `**` (Power)
2. `not`, unary `-`
3. `*`, `/`, `%`
4. `+`, `-`
5. `<`, `>`, `<=`, `>=`
6. `==`, `!=`
7. `and`
8. `or`

Use parentheses for clarity:

```volta
result := (a + b) * (c - d)
```
