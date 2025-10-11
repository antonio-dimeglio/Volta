#pragma once

#include <memory>
#include "error/ErrorTypes.hpp"
#include "semantic/Type.hpp"

namespace volta::ast {
    struct ASTNode; // Forward declaration
}

namespace volta::vir {

/**
 * Base class for all VIR nodes
 *
 * VIR nodes are immutable and track their source location for error reporting.
 */
class VIRNode {
public:
    virtual ~VIRNode() = default;

    // Source location for error messages
    volta::errors::SourceLocation getLocation() const { return location_; }

    // Original AST node (for debugging)
    const volta::ast::ASTNode* getOriginatingAST() const { return originatingAST_; }

protected:
    volta::errors::SourceLocation location_;
    const volta::ast::ASTNode* originatingAST_;

    VIRNode(volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
        : location_(loc), originatingAST_(ast) {}
};

/**
 * Base class for VIR expressions (produce values)
 */
class VIRExpr : public VIRNode {
public:
    // Every expression has a concrete type
    virtual std::shared_ptr<volta::semantic::Type> getType() const = 0;

protected:
    VIRExpr(volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
        : VIRNode(loc, ast) {}
};

/**
 * Base class for VIR statements (produce side effects)
 */
class VIRStmt : public VIRNode {
protected:
    VIRStmt(volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
        : VIRNode(loc, ast) {}
};

} // namespace volta::vir
