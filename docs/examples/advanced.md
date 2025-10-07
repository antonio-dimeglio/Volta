# Advanced Examples

More complex programs demonstrating advanced Volta features.

## Binary Search

```volta
fn binary_search(arr: Array[int], target: int) -> Option[int] {
    left: mut int = 0
    right: mut int = arr.len() - 1
    
    while left <= right {
        mid := (left + right) / 2
        
        if arr[mid] == target {
            return Some(mid)
        } else if arr[mid] < target {
            left = mid + 1
        } else {
            right = mid - 1
        }
    }
    
    return None
}

sorted_array := [1, 3, 5, 7, 9, 11, 13, 15]
result := binary_search(sorted_array, 7)

match result {
    Some(index) => print("Found at index: " + str(index)),
    None => print("Not found")
}
```

## Bubble Sort

```volta
fn bubble_sort(arr: Array[int]) -> Array[int] {
    result: mut Array[int] = arr
    n := result.len()
    
    for i in range(n) {
        for j in range(0, n - i - 1) {
            if result[j] > result[j + 1] {
                # Swap
                temp := result[j]
                result[j] = result[j + 1]
                result[j + 1] = temp
            }
        }
    }
    
    return result
}

unsorted := [64, 34, 25, 12, 22, 11, 90]
sorted := bubble_sort(unsorted)

print("Sorted: " + str(sorted))
```

## Fibonacci Sequence

```volta
fn fibonacci(n: int) -> Array[int] {
    if n <= 0 {
        return []
    }
    if n == 1 {
        return [0]
    }
    
    result: mut Array[int] = [0, 1]
    
    for i in range(2, n) {
        next := result[i - 1] + result[i - 2]
        result.push(next)
    }
    
    return result
}

fib := fibonacci(10)
print("First 10 Fibonacci numbers:")
for num in fib {
    print(str(num))
}
```

## Tower of Hanoi

```volta
fn hanoi(n: int, from: str, to: str, aux: str) {
    if n == 1 {
        print("Move disk 1 from " + from + " to " + to)
        return
    }
    
    hanoi(n - 1, from, aux, to)
    print("Move disk " + str(n) + " from " + from + " to " + to)
    hanoi(n - 1, aux, to, from)
}

print("Tower of Hanoi with 3 disks:")
hanoi(3, "A", "C", "B")
```

## Matrix Operations

```volta
struct Matrix {
    rows: int,
    cols: int,
    data: Array[Array[float]]
}

fn create_matrix(rows: int, cols: int, value: float) -> Matrix {
    data: mut Array[Array[float]] = []
    
    for i in range(rows) {
        row: mut Array[float] = []
        for j in range(cols) {
            row.push(value)
        }
        data.push(row)
    }
    
    return Matrix { rows: rows, cols: cols, data: data }
}

fn Matrix.get(self, i: int, j: int) -> float {
    return self.data[i][j]
}

fn Matrix.set(self, i: int, j: int, value: float) -> Matrix {
    new_data := self.data
    new_data[i][j] = value
    return Matrix { rows: self.rows, cols: self.cols, data: new_data }
}

fn Matrix.transpose(self) -> Matrix {
    result := create_matrix(self.cols, self.rows, 0.0)
    
    for i in range(self.rows) {
        for j in range(self.cols) {
            result = result.set(j, i, self.get(i, j))
        }
    }
    
    return result
}
```

## Word Counter

```volta
fn count_words(text: str) -> int {
    if text.len() == 0 {
        return 0
    }
    
    words := text.split(" ")
    return words.filter(fn(w: str) -> bool = w.len() > 0).len()
}

fn most_common_word(text: str) -> Option[str] {
    words := text.split(" ")
    
    if words.len() == 0 {
        return None
    }
    
    # Simple implementation - returns first word
    # Full implementation would count occurrences
    return Some(words[0])
}

text := "hello world hello volta"
count := count_words(text)
print("Word count: " + str(count))
```

## Vector Math

```volta
struct Vector2D {
    x: float,
    y: float
}

fn Vector2D.add(self, other: Vector2D) -> Vector2D {
    return Vector2D {
        x: self.x + other.x,
        y: self.y + other.y
    }
}

fn Vector2D.subtract(self, other: Vector2D) -> Vector2D {
    return Vector2D {
        x: self.x - other.x,
        y: self.y - other.y
    }
}

fn Vector2D.scale(self, factor: float) -> Vector2D {
    return Vector2D {
        x: self.x * factor,
        y: self.y * factor
    }
}

fn Vector2D.magnitude(self) -> float {
    return math.sqrt(self.x * self.x + self.y * self.y)
}

fn Vector2D.normalize(self) -> Vector2D {
    mag := self.magnitude()
    if mag == 0.0 {
        return self
    }
    return self.scale(1.0 / mag)
}

v1 := Vector2D { x: 3.0, y: 4.0 }
v2 := Vector2D { x: 1.0, y: 2.0 }

sum := v1.add(v2)
print("Sum: (" + str(sum.x) + ", " + str(sum.y) + ")")

mag := v1.magnitude()
print("Magnitude: " + str(mag))
```

## Statistical Analysis

```volta
fn mean(data: Array[float]) -> Option[float] {
    if data.len() == 0 {
        return None
    }
    
    sum := data.reduce(fn(a: float, b: float) -> float = a + b, 0.0)
    return Some(sum / float(data.len()))
}

fn variance(data: Array[float]) -> Option[float] {
    avg := mean(data)
    
    match avg {
        None => return None,
        Some(m) => {
            squared_diffs := data.map(fn(x: float) -> float = {
                diff := x - m
                return diff * diff
            })
            
            return mean(squared_diffs)
        }
    }
}

fn std_dev(data: Array[float]) -> Option[float] {
    var := variance(data)
    
    return match var {
        None => None,
        Some(v) => Some(math.sqrt(v))
    }
}

data := [2.5, 3.7, 4.1, 3.2, 2.9]

match mean(data) {
    Some(m) => print("Mean: " + str(m)),
    None => print("No data")
}

match std_dev(data) {
    Some(sd) => print("Std Dev: " + str(sd)),
    None => print("No data")
}
```

## Recursive Permutations

```volta
fn permutations(arr: Array[int]) -> Array[Array[int]] {
    if arr.len() == 0 {
        return [[]]
    }
    
    if arr.len() == 1 {
        return [arr]
    }
    
    result: mut Array[Array[int]] = []
    
    for i in range(arr.len()) {
        # Get element at i
        elem := arr[i]
        
        # Get remaining elements
        remaining: mut Array[int] = []
        for j in range(arr.len()) {
            if j != i {
                remaining.push(arr[j])
            }
        }
        
        # Get permutations of remaining
        sub_perms := permutations(remaining)
        
        # Add current element to each permutation
        for perm in sub_perms {
            new_perm: mut Array[int] = [elem]
            for x in perm {
                new_perm.push(x)
            }
            result.push(new_perm)
        }
    }
    
    return result
}

arr := [1, 2, 3]
perms := permutations(arr)

print("Permutations of [1, 2, 3]:")
for perm in perms {
    print(str(perm))
}
```

## Simple Interpreter

```volta
fn evaluate(expr: str) -> Option[int] {
    # Simple arithmetic expression evaluator
    # Supports +, -, *, /
    
    # Parse and evaluate
    # This is a simplified example
    
    if expr.contains("+") {
        parts := expr.split("+")
        if parts.len() == 2 {
            left := parse_int(parts[0])
            right := parse_int(parts[1])
            
            match (left, right) {
                (Some(l), Some(r)) => return Some(l + r),
                _ => return None
            }
        }
    }
    
    # Try parsing as single number
    return parse_int(expr)
}

expressions := ["42", "10+5", "20+22"]

for expr in expressions {
    result := evaluate(expr)
    match result {
        Some(value) => print(expr + " = " + str(value)),
        None => print(expr + " = Error")
    }
}
```
