# Loops

## For Loops with Ranges

```volta
# Exclusive range (0 to 4)
for i in 0..5 {
    print(i)  # 0, 1, 2, 3, 4
}

# Inclusive range (0 to 5)
for i in 0..=5 {
    print(i)  # 0, 1, 2, 3, 4, 5
}

# Range with negative numbers
for i in -3..3 {
    print(i)  # -3, -2, -1, 0, 1, 2
}
```

## For Loops with Arrays

```volta
numbers := [1, 2, 3, 4, 5]

for num in numbers {
    print(num)
}

# Array of strings
names := ["Alice", "Bob", "Charlie"]
for name in names {
    print("Hello, " + name)
}
```

## While Loops

```volta
counter: mut int = 0

while counter < 5 {
    print(counter)
    counter = counter + 1
}
```

## Break Statement

Exit a loop early:

```volta
for i in range(10) {
    if i == 5 {
        break
    }
    print(i)
}  # Prints 0, 1, 2, 3, 4
```

## Continue Statement

Skip to the next iteration:

```volta
for i in range(10) {
    if i % 2 == 0 {
        continue
    }
    print(i)
}  # Prints 1, 3, 5, 7, 9
```

## Nested Loops

```volta
for i in range(3) {
    for j in range(3) {
        print("(" + str(i) + ", " + str(j) + ")")
    }
}
```

## Loop Examples

### Sum of array elements

```volta
numbers := [1, 2, 3, 4, 5]
sum: mut int = 0

for num in numbers {
    sum = sum + num
}

print("Sum: " + str(sum))
```

### Finding an element

```volta
numbers := [10, 20, 30, 40, 50]
target := 30
found: mut bool = false

for num in numbers {
    if num == target {
        found = true
        break
    }
}

if found {
    print("Found!")
}
```

### Counting even numbers

```volta
numbers := [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
count: mut int = 0

for num in numbers {
    if num % 2 == 0 {
        count = count + 1
    }
}

print("Even numbers: " + str(count))
```

### While loop with condition

```volta
x: mut int = 1

while x <= 100 {
    print(x)
    x = x * 2
}  # Prints 1, 2, 4, 8, 16, 32, 64
```
