#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include "Type.hpp"
#include "../Lexer/Token.hpp"
#include "../Lexer/TokenType.hpp"
#include "ASTVisitor.hpp"

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

    // Accept for void visitor (traversal)
    virtual void accept(ExprVisitor<void>& v) = 0;

    // Accept for typed visitor (semantic analysis that returns types)
    virtual const Type::Type* accept(ExprVisitor<const Type::Type*>& v) = 0;
};

struct Stmt : ASTNode {
    Stmt() = default;
    Stmt(int line, int column) : ASTNode(line, column) {}
    virtual ~Stmt() = default;
    virtual void accept(StmtVisitor& v) = 0;
};


struct FnCall : Expr {
    std::string name;
    std::vector<std::unique_ptr<Expr>> args;

    FnCall(const std::string& name, std::vector<std::unique_ptr<Expr>> args, int line = 0, int column = 0)
        : Expr(line, column), name(name), args(std::move(args)) {}

    std::string toString() const override;

    void accept(ExprVisitor<void>& v) override {
        v.visitFnCall(*this);
    }

    const Type::Type* accept(ExprVisitor<const Type::Type*>& v) override {
        return v.visitFnCall(*this);
    }
};

// Static method call: Type::method(args)
struct StaticMethodCall : Expr {
    Token typeName;       // The type name (e.g., "Point")
    Token methodName;     // The method name (e.g., "new")
    std::vector<std::unique_ptr<Expr>> args;

    StaticMethodCall(Token type, Token method, std::vector<std::unique_ptr<Expr>> args, int line = 0, int column = 0)
        : Expr(line, column), typeName(type), methodName(method), args(std::move(args)) {}

    std::string toString() const override;

    void accept(ExprVisitor<void>& v) override {
        v.visitStaticMethodCall(*this);
    }

    const Type::Type* accept(ExprVisitor<const Type::Type*>& v) override {
        return v.visitStaticMethodCall(*this);
    }
};

// Instance method call: object.method(args) - when method has parentheses
struct InstanceMethodCall : Expr {
    std::unique_ptr<Expr> object;     // The object expression
    Token methodName;                 // The method name
    std::vector<std::unique_ptr<Expr>> args;

    InstanceMethodCall(std::unique_ptr<Expr> obj, Token method, std::vector<std::unique_ptr<Expr>> args, int line = 0, int column = 0)
        : Expr(line, column), object(std::move(obj)), methodName(method), args(std::move(args)) {}

    std::string toString() const override;

    void accept(ExprVisitor<void>& v) override {
        v.visitInstanceMethodCall(*this);
    }

    const Type::Type* accept(ExprVisitor<const Type::Type*>& v) override {
        return v.visitInstanceMethodCall(*this);
    }
};

struct BinaryExpr : Expr {
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
    TokenType op;

    BinaryExpr(std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs, TokenType op, int line = 0, int column = 0)
        : Expr(line, column), lhs(std::move(lhs)), rhs(std::move(rhs)), op(op) {}

    std::string toString() const override;

    void accept(ExprVisitor<void>& v) override {
        v.visitBinaryExpr(*this);
    }

    const Type::Type* accept(ExprVisitor<const Type::Type*>& v) override {
        return v.visitBinaryExpr(*this);
    }
};

struct UnaryExpr : Expr {
    std::unique_ptr<Expr> operand;
    TokenType op;

    UnaryExpr(std::unique_ptr<Expr> operand, TokenType op, int line = 0, int column = 0)
        : Expr(line, column), operand(std::move(operand)), op(op) {}

    std::string toString() const override;

    void accept(ExprVisitor<void>& v) override {
        v.visitUnaryExpr(*this);
    }

    const Type::Type* accept(ExprVisitor<const Type::Type*>& v) override {
        return v.visitUnaryExpr(*this);
    }
};

struct Literal : Expr {
    Token token;

    explicit Literal(const Token& token) : Expr(token.line, token.column), token(token) {}

    std::string toString() const override;
    
    void accept(ExprVisitor<void>& v) override {
        v.visitLiteral(*this);
    }

    const Type::Type* accept(ExprVisitor<const Type::Type*>& v) override {
        return v.visitLiteral(*this);
    }
};

struct Variable : Expr {
    Token token;

    explicit Variable(const Token& token) : Expr(token.line, token.column), token(token) {}

    std::string toString() const override;

    void accept(ExprVisitor<void>& v) override {
        v.visitVariable(*this);
    }

    const Type::Type* accept(ExprVisitor<const Type::Type*>& v) override {
        return v.visitVariable(*this);
    }
};

struct Assignment : Expr {
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> value;

    Assignment(std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> value, int line = 0, int column = 0)
        : Expr(line, column), lhs(std::move(lhs)), value(std::move(value)) {}

    std::string toString() const override;
    
    void accept(ExprVisitor<void>& v) override {
        v.visitAssignment(*this);
    }

    const Type::Type* accept(ExprVisitor<const Type::Type*>& v) override {
        return v.visitAssignment(*this);
    }
};

struct GroupingExpr : Expr {
    std::unique_ptr<Expr> expr;

    explicit GroupingExpr(std::unique_ptr<Expr> expr, int line = 0, int column = 0)
        : Expr(line, column), expr(std::move(expr)) {}

    std::string toString() const override;

    void accept(ExprVisitor<void>& v) override {
        v.visitGroupingExpr(*this);
    }

    const Type::Type* accept(ExprVisitor<const Type::Type*>& v) override {
        return v.visitGroupingExpr(*this);
    }
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

    void accept(ExprVisitor<void>& v) override {
        v.visitArrayLiteral(*this);
    }

    const Type::Type* accept(ExprVisitor<const Type::Type*>& v) override {
        return v.visitArrayLiteral(*this);
    }
};

struct IndexExpr : Expr {
    std::unique_ptr<Expr> array;
    std::unique_ptr<Expr> index;

    IndexExpr(std::unique_ptr<Expr> array, std::unique_ptr<Expr> index, int line = 0, int column = 0)
        : Expr(line, column), array(std::move(array)), index(std::move(index)) {}

    std::string toString() const override;

    void accept(ExprVisitor<void>& v) override {
        v.visitIndexExpr(*this);
    }

    const Type::Type* accept(ExprVisitor<const Type::Type*>& v) override {
        return v.visitIndexExpr(*this);
    }
};

struct AddrOf : Expr {
    std::unique_ptr<Expr> operand;
    AddrOf(std::unique_ptr<Expr> operand, size_t line = 0, size_t column = 0) 
        : Expr(line, column), operand(std::move(operand)) {}

    std::string toString() const override;

    void accept(ExprVisitor<void>& v) override {
        v.visitAddrOf(*this);
    }

    const Type::Type* accept(ExprVisitor<const Type::Type*>& v) override {
        return v.visitAddrOf(*this);
    }
};

struct CompoundAssign : Expr {
    std::unique_ptr<Variable> var;
    std::unique_ptr<Expr> value;
    TokenType op;

    CompoundAssign(std::unique_ptr<Variable> var, std::unique_ptr<Expr> value, TokenType op, int line = 0, int column = 0)
        : Expr(line, column), var(std::move(var)), value(std::move(value)), op(op) {}

    std::string toString() const override;

    void accept(ExprVisitor<void>& v) override {
        v.visitCompoundAssign(*this);
    }

    const Type::Type* accept(ExprVisitor<const Type::Type*>& v) override {
        return v.visitCompoundAssign(*this);
    }
};

struct Increment : Expr {
    std::unique_ptr<Variable> var;

    explicit Increment(std::unique_ptr<Variable> var, int line = 0, int column = 0)
        : Expr(line, column), var(std::move(var)) {}

    std::string toString() const override;

    void accept(ExprVisitor<void>& v) override {
        v.visitIncrement(*this);
    }

    const Type::Type* accept(ExprVisitor<const Type::Type*>& v) override {
        return v.visitIncrement(*this);
    }
};

struct Decrement : Expr {
    std::unique_ptr<Variable> var;

    explicit Decrement(std::unique_ptr<Variable> var, int line = 0, int column = 0)
        : Expr(line, column), var(std::move(var)) {}

    std::string toString() const override;

    void accept(ExprVisitor<void>& v) override {
        v.visitDecrement(*this);
    }

    const Type::Type* accept(ExprVisitor<const Type::Type*>& v) override {
        return v.visitDecrement(*this);
    }
};

struct Range : Expr {
    std::unique_ptr<Expr> from;
    std::unique_ptr<Expr> to;
    bool inclusive;

    explicit Range(std::unique_ptr<Expr> from,  std::unique_ptr<Expr> to, bool inclusive = false, int line = 0, int column = 0)
        : Expr(line, column), from(std::move(from)), to(std::move(to)), inclusive(inclusive) {}

    std::string toString() const override;

    void accept(ExprVisitor<void>& v) override {
        v.visitRange(*this);
    }

    const Type::Type* accept(ExprVisitor<const Type::Type*>& v) override {
        return v.visitRange(*this);
    }
};

// Struct literal: Point { x: 10, y: 20 }
struct StructLiteral : Expr {
    Token structName;  // "Point"
    std::vector<std::pair<Token, std::unique_ptr<Expr>>> fields;  // [(x, 10), (y, 20)]
    const Type::Type* resolvedType = nullptr;  // Will be set during semantic analysis

    StructLiteral(Token name, std::vector<std::pair<Token, std::unique_ptr<Expr>>> fields,
                  int line, int column)
        : Expr(line, column), structName(name), fields(std::move(fields)) {}

    std::string toString() const override;

    void accept(ExprVisitor<void>& v) override {
        v.visitStructLiteral(*this);
    }

    const Type::Type* accept(ExprVisitor<const Type::Type*>& v) override {
        return v.visitStructLiteral(*this);
    }
};

// Field access: p.x
struct FieldAccess : Expr {
    std::unique_ptr<Expr> object;  // The struct instance (could be variable, field access, etc.)
    Token fieldName;  // "x"
    const Type::StructType* resolvedStructType = nullptr;  // Set during semantic analysis
    int fieldIndex = -1;  // Index of field in struct (for MIR/codegen)

    FieldAccess(std::unique_ptr<Expr> obj, Token field, int line, int column)
        : Expr(line, column), object(std::move(obj)), fieldName(field) {}

    std::string toString() const override;

    void accept(ExprVisitor<void>& v) override {
        v.visitFieldAccess(*this);
    }

    const Type::Type* accept(ExprVisitor<const Type::Type*>& v) override {
        return v.visitFieldAccess(*this);
    }
};

struct Param {
    std::string name;
    const Type::Type* type;
    bool isRef;
    bool isMutRef;

    Param(const std::string& name, const Type::Type* type, bool isRef = false, bool isMutRef = false)
        : name(name), type(std::move(type)), isRef(isRef), isMutRef(isMutRef) {}

    std::string toString() const;
};

struct StructField {
    bool isPublic;
    Token name;
    const Type::Type* type;

    StructField(bool isPub, Token name, const Type::Type* type)
        : isPublic(isPub), name(name), type(type) {}
    std::string toString() const;
};

struct VarDecl : Stmt {
    bool mutable_;
    Token name;
    const Type::Type* typeAnnotation;
    std::unique_ptr<Expr> initValue;
    std::vector<int> array_dimensions;

    VarDecl(bool mutable_, const Token& name, const Type::Type* typeAnnotation,
            std::unique_ptr<Expr> init_value)
        : Stmt(name.line, name.column), mutable_(mutable_), name(name), typeAnnotation(typeAnnotation),
          initValue(std::move(init_value)), array_dimensions() {}

    std::string toString() const override;

    void accept(StmtVisitor& v) override {
        v.visitVarDecl(*this);
    }
};

struct FnDecl : Stmt {
    std::string name;
    std::vector<Param> params;
    const Type::Type* returnType;
    std::vector<std::unique_ptr<Stmt>> body;
    bool isExtern;
    bool isPublic;

    FnDecl(const std::string& name, std::vector<Param> params,
           const Type::Type* returnType, std::vector<std::unique_ptr<Stmt>> body, 
           bool isExtern = false, bool isPublic = false, int line = 0, int column = 0)
        : Stmt(line, column), name(name), params(std::move(params)), returnType(returnType),
          body(std::move(body)), isExtern(isExtern), isPublic(isPublic) {}

    std::string toString() const override;

    void accept(StmtVisitor& v) override {
        v.visitFnDecl(*this);
    }
};

class StructDecl : public Stmt {
public:
    bool isPublic;
    Token name;
    std::vector<StructField> fields;
    std::vector<std::unique_ptr<FnDecl>> methods;  // Methods defined in struct body

    StructDecl(bool isPub, Token name, std::vector<StructField> fields)
        : isPublic(isPub), name(name), fields(std::move(fields)) {}

    StructDecl(bool isPub, Token name, std::vector<StructField> fields,
               std::vector<std::unique_ptr<FnDecl>> methods)
        : isPublic(isPub), name(name), fields(std::move(fields)), methods(std::move(methods)) {}

    std::string toString() const override;

    void accept(StmtVisitor& visitor) override {
        visitor.visitStructDecl(*this);
    }
};

struct ReturnStmt : Stmt {
    std::unique_ptr<Expr> value;

    explicit ReturnStmt(std::unique_ptr<Expr> value, int line = 0, int column = 0)
        : Stmt(line, column), value(std::move(value)) {}

    std::string toString() const override;

    void accept(StmtVisitor& v) override {
        v.visitReturnStmt(*this);
    }
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

    void accept(StmtVisitor& v) override {
        v.visitIfStmt(*this);
    }
};

struct ExprStmt : Stmt {
    std::unique_ptr<Expr> expr;

    explicit ExprStmt(std::unique_ptr<Expr> expr, int line = 0, int column = 0)
        : Stmt(line, column), expr(std::move(expr)) {}

    std::string toString() const override;


    void accept(StmtVisitor& v) override {
        v.visitExprStmt(*this);
    }
};

struct WhileStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::vector<std::unique_ptr<Stmt>> thenBody;

    WhileStmt(std::unique_ptr<Expr> condition, std::vector<std::unique_ptr<Stmt>> thenBody, int line = 0, int column = 0)
        : Stmt(line, column), condition(std::move(condition)), thenBody(std::move(thenBody)) {}

    std::string toString() const override;

    void accept(StmtVisitor& v) override {
        v.visitWhileStmt(*this);
    }
};

struct ForStmt : Stmt {
    std::unique_ptr<Variable> var;
    std::unique_ptr<Range> range;
    std::vector<std::unique_ptr<Stmt>> body;

    ForStmt(std::unique_ptr<Variable> var, std::unique_ptr<Range> range, std::vector<std::unique_ptr<Stmt>> body, int line = 0, int column = 0)
        : Stmt(line, column), var(std::move(var)), range(std::move(range)), body(std::move(body)) {}

    std::string toString() const override;

    void accept(StmtVisitor& v) override {
        v.visitForStmt(*this);
    }
};

struct BlockStmt : Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;

    explicit BlockStmt(std::vector<std::unique_ptr<Stmt>> statements, int line = 0, int column = 0)
        : Stmt(line, column), statements(std::move(statements)) {}

    std::string toString() const override;

    void accept(StmtVisitor& v) override {
        v.visitBlockStmt(*this);
    }
};

struct BreakStmt : Stmt {
    std::string toString() const override;

    void accept(StmtVisitor& v) override {
        v.visitBreakStmt(*this);
    }
};


struct ContinueStmt : Stmt {
    std::string toString() const override;

    void accept(StmtVisitor& v) override {
        v.visitContinueStmt(*this);
    }
};

struct ExternBlock : Stmt {
    std::string abi;
    std::vector<std::unique_ptr<FnDecl>> declarations;

    ExternBlock(const std::string& abi, std::vector<std::unique_ptr<FnDecl>> declarations, int line = 0, int column = 0)
        : Stmt(line, column), abi(abi), declarations(std::move(declarations)) {}

    std::string toString() const override;

    void accept(StmtVisitor& v) override {
        v.visitExternBlock(*this);
    }
};

struct ImportStmt : Stmt {
    std::string modulePath; 
    std::vector<std::string> importedItems;

    ImportStmt(const std::string modulePath, std::vector<std::string> importedItems, int line = 0, int column = 0)
        : Stmt(line, column), modulePath(modulePath), importedItems(importedItems) {}

    std::string toString() const override;

    void accept(StmtVisitor& v) override {
        v.visitImportStmt(*this);
    }
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