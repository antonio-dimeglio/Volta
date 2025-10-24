#pragma once
#include <vector>
#include "Lexer/Token.hpp"
#include "Error/Error.hpp"
#include "Type/TypeRegistry.hpp"
#include "AST.hpp"

class Parser {
private:
    size_t idx;
    std::vector<Token> tokens;
    Type::TypeRegistry& types;
    DiagnosticManager& diag;

public:
    Parser(const std::vector<Token>& tokens, Type::TypeRegistry& types, DiagnosticManager& diag)  :
        idx(0), tokens(tokens), types(types), diag(diag) {}

    std::unique_ptr<Program> parseProgram();

private:
    // Token navigation helpers

    // Look ahead at token without consuming it
    [[nodiscard]] Token peek(size_t offset = 0) const;

    // Check if at end of token stream
    [[nodiscard]] bool isAtEnd() const;

    // Consume and return current token
    Token advance();

    // Check if token matches type
    [[nodiscard]] bool check(TokenType type, size_t offset = 0) const;

    // Consume token or report error if type doesn't match
    Token expect(TokenType expected);

    // Consume token if it matches any type
    bool match(const std::vector<TokenType>& types);

    // Check if current token starts a literal expression
    [[nodiscard]] bool isLiteralExpr() const;

    // Utility function to parse dimension of array
    int parsePositiveInteger();
    // Parse type annotation (i32, f64, bool, etc.)
    const Type::Type* parseType();

    // Parse function definition (with body)
    std::unique_ptr<Stmt> parseFnDef();

    // Parse function signature (without body) - used for extern blocks
    std::unique_ptr<FnDecl> parseFnSignature();
    
    // Parse struct definiton 
    std::unique_ptr<StructDecl> parseStructDecl();

    // Parse import statement
    std::unique_ptr<ImportStmt> parseImportStmt();

    // Parse extern block
    std::unique_ptr<Stmt> parseExternBlock();

    // Parse block body between braces
    std::vector<std::unique_ptr<Stmt>> parseBody();

    // Parse variable declaration (let/let mut)
    std::unique_ptr<Stmt> parseVarDecl();

    // Parse return statement
    std::unique_ptr<Stmt> parseReturnStmt();

    // Parse if/else statement
    std::unique_ptr<Stmt> parseIfStmt();

    // Parse while loop
    std::unique_ptr<Stmt> parseWhileStmt();

    // Parse for-in loop with range
    std::unique_ptr<Stmt> parseForStmt();

    // Parse standalone block statement
    std::unique_ptr<Stmt> parseBlockStmt();

    // Parse break statement
    std::unique_ptr<Stmt> parseBreakStmt();

    // Parse continue statement
    std::unique_ptr<Stmt> parseContinueStmt();

    // Parse expression statement
    std::unique_ptr<Stmt> parseExprStmt();

    // Parse assignment or higher precedence expression
    std::unique_ptr<Expr> parseExpression();

    // Parse logical OR (||)
    std::unique_ptr<Expr> parseLogicalOr();

    // Parse logical AND (&&)
    std::unique_ptr<Expr> parseLogicalAnd();

    // Parse comparison operators (==, !=, <, >, <=, >=)
    std::unique_ptr<Expr> parseComparison();

    // Parse addition/subtraction (+, -)
    std::unique_ptr<Expr> parseAddition();

    // Parse multiplication/division/modulo (*, /, %)
    std::unique_ptr<Expr> parseTerm();

    // Parse unary operators (!, -)
    std::unique_ptr<Expr> parseUnary();

    // Parse postfix operators (++, --, array indexing)
    std::unique_ptr<Expr> parsePostfix();

    // Parse function call with arguments
    std::unique_ptr<Expr> parseFunctionCall();

    // Parse primary expressions (literals, variables, grouping)
    std::unique_ptr<Expr> parsePrimary();

    // Parse array literal ([1, 2, 3] or [value; count])
    std::unique_ptr<Expr> parseArrayLiteral();

    std::unique_ptr<Expr> parseRangeExpr();

    // Parse struct literal: StructName { field: value, ... }
    std::unique_ptr<StructLiteral> parseStructLiteral(const Token& structName);
};
