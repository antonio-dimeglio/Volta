# Structs

Structs allow you to create custom data types with named fields.

## Defining Structs

```volta
struct Point {
    x: float,
    y: float
}
```

## Creating Struct Instances

```volta
# Create instance
p := Point { x: 3.0, y: 4.0 }

# With explicit type
origin: Point = Point { x: 0.0, y: 0.0 }
```

## Accessing Fields

```volta
p := Point { x: 3.0, y: 4.0 }

x_coord := p.x  # 3.0
y_coord := p.y  # 4.0

print("Point: (" + str(p.x) + ", " + str(p.y) + ")")
```

## Struct Methods

Define methods using the dot notation:

```volta
struct Point {
    x: float,
    y: float
}

fn Point.distance(self) -> float {
    return math.sqrt(self.x * self.x + self.y * self.y)
}

fn Point.translate(self, dx: float, dy: float) -> Point {
    return Point { x: self.x + dx, y: self.y + dy }
}

# Usage
p := Point { x: 3.0, y: 4.0 }
dist := p.distance()  # 5.0
moved := p.translate(1.0, 2.0)  # Point { x: 4.0, y: 6.0 }
```

## Struct Examples

### Rectangle

```volta
struct Rectangle {
    width: float,
    height: float
}

fn Rectangle.area(self) -> float {
    return self.width * self.height
}

fn Rectangle.perimeter(self) -> float {
    return 2.0 * (self.width + self.height)
}

rect := Rectangle { width: 5.0, height: 3.0 }
area := rect.area()      # 15.0
perim := rect.perimeter() # 16.0
```

### Person

```volta
struct Person {
    name: str,
    age: int,
    email: str
}

fn Person.greet(self) -> str {
    return "Hello, I'm " + self.name
}

fn Person.is_adult(self) -> bool {
    return self.age >= 18
}

alice := Person {
    name: "Alice",
    age: 30,
    email: "alice@example.com"
}

greeting := alice.greet()
adult := alice.is_adult()
```

## Nested Structs

```volta
struct Address {
    street: str,
    city: str
}

struct Employee {
    name: str,
    address: Address
}

emp := Employee {
    name: "Bob",
    address: Address {
        street: "123 Main St",
        city: "Springfield"
    }
}

city := emp.address.city
```

## Structs in Arrays

```volta
struct Point {
    x: float,
    y: float
}

points: Array[Point] = [
    Point { x: 0.0, y: 0.0 },
    Point { x: 1.0, y: 1.0 },
    Point { x: 2.0, y: 2.0 }
]

for p in points {
    print("(" + str(p.x) + ", " + str(p.y) + ")")
}
```

## Mutability

### Immutable Structs (Default)

Structs are immutable by default:

```volta
p := Point { x: 3.0, y: 4.0 }

# Cannot modify
# p.x = 5.0  # Error!

# Create new instance instead
new_p := Point { x: 5.0, y: 4.0 }
```

### Mutable Structs

To modify struct fields, declare the variable as mutable:

```volta
mut p: Point = Point { x: 3.0, y: 4.0 }
p.x = 5.0  # ✓ OK - p is mutable
p.y = 10.0 # ✓ OK
```

### Methods with Mutable Self

Methods can modify the struct by using `mut self`:

```volta
struct Counter {
    value: int
}

fn Counter.increment(mut self) {
    self.value = self.value + 1
}

fn Counter.reset(mut self) {
    self.value = 0
}

mut counter: Counter = Counter { value: 0 }
counter.increment()  # value is now 1
counter.increment()  # value is now 2
counter.reset()      # value is now 0
```

**Important:** You can only call methods with `mut self` on mutable instances:

```volta
counter := Counter { value: 0 }  # immutable
counter.increment()  # ✗ Error - cannot call mut method on immutable instance

mut counter2: Counter = Counter { value: 0 }  # mutable
counter2.increment()  # ✓ OK
```

## Generic Structs

Structs can be generic over types:

```volta
# Generic container that works with any type
struct Box[T] {
    value: T
}

# Create instances with different types
intBox: Box[int] = Box { value: 42 }
floatBox: Box[float] = Box { value: 3.14 }
strBox: Box[str] = Box { value: "hello" }

print(intBox.value)   # 42
print(floatBox.value) # 3.14
```

### Generic Methods

Methods on generic structs can use the type parameters:

```volta
struct Box[T] {
    value: T
}

fn Box.new(val: T) -> Box[T] {
    return Box { value: val }
}

fn Box.getValue(self) -> T {
    return self.value
}

fn Box.map[U](self, transform: fn(T) -> U) -> Box[U] {
    return Box { value: transform(self.value) }
}

# Usage
intBox: Box[int] = Box.new(42)
value: int = intBox.getValue()
```

### Multiple Type Parameters

Structs can have multiple type parameters:

```volta
struct Pair[T, U] {
    first: T,
    second: U
}

fn Pair.swap(self) -> Pair[U, T] {
    return Pair { first: self.second, second: self.first }
}

# Create a pair
p: Pair[int, str] = Pair { first: 42, second: "hello" }

# Swap creates a Pair[str, int]
swapped: Pair[str, int] = p.swap()
```

## Constructor-like Functions

```volta
struct Circle {
    radius: float
}

fn create_circle(r: float) -> Circle {
    return Circle { radius: r }
}

fn Circle.area(self) -> float {
    return 3.14159 * self.radius * self.radius
}

c := create_circle(5.0)
area := c.area()
```

## Pattern Matching

```volta
struct Point {
    x: int,
    y: int
}

p := Point { x: 0, y: 0 }

# Pattern matching with structs
match p {
    Point { x: 0, y: 0 } => print("Origin"),
    Point { x, y } => print("Point at " + str(x) + ", " + str(y))
}
```

## Practical Examples

### Vector math

```volta
struct Vector2D {
    x: float,
    y: float
}

fn Vector2D.magnitude(self) -> float {
    return math.sqrt(self.x * self.x + self.y * self.y)
}

fn Vector2D.add(self, other: Vector2D) -> Vector2D {
    return Vector2D {
        x: self.x + other.x,
        y: self.y + other.y
    }
}

fn Vector2D.scale(self, factor: float) -> Vector2D {
    return Vector2D {
        x: self.x * factor,
        y: self.y * factor
    }
}

v1 := Vector2D { x: 3.0, y: 4.0 }
v2 := Vector2D { x: 1.0, y: 2.0 }

v3 := v1.add(v2)
scaled := v1.scale(2.0)
```

### Date representation

```volta
struct Date {
    year: int,
    month: int,
    day: int
}

fn Date.to_string(self) -> str {
    return str(self.year) + "-" + str(self.month) + "-" + str(self.day)
}

today := Date { year: 2025, month: 10, day: 7 }
date_str := today.to_string()
```
