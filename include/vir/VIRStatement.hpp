#pragma once

#include "vir/VIRNode.hpp"
#include "vir/VIRExpression.hpp"
#include <vector>
#include <string>
#include <memory>

namespace volta::vir {

// ============================================================================
// Statements
// ============================================================================

class VIRBlock : public VIRStmt {
public:
    VIRBlock(volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    void addStatement(std::unique_ptr<VIRStmt> stmt);
    const std::vector<std::unique_ptr<VIRStmt>>& getStatements() const { return statements_; }

private:
    std::vector<std::unique_ptr<VIRStmt>> statements_;
};

class VIRVarDecl : public VIRStmt {
public:
    VIRVarDecl(std::string name, std::unique_ptr<VIRExpr> initializer, bool isMutable,
               std::shared_ptr<volta::semantic::Type> type,
               volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const std::string& getName() const { return name_; }
    const VIRExpr* getInitializer() const { return initializer_.get(); }
    bool isMutable() const { return isMutable_; }
    std::shared_ptr<volta::semantic::Type> getType() const { return type_; }

private:
    std::string name_;
    std::unique_ptr<VIRExpr> initializer_;
    bool isMutable_;
    std::shared_ptr<volta::semantic::Type> type_;
};

class VIRReturn : public VIRStmt {
public:
    VIRReturn(std::unique_ptr<VIRExpr> value,
              volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const VIRExpr* getValue() const { return value_.get(); }

private:
    std::unique_ptr<VIRExpr> value_;  // nullptr for void return
};

class VIRIfStmt : public VIRStmt {
public:
    VIRIfStmt(std::unique_ptr<VIRExpr> condition, std::unique_ptr<VIRBlock> thenBlock,
              std::unique_ptr<VIRBlock> elseBlock,
              volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const VIRExpr* getCondition() const { return condition_.get(); }
    const VIRBlock* getThenBlock() const { return thenBlock_.get(); }
    const VIRBlock* getElseBlock() const { return elseBlock_.get(); }

private:
    std::unique_ptr<VIRExpr> condition_;
    std::unique_ptr<VIRBlock> thenBlock_;
    std::unique_ptr<VIRBlock> elseBlock_;  // Optional (nullptr if no else)
};

class VIRWhile : public VIRStmt {
public:
    VIRWhile(std::unique_ptr<VIRExpr> condition, std::unique_ptr<VIRBlock> body,
             volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const VIRExpr* getCondition() const { return condition_.get(); }
    const VIRBlock* getBody() const { return body_.get(); }

private:
    std::unique_ptr<VIRExpr> condition_;
    std::unique_ptr<VIRBlock> body_;
};

// For range-based loops: for i in start..end or for i in start..=end
class VIRForRange : public VIRStmt {
public:
    VIRForRange(std::string loopVar, std::unique_ptr<VIRExpr> start, std::unique_ptr<VIRExpr> end,
                bool inclusive, std::unique_ptr<VIRBlock> body,
                std::shared_ptr<volta::semantic::Type> varType,
                volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const std::string& getLoopVar() const { return loopVar_; }
    const VIRExpr* getStart() const { return start_.get(); }
    const VIRExpr* getEnd() const { return end_.get(); }
    bool isInclusive() const { return inclusive_; }
    const VIRBlock* getBody() const { return body_.get(); }
    std::shared_ptr<volta::semantic::Type> getVarType() const { return varType_; }

private:
    std::string loopVar_;
    std::unique_ptr<VIRExpr> start_;
    std::unique_ptr<VIRExpr> end_;
    bool inclusive_;  // true for ..=, false for ..
    std::unique_ptr<VIRBlock> body_;
    std::shared_ptr<volta::semantic::Type> varType_;
};

// For general iteration (arrays, etc.) - not implemented yet
class VIRFor : public VIRStmt {
public:
    VIRFor(std::string iteratorVar, std::unique_ptr<VIRExpr> iterable,
           std::unique_ptr<VIRBlock> body,
           volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const std::string& getIteratorVar() const { return iteratorVar_; }
    const VIRExpr* getIterable() const { return iterable_.get(); }
    const VIRBlock* getBody() const { return body_.get(); }

private:
    std::string iteratorVar_;
    std::unique_ptr<VIRExpr> iterable_;
    std::unique_ptr<VIRBlock> body_;
};

class VIRBreak : public VIRStmt {
public:
    VIRBreak(volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);
};

class VIRContinue : public VIRStmt {
public:
    VIRContinue(volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);
};

class VIRExprStmt : public VIRStmt {
public:
    VIRExprStmt(std::unique_ptr<VIRExpr> expr,
                volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const VIRExpr* getExpr() const { return expr_.get(); }

private:
    std::unique_ptr<VIRExpr> expr_;
};

} // namespace volta::vir
