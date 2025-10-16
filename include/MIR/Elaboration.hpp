#pragma once

#include "../Parser/AST.hpp"
#include <memory>
#include <vector>
#include <map>

// Forward declarations
namespace Semantic {
    class SymbolTable;
}

// MIR Elaboration: Type-directed transformations on typed AST
//
// This phase runs after semantic analysis and before codegen.
// It performs transformations that require type information but
// are too complex for codegen to handle directly.
//
// Transformations performed:
// - Multi-dimensional array indexing → flat indexing with calculated offsets
// - (Future) Method/operator desugaring based on types
// - (Future) Bounds checking insertion
// - (Future) Generic instantiation/monomorphization
// - (Future) Automatic type conversions/coercions
// - (Future) Pattern matching lowering
//
// Input:  Typed AST (after semantic analysis) + SymbolTable
// Output: Elaborated AST ready for codegen

class MIRElaboration {
public:
    MIRElaboration(Semantic::SymbolTable* symTable);

    // Elaborate entire program
    Program elaborate(Program ast);

private:
    Semantic::SymbolTable* symbolTable;  // Access to type information

    // Symbol table for storing variable dimension metadata
    // Maps variable name → array dimensions (empty if scalar/1D)
    std::map<std::string, std::vector<int>> variableDimensions;

    // Elaborate statements
    std::unique_ptr<Stmt> elaborateStmt(std::unique_ptr<Stmt> stmt);
    std::unique_ptr<Stmt> elaborateVarDecl(std::unique_ptr<VarDecl> node);
    std::unique_ptr<Stmt> elaborateFnDecl(std::unique_ptr<FnDecl> node);
    std::unique_ptr<Stmt> elaborateReturnStmt(std::unique_ptr<ReturnStmt> node);
    std::unique_ptr<Stmt> elaborateIfStmt(std::unique_ptr<IfStmt> node);
    std::unique_ptr<Stmt> elaborateWhileStmt(std::unique_ptr<WhileStmt> node);
    std::unique_ptr<Stmt> elaborateBlockStmt(std::unique_ptr<BlockStmt> node);
    std::unique_ptr<Stmt> elaborateExprStmt(std::unique_ptr<ExprStmt> node);

    // Elaborate expressions
    std::unique_ptr<Expr> elaborateExpr(std::unique_ptr<Expr> expr);
    std::unique_ptr<Expr> elaborateBinaryExpr(std::unique_ptr<BinaryExpr> node);
    std::unique_ptr<Expr> elaborateUnaryExpr(std::unique_ptr<UnaryExpr> node);
    std::unique_ptr<Expr> elaborateFnCall(std::unique_ptr<FnCall> node);
    std::unique_ptr<Expr> elaborateAssignment(std::unique_ptr<Assignment> node);
    std::unique_ptr<Expr> elaborateGroupingExpr(std::unique_ptr<GroupingExpr> node);
    std::unique_ptr<Expr> elaborateArrayLiteral(std::unique_ptr<ArrayLiteral> node);
    std::unique_ptr<Expr> elaborateIndexExpr(std::unique_ptr<IndexExpr> node);

    // Multi-dimensional array transformations

    // Check if an IndexExpr is a chained multi-dimensional access (e.g., matrix[i][j])
    bool isMultiDimensionalIndexing(const IndexExpr* expr);

    // Collect all indices from a chained IndexExpr
    // Returns: (base variable name, [index1, index2, ...])
    struct IndexChain {
        std::string baseVariable;
        std::vector<Expr*> indices; 
    };
    IndexChain collectIndices(std::unique_ptr<IndexExpr> expr);

    // Calculate flat index expression from multi-dimensional indices
    // e.g., [i, j] with dimensions [3, 4] → i * 4 + j
    std::unique_ptr<Expr> calculateFlatIndex(
        std::vector<std::unique_ptr<Expr>>& indices,
        const std::vector<int>& dimensions
    );

    // Transform multi-dimensional indexing to flat indexing
    // matrix[i][j] → matrix[i * cols + j]
    std::unique_ptr<IndexExpr> transformMultiDimIndexing(std::unique_ptr<IndexExpr> node);

    // Helper: Clone an expression (needed for index calculations)
    std::unique_ptr<Expr> cloneExpr(const Expr* expr);

    // Helper: Flatten N-D array type to 1D
    std::unique_ptr<Type> flattenArrayType(const Type* type);
};
