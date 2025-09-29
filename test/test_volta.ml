open OUnit2

(*
   This is a test suite for the Volta lexer.

   In OCaml, we use OUnit2 for testing. Here's how it works:

   1. Each test is a function that takes a test context
   2. Use assert_equal to check expected vs actual values
   3. Group tests using >:: for single tests and >::: for test lists
   4. Run tests with "dune runtest" or "dune test"
*)

(* Example test structure - uncomment when you implement your lexer *)

(*
(* Helper function to test lexing a single token *)
let test_single_token input expected_token _test_ctxt =
  let lexer = Lexer.create input in
  let token = Lexer.next_token lexer in
  assert_equal expected_token token
    ~printer:Lexer.token_to_string

(* Helper function to test tokenizing an entire string *)
let test_tokenize input expected_tokens _test_ctxt =
  let tokens = Lexer.tokenize input in
  assert_equal expected_tokens tokens
    ~printer:(fun tokens ->
      String.concat ", " (List.map Lexer.token_to_string tokens))
*)

(* Keywords tests *)
let keyword_tests = "Keyword Tests" >::: [
  (* Example test structure - uncomment when lexer is ready *)
  (* "test fn keyword" >:: test_single_token "fn" Lexer.Fn; *)
  (* "test return keyword" >:: test_single_token "return" Lexer.Return; *)
  (* "test if keyword" >:: test_single_token "if" Lexer.If; *)
  (* "test else keyword" >:: test_single_token "else" Lexer.Else; *)
  (* "test while keyword" >:: test_single_token "while" Lexer.While; *)
  (* "test for keyword" >:: test_single_token "for" Lexer.For; *)
  (* "test in keyword" >:: test_single_token "in" Lexer.In; *)
  (* "test match keyword" >:: test_single_token "match" Lexer.Match; *)
  (* "test struct keyword" >:: test_single_token "struct" Lexer.Struct; *)
  (* "test import keyword" >:: test_single_token "import" Lexer.Import; *)
  (* "test mut keyword" >:: test_single_token "mut" Lexer.Mut; *)
  (* "test true keyword" >:: test_single_token "true" Lexer.True; *)
  (* "test false keyword" >:: test_single_token "false" Lexer.False; *)
  (* "test Some keyword" >:: test_single_token "Some" Lexer.Some; *)
  (* "test None keyword" >:: test_single_token "None" Lexer.None; *)
  (* "test and keyword" >:: test_single_token "and" Lexer.And; *)
  (* "test or keyword" >:: test_single_token "or" Lexer.Or; *)
  (* "test not keyword" >:: test_single_token "not" Lexer.Not; *)
  (* "test type keyword" >:: test_single_token "type" Lexer.Type; *)
]

(* Operator tests *)
let operator_tests = "Operator Tests" >::: [
  (* Arithmetic operators *)
  (* "test plus" >:: test_single_token "+" Lexer.Plus; *)
  (* "test minus" >:: test_single_token "-" Lexer.Minus; *)
  (* "test star" >:: test_single_token "*" Lexer.Star; *)
  (* "test slash" >:: test_single_token "/" Lexer.Slash; *)
  (* "test percent" >:: test_single_token "%" Lexer.Percent; *)
  (* "test power" >:: test_single_token "**" Lexer.StarStar; *)

  (* Comparison operators *)
  (* "test equal" >:: test_single_token "==" Lexer.Equal; *)
  (* "test not equal" >:: test_single_token "!=" Lexer.NotEqual; *)
  (* "test less" >:: test_single_token "<" Lexer.Less; *)
  (* "test greater" >:: test_single_token ">" Lexer.Greater; *)
  (* "test less equal" >:: test_single_token "<=" Lexer.LessEqual; *)
  (* "test greater equal" >:: test_single_token ">=" Lexer.GreaterEqual; *)

  (* Assignment operators *)
  (* "test assign" >:: test_single_token "=" Lexer.Assign; *)
  (* "test plus assign" >:: test_single_token "+=" Lexer.PlusAssign; *)
  (* "test minus assign" >:: test_single_token "-=" Lexer.MinusAssign; *)
  (* "test star assign" >:: test_single_token "*=" Lexer.StarAssign; *)
  (* "test slash assign" >:: test_single_token "/=" Lexer.SlashAssign; *)

  (* Special operators *)
  (* "test arrow" >:: test_single_token "->" Lexer.Arrow; *)
  (* "test fat arrow" >:: test_single_token "=>" Lexer.FatArrow; *)
  (* "test dot dot" >:: test_single_token ".." Lexer.DotDot; *)
  (* "test dot dot equal" >:: test_single_token "..=" Lexer.DotDotEqual; *)
  (* "test colon assign" >:: test_single_token ":=" Lexer.ColonAssign; *)
]

(* Delimiter tests *)
let delimiter_tests = "Delimiter Tests" >::: [
  (* "test left paren" >:: test_single_token "(" Lexer.LeftParen; *)
  (* "test right paren" >:: test_single_token ")" Lexer.RightParen; *)
  (* "test left brace" >:: test_single_token "{" Lexer.LeftBrace; *)
  (* "test right brace" >:: test_single_token "}" Lexer.RightBrace; *)
  (* "test left bracket" >:: test_single_token "[" Lexer.LeftBracket; *)
  (* "test right bracket" >:: test_single_token "]" Lexer.RightBracket; *)
  (* "test comma" >:: test_single_token "," Lexer.Comma; *)
  (* "test dot" >:: test_single_token "." Lexer.Dot; *)
  (* "test colon" >:: test_single_token ":" Lexer.Colon; *)
  (* "test semicolon" >:: test_single_token ";" Lexer.Semicolon; *)
]

(* Literal tests *)
let literal_tests = "Literal Tests" >::: [
  (* Integer literals *)
  (* "test int 0" >:: test_single_token "0" (Lexer.Int 0); *)
  (* "test int 42" >:: test_single_token "42" (Lexer.Int 42); *)
  (* "test int 123" >:: test_single_token "123" (Lexer.Int 123); *)

  (* Float literals *)
  (* "test float 3.14" >:: test_single_token "3.14" (Lexer.Float 3.14); *)
  (* "test float 0.5" >:: test_single_token "0.5" (Lexer.Float 0.5); *)
  (* "test float 2.0" >:: test_single_token "2.0" (Lexer.Float 2.0); *)

  (* String literals *)
  (* "test empty string" >:: test_single_token "\"\"" (Lexer.String ""); *)
  (* "test simple string" >:: test_single_token "\"hello\"" (Lexer.String "hello"); *)
  (* "test string with spaces" >:: test_single_token "\"hello world\"" (Lexer.String "hello world"); *)
  (* "test string with escape" >:: test_single_token "\"hello\\nworld\"" (Lexer.String "hello\nworld"); *)
]

(* Identifier tests *)
let identifier_tests = "Identifier Tests" >::: [
  (* "test simple identifier" >:: test_single_token "x" (Lexer.Identifier "x"); *)
  (* "test identifier with underscore" >:: test_single_token "my_var" (Lexer.Identifier "my_var"); *)
  (* "test identifier with numbers" >:: test_single_token "var123" (Lexer.Identifier "var123"); *)
  (* "test identifier starting with underscore" >:: test_single_token "_private" (Lexer.Identifier "_private"); *)
  (* "test camelCase identifier" >:: test_single_token "camelCase" (Lexer.Identifier "camelCase"); *)
]

(* Comment tests *)
let comment_tests = "Comment Tests" >::: [
  (* "test single line comment" >:: test_single_token "# this is a comment" (Lexer.Comment " this is a comment"); *)
  (* "test empty comment" >:: test_single_token "#" (Lexer.Comment ""); *)
]

(* Integration tests - testing multiple tokens *)
let integration_tests = "Integration Tests" >::: [
  (* Example: test a simple variable declaration *)
  (* "test variable declaration" >:: test_tokenize
      "x: int = 42"
      [Lexer.Identifier "x"; Lexer.Colon; Lexer.Identifier "int";
       Lexer.Assign; Lexer.Int 42; Lexer.Eof]; *)

  (* "test function signature" >:: test_tokenize
      "fn add(a: int, b: int) -> int"
      [Lexer.Fn; Lexer.Identifier "add"; Lexer.LeftParen;
       Lexer.Identifier "a"; Lexer.Colon; Lexer.Identifier "int"; Lexer.Comma;
       Lexer.Identifier "b"; Lexer.Colon; Lexer.Identifier "int"; Lexer.RightParen;
       Lexer.Arrow; Lexer.Identifier "int"; Lexer.Eof]; *)

  (* "test array literal" >:: test_tokenize
      "[1, 2, 3]"
      [Lexer.LeftBracket; Lexer.Int 1; Lexer.Comma;
       Lexer.Int 2; Lexer.Comma; Lexer.Int 3; Lexer.RightBracket; Lexer.Eof]; *)

  (* "test if expression" >:: test_tokenize
      "if x > 0 { 1 } else { 0 }"
      [Lexer.If; Lexer.Identifier "x"; Lexer.Greater; Lexer.Int 0;
       Lexer.LeftBrace; Lexer.Int 1; Lexer.RightBrace;
       Lexer.Else; Lexer.LeftBrace; Lexer.Int 0; Lexer.RightBrace; Lexer.Eof]; *)
]

(* Whitespace handling tests *)
let whitespace_tests = "Whitespace Tests" >::: [
  (* "test spaces between tokens" >:: test_tokenize
      "1 + 2"
      [Lexer.Int 1; Lexer.Plus; Lexer.Int 2; Lexer.Eof]; *)

  (* "test tabs between tokens" >:: test_tokenize
      "1\t+\t2"
      [Lexer.Int 1; Lexer.Plus; Lexer.Int 2; Lexer.Eof]; *)

  (* "test multiple spaces" >:: test_tokenize
      "1   +   2"
      [Lexer.Int 1; Lexer.Plus; Lexer.Int 2; Lexer.Eof]; *)
]

(* Main test suite *)
let suite = "Volta Lexer Test Suite" >::: [
  keyword_tests;
  operator_tests;
  delimiter_tests;
  literal_tests;
  identifier_tests;
  comment_tests;
  integration_tests;
  whitespace_tests;
]

(* Run the tests *)
let () = run_test_tt_main suite