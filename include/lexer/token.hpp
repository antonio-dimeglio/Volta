#pragma once

#include <string>
#include <unordered_map>

namespace volta::lexer {

enum class TokenType {
    // Literals
    INTEGER,
    REAL,
    STRING_LITERAL,
    BOOLEAN_LITERAL,

    // Operators - Arithmetic
    PLUS,
    MINUS,
    MULT,
    DIV,
    MODULO,
    POWER,

    // Operators - Comparison
    EQUALS,
    NOT_EQUALS,
    LT,
    GT,
    LEQ,
    GEQ,

    // Operators - Logical
    AND,
    OR,
    NOT,

    // Operators - Assignment
    ASSIGN,
    PLUS_ASSIGN,
    MINUS_ASSIGN,
    MULT_ASSIGN,
    DIV_ASSIGN,
    INFER_ASSIGN,

    // Range
    RANGE,
    INCLUSIVE_RANGE,

    // Identifiers
    IDENTIFIER,

    // Keywords
    FUNCTION,
    RETURN,
    IF,
    ELSE,
    WHILE,
    FOR,
    IN,
    BREAK,
    CONTINUE,
    MATCH,
    STRUCT,
    MUT,
    LET,
    IMPORT,
    AS,
    TYPE,
    ENUM,

    // Delimiters
    LPAREN,
    RPAREN,
    LSQUARE,
    RSQUARE,
    LBRACE,
    RBRACE,

    // Misc
    COLON,
    ARROW,
    MATCH_ARROW,
    DOT,
    COMMA,
    SEMICOLON,

    // End of file
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;

    Token(TokenType type, std::string lexeme, int line, int column)
        : type(type), lexeme(std::move(lexeme)), line(line), column(column) {}
};

// Token type utilities
std::string tokenTypeToString(TokenType type);
const std::unordered_map<std::string, TokenType>& getKeywords();

} // namespace volta
