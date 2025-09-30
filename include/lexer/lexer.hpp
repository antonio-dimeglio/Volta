#pragma once 

#include "token.hpp"
#include "error/ErrorReporter.hpp"
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace volta::lexer {

class Lexer {
public:
    explicit Lexer(std::string source);

    // Tokenize the entire source
    std::vector<Token> tokenize();

    // Scan a single token
    std::optional<Token> scanToken();

    // Get the error reporter
    const volta::errors::ErrorReporter& getErrorReporter() const { return errorReporter; }

private:
    std::string source_;
    size_t idx_;
    int line_;
    int column_;
    volta::errors::ErrorReporter errorReporter;

    // Helper methods
    bool isAtEnd() const;
    char peek() const;
    char peekNext() const;
    std::optional<char> currentChar() const;
    std::optional<char> advance();

    void skipWhitespace();
    void skipComment();

    // Token scanning methods
    std::optional<Token> scanNumber();
    std::optional<Token> scanString();
    Token scanIdentifierOrKeyword();
    std::optional<Token> scanSymbol();

    // Character classification
    static bool isDigit(char c);
    static bool isAlpha(char c);
    static bool isAlphanumeric(char c);
};

} // namespace volta
