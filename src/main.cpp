#include "lexer/lexer.hpp"
#include "lexer/token.hpp"
#include "ast/ASTDumper.hpp"
#include "parser/Parser.hpp"
#include "error/ErrorReporter.hpp"
#include "semantic/SemanticAnalyzer.hpp"
#include "IR/IRGenerator.hpp"
#include "IR/IRPrinter.hpp"
#include "IR/Verifier.hpp"
#include "IR/OptimizationPass.hpp"
#include "vm/BytecodeCompiler.hpp"
#include "vm/VM.hpp"
#include <iostream>
#include <fstream>
#include <sstream>


using namespace volta::lexer;
using namespace volta::parser;
using namespace volta::ast;
using namespace volta::errors;
using namespace volta::ir;
using namespace volta::vm;

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

        auto module = generateIR(*ast, analyzer, "program");
        if (!module) {
            std::cerr << "IR generation failed\n";
            exit(1);
        }

        PassManager pm; 
        pm.addPass(std::make_unique<ConstantFoldingPass>());
        pm.addPass(std::make_unique<ConstantPropagationPass>());
        pm.addPass(std::make_unique<Mem2RegPass>());
        pm.addPass(std::make_unique<InstructionSimplifyPass>());
        pm.addPass(std::make_unique<DeadCodeEliminationPass>());
        pm.addPass(std::make_unique<SimplifyCFG>());
        pm.addPass(std::make_unique<DeadCodeEliminationPass>());

        if (pm.run(*module)) {
            std::cout << "Performed IR code optimization \n";
        }

        IRPrinter irPrinter;
        std::cout << irPrinter.printModule(*module) << "\n";

        Verifier verifier;
        if (!verifier.verify(*module)) {
            std::cerr << "IR verification failed:\n";
            for (const auto& error : verifier.getErrors()) {
                std::cerr << "  - " << error << "\n";
            }
            exit(1);
        }


        BytecodeCompiler bc;
        auto bcModule = bc.compile(std::move(module));

        std::cout << "\n";
        bcModule->printSummary(std::cout);
        bcModule->printFunctionTable(std::cout);
        bcModule->printConstantPools(std::cout);

        VM vm;
        auto res = vm.execute(*bcModule, "__main");
        std::cout << "Final value after execution: " << res.toString() << "\n";

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