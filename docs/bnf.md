# Volta Language Grammar (BNF)

**Version:** 0.1.0

This document defines the syntax of the Volta programming language in Backus-Naur Form (BNF).

## Notation Conventions

- `<non-terminal>` - Non-terminal symbols (grammatical constructs)
- `"terminal"` - Terminal symbols (literal tokens)
- `|` - Alternation (or)
- `*` - Zero or more repetitions
- `+` - One or more repetitions
- `?` - Optional (zero or one)
- `()` - Grouping
- `ε` - Empty/epsilon (no tokens)

## Program Structure

```bnf
<program> ::= <statement>*

<statement> ::= <declaration>
              | <expression-stmt>
              | <if-stmt>
              | <while-stmt>
              | <for-stmt>
              | <return-stmt>
              | <import-stmt>
              | <block>

<block> ::= "{" <statement>* "}"
```

## Declarations

```bnf
<declaration> ::= <var-decl>
                | <fn-decl>
                | <struct-decl>

<var-decl> ::= <identifier> ":" <type-annotation> "=" <expression>
             | <identifier> ":=" <expression>

<type-annotation> ::= "mut"? <type>

<fn-decl> ::= "fn" <identifier> <type-params>? "(" <param-list>? ")" "->" <type> <fn-body>

<fn-body> ::= <block>
            | "=" <expression>

<param-list> ::= <param> ("," <param>)*

<param> ::= <identifier> ":" <type>

<type-params> ::= "[" <identifier> ("," <identifier>)* "]"

<struct-decl> ::= "struct" <identifier> "{" <struct-fields>? "}"

<struct-fields> ::= <struct-field> ("," <struct-field>)* ","?

<struct-field> ::= <identifier> ":" <type>
```

## Types

```bnf
<type> ::= <primitive-type>
         | <compound-type>
         | <generic-type>
         | <function-type>
         | <identifier>

<primitive-type> ::= "int"
                   | "float"
                   | "bool"
                   | "str"

<compound-type> ::= "Array" "[" <type> "]"
                  | "Matrix" "[" <type> "]"
                  | "Option" "[" <type> "]"
                  | "(" <type> ("," <type>)* ")"

<generic-type> ::= <identifier> "[" <type> ("," <type>)* "]"

<function-type> ::= "fn" "(" <type-list>? ")" "->" <type>

<type-list> ::= <type> ("," <type>)*
```

## Statements

```bnf
<expression-stmt> ::= <expression>

<if-stmt> ::= "if" <expression> <block> ("else" "if" <expression> <block>)* ("else" <block>)?

<while-stmt> ::= "while" <expression> <block>

<for-stmt> ::= "for" <identifier> "in" <expression> <block>

<return-stmt> ::= "return" <expression>?

<import-stmt> ::= "import" <identifier>
```

## Expressions

```bnf
<expression> ::= <assignment>

<assignment> ::= <identifier> <assign-op> <assignment>
               | <logical-or>

<assign-op> ::= "=" | "+=" | "-=" | "*=" | "/="

<logical-or> ::= <logical-and> ("or" <logical-and>)*

<logical-and> ::= <equality> ("and" <equality>)*

<equality> ::= <comparison> (("==" | "!=") <comparison>)*

<comparison> ::= <range> (("<" | ">" | "<=" | ">=") <range>)*

<range> ::= <term> ((".." | "..=") <term>)?

<term> ::= <factor> (("+" | "-") <factor>)*

<factor> ::= <unary> (("*" | "/" | "%") <unary>)*

<unary> ::= ("not" | "-") <unary>
          | <power>

<power> ::= <call> ("**" <unary>)*

<call> ::= <primary> <call-suffix>*

<call-suffix> ::= "(" <argument-list>? ")"
                | "[" <expression> "]"
                | "[" <expression>? ":" <expression>? "]"
                | "." <identifier>
                | "." <identifier> "(" <argument-list>? ")"

<argument-list> ::= <expression> ("," <expression>)*

<primary> ::= <literal>
            | <identifier>
            | <lambda>
            | <if-expression>
            | <match-expression>
            | <array-literal>
            | <tuple-literal>
            | <struct-literal>
            | "(" <expression> ")"

<if-expression> ::= "if" <expression> "{" <expression> "}" "else" "{" <expression> "}"

<match-expression> ::= "match" <expression> "{" <match-arms> "}"

<match-arms> ::= <match-arm> ("," <match-arm>)* ","?

<match-arm> ::= <pattern> ("if" <expression>)? "=>" <expression>

<pattern> ::= <literal-pattern>
            | <identifier-pattern>
            | <tuple-pattern>
            | <constructor-pattern>
            | "_"

<literal-pattern> ::= <number> | <string> | <boolean>

<identifier-pattern> ::= <identifier>

<tuple-pattern> ::= "(" <pattern> ("," <pattern>)* ")"

<constructor-pattern> ::= <identifier> ("(" <pattern> ("," <pattern>)* ")")?

<lambda> ::= "fn" "(" <param-list>? ")" "->" <type> "=" <expression>
           | "fn" "(" <param-list>? ")" "->" <type> <block>
```

## Literals

```bnf
<literal> ::= <number>
            | <string>
            | <boolean>
            | <none>
            | <some>

<number> ::= <integer>
           | <float>

<integer> ::= <digit>+

<float> ::= <digit>+ "." <digit>+
          | <digit>+ ("e" | "E") ("+" | "-")? <digit>+
          | <digit>+ "." <digit>+ ("e" | "E") ("+" | "-")? <digit>+

<string> ::= '"' <string-char>* '"'

<string-char> ::= <any-char-except-quote-or-backslash>
                | "\" <escape-sequence>

<escape-sequence> ::= "n" | "t" | "r" | '"' | "\" | "0"

<boolean> ::= "true" | "false"

<none> ::= "None"

<some> ::= "Some" "(" <expression> ")"

<array-literal> ::= "[" <expression-list>? "]"

<expression-list> ::= <expression> ("," <expression>)* ","?

<tuple-literal> ::= "(" <expression> "," <expression> ("," <expression>)* ")"

<struct-literal> ::= <identifier> "{" <field-init-list>? "}"

<field-init-list> ::= <field-init> ("," <field-init>)* ","?

<field-init> ::= <identifier> ":" <expression>
```

## Lexical Elements

```bnf
<identifier> ::= <letter> (<letter> | <digit> | "_")*

<letter> ::= "a" | "b" | ... | "z" | "A" | "B" | ... | "Z"

<digit> ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"

<comment> ::= "#" <any-char-until-newline>*
            | "#[" <any-char>* "]#"

<whitespace> ::= " " | "\t" | "\n" | "\r"
```

## Keywords (Reserved)

```
fn, return, if, else, while, for, in, match, struct, import,
mut, true, false, Some, None, and, or, not,
int, float, bool, str, Array, Matrix, Option
```

## Operators and Delimiters

```
+  -  *  /  %  **
==  !=  <  >  <=  >=
=  +=  -=  *=  /=
(  )  {  }  [  ]
,  .  :  ;  =>  ->
..  ..=
```

## Precedence Table (Highest to Lowest)

1. Primary (literals, identifiers, grouping)
2. Call, subscript, member access
3. Unary (`not`, `-`)
4. Power (`**`)
5. Multiplicative (`*`, `/`, `%`)
6. Additive (`+`, `-`)
7. Range (`..`, `..=`)
8. Comparison (`<`, `>`, `<=`, `>=`)
9. Equality (`==`, `!=`)
10. Logical AND (`and`)
11. Logical OR (`or`)
12. Assignment (`=`, `+=`, etc.)

## Associativity

- Left-associative: `+`, `-`, `*`, `/`, `%`, `and`, `or`, `==`, `!=`, `<`, `>`, `<=`, `>=`
- Right-associative: `**`, `=`, `+=`, `-=`, `*=`, `/=`
- Non-associative: `..`, `..=`

## Notes

1. **Newlines**: Treated as whitespace; statements don't require semicolons
2. **Type inference**: The `:=` operator allows omitting type annotations in variable declarations
3. **Expression-oriented**: Many constructs (`if`, `match`) can be used as expressions
4. **Method call syntax**: `.method()` is syntactic sugar for function calls
5. **Trailing commas**: Allowed in lists for better diff-friendliness

---

This grammar provides a complete syntactic specification for Volta and can be used as a reference when implementing the parser.