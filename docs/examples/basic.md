# Basic Examples

Simple programs to get started with Volta.

## Hello World

```volta
print("Hello, World!")
```

## Variables and Types

```volta
# Immutable variables
let name: str = "Alessandro"
let age: i32 = 25
let height: f64 = 1.75
let is_student: bool = true

print(name + " is " + str(age) + " years old")
```

## Simple Calculator

```volta
fn add(a: i32, b: i32) -> i32 {
    return a + b
}

fn subtract(a: i32, b: i32) -> i32 {
    return a - b
}

fn multiply(a: i32, b: i32) -> i32 {
    return a * b
}

fn divide(a: f64, b: f64) -> Option[f64] {
    if b == 0.0 {
        return None
    }
    return Some(a / b)
}

let x := 10
let y := 5

print("Sum: " + str(add(x, y)))
print("Difference: " + str(subtract(x, y)))
print("Product: " + str(multiply(x, y)))
```

## Temperature Converter

```volta
fn celsius_to_fahrenheit(c: f64) -> f64 {
    return c * 9.0 / 5.0 + 32.0
}

fn fahrenheit_to_celsius(f: f64) -> f64 {
    return (f - 32.0) * 5.0 / 9.0
}

let temp_c := 25.0
let temp_f := celsius_to_fahrenheit(temp_c)

print(str(temp_c) + "°C = " + str(temp_f) + "°F")
```

## Even or Odd

```volta
fn is_even(n: i32) -> bool {
    return n % 2 == 0
}

let numbers := [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]

for num in numbers {
    let result := if is_even(num) { "even" } else { "odd" }
    print(str(num) + " is " + result)
}
```

## Factorial

```volta
fn factorial(n: i32) -> i32 {
    if n <= 1 {
        return 1
    }
    return n * factorial(n - 1)
}

for i in range(1, 6) {
    let result := factorial(i)
    print("Factorial of " + str(i) + " is " + str(result))
}
```

## Array Sum

```volta
fn sum(arr: Array[i32]) -> i32 {
    let mut total: i32 = 0
    for num in arr {
        total = total + num
    }
    return total
}

let numbers := [1, 2, 3, 4, 5]
let total := sum(numbers)
print("Sum: " + str(total))
```

## Find Maximum

```volta
fn find_max(arr: Array[i32]) -> Option[i32] {
    if arr.len() == 0 {
        return None
    }

    let mut max: i32 = arr[0]
    for num in arr {
        if num > max {
            max = num
        }
    }

    return Some(max)
}

let numbers := [3, 7, 2, 9, 1, 5]
let result := find_max(numbers)

match result {
    Some(max) => print("Maximum: " + str(max)),
    None => print("Empty array")
}
```

## Counting

```volta
# Count to 10 (inclusive range)
for i in 1..=10 {
    print(i)
}

# Count to 10 (exclusive range)
for i in 1..11 {
    print(i)
}

# Count down
let mut counter: i32 = 10
while counter > 0 {
    print(counter)
    counter = counter - 1
}
print("Blast off!")
```

## FizzBuzz

```volta
for i in 1..=100 {
    let output := if i % 15 == 0 {
        "FizzBuzz"
    } else if i % 3 == 0 {
        "Fizz"
    } else if i % 5 == 0 {
        "Buzz"
    } else {
        str(i)
    }

    print(output)
}
```

## String Manipulation

```volta
fn repeat(text: str, times: i32) -> str {
    let mut result: str = ""
    for i in range(times) {
        result = result + text
    }
    return result
}

let stars := repeat("*", 10)
print(stars)

# Create a box
let line := repeat("-", 20)
print(line)
print("| Hello, Volta!    |")
print(line)
```

## Prime Number Check

```volta
fn is_prime(n: i32) -> bool {
    if n <= 1 {
        return false
    }
    if n == 2 {
        return true
    }

    for i in 2..n {
        if n % i == 0 {
            return false
        }
    }

    return true
}

for i in 1..20 {
    if is_prime(i) {
        print(str(i) + " is prime")
    }
}
```

## Average Calculator

```volta
fn average(numbers: Array[f64]) -> Option[f64] {
    if numbers.len() == 0 {
        return None
    }

    let mut sum: f64 = 0.0
    for num in numbers {
        sum = sum + num
    }

    return Some(sum / f64(numbers.len()))
}

let scores := [85.0, 90.0, 78.0, 92.0, 88.0]
let avg := average(scores)

match avg {
    Some(value) => print("Average: " + str(value)),
    None => print("No scores")
}
```

## Grade Calculator

```volta
fn letter_grade(score: i32) -> str {
    return if score >= 90 {
        "A"
    } else if score >= 80 {
        "B"
    } else if score >= 70 {
        "C"
    } else if score >= 60 {
        "D"
    } else {
        "F"
    }
}

let scores := [95, 87, 72, 64, 55]

for score in scores {
    let grade := letter_grade(score)
    print("Score " + str(score) + " = Grade " + grade)
}
```
