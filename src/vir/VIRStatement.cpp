#include "vir/VIRStatement.hpp"

namespace volta::vir {

// ============================================================================
// Statements - Constructors
// ============================================================================

VIRBlock::VIRBlock(volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRStmt(loc, ast) {}

void VIRBlock::addStatement(std::unique_ptr<VIRStmt> stmt) {
    statements_.push_back(std::move(stmt));
}

VIRVarDecl::VIRVarDecl(std::string name, std::unique_ptr<VIRExpr> initializer, bool isMutable,
                        std::shared_ptr<volta::semantic::Type> type,
                        volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRStmt(loc, ast), name_(std::move(name)), initializer_(std::move(initializer)),
      isMutable_(isMutable), type_(type) {}

VIRReturn::VIRReturn(std::unique_ptr<VIRExpr> value,
                      volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRStmt(loc, ast), value_(std::move(value)) {}

VIRIfStmt::VIRIfStmt(std::unique_ptr<VIRExpr> condition, std::unique_ptr<VIRBlock> thenBlock,
                      std::unique_ptr<VIRBlock> elseBlock,
                      volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRStmt(loc, ast), condition_(std::move(condition)), thenBlock_(std::move(thenBlock)),
      elseBlock_(std::move(elseBlock)) {}

VIRWhile::VIRWhile(std::unique_ptr<VIRExpr> condition, std::unique_ptr<VIRBlock> body,
                    volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRStmt(loc, ast), condition_(std::move(condition)), body_(std::move(body)) {}

VIRForRange::VIRForRange(std::string loopVar, std::unique_ptr<VIRExpr> start, std::unique_ptr<VIRExpr> end,
                          bool inclusive, std::unique_ptr<VIRBlock> body,
                          std::shared_ptr<volta::semantic::Type> varType,
                          volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRStmt(loc, ast), loopVar_(std::move(loopVar)), start_(std::move(start)), end_(std::move(end)),
      inclusive_(inclusive), body_(std::move(body)), varType_(varType) {}

VIRFor::VIRFor(std::string iteratorVar, std::unique_ptr<VIRExpr> iterable,
                std::unique_ptr<VIRBlock> body,
                volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRStmt(loc, ast), iteratorVar_(std::move(iteratorVar)), iterable_(std::move(iterable)),
      body_(std::move(body)) {}

VIRBreak::VIRBreak(volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRStmt(loc, ast) {}

VIRContinue::VIRContinue(volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRStmt(loc, ast) {}

VIRExprStmt::VIRExprStmt(std::unique_ptr<VIRExpr> expr,
                          volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRStmt(loc, ast), expr_(std::move(expr)) {}

} // namespace volta::vir
