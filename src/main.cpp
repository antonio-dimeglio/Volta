#include "lexer/lexer.hpp"
#include "lexer/token.hpp"
#include "ast/ASTDumper.hpp"
#include "parser/Parser.hpp"
#include "error/ErrorReporter.hpp"
#include "semantic/SemanticAnalyzer.hpp"
// #include "IR/IRGenerator.hpp"
// #include "IR/IRPrinter.hpp"
// #include "bytecode/BytecodeCompiler.hpp"
// #include "bytecode/Disassembler.hpp"
// #include "vm/VM.hpp"
#include <iostream>
#include <fstream>
#include <sstream>


using namespace volta::lexer;
using namespace volta::parser;
using namespace volta::ast;
using namespace volta::errors;
// using namespace volta::bytecode;
// using namespace volta::vm;

void printUsage(const char* programName) {
    std::cout << "Volta Programming Language\n";
    std::cout << "Usage:\n";
    std::cout << "  " << programName << "              Start REPL\n";
    std::cout << "  " << programName << " <file.vlt>  Run Volta script\n";
}

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void runFile(const std::string& filename) {
    try {
        std::string source = readFile(filename);
        Lexer lexer (source);
        auto tokens = lexer.tokenize();

        // For now, just print tokens
        std::cout << "Tokens:\n";
        for (const auto& token : tokens) {
            std::cout << "  " << tokenTypeToString(token.type)
                      << " '" << token.lexeme << "'"
                      << " (line " << token.line << ", col " << token.column << ")\n";
        }

        Parser parser(tokens);
        auto ast = parser.parseProgram();
        ASTDumper astDumper;

        std::cout << "\nAST:\n";
        std::cout << astDumper.dump(*ast);

        // Semantic analysis
        ErrorReporter semanticReporter;
        volta::semantic::SemanticAnalyzer analyzer(semanticReporter);
        analyzer.analyze(*ast);

        if (semanticReporter.hasErrors()) {
            std::cerr << "\nSemantic Analysis Errors:\n";
            semanticReporter.printErrors(std::cerr);
            return;
        }

        std::cout << "\nSemantic analysis passed!\n";

        // IR generation
        // ErrorReporter irReporter;
        // IRGenerator irGenerator(irReporter);
        // auto irModule = irGenerator.generate(*ast, analyzer);

        // if (irReporter.hasErrors()) {
        //     std::cerr << "\nIR Generation Errors:\n";
        //     irReporter.printErrors(std::cerr);
        //     return;
        // }

        // if (irModule) {
        //     std::cout << "\n=== Generated IR ===\n";
        //     IRPrinter printer(std::cout);
        //     printer.print(*irModule);
        //     std::cout << "\n";
        // }

        // BytecodeCompiler compiler;
        // auto res = compiler.compile(*irModule);

        // if (res) {
        //     Disassembler ds;
        //     auto asmbl = ds.disassembleModule(*res);
        //     std::cout << asmbl << "\n";
        // }

        // VM vm(std::move(res));

        // // Register native implementations of foreign functions
        // vm.registerNativeFunction("print", [](VM& vm) {
        //     volta::vm::Value arg = vm.pop();
        //     if (arg.type == volta::vm::ValueType::String) {
        //         if (arg.asStringIndex < vm.module()->stringPool().size()) {
        //             std::cout << vm.module()->stringPool()[arg.asStringIndex];
        //         }
        //     } else if (arg.type == volta::vm::ValueType::Int) {
        //         std::cout << arg.asInt;
        //     } else if (arg.type == volta::vm::ValueType::Float) {
        //         std::cout << arg.asFloat;
        //     } else if (arg.type == volta::vm::ValueType::Bool) {
        //         std::cout << (arg.asBool ? "true" : "false");
        //     } else if (arg.type == volta::vm::ValueType::Null) {
        //         std::cout << "null";
        //     }
        //     return 0; // No return value
        // });

        // vm.registerNativeFunction("println", [](VM& vm) {
        //     volta::vm::Value arg = vm.pop();
        //     if (arg.type == volta::vm::ValueType::String) {
        //         if (arg.asStringIndex < vm.module()->stringPool().size()) {
        //             std::cout << vm.module()->stringPool()[arg.asStringIndex] << "\n";
        //         }
        //     } else if (arg.type == volta::vm::ValueType::Int) {
        //         std::cout << arg.asInt << "\n";
        //     } else if (arg.type == volta::vm::ValueType::Float) {
        //         std::cout << arg.asFloat << "\n";
        //     } else if (arg.type == volta::vm::ValueType::Bool) {
        //         std::cout << (arg.asBool ? "true" : "false") << "\n";
        //     } else if (arg.type == volta::vm::ValueType::Null) {
        //         std::cout << "null\n";
        //     }
        //     return 0; // No return value
        // });

        // vm.registerNativeFunction("len", [](VM& vm) {
        //     volta::vm::Value arg = vm.pop();
        //     if (arg.type == volta::vm::ValueType::String) {
        //         if (arg.asStringIndex < vm.module()->stringPool().size()) {
        //             int64_t len = vm.module()->stringPool()[arg.asStringIndex].length();
        //             vm.push(volta::vm::Value::makeInt(len));
        //         } else {
        //             vm.push(volta::vm::Value::makeInt(0));
        //         }
        //     } else {
        //         vm.push(volta::vm::Value::makeInt(0));
        //     }
        //     return 1; // One return value
        // });

        // vm.registerNativeFunction("assert", [](VM& vm) {
        //     volta::vm::Value arg = vm.pop();
        //     if (arg.type == volta::vm::ValueType::Bool && !arg.asBool) {
        //         std::cerr << "Assertion failed!\n";
        //         exit(1);
        //     }
        //     return 0; // No return value
        // });

        // vm.registerNativeFunction("type_of", [](VM& vm) {
        //     volta::vm::Value arg = vm.pop();
        //     std::string typeName;
        //     switch (arg.type) {
        //         case volta::vm::ValueType::Int: typeName = "int"; break;
        //         case volta::vm::ValueType::Float: typeName = "float"; break;
        //         case volta::vm::ValueType::Bool: typeName = "bool"; break;
        //         case volta::vm::ValueType::String: typeName = "string"; break;
        //         case volta::vm::ValueType::Object: typeName = "object"; break;
        //         case volta::vm::ValueType::Null: typeName = "null"; break;
        //     }
        //     // Add type name to string pool and push
        //     // For now, just push a dummy string index
        //     vm.push(volta::vm::Value::makeString(0)); // TODO: properly add to string pool
        //     return 1; // One return value
        // });

        // vm.registerNativeFunction("to_string", [](VM& vm) {
        //     volta::vm::Value arg = vm.pop();
        //     // For now, just return the input if it's already a string
        //     // TODO: convert other types to strings
        //     if (arg.type == volta::vm::ValueType::String) {
        //         vm.push(arg);
        //     } else {
        //         vm.push(volta::vm::Value::makeString(0)); // Dummy
        //     }
        //     return 1; // One return value
        // });

        // // vm.setDebugTrace(true);  // Enable debug trace
        // int execRes = vm.execute();
        // std::cout << "Exec res: " <<  execRes << "\n";
        // if (vm.stackSize() > 0) {
        //     auto retVal = vm.pop();
        //     std::cout << "Return value: " << retVal.asInt << "\n";
        // }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void runRepl() {
    std::cout << "Volta REPL (type 'exit' to quit)\n";

    std::string line;
    while (true) {
        std::cout << ">>> ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line == "exit" || line == "quit") {
            break;
        }

        if (line.empty()) {
            continue;
        }

        try {
            Lexer lexer(line);
            auto tokens = lexer.tokenize();

            for (const auto& token : tokens) {
                if (token.type != TokenType::END_OF_FILE) {
                    std::cout << tokenTypeToString(token.type)
                              << " '" << token.lexeme << "'\n";
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    std::cout << "Goodbye!\n";
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        // No arguments - start REPL
        runRepl();
    } else if (argc == 2) {
        // One argument - run file
        std::string filename = argv[1];
        if (filename == "--help" || filename == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        runFile(filename);
    } else {
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}