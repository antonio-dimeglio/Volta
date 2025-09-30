
open Volta.Lexer
module T = Volta.Token_type 
module Lexer = Volta.Lexer
open OUnit2

(* Helper function to create a token for testing *)
let make_token ttype lexeme line column =
  { Lexer.ttype; lexeme; line; column }

(* Helper function to test lexing a single token *)
let test_single_token input expected_token _test_ctxt =
  let lexer = Lexer.create input in
  let token = Lexer.scan_token lexer in
  assert_equal expected_token token
    ~printer:(fun t -> Printf.sprintf "{ ttype = %s; lexeme = \"%s\"; line = %d; column = %d }"
      (T.token_to_string t.ttype) t.lexeme t.line t.column)

(* Helper function to test tokenizing an entire string *)
let test_tokenize input expected_tokens _test_ctxt =
  let lexer = Lexer.create input in
  let tokens = Lexer.tokenize lexer in
  assert_equal expected_tokens tokens
    ~printer:(fun tokens ->
      String.concat "; " (List.map (fun t -> T.token_to_string t.ttype) tokens))

(* Keywords tests *)
let keyword_tests = "Keyword Tests" >::: [
  "test fn keyword" >:: test_single_token "fn" (make_token T.Function "fn" 1 1);
  "test return keyword" >:: test_single_token "return" (make_token T.Return "return" 1 1);
  "test if keyword" >:: test_single_token "if" (make_token T.If "if" 1 1);
  "test else keyword" >:: test_single_token "else" (make_token T.Else "else" 1 1);
  "test while keyword" >:: test_single_token "while" (make_token T.While "while" 1 1);
  "test for keyword" >:: test_single_token "for" (make_token T.For "for" 1 1);
  "test in keyword" >:: test_single_token "in" (make_token T.In "in" 1 1);
  "test match keyword" >:: test_single_token "match" (make_token T.Match "match" 1 1);
  "test struct keyword" >:: test_single_token "struct" (make_token T.Struct "struct" 1 1);
  "test mut keyword" >:: test_single_token "mut" (make_token T.Mut "mut" 1 1);
  "test some keyword" >:: test_single_token "some" (make_token T.SomeKeyword "some" 1 1);
  "test none keyword" >:: test_single_token "none" (make_token T.NoneKeyword "none" 1 1);
]

(* Operator tests *)
let operator_tests = "Operator Tests" >::: [
  (* Arithmetic operators *)
  "test plus" >:: test_single_token "+" (make_token Plus "+" 1 1);
  "test minus" >:: test_single_token "-" (make_token Minus "-" 1 1);
  "test mult" >:: test_single_token "*" (make_token Mult "*" 1 1);
  "test div" >:: test_single_token "/" (make_token Div "/" 1 1);
  "test modulo" >:: test_single_token "%" (make_token Modulo "%" 1 1);
  "test power" >:: test_single_token "**" (make_token Power "**" 1 1);

  (* Comparison operators *)
  "test equals" >:: test_single_token "==" (make_token Equals "==" 1 1);
  "test not equals" >:: test_single_token "!=" (make_token NotEquals "!=" 1 1);
  "test less than" >:: test_single_token "<" (make_token LT "<" 1 1);
  "test greater than" >:: test_single_token ">" (make_token GT ">" 1 1);
  "test less equal" >:: test_single_token "<=" (make_token LEQ "<=" 1 1);
  "test greater equal" >:: test_single_token ">=" (make_token GEQ ">=" 1 1);

  (* Assignment operators *)
  "test assign" >:: test_single_token "=" (make_token Assign "=" 1 1);
  "test plus assign" >:: test_single_token "+=" (make_token PlusAssign "+=" 1 1);
  "test minus assign" >:: test_single_token "-=" (make_token MinusAssign "-=" 1 1);
  "test mult assign" >:: test_single_token "*=" (make_token MultAssign "*=" 1 1);
  "test div assign" >:: test_single_token "/=" (make_token DivAssign "/=" 1 1);
  "test infer assign" >:: test_single_token ":=" (make_token InferAssign ":=" 1 1);
]

(* Delimiter tests *)
let delimiter_tests = "Delimiter Tests" >::: [
  "test left paren" >:: test_single_token "(" (make_token LParen "(" 1 1);
  "test right paren" >:: test_single_token ")" (make_token RParen ")" 1 1);
  "test left brace" >:: test_single_token "{" (make_token LBrace "{" 1 1);
  "test right brace" >:: test_single_token "}" (make_token RBrace "}" 1 1);
  "test left bracket" >:: test_single_token "[" (make_token LSquare "[" 1 1);
  "test right bracket" >:: test_single_token "]" (make_token RSquare "]" 1 1);
  "test colon" >:: test_single_token ":" (make_token Colon ":" 1 1);
]

(* Literal tests *)
let literal_tests = "Literal Tests" >::: [
  (* Integer literals *)
  "test int 0" >:: test_single_token "0" (make_token Integer "0" 1 1);
  "test int 42" >:: test_single_token "42" (make_token Integer "42" 1 1);
  "test int 123" >:: test_single_token "123" (make_token Integer "123" 1 1);

  (* Float literals *)
  "test float 3.14" >:: test_single_token "3.14" (make_token Real "3.14" 1 1);
  "test float 0.5" >:: test_single_token "0.5" (make_token Real "0.5" 1 1);
  "test float 2.0" >:: test_single_token "2.0" (make_token Real "2.0" 1 1);

  (* String literals *)
  "test empty string" >:: test_single_token "\"\"" (make_token StringLiteral "" 1 1);
  "test simple string" >:: test_single_token "\"hello\"" (make_token StringLiteral "hello" 1 1);
  "test string with spaces" >:: test_single_token "\"hello world\"" (make_token StringLiteral "hello world" 1 1);
  "test string with escape" >:: test_single_token "\"hello\\nworld\"" (make_token StringLiteral "hello\nworld" 1 1);
]

(* Identifier tests *)
let identifier_tests = "Identifier Tests" >::: [
  "test simple identifier" >:: test_single_token "x" (make_token Identifier "x" 1 1);
  "test identifier with underscore" >:: test_single_token "my_var" (make_token Identifier "my_var" 1 1);
  "test identifier with numbers" >:: test_single_token "var123" (make_token Identifier "var123" 1 1);
  "test identifier starting with underscore" >:: test_single_token "_private" (make_token Identifier "_private" 1 1);
  "test camelCase identifier" >:: test_single_token "camelCase" (make_token Identifier "camelCase" 1 1);
]

(* Integration tests - testing multiple tokens *)
let integration_tests = "Integration Tests" >::: [
  "test simple expression" >:: test_tokenize "1 + 2" [
    make_token Integer "1" 1 1;
    make_token Plus "+" 1 3;
    make_token Integer "2" 1 5;
    make_token Eof "" 1 6
  ];

  "test variable assignment" >:: test_tokenize "x = 42" [
    make_token Identifier "x" 1 1;
    make_token Assign "=" 1 3;
    make_token Integer "42" 1 5;
    make_token Eof "" 1 7
  ];

  "test function call" >:: test_tokenize "add(1, 2)" [
    make_token Identifier "add" 1 1;
    make_token LParen "(" 1 4;
    make_token Integer "1" 1 5;
    make_token Comma "," 1 6;
    make_token Integer "2" 1 8;
    make_token RParen ")" 1 9;
    make_token Eof "" 1 10
  ];
]

(* Whitespace handling tests *)
let whitespace_tests = "Whitespace Tests" >::: [
  "test spaces between tokens" >:: test_tokenize "1 + 2" [
    make_token Integer "1" 1 1;
    make_token Plus "+" 1 3;
    make_token Integer "2" 1 5;
    make_token Eof "" 1 6
  ];

  "test multiple spaces" >:: test_tokenize "1   +   2" [
    make_token Integer "1" 1 1;
    make_token Plus "+" 1 5;
    make_token Integer "2" 1 9;
    make_token Eof "" 1 10
  ];
]

(* Main test suite *)
let suite = "Volta Lexer Test Suite" >::: [
  keyword_tests;
  operator_tests;
  delimiter_tests;
  literal_tests;
  identifier_tests;
  integration_tests;
  whitespace_tests;
]

(* Run the tests *)
let () = run_test_tt_main suite 