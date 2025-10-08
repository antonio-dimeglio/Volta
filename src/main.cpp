#include "lexer/lexer.hpp"
#include "lexer/token.hpp"
#include "ast/ASTDumper.hpp"
#include "parser/Parser.hpp"
#include "error/ErrorReporter.hpp"
#include "semantic/SemanticAnalyzer.hpp"
#include "llvm/LLVMCodegen.hpp"
#include "lsp/LSPProvider.hpp"
#include "CLI11.hpp"
#include <iostream>
#include <fstream>
#include <sstream>


using namespace volta::lexer;
using namespace volta::parser;
using namespace volta::ast;
using namespace volta::errors;
using namespace volta::llvm_codegen;

struct CompilerOptions {
    std::string inputFile;
    std::vector<std::string> lspInfoArgs;
    bool dumpTokens = false;
    bool dumpAST = false;
    bool optimize = true;
    bool repl = false;
    bool emitLLVM = false;
    bool compile = false;
    std::string outputFile;
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

    // ===== LLVM Generation ====
    LLVMCodegen codegen("volta_program");
    if (!codegen.generate(ast.get(), &analyzer)) {
        std::cerr << "Code generation failed\n";
        return;
    }

    if (options.emitLLVM) {
        // Emit LLVM IR (.ll file)
        std::string llFile = options.outputFile.empty()
            ? options.inputFile.substr(0, options.inputFile.find_last_of('.')) + ".ll"
            : options.outputFile;

        if (codegen.emitLLVMIR(llFile)) {
            std::cout << "LLVM IR written to " << llFile << "\n";
        } else {
            std::cerr << "Failed to emit LLVM IR\n";
        }
    }

    if (options.compile) {
        // Compile to executable
        std::string llFile = "/tmp/volta_temp.ll";
        if (!codegen.emitLLVMIR(llFile)) {
            std::cerr << "Failed to emit LLVM IR\n";
            return;
        }

        std::string exeName = options.outputFile.empty()
            ? options.inputFile.substr(0, options.inputFile.find_last_of('.'))
            : options.outputFile;

        std::string compileCmd = "clang-18 " + llFile +
                                " bin/libvolta.a -lgc -o " + exeName;

        std::cout << "Compiling: " << compileCmd << "\n";
        if (system(compileCmd.c_str()) == 0) {
            std::cout << "Executable created: " << exeName << "\n";
        } else {
            std::cerr << "Compilation failed\n";
        }
    }
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

    // Optimization flags
    auto* optGroup = app.add_option_group("Optimization", "Optimization options");
    optGroup->add_flag("-O,--optimize", options.optimize, "Enable optimizations (default: on)")
        ->default_val(true);
    optGroup->add_flag("--no-optimize", [&](int64_t) { options.optimize = false; },
        "Disable optimizations (-O0)");

    // REPL mode
    app.add_flag("-r,--repl", options.repl, "Start REPL (interactive mode)");

    // LLVM compilation flags
    app.add_flag("--emit-llvm", options.emitLLVM, "Emit LLVM IR to .ll file");
    app.add_flag("-c,--compile", options.compile, "Compile to native executable");
    app.add_option("-o,--output", options.outputFile, "Output file name");

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
