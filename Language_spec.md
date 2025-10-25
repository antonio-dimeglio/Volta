The following is a document outlining the language specs of Volta.
Volta is a Garbage Collected imperative language with scientific computing in mind, which allows user to easily bind to C code by providing ergonomic infrastructure to do so.
Each .vlt executable must have a main function. The smallest Volta language is the following
```{vlt}
fn main() -> i32 {
    return 0;
}
```
An example of the classic Hello world program is the following:
```{vlt}
fn main() -> i32 {
    println("Hello, world!");
    return 0;
}
```
## Types
Volta supports a large body of primitive types, those being:

##### Numeric
U-I8, U-I16, U-I32, U-I64, U-I128, alongside with F8 (for GPU computing), F16, F32, F64 and complex types such as Complex32, Complex64, Complex128 (per IEEE standard, which are equivlaent to float, double and long double _Complex). There is __NO__ implicit conversion in the language, only explicit. This is to prevent unexpected behaviour when writing code. A special type is defined for c calls, usize, which is system specific.
##### Logic
Boolean type, can only be true or false.
##### String
The language supports both Char and String types, both internally use UTF8 representation instead of raw bytes (i.e., if I do len("a🙂b") I expect to get 3, not 6.)
##### Variables
Variables in the language are immutable by default, and are declared with one of the following syntaxes `let x: int`, or if we want to assign a value we can do either `let x: int = 42` or `let x := 42` for type inference. Mutable variables can be declared as such by writing `let mut x: int = 42` or `let mut x := 42`. Compound assignment operators (`+=`, `-=`, `*=`, `/=`, `%=`) and increment/decrement operators (`++`, `--`) are supported for mutable variables.
##### Pointers
Pointers are a critical aspect of the language, as they are necessary for binding to c libraries. The language provides a unique interface for pointers, `Ptr<Type>`. Elements declared as pointers are NOT collected by GC, and users are expected to do their due diligence with them. Uninitialized ptrs are set to null, and ptrs are the only type in the language that can be rendered nullable.
##### Array
Two arrays exist, one, beign a fixed size array, denoted with the type syntax `[Type; Size]` and generic, dynamically allocated homogenous array, declared with the generic syntax `Array<Type>`. 
Fixed size array require its size declared at compile time, and can be declared and indexed in the following fashion:
```volta
// 1D array
let arr: [i32; 5] = [1, 2, 3, 4, 5];
// All 100 elements = 42
let arr: [i32; 100] = [42];        
// Indexing
let val: i32 = arr[42];
// 2D array
let arr: [i32; 2, 3] = [[1, 2, 3], [4, 5, 6]];
// 3D array
let arr: [f64; 10, 10, 10] = [1.0]; 
let val: f64 = arr[3, 4, 5];
```
##### HashMap and HashSet
Hashmap and hashset and are declared respectively using the type syntax `{KeyType:ValueType}` and `{ValueType}`.
##### Composite types (Structs)
Structs in Volta define custom data types with fields and methods. Unlike Rust, methods are defined directly inside the struct body rather than in separate `impl` blocks.

**Visibility:**
- Structs can be `pub` (importable) or private (file-scoped only)
- Fields can be `pub` (accessible from outside) or private (only accessible within methods)
- Methods can be `pub` or private

**Struct Literals:**
- If a struct has any private fields, you cannot create it with a struct literal from outside the module
- You must use a constructor method (typically `new`)

**Method Types:**
- **Static methods**: No `self` parameter, called with `::` operator (e.g., `Point::new(x, y)`)
- **Instance methods**: Have `self` or `mut self` parameter, called with `.` operator (e.g., `p.distance()`)

**Mutability:**
- Struct field mutability is tied to the variable mutability (not per-field)
- Methods taking `mut self` can only be called on mutable variables
- Methods taking `self` can be called on both mutable and immutable variables
- The `self` parameter is always passed by reference (automatic, user doesn't specify `&`)

**Examples:**

```rust
// Public struct with public and private fields
pub struct Point {
    pub x: f32,      // Public field
    pub y: f32,      // Public field
    cache: f32,      // Private field

    // Static method (constructor) - called with Point::new()
    pub fn new(x: f32, y: f32) -> Point {
        return Point { x: x, y: y, cache: 0.0 };
    }

    // Instance method with immutable self - called with p.distance()
    pub fn distance(self) -> f32 {
        return sqrt(self.x * self.x + self.y * self.y);
    }

    // Instance method with mutable self - called with p.move_by()
    pub fn move_by(mut self, dx: f32, dy: f32) {
        self.x = self.x + dx;
        self.y = self.y + dy;
    }

    // Private helper method
    fn update_cache(mut self) {
        self.cache = self.distance();
    }
}

// Usage:
let mut p = Point.new(10.0, 20.0);  
let d = p.distance();
p.move_by(5.0, 5.0);                 // Mutable method on mutable var

// Can access public fields directly:
let x = p.x;
p.x = 30.0;

// Cannot access private fields:
// let c = p.cache;  // ERROR: field 'cache' is private
```

**Private structs:**

```rust
// Private struct - cannot be imported by other modules
struct InternalCounter {
    count: i32,

    fn new() -> InternalCounter {
        return InternalCounter { count: 0 };
    }

    fn increment(mut self) {
        self.count = self.count + 1;
    }
}
```

**Structs with all private fields:**

```rust
pub struct OpaqueHandle {
    internal_id: i32,     // Private field
    internal_ptr: ptr<opaque>,  // Private field

    // Must provide constructor since struct literal is not allowed
    pub fn new(id: i32) -> OpaqueHandle {
        return OpaqueHandle {
            internal_id: id,
            internal_ptr: malloc(128)
        };
    }

    pub fn get_id(self) -> i32 {
        return self.internal_id;
    }
}

// In another module:
let h = OpaqueHandle.new(42);  // OK - calls static method
let id = h.get_id();            // OK - calls instance method
// let h2 = OpaqueHandle { internal_id: 42, internal_ptr: ... };  // ERROR: private fields
```

##### Generics

Volta supports parametric polymorphism through generics, allowing you to write code that works with multiple types while maintaining type safety. Generics are implemented via monomorphization at compile-time, meaning each unique instantiation generates specialized code with zero runtime overhead.

**Type Parameters:**

Type parameters are declared in angle brackets `<>` and can use any valid identifier (not just single letters). Common conventions include:
- Single uppercase letters: `T`, `U`, `V`, `K` (Key), `V` (Value)
- Descriptive names: `Item`, `Element`, `Key`, `Value`

Type parameters must be unique within their scope and cannot shadow local variables or other type parameters.

**Generic Functions:**

Functions can be generic over one or more type parameters. Type arguments must be explicitly provided at the call site.

```volta
// Single type parameter
fn swap<T>(a: mut ref T, b: mut ref T) -> void {
    let temp: T = a;
    a = b;
    b = temp;
}

// Multiple type parameters with descriptive names
fn pair<First, Second>(a: First, b: Second) -> Pair<First, Second> {
    return Pair<First, Second> { first: a, second: b };
}

// Usage - type arguments are required
let x: i32 = 5;
let y: i32 = 10;
swap<i32>(x, y);

let p = pair<i32, f64>(42, 3.14);
```

**Generic Structs:**

Structs can be parameterized over types, creating flexible data structures.

```volta
// Simple generic container
pub struct Box<T> {
    pub value: T
}

// Generic with multiple parameters
pub struct Pair<T, U> {
    pub first: T,
    pub second: U
}

// Generic struct with methods
pub struct Container<Element> {
    data: Element,
    count: i32

    // Static method - returns generic type
    pub fn new(value: Element) -> Container<Element> {
        return Container<Element> { data: value, count: 1 };
    }

    // Instance method using type parameter
    pub fn get(self) -> Element {
        return self.data;
    }

    // Instance method with mutable self
    pub fn set(mut self, value: Element) -> void {
        self.data = value;
    }
}

// Usage - type arguments required for instantiation
let box_int: Box<i32> = Box<i32> { value: 42 };
let box_str: Box<String> = Box<String> { value: "hello" };

// Type arguments on static method calls
let container = Container<i32>.new(100);
let value = container.get();

// Type inference from variable declaration
let pair: Pair<i32, f64> = Pair<i32, f64> { first: 1, second: 3.14 };
```

**Nested Generics:**

Generic types can be nested arbitrarily.

```volta
let nested: Box<Pair<i32, f64>> = Box<Pair<i32, f64>> {
    value: Pair<i32, f64> { first: 1, second: 2.0 }
};

let array_of_boxes: [Box<i32>; 3] = [
    Box<i32> { value: 1 },
    Box<i32> { value: 2 },
    Box<i32> { value: 3 }
];
```

**Monomorphization:**

Volta uses compile-time monomorphization, similar to C++ templates and Rust generics. When you use a generic function or struct with concrete types, the compiler generates a specialized version of that code.

```volta
swap<i32>(a, b);  // Generates swap$i32
swap<f64>(x, y);  // Generates swap$f64
```

**Restrictions:**

1. **No recursive type bounds**: Currently, generic parameters cannot reference themselves or create cycles
2. **Explicit type arguments**: Type inference at call sites is not yet supported - you must always provide type arguments explicitly
3. **No higher-kinded types**: Cannot abstract over generic types themselves (e.g., cannot write `Container<Box>` where `Box` itself is generic)
4. **No default type parameters**: All type parameters must be specified

**Error Examples:**

```volta
// ERROR: Duplicate type parameter 'T'
fn bad<T, T>(a: T, b: T) -> T { }

// ERROR: Type parameter 'T' shadows local variable
fn shadow<T>(x: i32) -> T {
    let T: i32 = 42;  // Identifier 'T' conflicts with type parameter
}

// ERROR: Missing type arguments
let b = Box { value: 42 };  // Must specify Box<i32>

// ERROR: Wrong number of type arguments
let p: Pair<i32> = ...;  // Pair requires 2 type arguments
```

**Future Extensions:**

Traits (planned) will enable bounded generic programming, allowing you to constrain which types can be used with a generic function or struct based on their capabilities. See the Traits section for more details.

##### Functions

Functions are defined with syntax `fn fn_name(arg1: type, arg2: type) -> Type {}`.

**Parameter Passing Modes:**

Volta supports four parameter passing modes for all types:

1. **By Constant Value** (default): `param: Type`
   - Creates an immutable copy of the argument
   - Modifications inside the function are not allowed (immutable)
   - For primitives (i32, f64, bool): efficient, passed in registers
   - For arrays and structs: creates a deep copy (can be expensive for large types)

2. **By Mutable Value**: `param: mut Type`
   - Creates a mutable copy of the argument
   - Modifications inside the function do not affect the caller (it's a copy)
   - Useful when you need to modify a parameter locally without affecting the original

3. **By Immutable Reference**: `param: ref Type`
   - Passes a reference to the original value
   - More efficient for large types (no copy)
   - Cannot modify the value inside the function
   - Compiler enforces immutability

4. **By Mutable Reference**: `param: mut ref Type`
   - Passes a reference to the original value
   - Can modify the value, changes affect the caller
   - Requires the caller to pass a mutable variable (`let mut`)

**Examples:**

```volta
// 1. Pass by immutable value - copy semantics, cannot modify
fn read_array(arr: [i32; 3]) -> i32 {
    // arr[0] = 999;  // ERROR: cannot modify immutable parameter
    return arr[0] + arr[1] + arr[2];
}

let data = [1, 2, 3];
read_array(data);  // data is copied, original unchanged

// 2. Pass by mutable value - copy semantics, can modify copy
fn modify_array(arr: mut [i32; 3]) -> i32 {
    arr[0] = 999;  // Modifies the local copy only
    return arr[0];
}

let original = [1, 2, 3];
modify_array(original);  // original[0] is still 1

// 3. Pass by immutable reference - no copy, read-only
fn sum_array(arr: ref [i32; 100]) -> i32 {
    let total = 0;
    for i in 0..100 {
        total += arr[i];  // Can read
    }
    // arr[0] = 999;  // ERROR: cannot modify immutable reference
    return total;
}

sum_array(ref data);  // No copy made, efficient

// 4. Pass by mutable reference - no copy, can modify original
fn zero_array(arr: mut ref [i32; 100]) {
    for i in 0..100 {
        arr[i] = 0;  // Modifies the original array
    }
}

let mut data = [1, 2, 3];
zero_array(mut ref data);  // data is now all zeros

// Same applies to structs
struct Point { pub x: f64, pub y: f64 }

fn distance(p1: ref Point, p2: ref Point) -> f64 {
    // Read-only access, efficient (no copy)
    let dx = p2.x - p1.x;
    let dy = p2.y - p1.y;
    return sqrt(dx*dx + dy*dy);
}

fn translate(p: mut ref Point, dx: f64, dy: f64) {
    p.x += dx;  // Modifies the original struct
    p.y += dy;
}
```

**Important Notes:**

- All four modes work for all types (primitives, arrays, structs)
- By-value copies can be expensive for large types - consider using references for efficiency
- Mutable references (`mut ref`) require the caller to pass a mutable variable (`let mut`)
- Immutable references prevent any modification, enforced at compile time
- The compiler uses allocation analysis to decide stack vs heap allocation based on escape analysis
## Syntax
##### If
If statement are written as `if cond1 and cond2 or cond3 and notcond4 {} else if cond5 {} else {}`
##### While
While statements, similarly, have the condition without necessary parentheses, deifned as while cond {} and support both `break` and `continue` keywords.
##### For
For loops can be performed on anything that is iterable, such as statically sized arrays, dynamically sized arrays, or ranges such as `0..42`.
##### Match
The match syntax is also the same as rust one, with the exception of the `else` keyword for default matching. Example of the match structure can be found in enum!
##### Enums
Enums are user-defined data type consisting of a set of named constant, and use the same syntax of C++:
```{Rust}
pub enum Status {
    Success = 0,
    Error = 1,
    Pending = 2,
}

// Auto-numbered (starts at 0)
enum Color {
    Red,      // 0
    Green,    // 1
    Blue,     // 2
}

// Specify underlying type
enum Priority: u8 {
    Low = 0,
    Medium = 1,
    High = 2,
}
```
# Variants
Variants are fully algebraic data types, similar to Rust enums, so as an example:
```
Variant Expr {
    Number(i32),
    Add(Expr, Expr),
    Multiply(Expr, Expr),
    Negate(Expr),

    fn eval(self) -> i32 {
        return match self {
            Expr::Number(n) => n,
            Expr::Add(lhs, rhs) => lhs.eval() + rhs.eval(),
            Expr::Multiply(lhs, rhs) => lhs.eval() * rhs.eval(),
            Expr::Negate(expr) => -expr.eval(),
            else => unimplemented!()
        }
    }
}
```
##### Errors
Volta relies on result types, rather than exceptions, declared as `Result<Type, Err>`, theres also the optional type `Optional<T>`
For example:
```{rust}
fn divide(a: i32, b: i32) -> Result<i32, String> {
    if b == 0 {
        Err("Division by zero".to_string())
    } else {
        Ok(a / b)
    }
}
``` 
and

```{rust}
fn find_even(numbers: &[i32]) -> Option<i32> {
    for &n in numbers {
        if n % 2 == 0 {
            return Some(n);
        }
    }
    None
}
```

### FFI

FFI calls require extern blocks, which specify (and wrap) c functions:
```
extern "C" {
    fn strlen(s: ptr<u8>) -> usize
    fn malloc(size: usize) -> ptr<opaque>
}
```
Note the usage of the opaque keyword, which is just a fancy way of declaring a void pointer, for which the size is system specific.
char* is a ptr<u8>, while numeric types can be directly translated with the supported Volta types. Conversion from utf8 string to C strings can be done by performing `my_string.to_c_str()` calls. The language also supports the use of the `addrof` keyword to obtain the address of a variable, in order to pass it to C definitiions.

#### Struct Layout Compatibility
When a Volta struct matches a C struct layout exactly (same field types, same order), pointers can be passed directly to C functions without conversion overhead. Volta generates LLVM struct types that are binary-compatible with C structs.

Example:
```volta
// C side: typedef struct { char* data; size_t size; } MyStruct;

// Volta side: Must match C layout exactly
pub struct MyStruct {
    data: ptr<i8>,    // char* in C
    size: u64         // size_t in C (on 64-bit systems)
}

extern "C" {
    fn process_struct(s: ptr<MyStruct>) -> void;
}

fn main() -> i32 {
    let s = MyStruct { data: null, size: 0 };
    process_struct(addrof(s));  // Direct pointer passing, no conversion
    return 0;
}
```

The Volta struct and C struct must have:
- Same field types
- Same field order
- Same sizes (ptr<i8> = char*, u64 = size_t on 64-bit, etc.)

When these conditions are met, Volta pointers and C pointers are fully interchangeable.

#### GC and Persistent Pointers

**Warning**: Volta uses garbage collection (Boehm GC). If C code stores pointers to GC-allocated Volta objects, you must ensure proper lifetime management:

**Safe Usage** (Temporary pointers):
```volta
extern "C" {
    fn process_data(data: ptr<i8>, len: u64) -> i32;
}

fn safe_ffi() -> i32 {
    let s = String.from_cstr("Hello");
    // Safe: C function uses the pointer temporarily and doesn't store it
    return process_data(s.to_cstr(), s.length());
}
```

**Unsafe Usage** (Persistent storage):
```volta
extern "C" {
    fn store_string_globally(s: ptr<i8>) -> void;  // C stores this pointer!
}

fn unsafe_ffi() {
    let s = String.from_cstr("Hello");
    store_string_globally(s.to_cstr());  // DANGER: GC may collect s later!
    // If s gets garbage collected, the C pointer becomes invalid
}
```

**Solutions for Persistent Storage**:

1. **Deep Copy with Manual Allocation**: If C needs to store the data permanently, allocate with malloc (not GC) and document that C must free() the memory:

```volta
extern "C" {
    fn malloc(size: u64) -> ptr<opaque>;
    fn memcpy(dst: ptr<opaque>, src: ptr<opaque>, n: u64) -> ptr<opaque>;
    fn store_string_globally(s: ptr<i8>) -> void;
}

fn safe_persistent_ffi() {
    let s = String.from_cstr("Hello");

    // Allocate non-GC memory
    let persistent_copy = malloc(s.byte_length() + 1);
    memcpy(persistent_copy, s.to_cstr(), s.byte_length() + 1);

    // Now C owns this memory and must free() it when done
    store_string_globally(persistent_copy);
}
```

2. **Keep References Alive**: Ensure the Volta object remains reachable for the entire duration that C holds the pointer:

```volta
// Global or long-lived variable prevents GC
let global_string = String.from_cstr("Persistent");

extern "C" {
    fn use_string_pointer(s: ptr<i8>) -> void;
}

fn main() -> i32 {
    // Safe: global_string won't be collected
    use_string_pointer(global_string.to_cstr());
    return 0;
}
```

**Summary**:
- Temporary C usage (function doesn't store pointer): Safe
- Persistent C storage (function stores pointer): Must deep copy or ensure lifetime
- When in doubt, use manual allocation (malloc/free) for C-owned data

### Modules and Imports

Volta uses a file-based module system. Each `.vlt` file is a module, and the module path corresponds to the file path.

#### Visibility

By default, all definitions are private to the file. Use the `pub` keyword to export symbols:

```rust
pub fn add(a: i32, b: i32) -> i32 {
    return a + b;
}

fn helper() -> i32 {  // Private
    return 42;
}

Import Syntax

Import specific items from other modules using dot notation:

```rust
import std.io.{print, println};
import std.math.{abs, sqrt};
import std.collections.{Vec, HashMap};

fn main() -> i32 {
    print("Hello!");
    let x: i32 = abs(-42);
    return 0;
}
```

Module paths map to file paths:
- import std.io.{print} looks for std/io.vlt
- import utils.math.{add} looks for utils/math.vlt

Extern Blocks

extern blocks are file-scoped and cannot be imported. To expose C functions, wrap them in public Volta functions:

```rust
// In std/io.vlt
extern "C" {
    fn puts(s: Ptr<u8>) -> i32;  // File-scoped only
}

pub fn print(s: str) -> i32 {    // Public wrapper
    return puts(s);
}
```

Compilation

Compile multiple files together:
```bash
volta main.vlt lib.vlt -o program
```
When compiling, a main function definition is required. Multiple function definitions are not allowed. 