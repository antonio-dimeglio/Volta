#pragma once

#include "../Parser/ASTVisitor.hpp"
#include "../Parser/AST.hpp"
#include "../HIR/HIR.hpp"
#include "../Type/TypeRegistry.hpp"
#include "SymbolTable.hpp"
#include "FunctionRegistry.hpp"
#include "../Error/Error.hpp"
#include <unordered_map>

namespace Semantic {

class SemanticAnalyzer : public HIR::HIRStmtVisitor, public ExprVisitor<const Type::Type*> {
private:
    Type::TypeRegistry& typeRegistry;
    SymbolTable symbolTable;
    DiagnosticManager& diag;
    FunctionRegistry* functionRegistry = nullptr;  // Optional: for cross-module function resolution

    const Type::Type* currentReturnType = nullptr;
    bool inLoop = false;
    std::string currentFunctionName;

    // Type map: stores computed type for each expression
    std::unordered_map<const Expr*, const Type::Type*> exprTypes;

public:
    SemanticAnalyzer(Type::TypeRegistry& types, DiagnosticManager& diag);

    // Set the function registry for cross-module function resolution
    void setFunctionRegistry(FunctionRegistry* registry) { functionRegistry = registry; }

    void analyzeProgram(const HIR::HIRProgram& program);
    SymbolTable& getSymbolTable() { return symbolTable; }
    const SymbolTable& getSymbolTable() const { return symbolTable; }

    // Get the type map (for HIR to MIR lowering)
    const std::unordered_map<const Expr*, const Type::Type*>& getExprTypes() const { return exprTypes; }

    // Public methods for cross-module type management
    void registerStructTypes(const HIR::HIRProgram& program);
    void resolveUnresolvedTypes(HIR::HIRProgram& program);

    // HIR Statement visitors
    void visitVarDecl(HIR::HIRVarDecl& node) override;
    void visitFnDecl(HIR::HIRFnDecl& node) override;
    void visitReturnStmt(HIR::HIRReturnStmt& node) override;
    void visitIfStmt(HIR::HIRIfStmt& node) override;
    void visitWhileStmt(HIR::HIRWhileStmt& node) override;
    void visitExprStmt(HIR::HIRExprStmt& node) override;
    void visitBlockStmt(HIR::HIRBlockStmt& node) override;
    void visitBreakStmt(HIR::HIRBreakStmt& node) override;
    void visitContinueStmt(HIR::HIRContinueStmt& node) override;
    void visitExternBlock(HIR::HIRExternBlock& node) override;
    void visitImportStmt(HIR::HIRImportStmt& node) override;
    void visitStructDecl(HIR::HIRStructDecl& node) override;

    // Expression visitors
    const Type::Type* visitLiteral(Literal& node) override;
    const Type::Type* visitVariable(Variable& node) override;
    const Type::Type* visitBinaryExpr(BinaryExpr& node) override;
    const Type::Type* visitUnaryExpr(UnaryExpr& node) override;
    const Type::Type* visitFnCall(FnCall& node) override;
    const Type::Type* visitAssignment(Assignment& node) override;
    const Type::Type* visitGroupingExpr(GroupingExpr& node) override;
    const Type::Type* visitArrayLiteral(ArrayLiteral& node) override;
    const Type::Type* visitIndexExpr(IndexExpr& node) override;
    const Type::Type* visitAddrOf(AddrOf& node) override;
    const Type::Type* visitCompoundAssign(CompoundAssign& node) override;
    const Type::Type* visitIncrement(Increment& node) override;
    const Type::Type* visitDecrement(Decrement& node) override;
    const Type::Type* visitRange(Range& node) override;
    const Type::Type* visitStructLiteral(StructLiteral& node) override;
    const Type::Type* visitFieldAccess(FieldAccess& node) override;
    const Type::Type* visitStaticMethodCall(StaticMethodCall& node) override;
    const Type::Type* visitInstanceMethodCall(InstanceMethodCall& node) override;

private:
    void collectFunctionSignatures(const HIR::HIRProgram& program);
    void registerFunction(HIR::HIRFnDecl& node);
    void resolveTypesInStmts(std::vector<std::unique_ptr<HIR::HIRStmt>>& stmts);
    void resolveTypesInStmt(HIR::HIRStmt* stmt);
    const Type::Type* resolveType(const Type::Type* type);
    const Type::Type* errorType();
    bool isNumericType(const Type::Type* type);
    bool isIntegerType(const Type::Type* type);
    bool isTypeCompatible(const Type::Type* from, const Type::Type* to);
};

} // namespace Semantic
