#include "lexer/lexer.hpp"
#include <stdexcept>
#include <cctype>


namespace volta::lexer {

Lexer::Lexer(std::string source)
    : source_(std::move(source)), idx_(0), line_(1), column_(1) {}

bool Lexer::isAtEnd() const {
    return idx_ >= source_.length();
}

char Lexer::peek() const {
    if (idx_ + 1 < source_.length()) {
        return source_[idx_ + 1];
    }
    return '\0';
}

char Lexer::peekNext() const {
    if (idx_ + 2 < source_.length()) {
        return source_[idx_ + 2];
    }
    return '\0';
}

std::optional<char> Lexer::currentChar() const {
    if (isAtEnd()) {
        return std::nullopt;
    }
    return source_[idx_];
}

std::optional<char> Lexer::advance() {
    if (isAtEnd()) {
        return std::nullopt;
    }

    char c = source_[idx_];
    idx_++;

    if (c == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }

    return c;
}

void Lexer::skipWhitespace() {
    while (!isAtEnd() && std::isspace(static_cast<unsigned char>(source_[idx_]))) {
        advance();
    }
}

void Lexer::skipComment() {
    // Skip until newline or end of file
    while (!isAtEnd() && source_[idx_] != '\n') {
        advance();
    }
}

std::optional<Token> Lexer::scanNumber() {
    int startIdx = idx_;
    int startLine = line_;
    int startCol = column_;

    bool hasDot = false;
    bool hasExp = false;

    auto current = currentChar();
    while (current.has_value()) {
        char c = current.value();

        if (isDigit(c)) {
            advance();
        } else if (c == '.' && !hasDot && !hasExp && isDigit(peek())) {
            hasDot = true;
            advance();
        } else if ((c == 'e' || c == 'E') && !hasExp) {
            hasExp = true;
            advance();

            // Handle optional +/- after 'e'
            auto next = currentChar();
            if (next.has_value() && (next.value() == '+' || next.value() == '-')) {
                advance();
            }

            // Verify at least one digit follows
            if (!currentChar().has_value() || !isDigit(currentChar().value())) {
                errorReporter.reportSyntaxError(
                    "Invalid number: expected digit after 'e'", 
                    volta::errors::SourceLocation("", line_, column_, 1) // TODO: Improve the error reporting by having a file reference and length variable
                );
                return {};
            }
        } else {
            break;
        }

        current = currentChar();
    }

    std::string lexeme = source_.substr(startIdx, idx_ - startIdx);
    TokenType type = (hasDot || hasExp) ? TokenType::REAL : TokenType::INTEGER;

    return Token(type, lexeme, startLine, startCol);
}

std::optional<Token> Lexer::scanString() {
    int startLine = line_;
    int startCol = column_;

    advance(); // Skip opening quote

    std::string content;

    while (true) {
        auto c = currentChar();

        if (!c.has_value()) {
                errorReporter.reportSyntaxError(
                    "Unterminated string.", 
                    volta::errors::SourceLocation("", line_, column_, 1) // TODO: Improve the error reporting by having a file reference and length variable
                );
            return {};
        }

        if (c.value() == '"') {
            advance(); // Skip closing quote
            break;
        }

        if (c.value() == '\\') {
            advance();
            auto escaped = currentChar();

            if (!escaped.has_value()) {
                errorReporter.reportSyntaxError(
                    "Unterminated string.", 
                    volta::errors::SourceLocation("", line_, column_, 1) // TODO: Improve the error reporting by having a file reference and length variable
                );
            return {};
            }

            switch (escaped.value()) {
                case 'n': content += '\n'; break;
                case 't': content += '\t'; break;
                case 'r': content += '\r'; break;
                case '\\': content += '\\'; break;
                case '"': content += '"'; break;
                default: content += escaped.value(); break;
            }
            advance();
        } else {
            content += c.value();
            advance();
        }
    }

    return Token(TokenType::STRING_LITERAL, content, startLine, startCol);
}

Token Lexer::scanIdentifierOrKeyword() {
    int startIdx = idx_;
    int startLine = line_;
    int startCol = column_;

    while (currentChar().has_value() && isAlphanumeric(currentChar().value())) {
        advance();
    }

    std::string lexeme = source_.substr(startIdx, idx_ - startIdx);

    // Check if it's a keyword
    const auto& keywords = getKeywords();
    auto it = keywords.find(lexeme);

    TokenType type = (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;

    return Token(type, lexeme, startLine, startCol);
}

std::optional<Token> Lexer::scanSymbol() {
    int startLine = line_;
    int startCol = column_;

    auto c1Opt = advance();
    if (!c1Opt.has_value()) {
        errorReporter.reportSyntaxError(
            "Unexpected EOF.", 
            volta::errors::SourceLocation("", line_, column_, 1) // TODO: Improve the error reporting by having a file reference and length variable
        );
        return {};
    }

    char c1 = c1Opt.value();
    auto c2Opt = currentChar();

    // Try two-character operators first
    if (c2Opt.has_value()) {
        char c2 = c2Opt.value();
        std::string twoChar{c1, c2};

        // Check two-character operators
        if (twoChar == "+=") { advance(); return Token(TokenType::PLUS_ASSIGN, twoChar, startLine, startCol); }
        if (twoChar == "-=") { advance(); return Token(TokenType::MINUS_ASSIGN, twoChar, startLine, startCol); }
        if (twoChar == "*=") { advance(); return Token(TokenType::MULT_ASSIGN, twoChar, startLine, startCol); }
        if (twoChar == "**") { advance(); return Token(TokenType::POWER, twoChar, startLine, startCol); }
        if (twoChar == "/=") { advance(); return Token(TokenType::DIV_ASSIGN, twoChar, startLine, startCol); }
        if (twoChar == "==") { advance(); return Token(TokenType::EQUALS, twoChar, startLine, startCol); }
        if (twoChar == "!=") { advance(); return Token(TokenType::NOT_EQUALS, twoChar, startLine, startCol); }
        if (twoChar == ">=") { advance(); return Token(TokenType::GEQ, twoChar, startLine, startCol); }
        if (twoChar == "<=") { advance(); return Token(TokenType::LEQ, twoChar, startLine, startCol); }
        if (twoChar == ":=") { advance(); return Token(TokenType::INFER_ASSIGN, twoChar, startLine, startCol); }
        if (twoChar == "->") { advance(); return Token(TokenType::ARROW, twoChar, startLine, startCol); }
        if (twoChar == "=>") { advance(); return Token(TokenType::MATCH_ARROW, twoChar, startLine, startCol); }
    }

    // Single-character operators
    std::string oneChar{c1};

    switch (c1) {
        case '+': return Token(TokenType::PLUS, oneChar, startLine, startCol);
        case '-': return Token(TokenType::MINUS, oneChar, startLine, startCol);
        case '*': return Token(TokenType::MULT, oneChar, startLine, startCol);
        case '/': return Token(TokenType::DIV, oneChar, startLine, startCol);
        case '%': return Token(TokenType::MODULO, oneChar, startLine, startCol);
        case '=': return Token(TokenType::ASSIGN, oneChar, startLine, startCol);
        case '!': return Token(TokenType::NOT, oneChar, startLine, startCol);
        case '>': return Token(TokenType::GT, oneChar, startLine, startCol);
        case '<': return Token(TokenType::LT, oneChar, startLine, startCol);
        case ':': return Token(TokenType::COLON, oneChar, startLine, startCol);
        case '(': return Token(TokenType::LPAREN, oneChar, startLine, startCol);
        case ')': return Token(TokenType::RPAREN, oneChar, startLine, startCol);
        case '[': return Token(TokenType::LSQUARE, oneChar, startLine, startCol);
        case ']': return Token(TokenType::RSQUARE, oneChar, startLine, startCol);
        case '{': return Token(TokenType::LBRACE, oneChar, startLine, startCol);
        case '}': return Token(TokenType::RBRACE, oneChar, startLine, startCol);
        case '.':
            if (c2Opt.has_value() && c2Opt.value() == '.') {
                advance();  // consume second '.'
                auto c3Opt = currentChar();
                if (c3Opt.has_value() && c3Opt.value() == '=') {
                    advance();  // consume '='
                    return Token(TokenType::INCLUSIVE_RANGE, "..=", startLine, startCol);
                }
                return Token(TokenType::RANGE, "..", startLine, startCol);
            }
            return Token(TokenType::DOT, oneChar, startLine, startCol);
        case ',': return Token(TokenType::COMMA, oneChar, startLine, startCol);
        case ';': return Token(TokenType::SEMICOLON, oneChar, startLine, startCol);
        default:
            errorReporter.reportSyntaxError(
                "Unexpected character.", 
                volta::errors::SourceLocation("", line_, column_, 1) // TODO: Improve the error reporting by having a file reference and length variable
            );
            return {};
    }
}

std::optional<Token> Lexer::scanToken() {
    skipWhitespace();

    if (isAtEnd()) {
        return Token(TokenType::END_OF_FILE, "", line_, column_);
    }

    auto c = currentChar();
    if (!c.has_value()) {
        return Token(TokenType::END_OF_FILE, "", line_, column_);
    }

    char ch = c.value();

    // Skip comments
    if (ch == '#') {
        skipComment();
        return scanToken(); // Recursively get next token
    }

    // Numbers
    if (isDigit(ch)) {
        return scanNumber();
    }

    // Strings
    if (ch == '"') {
        return scanString();
    }

    // Identifiers and keywords
    if (isAlpha(ch)) {
        return scanIdentifierOrKeyword();
    }

    // Handle dots that might be part of numbers
    if (ch == '.' && isDigit(peek())) {
        return scanNumber();
    }

    // Symbols
    return scanSymbol();
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!isAtEnd()) {
        auto maybeToken = scanToken();
        if (!maybeToken.has_value()) {
            advance();
            continue;
        }

        Token token = maybeToken.value();
        tokens.push_back(token);

        if (token.type == TokenType::END_OF_FILE) {
            break;
        }
    }

    // Ensure EOF token is present
    if (tokens.empty() || tokens.back().type != TokenType::END_OF_FILE) {
        tokens.emplace_back(TokenType::END_OF_FILE, "", line_, column_);
    }

    return tokens;
}

bool Lexer::isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool Lexer::isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::isAlphanumeric(char c) {
    return isDigit(c) || isAlpha(c);
}

} // namespace volta