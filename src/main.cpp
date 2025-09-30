#include "lexer/lexer.hpp"
#include "lexer/token.hpp"
#include <iostream>
#include <fstream>
#include <sstream>


using namespace volta::lexer;

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