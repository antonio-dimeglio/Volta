type lexer = { 
    program: string;
    mutable idx: int; 
    mutable line: int;
    mutable column: int;
  }

let create program =
  { program; idx = 0; line = 1; column = 1; }

let is_at_end lexer =
    lexer.idx >= String.length lexer.program

let skip_whitspace lexer = 
  if  
    lexer.program.[lexer.idx] = ' ' || 
    lexer.program.[lexer.idx] = '\t' ||
    lexer.program.[lexer.idx] = '\r' then 
    
    lexer.idx <- lexer.idx + 1;
    skip