#include "Lexer/Lexer.hpp"
#include "Parser/Parser.hpp"
#include "Parser/ASTPrinter.hpp"
#include "HIR/Lowering.hpp"
#include "MIR/Elaboration.hpp"
#include "Semantic/TypeRegistry.hpp"
#include "Semantic/SemanticAnalyzer.hpp"
#include "Codegen/Codegen.hpp"
#include "Codegen/ModuleEmitter.hpp"
#include "Error/Error.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>

void printUsage(const char* progName) {
    std::cerr << "Usage: " << progName << " [options] <source_file>\n";
    std::cerr << "Options:\n";
    std::cerr << "  --dump-tokens    Dump tokens after lexing\n";
    std::cerr << "  --dump-ast       Dump AST after parsing\n";
    std::cerr << "  --dump-hir       Dump HIR after desugaring\n";
    std::cerr << "  --dump-llvm      Dump LLVM IR to stdout\n";
    std::cerr << "  -O<level>        Optimization level (0-3, default: 0)\n";
    std::cerr << "  -o <output>      Output executable name (default: a.out)\n";
    std::cerr << "  --help           Show this help message\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    bool dumpTokens = false;
    bool dumpAST = false;
    bool dumpHIR = false;
    bool dumpLLVM = false;
    int optLevel = 0;
    std::string outputFile = "a.out";
    const char* sourceFile = nullptr;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        } else if (std::strcmp(argv[i], "--dump-tokens") == 0) {
            dumpTokens = true;
        } else if (std::strcmp(argv[i], "--dump-ast") == 0) {
            dumpAST = true;
        } else if (std::strcmp(argv[i], "--dump-hir") == 0) {
            dumpHIR = true;
        } else if (std::strcmp(argv[i], "--dump-llvm") == 0) {
            dumpLLVM = true;
        } else if (std::strncmp(argv[i], "-O", 2) == 0) {
            optLevel = argv[i][2] - '0';
            if (optLevel < 0 || optLevel > 3) {
                std::cerr << "Error: Invalid optimization level: " << argv[i] << "\n";
                return 1;
            }
        } else if (std::strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                outputFile = argv[++i];
            } else {
                std::cerr << "Error: -o requires an argument\n";
                return 1;
            }
        } else {
            sourceFile = argv[i];
        }
    }

    if (!sourceFile) {
        std::cerr << "Error: No source file specified\n";
        printUsage(argv[0]);
        return 1;
    }

    std::ifstream file(sourceFile);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << sourceFile << "'\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();

    DiagnosticManager diag;

    Lexer lexer(source, diag);
    auto tokens = lexer.tokenize();

    if (diag.hasErrors()) {
        diag.printAll(std::cerr, sourceFile);
        return 1;
    }

    if (dumpTokens) {
        std::cout << "=== Token Dump ===\n";
        for (const auto& token : tokens) {
            std::cout << token.toString() << "\n";
        }
        std::cout << "\n";
    }

    Parser parser(tokens, diag);
    auto astProgram = parser.parseProgram();

    if (diag.hasErrors()) {
        diag.printAll(std::cerr, sourceFile);
        return 1;
    }

    if (dumpAST) {
        std::cout << "=== AST Dump ===\n";
        ASTPrinter printer;
        printer.print(*astProgram, std::cout);
        std::cout << "\n";
    }

    HIRLowering lowering;
    auto hirProgram = lowering.lower(std::move(*astProgram));


    if (dumpHIR) {
        std::cout << "=== HIR Dump (Desugared AST) ===\n";
        ASTPrinter printer;
        printer.print(hirProgram, std::cout);
        std::cout << "\n";
    }

    Semantic::TypeRegistry typeRegistry;
    Semantic::SemanticAnalyzer analyzer(typeRegistry, diag);
    analyzer.analyzeProgram(hirProgram);

    if (diag.hasErrors()) {
        diag.printAll(std::cerr, sourceFile);
        return 1;
    }

    MIRElaboration elaboration(&analyzer.getSymbolTable());
    auto mirProgram = elaboration.elaborate(std::move(hirProgram));

    ModuleEmitter::initializeTargets();

    Codegen codegen(mirProgram, typeRegistry, diag);
    auto module = codegen.generate();

    if (diag.hasErrors()) {
        diag.printAll(std::cerr, sourceFile);
        return 1;
    }

    ModuleEmitter emitter(module);
    emitter.compileToExecutable(outputFile, optLevel, dumpLLVM);


    return 0;
}
