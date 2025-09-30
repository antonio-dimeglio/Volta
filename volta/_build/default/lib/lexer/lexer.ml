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

let is_at_end (lexer: lexer)=
    lexer.idx >= String.length lexer.program
  
let peek (lexer: lexer) = 
  if lexer.idx + 1 < String.length lexer.program then 
    Some lexer.program.[lexer.idx + 1]
  else 
    None 

let current_char (lexer: lexer) =
  if is_at_end lexer then None
  else Some lexer.program.[lexer.idx]

let advance (lexer: lexer) =
  if is_at_end lexer then 
    None
  else (
    let c = lexer.program.[lexer.idx] in
    lexer.idx <- lexer.idx + 1;
    if c = '\n' then (
      lexer.line <- lexer.line + 1;
      lexer.column <- 1
    ) else
      lexer.column <- lexer.column + 1;
    Some c
  )

let is_digit c = c >= '0' && c <= '9'
let is_alpha c = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c = '_'
let is_alphanumeric c = is_digit c || is_alpha c

let scan_number (lexer: lexer) =
  let start_idx = lexer.idx in
  let start_line = lexer.line in
  let start_col = lexer.column in
  
  let rec scan_digits has_dot has_exp =
  match current_char lexer with
  | Some c when is_digit c -> 
      ignore (advance lexer);
      scan_digits has_dot has_exp
  | Some '.' when not has_dot && not has_exp ->
      (match peek lexer with
       | Some c when is_digit c ->
           ignore (advance lexer);
           scan_digits true has_exp
       | _ -> (has_dot, has_exp))
  | Some ('e' | 'E') when not has_exp ->
      ignore (advance lexer);
      (match current_char lexer with
       | Some ('+' | '-') -> ignore (advance lexer)
       | _ -> ());
      (* Verify at least one digit follows *)
      (match current_char lexer with
       | Some c when is_digit c -> scan_digits has_dot true
       | _ -> failwith "Invalid number: expected digit after 'e'")
  | _ -> (has_dot, has_exp)
  in
  
  let (has_dot, has_exp) = scan_digits false false in
  let lexeme = String.sub lexer.program start_idx (lexer.idx - start_idx) in
  let ttype = 
    if has_dot || has_exp then Real 
    else Integer 
  in
  {
    ttype;
    lexeme;
    line = start_line;
    column = start_col;
  }

let scan_symbol (lexer:lexer) = 
  let start_line = lexer.line in 
  let start_col = lexer.column in 
  let c1 = match advance lexer with Some c -> c | None -> failwith "EOF" in 

  let two_char = String.make 1 c1 ^ 
    (match current_char lexer with Some c -> String.make 1 c | None -> "") in

  match Hashtbl.find_opt Token_type.two_char_ops two_char with
  | Some ttype -> 
      ignore (advance lexer);
      { ttype; lexeme = two_char; line = start_line; column = start_col }
  | None ->
      match Hashtbl.find_opt Token_type.one_char_ops c1 with
      | Some ttype -> 
          { ttype; lexeme = String.make 1 c1; line = start_line; column = start_col }
      | None -> failwith ("Unexpected character: " ^ String.make 1 c1)

let scan_string (lexer:lexer) =
  let start_line = lexer.line in 
  let start_col = lexer.column in 

  ignore (advance lexer); (* consume the token*)
  let buffer = Buffer.create 16 in
  let rec scan_chars () =
    match current_char lexer with
    | Some '"' -> 
        ignore (advance lexer); 
        Buffer.contents buffer
    | Some '\\' ->
        ignore (advance lexer);
        (match current_char lexer with
         | Some 'n' -> Buffer.add_char buffer '\n'; ignore (advance lexer); scan_chars ()
         | Some 't' -> Buffer.add_char buffer '\t'; ignore (advance lexer); scan_chars ()
         | Some 'r' -> Buffer.add_char buffer '\r'; ignore (advance lexer); scan_chars ()
         | Some '\\' -> Buffer.add_char buffer '\\'; ignore (advance lexer); scan_chars ()
         | Some '"' -> Buffer.add_char buffer '"'; ignore (advance lexer); scan_chars ()
         | Some c -> Buffer.add_char buffer c; ignore (advance lexer); scan_chars ()
         | None -> failwith "Unterminated string")
    | Some c ->
        Buffer.add_char buffer c;
        ignore (advance lexer);
        scan_chars ()
    | None -> failwith "Unterminated string"
  in

  let content = scan_chars () in 
  { ttype = StringLiteral; lexeme = content; line = start_line; column = start_col}

let scan_literal_or_keyword (lexer: lexer) = 
  let start_line = lexer.line in 
  let start_col = lexer.column in 
  let buffer = Buffer.create 16 in
  let rec scan_lit_or_kw () = 
    match current_char lexer with 
    | Some c when is_alphanumeric c ->
      Buffer.add_char buffer c;
      ignore (advance lexer);
      scan_lit_or_kw ()
    | Some _ -> 
      Buffer.contents buffer 
    | None -> 
      Buffer.contents buffer
  in
  
  let lexeme = scan_lit_or_kw () in
  let ttype = 
    try Token_type.StringMap.find lexeme Token_type.keywords
    with Not_found -> Identifier
  in
  { ttype; lexeme; line = start_line; column = start_col }

let rec skip_comment (lexer: lexer) = 
  match current_char lexer with 
  | Some '\n' -> ()
  | Some _ -> 
    ignore (advance lexer);
    skip_comment lexer 
  | None -> ()


let rec scan_token lexer =
  if is_at_end lexer then
    { ttype = Eof; lexeme = ""; line = lexer.line; column = lexer.column }
  else
    match current_char lexer with
    | Some ' ' | Some '\t' | Some '\r' | Some '\n' ->
        ignore (advance lexer);
        scan_token lexer
    | Some '#' ->
        skip_comment lexer;
        scan_token lexer
    | Some '"' -> scan_string lexer
    | Some '.' ->
        (match peek lexer with
         | Some c when is_digit c -> scan_number lexer
         | _ -> scan_symbol lexer)
    | Some ('0'..'9') -> scan_number lexer
    | Some ('a'..'z' | 'A'..'Z' | '_') -> scan_literal_or_keyword lexer
    | Some _ -> scan_symbol lexer
    | None -> { ttype = Eof; lexeme = ""; line = lexer.line; column = lexer.column }

let tokenize (lexer: lexer) =
  let rec aux acc =
    let tok = scan_token lexer in
    if tok.ttype = Eof then List.rev (tok :: acc)
    else aux (tok :: acc)
  in
  aux []