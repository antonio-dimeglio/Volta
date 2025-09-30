type token_type = 
  (* Literal values *)
  Integer | Real | StringLiteral | BooleanLiteral |
  (* Operators *)
  (* Arithmetics *)
  Plus | Minus | Mult | Div | Modulo | Power |
  (* Comparison *)
  Equals | NotEquals | LT | GT | LEQ | GEQ |
  (* Logical *)
  And | Or | Not |
  (* Assignment *)
  Assign | PlusAssign | MinusAssign | MultAssign | DivAssign | InferAssign |
  (* Range *)
  Range | InclusiveRange |

  Identifier |

  (* Keywords *)
  Function | Return | If | Else | While | For | In | Match |
  Struct | Mut | SomeKeyword | NoneKeyword | 

  (* Parentheses *)
  LParen | RParen | LSquare | RSquare | LBrace | RBrace | 

  (* Misc *)
  Colon | Arrow | MatchArrow | Dot |Eof

  module StringMap = Map.Make(String)

let keywords =
  List.fold_left (fun m (k,v) -> StringMap.add k v m) StringMap.empty
    [ "fn", Function;
      "return", Return;
      "if", If;
      "else", Else;
      "while", While;
      "for", For;
      "in", In;
      "match", Match;
      "struct", Struct;
      "mut", Mut;
      "some", SomeKeyword;
      "none", NoneKeyword ]


let token_to_string = function 
  | Integer -> "Integer" | Real -> "Real"  
  | StringLiteral -> "StringLiteral" | BooleanLiteral -> "BooleanLiteral"

  | Plus -> "Plus" | Minus -> "Minus" | Mult -> "Mult" 
  | Div -> "Div"  | Modulo -> "Modulo" | Power -> "Power"

  | Equals -> "Equals"| NotEquals -> "NotEquals"
  | LT -> "LT" | GT -> "GT"
  | LEQ -> "LEQ" | GEQ -> "GEQ"

  | And -> "And" | Or -> "Or" | Not -> "Not"

  | Assign -> "Assign" | PlusAssign -> "PlusAssign" | MinusAssign -> "MinusAssign"
  | MultAssign -> "MultAssign" | DivAssign -> "DivAssign" | InferAssign -> "InferAssign"

  | Range -> "Range" | InclusiveRange -> "InclusiveRange"

  | Identifier -> "Identifier"

  | Function -> "Function" | Return -> "Return" | If -> "If" | Else -> "Else"
  | While -> "While" | For -> "For" | In -> "In" | Match -> "Match"
  | Struct -> "Struct" | Mut -> "Mut" | SomeKeyword -> "Some" | NoneKeyword -> "None"

  | LParen -> "LParen" | RParen -> "RParen" | LSquare -> "LSquare"
  | RSquare -> "RSquare" | LBrace -> "LBrace" | RBrace -> "RBrace"

  | Colon -> "Colon" | Arrow -> "Arrow" 
  | MatchArrow -> "MatchArrow" | Dot -> "Dot"
  | Eof -> "Eof"