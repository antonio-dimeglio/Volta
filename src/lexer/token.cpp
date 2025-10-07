#include "lexer/token.hpp"

namespace volta::lexer {

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::INTEGER: return "INTEGER";
        case TokenType::REAL: return "REAL";
        case TokenType::STRING_LITERAL: return "STRING_LITERAL";
        case TokenType::BOOLEAN_LITERAL: return "BOOLEAN_LITERAL";

        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::MULT: return "MULT";
        case TokenType::DIV: return "DIV";
        case TokenType::MODULO: return "MODULO";
        case TokenType::POWER: return "POWER";

        case TokenType::EQUALS: return "EQUALS";
        case TokenType::NOT_EQUALS: return "NOT_EQUALS";
        case TokenType::LT: return "LT";
        case TokenType::GT: return "GT";
        case TokenType::LEQ: return "LEQ";
        case TokenType::GEQ: return "GEQ";

        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::NOT: return "NOT";

        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::PLUS_ASSIGN: return "PLUS_ASSIGN";
        case TokenType::MINUS_ASSIGN: return "MINUS_ASSIGN";
        case TokenType::MULT_ASSIGN: return "MULT_ASSIGN";
        case TokenType::DIV_ASSIGN: return "DIV_ASSIGN";
        case TokenType::INFER_ASSIGN: return "INFER_ASSIGN";

        case TokenType::RANGE: return "RANGE";
        case TokenType::INCLUSIVE_RANGE: return "INCLUSIVE_RANGE";

        case TokenType::IDENTIFIER: return "IDENTIFIER";

        case TokenType::FUNCTION: return "FUNCTION";
        case TokenType::RETURN: return "RETURN";
        case TokenType::IF: return "IF";
        case TokenType::ELSE: return "ELSE";
        case TokenType::WHILE: return "WHILE";
        case TokenType::FOR: return "FOR";
        case TokenType::IN: return "IN";
        case TokenType::MATCH: return "MATCH";
        case TokenType::STRUCT: return "STRUCT";
        case TokenType::MUT: return "MUT";
        case TokenType::SOME_KEYWORD: return "SOME";
        case TokenType::NONE_KEYWORD: return "NONE";
        case TokenType::AS: return "AS";
        case TokenType::IMPORT: return "IMPORT";

        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LSQUARE: return "LSQUARE";
        case TokenType::RSQUARE: return "RSQUARE";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";

        case TokenType::COLON: return "COLON";
        case TokenType::ARROW: return "ARROW";
        case TokenType::MATCH_ARROW: return "MATCH_ARROW";
        case TokenType::DOT: return "DOT";
        case TokenType::COMMA: return "COMMA";
        case TokenType::SEMICOLON: return "SEMICOLON";

        case TokenType::END_OF_FILE: return "EOF";

        default: return "UNKNOWN";
    }
}

const std::unordered_map<std::string, TokenType>& getKeywords() {
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"fn", TokenType::FUNCTION},
        {"return", TokenType::RETURN},
        {"if", TokenType::IF},
        {"else", TokenType::ELSE},
        {"while", TokenType::WHILE},
        {"for", TokenType::FOR},
        {"in", TokenType::IN},
        {"break", TokenType::BREAK},
        {"continue", TokenType::CONTINUE},
        {"match", TokenType::MATCH},
        {"struct", TokenType::STRUCT},
        {"mut", TokenType::MUT},
        {"Some", TokenType::SOME_KEYWORD},
        {"None", TokenType::NONE_KEYWORD},
        {"type", TokenType::TYPE},
        {"import", TokenType::IMPORT},
        {"true", TokenType::BOOLEAN_LITERAL},
        {"false", TokenType::BOOLEAN_LITERAL},
        {"and", TokenType::AND},
        {"or", TokenType::OR},
        {"not", TokenType::NOT},
        {"as", TokenType::AS}
    };
    return keywords;
}

} // namespace volta