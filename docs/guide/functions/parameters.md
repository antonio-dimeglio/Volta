# Parameters & Return Values

## Function Parameters

All parameters must have explicit type annotations:

```volta
fn greet(name: str, age: int) -> str {
    return name + " is " + str(age) + " years old"
}
```

## Multiple Parameters

```volta
fn calculate_bmi(weight: float, height: float) -> float {
    return weight / (height * height)
}

bmi := calculate_bmi(70.0, 1.75)
```

## Return Values

Functions specify their return type with `->`:

```volta
fn add(a: int, b: int) -> int {
    return a + b
}

fn is_even(n: int) -> bool {
    return n % 2 == 0
}

fn get_name() -> str {
    return "Alessandro"
}
```

## Multiple Return Points

```volta
fn absolute(n: int) -> int {
    if n < 0 {
        return -n
    }
    return n
}
```

## Returning Complex Types

### Returning Arrays

```volta
fn create_range(start: int, end: int) -> Array[int] {
    result: mut Array[int] = []
    for i in range(start, end) {
        result.push(i)
    }
    return result
}

numbers := create_range(1, 6)
```

### Returning Options

```volta
fn divide(a: float, b: float) -> Option[float] {
    if b == 0.0 {
        return None
    }
    return Some(a / b)
}

result := divide(10.0, 2.0)
```

### Returning Tuples

```volta
fn min_max(numbers: Array[int]) -> (int, int) {
    min := numbers[0]
    max := numbers[0]
    
    for num in numbers {
        if num < min {
            min = num
        }
        if num > max {
            max = num
        }
    }
    
    return (min, max)
}

(minimum, maximum) := min_max([3, 1, 4, 1, 5, 9])
```

## Void Functions

Functions that don't return a value:

```volta
fn print_array(arr: Array[int]) {
    for item in arr {
        print(str(item))
    }
}
```

## Practical Examples

### String manipulation

```volta
fn substring(text: str, start: int, length: int) -> str {
    # Implementation
    return text[start:start+length]
}
```

### Array search

```volta
fn find_index(arr: Array[int], target: int) -> Option[int] {
    for i in range(arr.len()) {
        if arr[i] == target {
            return Some(i)
        }
    }
    return None
}
```

### Statistics

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
```
