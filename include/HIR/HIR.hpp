#pragma once

#include "../Type/Type.hpp"
#include "../Parser/AST.hpp"
#include <memory>
#include <utility>
#include <utility>
#include <vector>
#include <string>

namespace HIR {
class HIRStmtVisitor;

struct HIRStmt {
    int line = 0;
    int column = 0;

    HIRStmt(int line = 0, int column = 0) : line(line), column(column) {}
    virtual ~HIRStmt() = default;

    virtual void accept(HIRStmtVisitor& visitor) = 0;
    [[nodiscard]] virtual std::string toString() const = 0;
};

struct HIRVarDecl : HIRStmt {
    Token name;
    const Type::Type* typeAnnotation;
    std::unique_ptr<Expr> initValue;
    bool mutable_;

    HIRVarDecl(Token name, const Type::Type* type, std::unique_ptr<Expr> init, bool mut,
               int line = 0, int column = 0)
        : HIRStmt(line, column), name(std::move(std::move(name))), typeAnnotation(type),
          initValue(std::move(init)), mutable_(mut) {}

    void accept(HIRStmtVisitor& visitor) override;
    [[nodiscard]] std::string toString() const override { return "HIRVarDecl"; }
};

struct HIRExprStmt : HIRStmt {
    std::unique_ptr<Expr> expr;

    explicit HIRExprStmt(std::unique_ptr<Expr> expr, int line = 0, int column = 0)
        : HIRStmt(line, column), expr(std::move(expr)) {}

    void accept(HIRStmtVisitor& visitor) override;
    [[nodiscard]] std::string toString() const override { return "HIRExprStmt"; }
};

struct HIRReturnStmt : HIRStmt {
    std::unique_ptr<Expr> value;

    explicit HIRReturnStmt(std::unique_ptr<Expr> value = nullptr,
                           int line = 0, int column = 0)
        : HIRStmt(line, column), value(std::move(value)) {}

    void accept(HIRStmtVisitor& visitor) override;
    [[nodiscard]] std::string toString() const override { return "HIRReturnStmt"; }
};

struct HIRIfStmt : HIRStmt {
    std::unique_ptr<Expr> condition;
    std::vector<std::unique_ptr<HIRStmt>> thenBody;
    std::vector<std::unique_ptr<HIRStmt>> elseBody;

    HIRIfStmt(std::unique_ptr<Expr> cond,
              std::vector<std::unique_ptr<HIRStmt>> thenB,
              std::vector<std::unique_ptr<HIRStmt>> elseB = {},
              int line = 0, int column = 0)
        : HIRStmt(line, column), condition(std::move(cond)),
          thenBody(std::move(thenB)), elseBody(std::move(elseB)) {}

    void accept(HIRStmtVisitor& visitor) override;
    [[nodiscard]] std::string toString() const override { return "HIRIfStmt"; }
};

struct HIRWhileStmt : HIRStmt {
    std::unique_ptr<Expr> condition;
    std::vector<std::unique_ptr<HIRStmt>> body;
    std::unique_ptr<Expr> increment;  // Optional (for desugared for loops)

    HIRWhileStmt(std::unique_ptr<Expr> cond,
                 std::vector<std::unique_ptr<HIRStmt>> body,
                 std::unique_ptr<Expr> incr = nullptr,
                 int line = 0, int column = 0)
        : HIRStmt(line, column), condition(std::move(cond)),
          body(std::move(body)), increment(std::move(incr)) {}

    void accept(HIRStmtVisitor& visitor) override;
    [[nodiscard]] std::string toString() const override { return "HIRWhileStmt"; }
};

struct HIRBlockStmt : HIRStmt {
    std::vector<std::unique_ptr<HIRStmt>> statements;

    explicit HIRBlockStmt(std::vector<std::unique_ptr<HIRStmt>> stmts,
                          int line = 0, int column = 0)
        : HIRStmt(line, column), statements(std::move(stmts)) {}

    void accept(HIRStmtVisitor& visitor) override;
    [[nodiscard]] std::string toString() const override { return "HIRBlockStmt"; }
};

struct HIRBreakStmt : HIRStmt {
    HIRBreakStmt(int line = 0, int column = 0) : HIRStmt(line, column) {}

    void accept(HIRStmtVisitor& visitor) override;
    [[nodiscard]] std::string toString() const override { return "HIRBreakStmt"; }
};

struct HIRContinueStmt : HIRStmt {
    HIRContinueStmt(int line = 0, int column = 0) : HIRStmt(line, column) {}

    void accept(HIRStmtVisitor& visitor) override;
    [[nodiscard]] std::string toString() const override { return "HIRContinueStmt"; }
};

struct Param {
    std::string name;
    const Type::Type* type;
    bool isRef;
    bool isMutRef;

    Param(std::string n, const Type::Type* t, bool ref = false, bool mutRef = false)
        : name(std::move(std::move(n))), type(t), isRef(ref), isMutRef(mutRef) {}
};

struct HIRFnDecl : HIRStmt {
    std::string name;
    std::vector<Param> params;
    const Type::Type* returnType;
    std::vector<std::unique_ptr<HIRStmt>> body;
    bool isExtern;
    bool isPublic;

    HIRFnDecl(std::string name, std::vector<Param> params,
              const Type::Type* retType, std::vector<std::unique_ptr<HIRStmt>> body,
              bool isExtern = false, bool isPub = false, int line = 0, int column = 0)
        : HIRStmt(line, column), name(std::move(std::move(name))), params(std::move(params)),
          returnType(retType), body(std::move(body)), isExtern(isExtern), isPublic(isPub) {}

    void accept(HIRStmtVisitor& visitor) override;
    [[nodiscard]] std::string toString() const override { return "HIRFnDecl: " + name; }
};

struct HIRExternBlock : HIRStmt {
    std::vector<std::unique_ptr<HIRFnDecl>> declarations;

    explicit HIRExternBlock(std::vector<std::unique_ptr<HIRFnDecl>> decls,
                            int line = 0, int column = 0)
        : HIRStmt(line, column), declarations(std::move(decls)) {}

    void accept(HIRStmtVisitor& visitor) override;
    [[nodiscard]] std::string toString() const override { return "HIRExternBlock"; }
};

struct HIRImportStmt : HIRStmt {
    std::string modulePath;
    std::vector<std::string> symbols;  // Empty means import all

    HIRImportStmt(std::string path, std::vector<std::string> syms = {},
                  int line = 0, int column = 0)
        : HIRStmt(line, column), modulePath(std::move(std::move(path))), symbols(std::move(syms)) {}

    void accept(HIRStmtVisitor& visitor) override;
    [[nodiscard]] std::string toString() const override { return "HIRImportStmt"; }
};

struct HIRStructDecl : HIRStmt {
    bool isPublic;
    Token name;
    std::vector<StructField> fields;
    std::vector<std::unique_ptr<HIRFnDecl>> methods;  // Methods defined in struct body

    HIRStructDecl(bool isPub, Token name, std::vector<StructField> fields,
                  int line = 0, int column = 0)
        : HIRStmt(line, column), isPublic(isPub), name(std::move(std::move(name))), fields(std::move(fields)) {}

    HIRStructDecl(bool isPub, Token name, std::vector<StructField> fields,
                  std::vector<std::unique_ptr<HIRFnDecl>> methods,
                  int line = 0, int column = 0)
        : HIRStmt(line, column), isPublic(isPub), name(std::move(std::move(name))), fields(std::move(fields)),
          methods(std::move(methods)) {}

    void accept(HIRStmtVisitor& visitor) override;
    [[nodiscard]] std::string toString() const override { return "HIRStructDecl"; }
};
struct HIRProgram {
    std::vector<std::unique_ptr<HIRStmt>> statements;

    HIRProgram() = default;
    explicit HIRProgram(std::vector<std::unique_ptr<HIRStmt>> stmts)
        : statements(std::move(stmts)) {}
};
class HIRStmtVisitor {
public:
    virtual ~HIRStmtVisitor() = default;

    // Statement visitors
    virtual void visitVarDecl(HIRVarDecl& node) = 0;
    virtual void visitExprStmt(HIRExprStmt& node) = 0;
    virtual void visitReturnStmt(HIRReturnStmt& node) = 0;
    virtual void visitIfStmt(HIRIfStmt& node) = 0;
    virtual void visitWhileStmt(HIRWhileStmt& node) = 0;
    virtual void visitBlockStmt(HIRBlockStmt& node) = 0;
    virtual void visitBreakStmt(HIRBreakStmt& node) = 0;
    virtual void visitContinueStmt(HIRContinueStmt& node) = 0;
    virtual void visitFnDecl(HIRFnDecl& node) = 0;
    virtual void visitExternBlock(HIRExternBlock& node) = 0;
    virtual void visitImportStmt(HIRImportStmt& node) = 0;
    virtual void visitStructDecl(HIRStructDecl& node) = 0;
};

} // namespace HIR
