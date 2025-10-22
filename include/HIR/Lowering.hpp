#pragma once

#include "HIR.hpp"
#include "../Parser/AST.hpp"
#include "../Type/TypeRegistry.hpp"
#include <memory>
#include <vector>

class HIRLowering {
private:
    Type::TypeRegistry& typeRegistry;

    // Helper methods for lowering different statement types
    std::unique_ptr<HIR::HIRStmt> lowerStmt(Stmt& stmt);
    std::unique_ptr<HIR::HIRStmt> lowerVarDecl(VarDecl& decl);
    std::unique_ptr<HIR::HIRStmt> lowerFnDecl(FnDecl& decl);
    std::unique_ptr<HIR::HIRStmt> lowerIfStmt(IfStmt& stmt);
    std::unique_ptr<HIR::HIRStmt> lowerWhileStmt(WhileStmt& stmt);
    std::unique_ptr<HIR::HIRStmt> lowerForStmt(ForStmt& stmt);
    std::unique_ptr<HIR::HIRStmt> lowerBlockStmt(BlockStmt& stmt);
    std::unique_ptr<HIR::HIRStmt> lowerReturnStmt(ReturnStmt& stmt);
    std::unique_ptr<HIR::HIRStmt> lowerExprStmt(ExprStmt& stmt);
    std::unique_ptr<HIR::HIRStmt> lowerBreakStmt(BreakStmt& stmt);
    std::unique_ptr<HIR::HIRStmt> lowerContinueStmt(ContinueStmt& stmt);
    std::unique_ptr<HIR::HIRStmt> lowerExternBlock(ExternBlock& block);
    std::unique_ptr<HIR::HIRStmt> lowerImportStmt(ImportStmt& stmt);
    std::unique_ptr<HIR::HIRStmt> lowerStructDecl(StructDecl& decl);

    // Helper for lowering expressions (desugaring compound assignments, etc.)
    std::unique_ptr<Expr> desugarExpr(Expr& expr);

public:
    explicit HIRLowering(Type::TypeRegistry& typeRegistry) : typeRegistry(typeRegistry) {}

    // Lower entire program from AST to HIR
    HIR::HIRProgram lower(Program& ast);
};
