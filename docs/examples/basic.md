# Basic Examples

Simple programs to get started with Volta.

## Hello World

```volta
print("Hello, World!")
```

## Variables and Types

```volta
# Immutable variables
name: str = "Alessandro"
age: int = 25
height: float = 1.75
is_student: bool = true

print(name + " is " + str(age) + " years old")
```

## Simple Calculator

```volta
fn add(a: int, b: int) -> int {
    return a + b
}

fn subtract(a: int, b: int) -> int {
    return a - b
}

fn multiply(a: int, b: int) -> int {
    return a * b
}

fn divide(a: float, b: float) -> Option[float] {
    if b == 0.0 {
        return None
    }
    return Some(a / b)
}

x := 10
y := 5

print("Sum: " + str(add(x, y)))
print("Difference: " + str(subtract(x, y)))
print("Product: " + str(multiply(x, y)))
```

## Temperature Converter

```volta
fn celsius_to_fahrenheit(c: float) -> float {
    return c * 9.0 / 5.0 + 32.0
}

fn fahrenheit_to_celsius(f: float) -> float {
    return (f - 32.0) * 5.0 / 9.0
}

temp_c := 25.0
temp_f := celsius_to_fahrenheit(temp_c)

print(str(temp_c) + "°C = " + str(temp_f) + "°F")
```

## Even or Odd

```volta
fn is_even(n: int) -> bool {
    return n % 2 == 0
}

numbers := [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]

for num in numbers {
    result := if is_even(num) { "even" } else { "odd" }
    print(str(num) + " is " + result)
}
```

## Factorial

```volta
fn factorial(n: int) -> int {
    if n <= 1 {
        return 1
    }
    return n * factorial(n - 1)
}

for i in range(1, 6) {
    result := factorial(i)
    print("Factorial of " + str(i) + " is " + str(result))
}
```

## Array Sum

```volta
fn sum(arr: Array[int]) -> int {
    total: mut int = 0
    for num in arr {
        total = total + num
    }
    return total
}

numbers := [1, 2, 3, 4, 5]
total := sum(numbers)
print("Sum: " + str(total))
```

## Find Maximum

```volta
fn find_max(arr: Array[int]) -> Option[int] {
    if arr.len() == 0 {
        return None
    }
    
    max: mut int = arr[0]
    for num in arr {
        if num > max {
            max = num
        }
    }
    
    return Some(max)
}

numbers := [3, 7, 2, 9, 1, 5]
result := find_max(numbers)

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
counter: mut int = 10
while counter > 0 {
    print(counter)
    counter = counter - 1
}
print("Blast off!")
```

## FizzBuzz

```volta
for i in 1..=100 {
    output := if i % 15 == 0 {
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
fn repeat(text: str, times: int) -> str {
    result: mut str = ""
    for i in range(times) {
        result = result + text
    }
    return result
}

stars := repeat("*", 10)
print(stars)

# Create a box
line := repeat("-", 20)
print(line)
print("| Hello, Volta!    |")
print(line)
```

## Prime Number Check

```volta
fn is_prime(n: int) -> bool {
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
fn average(numbers: Array[float]) -> Option[float] {
    if numbers.len() == 0 {
        return None
    }
    
    sum: mut float = 0.0
    for num in numbers {
        sum = sum + num
    }
    
    return Some(sum / float(numbers.len()))
}

scores := [85.0, 90.0, 78.0, 92.0, 88.0]
avg := average(scores)

match avg {
    Some(value) => print("Average: " + str(value)),
    None => print("No scores")
}
```

## Grade Calculator

```volta
fn letter_grade(score: int) -> str {
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

scores := [95, 87, 72, 64, 55]

for score in scores {
    grade := letter_grade(score)
    print("Score " + str(score) + " = Grade " + grade)
}
```
