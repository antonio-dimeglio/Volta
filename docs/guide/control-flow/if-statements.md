# If Statements

## Basic If Statement

```volta
let x := 10

if x > 5 {
    print("x is greater than 5")
}
```

## If-Else

```volta
let age := 18

if age >= 18 {
    print("Adult")
} else {
    print("Minor")
}
```

## If-Else If-Else

```volta
let score := 85

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
let x := 10
let y := 20

let max := if x > y { x } else { y }

let status := if score >= 60 { "Pass" } else { "Fail" }
```

## Nested If Statements

```volta
let x := 15
let y := 20

if x > 10 {
    if y > 15 {
        print("Both conditions met")
    }
}
```

## Complex Conditions

```volta
let age := 25
let has_license := true

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
fn classify(n: i32) -> str {
    return if n > 0 {
        "positive"
    } else if n < 0 {
        "negative"
    } else {
        "zero"
    }
}

# Temperature warning system
let temp := 35
if temp > 30 {
    print("Hot!")
} else if temp < 10 {
    print("Cold!")
} else {
    print("Comfortable")
}
```
