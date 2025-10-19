#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include "Type.hpp"
#include "../Lexer/Token.hpp"
#include "../Lexer/TokenType.hpp"

struct ASTNode;
struct Expr;
struct Stmt;


struct ASTNode {
    int line = 0;
    int column = 0;

    ASTNode() = default;
    ASTNode(int line, int column) : line(line), column(column) {}
    virtual ~ASTNode() = default;
    virtual std::string toString() const = 0;
};

struct Expr : ASTNode {
    Expr() = default;
    Expr(int line, int column) : ASTNode(line, column) {}
    virtual ~Expr() = default;
};

struct Stmt : ASTNode {
    Stmt() = default;
    Stmt(int line, int column) : ASTNode(line, column) {}
    virtual ~Stmt() = default;
};


struct FnCall : Expr {
    std::string name;
    std::vector<std::unique_ptr<Expr>> args;

    FnCall(const std::string& name, std::vector<std::unique_ptr<Expr>> args, int line = 0, int column = 0)
        : Expr(line, column), name(name), args(std::move(args)) {}

    std::string toString() const override;
};

struct BinaryExpr : Expr {
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
    TokenType op;

    BinaryExpr(std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs, TokenType op, int line = 0, int column = 0)
        : Expr(line, column), lhs(std::move(lhs)), rhs(std::move(rhs)), op(op) {}

    std::string toString() const override;
};

struct UnaryExpr : Expr {
    std::unique_ptr<Expr> operand;
    TokenType op;

    UnaryExpr(std::unique_ptr<Expr> operand, TokenType op, int line = 0, int column = 0)
        : Expr(line, column), operand(std::move(operand)), op(op) {}

    std::string toString() const override;
};

struct Literal : Expr {
    Token token;

    explicit Literal(const Token& token) : Expr(token.line, token.column), token(token) {}

    std::string toString() const override;
};

struct Variable : Expr {
    Token token;

    explicit Variable(const Token& token) : Expr(token.line, token.column), token(token) {}

    std::string toString() const override;
};

struct Assignment : Expr {
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> value;

    Assignment(std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> value, int line = 0, int column = 0)
        : Expr(line, column), lhs(std::move(lhs)), value(std::move(value)) {}

    std::string toString() const override;
};

struct GroupingExpr : Expr {
    std::unique_ptr<Expr> expr;

    explicit GroupingExpr(std::unique_ptr<Expr> expr, int line = 0, int column = 0)
        : Expr(line, column), expr(std::move(expr)) {}

    std::string toString() const override;
};

struct ArrayLiteral : Expr {
    std::vector<std::unique_ptr<Expr>> elements;
    std::unique_ptr<Expr> repeat_value;
    std::optional<int> repeat_count;
    std::vector<int> array_dimensions;
    explicit ArrayLiteral(std::vector<std::unique_ptr<Expr>> elements, int line = 0, int column = 0)
        : Expr(line, column), elements(std::move(elements)), repeat_value(nullptr), repeat_count(std::nullopt), array_dimensions() {}

    ArrayLiteral(std::unique_ptr<Expr> repeat_value, int repeat_count, int line = 0, int column = 0)
        : Expr(line, column), elements(), repeat_value(std::move(repeat_value)), repeat_count(repeat_count), array_dimensions() {}

    std::string toString() const override;
};

struct IndexExpr : Expr {
    std::unique_ptr<Expr> array;
    std::unique_ptr<Expr> index;

    IndexExpr(std::unique_ptr<Expr> array, std::unique_ptr<Expr> index, int line = 0, int column = 0)
        : Expr(line, column), array(std::move(array)), index(std::move(index)) {}

    std::string toString() const override;
};

struct AddrOf : Expr {
    std::unique_ptr<Expr> operand;
    AddrOf(std::unique_ptr<Expr> operand, size_t line = 0, size_t column = 0) 
        : Expr(line, column), operand(std::move(operand)) {}

    std::string toString() const override;
};

struct CompoundAssign : Expr {
    std::unique_ptr<Variable> var;
    std::unique_ptr<Expr> value;
    TokenType op;

    CompoundAssign(std::unique_ptr<Variable> var, std::unique_ptr<Expr> value, TokenType op, int line = 0, int column = 0)
        : Expr(line, column), var(std::move(var)), value(std::move(value)), op(op) {}

    std::string toString() const override;
};

struct Increment : Expr {
    std::unique_ptr<Variable> var;

    explicit Increment(std::unique_ptr<Variable> var, int line = 0, int column = 0)
        : Expr(line, column), var(std::move(var)) {}

    std::string toString() const override;
};

struct Decrement : Expr {
    std::unique_ptr<Variable> var;

    explicit Decrement(std::unique_ptr<Variable> var, int line = 0, int column = 0)
        : Expr(line, column), var(std::move(var)) {}

    std::string toString() const override;
};

struct Range : Expr {
    std::unique_ptr<Expr> from;
    std::unique_ptr<Expr> to;
    bool inclusive;  // true for ..=, false for ..

    explicit Range(std::unique_ptr<Expr> from,  std::unique_ptr<Expr> to, bool inclusive = false, int line = 0, int column = 0)
        : Expr(line, column), from(std::move(from)), to(std::move(to)), inclusive(inclusive) {}

    std::string toString() const override;
};

struct Param {
    std::string name;
    std::unique_ptr<Type> type;
    bool isRef;
    bool isMutRef;

    Param(const std::string& name, std::unique_ptr<Type> type, bool isRef = false, bool isMutRef = false)
        : name(name), type(std::move(type)), isRef(isRef), isMutRef(isMutRef) {}

    std::string toString() const;
};

struct VarDecl : Stmt {
    bool mutable_;
    Token name;
    std::unique_ptr<Type> type_annotation;
    std::unique_ptr<Expr> init_value;
    std::vector<int> array_dimensions;

    VarDecl(bool mutable_, const Token& name, std::unique_ptr<Type> type_annotation,
            std::unique_ptr<Expr> init_value)
        : Stmt(name.line, name.column), mutable_(mutable_), name(name), type_annotation(std::move(type_annotation)),
          init_value(std::move(init_value)), array_dimensions() {}

    std::string toString() const override;
};

struct FnDecl : Stmt {
    std::string name;
    std::vector<Param> params;
    std::unique_ptr<Type> returnType;
    std::vector<std::unique_ptr<Stmt>> body;
    bool isExtern;

    FnDecl(const std::string& name, std::vector<Param> params,
           std::unique_ptr<Type> returnType, std::vector<std::unique_ptr<Stmt>> body, 
           bool isExtern = false, int line = 0, int column = 0)
        : Stmt(line, column), name(name), params(std::move(params)), returnType(std::move(returnType)),
          body(std::move(body)), isExtern(isExtern) {}

    std::string toString() const override;
};

struct ReturnStmt : Stmt {
    std::unique_ptr<Expr> value;

    explicit ReturnStmt(std::unique_ptr<Expr> value, int line = 0, int column = 0)
        : Stmt(line, column), value(std::move(value)) {}

    std::string toString() const override;
};

struct IfStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::vector<std::unique_ptr<Stmt>> thenBody;
    std::vector<std::unique_ptr<Stmt>> elseBody;

    IfStmt(std::unique_ptr<Expr> condition,
           std::vector<std::unique_ptr<Stmt>> thenBody,
           std::vector<std::unique_ptr<Stmt>> elseBody = {}, int line = 0, int column = 0)
        : Stmt(line, column), condition(std::move(condition)), thenBody(std::move(thenBody)),
          elseBody(std::move(elseBody)) {}

    std::string toString() const override;
};

struct ExprStmt : Stmt {
    std::unique_ptr<Expr> expr;

    explicit ExprStmt(std::unique_ptr<Expr> expr, int line = 0, int column = 0)
        : Stmt(line, column), expr(std::move(expr)) {}

    std::string toString() const override;
};

struct WhileStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::vector<std::unique_ptr<Stmt>> thenBody;

    WhileStmt(std::unique_ptr<Expr> condition, std::vector<std::unique_ptr<Stmt>> thenBody, int line = 0, int column = 0)
        : Stmt(line, column), condition(std::move(condition)), thenBody(std::move(thenBody)) {}

    std::string toString() const override;
};

struct ForStmt : Stmt {
    std::unique_ptr<Variable> var;
    std::unique_ptr<Range> range;
    std::vector<std::unique_ptr<Stmt>> body;

    ForStmt(std::unique_ptr<Variable> var, std::unique_ptr<Range> range, std::vector<std::unique_ptr<Stmt>> body, int line = 0, int column = 0)
        : Stmt(line, column), var(std::move(var)), range(std::move(range)), body(std::move(body)) {}

    std::string toString() const override;
};

struct BlockStmt : Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;

    explicit BlockStmt(std::vector<std::unique_ptr<Stmt>> statements, int line = 0, int column = 0)
        : Stmt(line, column), statements(std::move(statements)) {}

    std::string toString() const override;
};

struct BreakStmt : Stmt {
    std::string toString() const override;
};


struct ContinueStmt : Stmt {
    std::string toString() const override;
};

struct ExternBlock : Stmt {
    std::string abi;  // e.g., "C"
    std::vector<std::unique_ptr<FnDecl>> declarations;

    ExternBlock(const std::string& abi, std::vector<std::unique_ptr<FnDecl>> declarations, int line = 0, int column = 0)
        : Stmt(line, column), abi(abi), declarations(std::move(declarations)) {}

    std::string toString() const override;
};


struct Program : ASTNode {
    std::vector<std::unique_ptr<Stmt>> statements;

    Program() = default;

    Program(const Program&) = delete;
    Program& operator=(const Program&) = delete;

    Program(Program&&) = default;
    Program& operator=(Program&&) = default;

    void add_statement(std::unique_ptr<Stmt> stmt) {
        statements.push_back(std::move(stmt));
    }

    std::string toString() const override;
};