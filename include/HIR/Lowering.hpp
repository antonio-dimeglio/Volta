#pragma once

#include "HIR.hpp"
#include "../Parser/AST.hpp"
#include <memory>
#include <vector>

class HIRLowering {
public:
    HIRLowering() = default;

    // Lower entire program
    Program lower(Program ast);

private:
    // Lower statements
    std::unique_ptr<Stmt> lowerStmt(std::unique_ptr<Stmt> stmt);
    std::unique_ptr<Stmt> lowerVarDecl(std::unique_ptr<VarDecl> node);
    std::unique_ptr<Stmt> lowerFnDecl(std::unique_ptr<FnDecl> node);
    std::unique_ptr<Stmt> lowerReturnStmt(std::unique_ptr<ReturnStmt> node);
    std::unique_ptr<Stmt> lowerIfStmt(std::unique_ptr<IfStmt> node);
    std::unique_ptr<Stmt> lowerWhileStmt(std::unique_ptr<WhileStmt> node);
    std::unique_ptr<Stmt> lowerForStmt(std::unique_ptr<ForStmt> node);
    std::unique_ptr<Stmt> lowerExprStmt(std::unique_ptr<ExprStmt> node);
    std::unique_ptr<Stmt> lowerBlockStmt(std::unique_ptr<BlockStmt> node);

    // Lower expressions
    std::unique_ptr<Expr> lowerExpr(std::unique_ptr<Expr> expr);
    std::unique_ptr<Expr> lowerBinaryExpr(std::unique_ptr<BinaryExpr> node);
    std::unique_ptr<Expr> lowerUnaryExpr(std::unique_ptr<UnaryExpr> node);
    std::unique_ptr<Expr> lowerFnCall(std::unique_ptr<FnCall> node);
    std::unique_ptr<Expr> lowerAssignment(std::unique_ptr<Assignment> node);
    std::unique_ptr<Expr> lowerGroupingExpr(std::unique_ptr<GroupingExpr> node);
    std::unique_ptr<Expr> lowerArrayLiteral(std::unique_ptr<ArrayLiteral> node);
    std::unique_ptr<Expr> lowerIndexExpr(std::unique_ptr<IndexExpr> node);

    // Desugar compound operations
    std::unique_ptr<Expr> desugarCompoundAssign(std::unique_ptr<CompoundAssign> node);
    std::unique_ptr<Expr> desugarIncrement(std::unique_ptr<Increment> node);
    std::unique_ptr<Expr> desugarDecrement(std::unique_ptr<Decrement> node);
    std::unique_ptr<Stmt> desugarForLoop(std::unique_ptr<ForStmt> node);

    // Helper to clone a Variable node (needed for desugaring)
    std::unique_ptr<Variable> cloneVariable(const Variable* var);

    // N-dimensional array helpers
    struct ArrayDimensionInfo {
        Type* element_type;           // Base element type (e.g., i32)
        std::vector<int> dimensions;  // Dimension sizes (e.g., [3, 4] for [[T; 4]; 3])
    };

    // Analyze array type to extract dimensions and base element type
    ArrayDimensionInfo analyzeArrayDimensions(const Type* type);

    // Flatten N-D array type to 1D (e.g., [[i32; 4]; 3] -> [i32; 12])
    std::unique_ptr<Type> flattenArrayType(const Type* type);

    // Calculate flat index expression for N-D indexing
    // indices: [i, j, k], dimensions: [d1, d2, d3] -> i*(d2*d3) + j*d3 + k
    std::unique_ptr<Expr> calculateFlatIndex(std::vector<std::unique_ptr<Expr>>& indices,
                                              const std::vector<int>& dimensions);
};
