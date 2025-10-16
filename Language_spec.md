The following is a document outlining the language specs of Volta.
Volta is a Garbage Collected imperative language with scientific computing in mind, which allows user to easily bind to C code by providing ergonomic infrastructure to do so.
Each .rev executable must have a main function. The smallest Volta language is the following
```{rev}
fn main() -> i32 {
    return 0;
}
```
An example of the classic Hello world program is the following:
```{rev}
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
Structs in Volta are basically a copy and paste of Structs type in rust, how convenient! They also have methods defined in impl blocks and traits for shared behaviour. They also rely on the `self` keyword, though all structures are mutable, unless the self is preceeded by the `const` keyword. Structs (and their implementatio) can be generic. Examples of structs are the following
```{rust}
pub struct Point {
    x: f32,
    y: f32
}

impl Point {
    pub fn new(x: f32, y: f32) -> Point {
        return Point {
            x: x,
            y: y
        }
    }

    pub fn get_x(self) -> f32 {
        return self.x
    }
}

pub struct Array<T> {
    data: Ptr<T>,
    length: u32,
    capacity: u32
}

impl<T> Array<T> {
    pub fn new() -> Array {
        Array {    
            data: malloc(sizeof(T) * 8),
            length: 0,
            capacity: 8
        }
    }
}
```
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
char* is a Ptr<u8>, while numeric types can be directly translated with the supported Volta types. Conversion from utf8 string to C strings can be done by performing `my_string.to_c_str()` calls.