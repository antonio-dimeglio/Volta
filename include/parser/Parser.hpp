#pragma once
#include <memory>
#include <vector>
#include <optional>
#include "lexer/token.hpp"
#include "ast/Statement.hpp"
#include "ast/Expression.hpp"
#include "ast/Type.hpp"
#include "error/ErrorReporter.hpp"

namespace volta::parser {

class Parser {
public:
    explicit Parser(std::vector<volta::lexer::Token> tokens);

    // Main parsing entry point
    std::unique_ptr<volta::ast::Program> parseProgram();

    // Get the error reporter
    const volta::errors::ErrorReporter& getErrorReporter() const { return errorReporter; }

    // Statement parsing
    std::unique_ptr<volta::ast::Statement> parseStatement();
    std::unique_ptr<volta::ast::Block> parseBlock();
    std::unique_ptr<volta::ast::ExpressionStatement> parseExpressionStatement();
    std::unique_ptr<volta::ast::IfStatement> parseIfStatement();
    std::unique_ptr<volta::ast::WhileStatement> parseWhileStatement();
    std::unique_ptr<volta::ast::ForStatement> parseForStatement();
    std::unique_ptr<volta::ast::ReturnStatement> parseReturnStatement();
    std::unique_ptr<volta::ast::BreakStatement> parseBreakStatement();
    std::unique_ptr<volta::ast::ContinueStatement> parseContinueStatement();
    std::unique_ptr<volta::ast::ImportStatement> parseImportStatement();

    // Declaration parsing
    std::unique_ptr<volta::ast::VarDeclaration> parseVarDeclaration();
    std::unique_ptr<volta::ast::FnDeclaration> parseFnDeclaration();
    std::unique_ptr<volta::ast::StructDeclaration> parseStructDeclaration();

    // Expression parsing (by precedence)
    std::unique_ptr<volta::ast::Expression> parseExpression();
    std::unique_ptr<volta::ast::Expression> parseAssignment();
    std::unique_ptr<volta::ast::Expression> parseLogicalOr();
    std::unique_ptr<volta::ast::Expression> parseLogicalAnd();
    std::unique_ptr<volta::ast::Expression> parseEquality();
    std::unique_ptr<volta::ast::Expression> parseCast();
    std::unique_ptr<volta::ast::Expression> parseComparison();
    std::unique_ptr<volta::ast::Expression> parseRange();
    std::unique_ptr<volta::ast::Expression> parseTerm();
    std::unique_ptr<volta::ast::Expression> parseFactor();
    std::unique_ptr<volta::ast::Expression> parseUnary();
    std::unique_ptr<volta::ast::Expression> parsePower();
    std::unique_ptr<volta::ast::Expression> parseCall();
    std::unique_ptr<volta::ast::Expression> parsePrimary();

    // Literal parsing
    std::unique_ptr<volta::ast::Expression> parseArrayLiteral();
    std::unique_ptr<volta::ast::Expression> parseTupleLiteral();
    std::unique_ptr<volta::ast::Expression> parseStructLiteral();
    std::unique_ptr<volta::ast::Expression> parseIfExpression();
    std::unique_ptr<volta::ast::Expression> parseMatchExpression();
    std::unique_ptr<volta::ast::Expression> parseLambdaExpression();

    // Type parsing
    std::unique_ptr<volta::ast::Type> parseType();
    std::unique_ptr<volta::ast::TypeAnnotation> parseTypeAnnotation();
    std::unique_ptr<volta::ast::Type> parsePrimitiveType();
    std::unique_ptr<volta::ast::Type> parseCompoundType();
    std::unique_ptr<volta::ast::Type> parseFunctionType();

    // Pattern parsing
    std::unique_ptr<volta::ast::Pattern> parsePattern();

private:
    std::vector<volta::lexer::Token> tokens_;
    size_t current_;
    volta::errors::ErrorReporter errorReporter;

    // Token navigation
    const volta::lexer::Token& peek() const;
    const volta::lexer::Token& peekNext() const;
    const volta::lexer::Token& previous() const;
    bool isAtEnd() const;
    const volta::lexer::Token& advance();
    bool check(volta::lexer::TokenType type) const;
    bool match(volta::lexer::TokenType type);
    bool match(std::initializer_list<volta::lexer::TokenType> types);
    const volta::lexer::Token& consume(volta::lexer::TokenType type, const std::string& message);

    // Helper methods
    volta::errors::SourceLocation currentLocation() const;
    void synchronize();
};

} // namespace volta::parser