#pragma once

#include "../Parser/AST.hpp"
#include <memory>
#include <vector>

// HIR (High-level Intermediate Representation)
// This is a desugared version of the AST where:
// - Compound assignments (+=, -=, etc.) are expanded to simple assignments
// - Increment/decrement (++, --) are expanded to assignments
// - For loops are expanded to while loops with optional increment
// - Other syntactic sugar is removed

namespace HIR {


struct HIRStmt : Stmt {
    using Stmt::Stmt;
};

struct HIRWhileStmt : HIRStmt {
    std::unique_ptr<Expr> condition;
    std::vector<std::unique_ptr<Stmt>> thenBody;
    std::unique_ptr<Expr> increment; 

    HIRWhileStmt(std::unique_ptr<Expr> condition,
                 std::vector<std::unique_ptr<Stmt>> thenBody,
                 std::unique_ptr<Expr> increment = nullptr,
                 int line = 0, int column = 0)
        : HIRStmt(line, column),
          condition(std::move(condition)),
          thenBody(std::move(thenBody)),
          increment(std::move(increment)) {}

    std::string toString() const override {
        std::string result = "HIRWhileStmt(condition: " + condition->toString();
        result += ", body: [";
        for (size_t i = 0; i < thenBody.size(); i++) {
            if (i > 0) result += ", ";
            result += thenBody[i]->toString();
        }
        result += "]";
        if (increment) {
            result += ", increment: " + increment->toString();
        }
        result += ")";
        return result;
    }
};

struct HIRIfStmt : HIRStmt {
    std::unique_ptr<Expr> condition;
    std::vector<std::unique_ptr<Stmt>> thenBody;
    std::vector<std::unique_ptr<Stmt>> elseBody;

    HIRIfStmt(std::unique_ptr<Expr> condition,
              std::vector<std::unique_ptr<Stmt>> thenBody,
              std::vector<std::unique_ptr<Stmt>> elseBody = {},
              int line = 0, int column = 0)
        : HIRStmt(line, column),
          condition(std::move(condition)),
          thenBody(std::move(thenBody)),
          elseBody(std::move(elseBody)) {}

    std::string toString() const override {
        return "HIRIfStmt";
    }
};

struct HIRBlockStmt : HIRStmt {
    std::vector<std::unique_ptr<Stmt>> statements;

    explicit HIRBlockStmt(std::vector<std::unique_ptr<Stmt>> statements,
                          int line = 0, int column = 0)
        : HIRStmt(line, column), statements(std::move(statements)) {}

    std::string toString() const override {
        return "HIRBlockStmt";
    }
};

struct HIRBreakStmt : HIRStmt {
    HIRBreakStmt(int line = 0, int column = 0) : HIRStmt(line, column) {}

    std::string toString() const override {
        return "HIRBreakStmt";
    }
};

struct HIRContinueStmt : HIRStmt {
    HIRContinueStmt(int line = 0, int column = 0) : HIRStmt(line, column) {}

    std::string toString() const override {
        return "HIRContinueStmt";
    }
};

struct HIRReturnStmt : HIRStmt {
    std::unique_ptr<Expr> value;

    explicit HIRReturnStmt(std::unique_ptr<Expr> value = nullptr,
                           int line = 0, int column = 0)
        : HIRStmt(line, column), value(std::move(value)) {}

    std::string toString() const override {
        return "HIRReturnStmt";
    }
};

struct HIRExprStmt : HIRStmt {
    std::unique_ptr<Expr> expr;

    explicit HIRExprStmt(std::unique_ptr<Expr> expr,
                         int line = 0, int column = 0)
        : HIRStmt(line, column), expr(std::move(expr)) {}

    std::string toString() const override {
        return "HIRExprStmt";
    }
};

using Program = ::Program;
using Expr = ::Expr;
using Type = ::Type;

using VarDecl = ::VarDecl;
using FnDecl = ::FnDecl;

} // namespace HIR
