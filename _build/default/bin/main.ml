let usage_msg = "volta <file>"
let dump_tokens = ref false 
let dump_ast = ref false
let input_file = ref ""
let output_file = ref ""

let anon_fun filename = input_file := filename

let speclist = 
  [
    ("-dump_tokens", Arg.Set dump_tokens, "Dump tokens after lexing");
    ("-dump_ast", Arg.Set dump_ast, "Dump ast after parsing");
    ("-output_file", Arg.Set_string output_file, "Output file for compiled file");
  ]

let () = 
  Arg.parse speclist anon_fun usage_msg;
  Printf.printf "dump_tokens = %b\n" !dump_tokens;
  Printf.printf "dump_ast    = %b\n" !dump_ast;
  Printf.printf "input_file  = %s\n" !input_file;
  Printf.printf "output_file = %s\n" !output_file