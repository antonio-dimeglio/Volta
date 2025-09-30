open Token_type

type lexer = { 
    program: string;
    mutable idx: int; 
    mutable line: int;
    mutable column: int;
  }

type token = {
  ttype: token_type;
  lexeme: string; 
  line: int;
  column: int;
}

let create program =
  { program; idx = 0; line = 1; column = 1; }

let is_at_end lexer =
    lexer.idx >= String.length lexer.program
  
let peek lexer = 
  if lexer.idx + 1 < String.length lexer.program then 
    Some lexer.program.[lexer.idx + 1]
  else 
    None 

let current_char lexer =
  if is_at_end lexer then None
  else Some lexer.program.[lexer.idx]

let advance lexer =
  if not (is_at_end lexer) then (
    if lexer.program.[lexer.idx] = '\n' then (
      lexer.line <- lexer.line + 1;
      lexer.column <- 1
    ) else
      lexer.column <- lexer.column + 1;
    lexer.idx <- lexer.idx + 1
  )

let rec skip_whitespace lexer =
  if not (is_at_end lexer) &&
    (lexer.program.[lexer.idx] = ' ' ||
    lexer.program.[lexer.idx] = '\t' ||
    lexer.program.[lexer.idx] = '\r' ||
    lexer.program.[lexer.idx] = '\n') then (

    advance lexer;
    skip_whitespace lexer
  ) else
    ()

let is_digit c = c >= '0' && c <= '9'

let scan_number lexer =
  let start = lexer.idx in
  let dot = ref false in
  while (match current_char lexer with
    | Some c when is_digit c -> true
    | Some '.' when not !dot -> (dot := true; true)
    | _ -> false) do
    advance lexer
  done;
  let lexeme =
    String.sub lexer.program start (lexer.idx - start)
  in
  {
    ttype = (if !dot then Real else Integer);
    lexeme;
    line = lexer.line;
    column = lexer.column;
  }
  
(* inclusive range requires lookahead of 2 as it is ..=, but this doesnt allow it
  alas, for now this impl is okay. *)
let scan_symbol lexer =
  match current_char lexer, peek lexer with
  | Some '+', Some '=' ->
      advance lexer;  
      advance lexer;  
      { ttype = PlusAssign;
        lexeme = "+=";
        line = lexer.line;
        column = lexer.column }
  | Some '+', _ ->
      advance lexer;
      { ttype = Plus;
        lexeme = "+";
        line = lexer.line;
        column = lexer.column }
  | Some '-', Some '=' ->
    advance lexer;
    advance lexer; 
    { ttype = MinusAssign;
      lexeme = "-=";
      line = lexer.line;
      column = lexer.column }
  | Some '-', _ ->
    advance lexer; 
    { ttype = Minus;
      lexeme = "-";
      line = lexer.line;
      column = lexer.column }
  | Some '*', Some '=' ->
      advance lexer;  
      advance lexer;  
      { ttype = MultAssign;
        lexeme = "*=";
        line = lexer.line;
        column = lexer.column }
  | Some '*', Some '*' ->
      advance lexer;  
      advance lexer;  
      { ttype = Power;
        lexeme = "**";
        line = lexer.line;
        column = lexer.column }
  | Some '*', _ ->
      advance lexer;
      { ttype = Mult;
        lexeme = "*";
        line = lexer.line;
        column = lexer.column }
  | Some '/', Some '=' ->
    advance lexer;
    advance lexer; 
    { ttype = DivAssign;
      lexeme = "/=";
      line = lexer.line;
      column = lexer.column }
  | Some '/', _ ->
    advance lexer; 
    { ttype = Div;
      lexeme = "/";
      line = lexer.line;
      column = lexer.column }
  | Some '%', _ ->
    advance lexer; 
    { ttype = Modulo;
      lexeme = "%";
      line = lexer.line;
      column = lexer.column }
  | Some '=', Some '=' ->
      advance lexer;  
      advance lexer;  
      { ttype = Equals;
        lexeme = "==";
        line = lexer.line;
        column = lexer.column }
  | Some '=', _ ->
      advance lexer;
      { ttype = Assign;
        lexeme = "=";
        line = lexer.line;
        column = lexer.column }
  | Some '!', Some '=' ->
      advance lexer;  
      advance lexer;  
      { ttype = NotEquals;
        lexeme = "!=";
        line = lexer.line;
        column = lexer.column }
  | Some '!', _ ->
      advance lexer;
      { ttype = Not;
        lexeme = "!";
        line = lexer.line;
        column = lexer.column }
  | Some '>', Some '=' ->
      advance lexer;  
      advance lexer;  
      { ttype = GEQ;
        lexeme = ">=";
        line = lexer.line;
        column = lexer.column }
  | Some '>', _ ->
      advance lexer;
      { ttype = GT;
        lexeme = ">";
        line = lexer.line;
        column = lexer.column }
  | Some '<', Some '=' ->
    advance lexer;
    advance lexer; 
    { ttype = LEQ;
      lexeme = "<=";
      line = lexer.line;
      column = lexer.column }
  | Some '<', _ ->
    advance lexer;
    { ttype = LT;
      lexeme = "<";
      line = lexer.line;
      column = lexer.column }
  | Some ':', Some '=' ->
    advance lexer;
    advance lexer;
    { ttype = InferAssign;
      lexeme = ":=";
      line = lexer.line;
      column = lexer.column }
  | Some ':', _ ->
    advance lexer;
    { ttype = Colon;
      lexeme = ":";
      line = lexer.line;
      column = lexer.column }
  | Some '(', _ ->
    advance lexer;
    { ttype = LParen; lexeme = "("; line = lexer.line; column = lexer.column }
  | Some ')', _ ->
    advance lexer;
    { ttype = RParen; lexeme = ")"; line = lexer.line; column = lexer.column }
  | Some '[', _ ->
    advance lexer;
    { ttype = LSquare; lexeme = "["; line = lexer.line; column = lexer.column }
  | Some ']', _ ->
    advance lexer;
    { ttype = RSquare; lexeme = "]"; line = lexer.line; column = lexer.column }
  | Some '{', _ ->
    advance lexer;
    { ttype = LBrace; lexeme = "{"; line = lexer.line; column = lexer.column }
  | Some '}', _ ->
    advance lexer;
    { ttype = RBrace; lexeme = "}"; line = lexer.line; column = lexer.column }
  | _, _ ->
    failwith ("Unexpected character: " ^
      match current_char lexer with
      | Some c -> String.make 1 c
      | None -> "EOF")

let scan_string lexer =
  advance lexer; (* skip opening quote *)
  let buffer = Buffer.create 16 in
  let rec loop () =
    match current_char lexer with
    | Some '"' ->
        advance lexer; (* skip closing quote *)
        Buffer.contents buffer
    | Some '\\' ->
        advance lexer;
        (match current_char lexer with
        | Some 'n' -> Buffer.add_char buffer '\n'; advance lexer; loop ()
        | Some 't' -> Buffer.add_char buffer '\t'; advance lexer; loop ()
        | Some 'r' -> Buffer.add_char buffer '\r'; advance lexer; loop ()
        | Some '\\' -> Buffer.add_char buffer '\\'; advance lexer; loop ()
        | Some '"' -> Buffer.add_char buffer '"'; advance lexer; loop ()
        | Some c -> Buffer.add_char buffer c; advance lexer; loop ()
        | None -> failwith "Unterminated string: unexpected end of file")
    | Some c ->
        Buffer.add_char buffer c;
        advance lexer;
        loop ()
    | None -> failwith "Unterminated string: unexpected end of file"
  in
  let content = loop () in
  {
    ttype = StringLiteral;
    lexeme = content;
    line = lexer.line;
    column = lexer.column;
  }

let is_alpha c = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c = '_'

let is_alnum c = is_alpha c || is_digit c

let scan_identifier_or_keyword lexer =
  let start = lexer.idx in
  (* Advance through all alphanumeric characters *)
  while (match current_char lexer with
    | Some c when is_alnum c -> true
    | _ -> false) do
    advance lexer
  done;
  let lexeme = String.sub lexer.program start (lexer.idx - start) in
  (* Check if it's a keyword using the map from Token_type *)
  let ttype =
    try Token_type.StringMap.find lexeme Token_type.keywords
    with Not_found -> Identifier
  in
  {
    ttype;
    lexeme;
    line = lexer.line;
    column = lexer.column;
  }

let scan_token lexer =
  skip_whitespace lexer;
  match current_char lexer with
    | Some ('0'..'9') -> scan_number lexer
    | Some '.' -> scan_number lexer  (* can be either a number or the . operator*)
    | Some '"' -> scan_string lexer
    | Some ('a'..'z' | 'A'..'Z' | '_') -> scan_identifier_or_keyword lexer
    | Some _ -> scan_symbol lexer
    | None -> { ttype = Eof; lexeme = ""; line = lexer.line; column = lexer.column }


let tokenize lexer = 
  let tokens = ref [] in 
  while not (is_at_end lexer) do
    let token = scan_token lexer in
    tokens := token :: !tokens 
  done; 
  List.rev !tokens 