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
#include "lsp/LSPProvider.hpp"
#include "CLI11.hpp"
#include <iostream>
#include <fstream>
#include <sstream>


using namespace volta::lexer;
using namespace volta::parser;
using namespace volta::ast;
using namespace volta::errors;

struct CompilerOptions {
    std::string inputFile;
    std::vector<std::string> lspInfoArgs;
    bool dumpTokens = false;
    bool dumpAST = false;
    bool dumpIR = false;
    bool dumpBytecode = false;
    bool optimize = true;
    bool verifyIR = false;
    bool execute = true;
    bool repl = false;
};

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void compileAndRun(const std::string& source, const CompilerOptions& options) {
    // ===== Lexer =====
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    if (options.dumpTokens) {
        std::cout << "=== TOKENS ===\n";
        for (const auto& token : tokens) {
            if (token.type != TokenType::END_OF_FILE) {
                std::cout << "  " << tokenTypeToString(token.type)
                          << " '" << token.lexeme << "'"
                          << " (line " << token.line << ", col " << token.column << ")\n";
            }
        }
        std::cout << "\n";
    }

    // ===== Parser =====
    Parser parser(tokens);
    auto ast = parser.parseProgram();

    if (options.dumpAST) {
        std::cout << "=== AST ===\n";
        ASTDumper astDumper;
        std::cout << astDumper.dump(*ast);
        std::cout << "\n";
    }

    // ===== Semantic Analysis =====
    ErrorReporter semanticReporter;
    volta::semantic::SemanticAnalyzer analyzer(semanticReporter);
    analyzer.analyze(*ast);

    if (semanticReporter.hasErrors()) {
        std::cerr << "\n=== SEMANTIC ANALYSIS ERRORS ===\n";
        semanticReporter.printErrors(std::cerr);
        return;
    }

    std::cout << "Semantic analysis passed!\n";

    
}

void runFile(const std::string& filename, const CompilerOptions& options) {
    try {
        std::string source = readFile(filename);
        compileAndRun(source, options);
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

void handleLSPInfo(const std::vector<std::string>& args) {
    using namespace volta::lsp;

    if (args.size() != 3) {
        outputErrorJSON("INVALID_ARGS", "Expected 3 arguments: file line column");
        return;
    }

    std::string filename = args[0];
    size_t line = std::stoul(args[1]);
    size_t column = std::stoul(args[2]);

    try {
        // Read and parse the file
        std::string source = readFile(filename);

        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        Parser parser(tokens);
        auto ast = parser.parseProgram();

        // Run semantic analysis
        ErrorReporter reporter;
        volta::semantic::SemanticAnalyzer analyzer(reporter);
        analyzer.analyze(*ast);

        if (reporter.hasErrors()) {
            outputErrorJSON("SEMANTIC_ERROR", "File has semantic errors");
            return;
        }

        // Create LSP provider and query
        LSPProvider lspProvider(ast.get(), &analyzer);
        Position pos(line, column);

        auto info = lspProvider.getSymbolInfo(filename, pos);

        if (info) {
            outputSuccessJSON(*info);
        } else {
            outputErrorJSON("SYMBOL_NOT_FOUND", "No symbol found at position");
        }

    } catch (const std::exception& e) {
        outputErrorJSON("INTERNAL_ERROR", e.what());
    }
}

int main(int argc, char* argv[]) {
    CLI::App app{"Volta Programming Language Compiler"};

    CompilerOptions options;

    // Positional argument for input file
    app.add_option("input", options.inputFile, "Input Volta source file (.volta)")
        ->check(CLI::ExistingFile);

    // Lsp info
    app.add_option("--lsp-info", options.lspInfoArgs, "Get symbol at position")
        ->expected(3) // File, line and column
        ->group("LSP");

    // Dump flags
    app.add_flag("--dump-tokens", options.dumpTokens, "Dump lexer tokens");
    app.add_flag("--dump-ast", options.dumpAST, "Dump abstract syntax tree");
    app.add_flag("--dump-ir", options.dumpIR, "Dump intermediate representation");
    app.add_flag("--dump-bytecode", options.dumpBytecode, "Dump bytecode disassembly");

    // Optimization flags
    auto* optGroup = app.add_option_group("Optimization", "Optimization options");
    optGroup->add_flag("-O,--optimize", options.optimize, "Enable optimizations (default: on)")
        ->default_val(true);
    optGroup->add_flag("--no-optimize", [&](int64_t) { options.optimize = false; },
        "Disable optimizations (-O0)");

    // Verification
    app.add_flag("--verify-ir", options.verifyIR, "Enable IR verification");

    // Execution flags
    auto* execGroup = app.add_option_group("Execution", "Execution options");
    execGroup->add_flag("-e,--execute", options.execute, "Execute the compiled bytecode (default: on)")
        ->default_val(true);
    execGroup->add_flag("--no-execute", [&](int64_t) { options.execute = false; },
        "Compile only, don't execute");

    // REPL mode
    app.add_flag("-r,--repl", options.repl, "Start REPL (interactive mode)");

    CLI11_PARSE(app, argc, argv);

    // Handle LSP mode
    if (!options.lspInfoArgs.empty()) {
        handleLSPInfo(options.lspInfoArgs);
        return 0;
    }

    // If no input file and not REPL mode, start REPL
    if (options.inputFile.empty() && !options.repl) {
        runRepl();
    } else if (options.repl) {
        runRepl();
    } else {
        runFile(options.inputFile, options);
    }

    return 0;
}
