#pragma once

#include "../Parser/AST.hpp"
#include "../HIR/HIR.hpp"
#include <memory>
#include <vector>
#include <map>
namespace Semantic {
    class SymbolTable;
}

class MIRElaboration {
public:
    MIRElaboration(Semantic::SymbolTable* symTable);

    Program elaborate(Program ast);

private:
    Semantic::SymbolTable* symbolTable;

    std::map<std::string, std::vector<int>> variableDimensions;

    std::unique_ptr<Stmt> elaborateStmt(std::unique_ptr<Stmt> stmt);

    std::unique_ptr<Stmt> elaborateVarDecl(std::unique_ptr<VarDecl> node);
    std::unique_ptr<Stmt> elaborateFnDecl(std::unique_ptr<FnDecl> node);

    std::unique_ptr<Stmt> elaborateReturnStmt(std::unique_ptr<HIR::HIRReturnStmt> node);
    std::unique_ptr<Stmt> elaborateIfStmt(std::unique_ptr<HIR::HIRIfStmt> node);
    std::unique_ptr<Stmt> elaborateWhileStmt(std::unique_ptr<HIR::HIRWhileStmt> node);
    std::unique_ptr<Stmt> elaborateBlockStmt(std::unique_ptr<HIR::HIRBlockStmt> node);
    std::unique_ptr<Stmt> elaborateExprStmt(std::unique_ptr<HIR::HIRExprStmt> node);
    std::unique_ptr<Stmt> elaborateBreakStmt(std::unique_ptr<HIR::HIRBreakStmt> node);
    std::unique_ptr<Stmt> elaborateContinueStmt(std::unique_ptr<HIR::HIRContinueStmt> node);

    std::unique_ptr<Expr> elaborateExpr(std::unique_ptr<Expr> expr);
    std::unique_ptr<Expr> elaborateBinaryExpr(std::unique_ptr<BinaryExpr> node);
    std::unique_ptr<Expr> elaborateUnaryExpr(std::unique_ptr<UnaryExpr> node);
    std::unique_ptr<Expr> elaborateFnCall(std::unique_ptr<FnCall> node);
    std::unique_ptr<Expr> elaborateAssignment(std::unique_ptr<Assignment> node);
    std::unique_ptr<Expr> elaborateGroupingExpr(std::unique_ptr<GroupingExpr> node);
    std::unique_ptr<Expr> elaborateArrayLiteral(std::unique_ptr<ArrayLiteral> node);
    std::unique_ptr<Expr> elaborateIndexExpr(std::unique_ptr<IndexExpr> node);

    bool isMultiDimensionalIndexing(const IndexExpr* expr);

    struct IndexChain {
        std::string baseVariable;
        std::vector<Expr*> indices; 
    };
    IndexChain collectIndices(std::unique_ptr<IndexExpr> expr);

    std::unique_ptr<Expr> calculateFlatIndex(
        std::vector<std::unique_ptr<Expr>>& indices,
        const std::vector<int>& dimensions
    );

    std::unique_ptr<IndexExpr> transformMultiDimIndexing(std::unique_ptr<IndexExpr> node);

    std::unique_ptr<Expr> cloneExpr(const Expr* expr);

    std::unique_ptr<Type> flattenArrayType(const Type* type);
};
