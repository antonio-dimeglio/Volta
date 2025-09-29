type lexer = { 
    program: string;
    mutable idx: int; 
    mutable line: int;
    mutable column: int;
  }

let create program =
    { program; idx = 0; line = 1; column = 1; }

let advance lexer = 
  lexer.idx <- lexer.idx + 1;
  lexer.column <- lexer.column + 1;

let peek lexer = 
  if lexer.idx < String.length lexer.program then
    Some lexer.program.[lexer.idx]
  else 
    None
