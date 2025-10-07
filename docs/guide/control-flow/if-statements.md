# If Statements

## Basic If Statement

```volta
x := 10

if x > 5 {
    print("x is greater than 5")
}
```

## If-Else

```volta
age := 18

if age >= 18 {
    print("Adult")
} else {
    print("Minor")
}
```

## If-Else If-Else

```volta
score := 85

if score >= 90 {
    print("Grade: A")
} else if score >= 80 {
    print("Grade: B")
} else if score >= 70 {
    print("Grade: C")
} else {
    print("Grade: F")
}
```

## If as an Expression

If statements can return values:

```volta
x := 10
y := 20

max := if x > y { x } else { y }

status := if score >= 60 { "Pass" } else { "Fail" }
```

## Nested If Statements

```volta
x := 15
y := 20

if x > 10 {
    if y > 15 {
        print("Both conditions met")
    }
}
```

## Complex Conditions

```volta
age := 25
has_license := true

if age >= 18 and has_license {
    print("Can drive")
}

# Multiple conditions
if (x > 0 and x < 100) or x == -1 {
    print("Valid range")
}
```

## Examples

```volta
# Check if number is positive, negative, or zero
fn classify(n: int) -> str {
    return if n > 0 {
        "positive"
    } else if n < 0 {
        "negative"
    } else {
        "zero"
    }
}

# Temperature warning system
temp := 35
if temp > 30 {
    print("Hot!")
} else if temp < 10 {
    print("Cold!")
} else {
    print("Comfortable")
}
```
