# Grammar

Volta's syntax grammar reference.

## Variables

```ebnf
variable_declaration :=
    "let" identifier ":" type "=" expression
  | "let" "mut" identifier ":" type "=" expression
  | "let" identifier ":=" expression
```

Examples:
```volta
let x: i32 = 42
let mut counter: i32 = 0
let y := 100
```

## Functions

```ebnf
function_declaration :=
    "fn" identifier "(" parameters ")" "->" type "{" statements "}"
  | "fn" identifier "(" parameters ")" "->" type "=" expression

parameters := 
    parameter ("," parameter)*
  | ε

parameter := identifier ":" type
```

Examples:
```volta
fn add(a: i32, b: i32) -> i32 {
    return a + b
}

fn square(x: i32) -> i32 = x * x
```

## Expressions

```ebnf
expression :=
    literal
  | identifier
  | binary_expression
  | unary_expression
  | function_call
  | if_expression
  | match_expression
  | array_literal
  | tuple_literal
  | struct_literal

binary_expression := expression operator expression

unary_expression := unary_operator expression
```

Examples:
```volta
42
x + y
-value
foo(1, 2)
if x > 0 { 1 } else { 0 }
[1, 2, 3]
(x, y)
```

## Control Flow

### If Statements

```ebnf
if_statement :=
    "if" expression "{" statements "}"
  | "if" expression "{" statements "}" "else" "{" statements "}"
  | "if" expression "{" statements "}" "else" if_statement
```

### While Loops

```ebnf
while_statement := "while" expression "{" statements "}"
```

### For Loops

```ebnf
for_statement := "for" identifier "in" expression "{" statements "}"
```

### Match Expressions

```ebnf
match_expression := "match" expression "{" match_arms "}"

match_arms := match_arm ("," match_arm)* ","?

match_arm := pattern ("if" expression)? "=>" expression
```

## Types

```ebnf
type :=
    "i8" | "i16" | "i32" | "i64"
  | "u8" | "u16" | "u32" | "u64"
  | "f16" | "f32" | "f64" | "f8"
  | "bool"
  | "str"
  | "Array" "[" type "]"
  | "Option" "[" type "]"
  | tuple_type
  | identifier

tuple_type := "(" type ("," type)* ")"
```

Examples:
```volta
i32
f64
Array[i32]
Option[str]
(i32, i32)
Point
```

## Structs

```ebnf
struct_declaration := "struct" identifier "{" fields "}"

fields := field ("," field)* ","?

field := identifier ":" type
```

Example:
```volta
struct Point {
    x: f64,
    y: f64
}
```

## Operators

### Arithmetic
```
+   addition
-   subtraction
*   multiplication
/   division
%   modulo
**  exponentiation
```

### Comparison
```
==  equal
!=  not equal
<   less than
>   greater than
<=  less than or equal
>=  greater than or equal
```

### Logical
```
and  logical AND
or   logical OR
not  logical NOT
```

### Assignment
```
=   assignment
+=  add and assign
-=  subtract and assign
*=  multiply and assign
/=  divide and assign
```

## Literals

```ebnf
literal :=
    integer_literal
  | float_literal
  | string_literal
  | boolean_literal
  | array_literal
  | tuple_literal

integer_literal := digit+

float_literal := digit+ "." digit+

string_literal := '"' character* '"'

boolean_literal := "true" | "false"

array_literal := "[" (expression ("," expression)*)? "]"

tuple_literal := "(" expression ("," expression)+ ")"
```

## Comments

```ebnf
single_line_comment := "#" any_character* newline

multi_line_comment := "#[" any_character* "]#"
```

## Patterns

```ebnf
pattern :=
    literal_pattern
  | identifier_pattern
  | wildcard_pattern
  | tuple_pattern
  | struct_pattern

literal_pattern := literal

identifier_pattern := identifier

wildcard_pattern := "_"

tuple_pattern := "(" pattern ("," pattern)* ")"

struct_pattern := identifier "{" field_patterns "}"
```

## Operator Precedence

From highest to lowest:

1. `**` (exponentiation)
2. unary `-`, `not`
3. `*`, `/`, `%`
4. `+`, `-`
5. `<`, `>`, `<=`, `>=`
6. `==`, `!=`
7. `and`
8. `or`
9. `=`, `+=`, `-=`, `*=`, `/=`

## Whitespace

- Whitespace (spaces, tabs, newlines) is generally insignificant
- Used to separate tokens
- Indentation is not significant (unlike Python)

## Semicolons

- Statements do not require semicolons
- Each statement on its own line
- Multiple statements per line not recommended

## Identifiers

```ebnf
identifier := (letter | "_") (letter | digit | "_")*
```

Rules:
- Start with letter or underscore
- Contain letters, digits, underscores
- Cannot be keywords
- Case-sensitive

Examples:
```volta
x
counter
my_variable
Array2D
_private
```
