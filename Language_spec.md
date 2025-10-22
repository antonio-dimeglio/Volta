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
Pointers are a critical aspect of the language, as they are necessary for binding to c libraries. The language provides a unique interface for pointers, `Ptr<Type>`. Elements declared as pointers are NOT collected by GC, and users are expected to do their due diligence with them.
##### Array
Two arrays exist, one, beign a fixed size array, denoted with the type syntax `[Type; Size]` and generic, dynamically allocated homogenous array, declared with the generic syntax `Array<Type>`.
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
let mut p = Point::new(10.0, 20.0);  // Static method with ::
let d = p.distance();                // Instance method with .
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
let h = OpaqueHandle::new(42);  // OK - calls static method
let id = h.get_id();            // OK - calls instance method
// let h2 = OpaqueHandle { internal_id: 42, internal_ptr: ... };  // ERROR: private fields
```

Note: Generic structs will be supported in a future version.
##### Functions 
Functions are defined with syntax `fn fn_name(arg1: type, arg2: type, varargs) -> Type {}`. Arguments can either be passed by value or by reference. eg `arg1: type` is by value `arg2: ref type` is by reference and `arg3: const ref type` is a constant reference. of course passing references to values that are immutable will give an error.
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
Enums are fully algebraic data types, like Rust, so as an example:
```{rust}
enum Expr {
    Number(i32),
    Add(Expr, Expr),
    Multiply(Expr, Expr),
    Negate(Expr),
}

impl Expr {
    fn eval(self) -> i32 {
        match self {
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
    fn strlen(s: Ptr<u8>) -> usize
    fn malloc(size: usize) -> Ptr<opaque>
}
```
Note the usage of the opaque keyword, which is just a fancy way of declaring a void pointer, for which the size is system specific.
char* is a Ptr<u8>, while numeric types can be directly translated with the supported Volta types. Conversion from utf8 string to C strings can be done by performing `my_string.to_c_str()` calls. The language also supports the use of the `addrof` keyword to obtain the address of a variable, in order to pass it to C definitiions.

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