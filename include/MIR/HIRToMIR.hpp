#pragma once

#include "MIR/MIR.hpp"
#include "MIR/MIRBuilder.hpp"
#include "HIR/HIR.hpp"
#include "Parser/AST.hpp"
#include "Parser/ASTVisitor.hpp"
#include "Type/TypeRegistry.hpp"
#include "Error/Error.hpp"
#include <unordered_map>
#include <string>

namespace MIR {

class HIRToMIR : public StmtVisitor, public ExprVisitor<Value> {
private:
    MIRBuilder builder;
    Type::TypeRegistry& typeRegistry;
    DiagnosticManager& diag;

    // Type map: AST node -> computed type (from semantic analysis)
    std::unordered_map<const Expr*, const Type::Type*> exprTypes;

    // Symbol table: HIR variable name -> MIR pointer value (for mutable vars)
    // Immutable variables are inlined directly
    std::unordered_map<std::string, Value> varPointers;

    // Track whether we're in a loop (for break/continue)
    struct LoopContext {
        std::string breakLabel;
        std::string continueLabel;
    };
    std::vector<LoopContext> loopStack;

public:
    HIRToMIR(Type::TypeRegistry& tr, DiagnosticManager& d,
             const std::unordered_map<const Expr*, const Type::Type*>& types)
        : builder(tr), typeRegistry(tr), diag(d), exprTypes(types) {}

    // Lower an entire HIR program to MIR
    MIR::Program lower(const HIR::HIRProgram& hirProgram);


private:

    void visitFnDecl(::FnDecl& fnDecl) override;
    void visitStructDecl(::StructDecl& node) override;

    // Visitor methods for statements
    void visitVarDecl(::VarDecl& node) override;
    void visitExprStmt(::ExprStmt& node) override;
    void visitReturnStmt(::ReturnStmt& node) override;
    void visitIfStmt(::IfStmt& node) override;
    void visitWhileStmt(::WhileStmt& node) override;
    void visitForStmt(::ForStmt& node) override;  // Should be lowered by HIR already
    void visitBlockStmt(::BlockStmt& node) override;
    void visitBreakStmt(::BreakStmt& node) override;
    void visitContinueStmt(::ContinueStmt& node) override;
    void visitExternBlock(::ExternBlock& node) override;  // Skip
    void visitImportStmt(::ImportStmt& node) override;  // Skip

    // Visitor methods for expressions (return MIR Values)
    Value visitLiteral(::Literal& node) override;
    Value visitVariable(::Variable& node) override;
    Value visitBinaryExpr(::BinaryExpr& node) override;
    Value visitUnaryExpr(::UnaryExpr& node) override;
    Value visitFnCall(::FnCall& node) override;
    Value visitAssignment(::Assignment& node) override;
    Value visitGroupingExpr(::GroupingExpr& node) override;
    Value visitArrayLiteral(::ArrayLiteral& node) override;
    Value visitIndexExpr(::IndexExpr& node) override;
    Value visitAddrOf(::AddrOf& node) override;
    Value visitCompoundAssign(::CompoundAssign& node) override;  // Should be lowered by HIR
    Value visitIncrement(::Increment& node) override;  // Should be lowered by HIR
    Value visitDecrement(::Decrement& node) override;  // Should be lowered by HIR
    Value visitRange(::Range& node) override;  // Should be lowered by HIR
    Value visitStructLiteral(::StructLiteral& node) override;
    Value visitFieldAccess(::FieldAccess& node) override;
    Value visitStaticMethodCall(::StaticMethodCall& node) override;
    Value visitInstanceMethodCall(::InstanceMethodCall& node) override;

    // Dispatch statement lowering (handles HIR-specific nodes)
    void lowerHIRStmt(HIR::HIRStmt& stmt);
    void lowerStmt(Stmt& stmt);  // For AST nodes (old compatibility)

    // Dispatch expression lowering (since visitor pattern doesn't work for returning values)
    Value lowerExpr(Expr& expr);

    // Helper: Get pointer to array element without loading (for assignment)
    Value getIndexExprPtr(::IndexExpr& expr);

    // Helper: Get pointer to struct field without loading (for assignment)
    Value getFieldAccessPtr(::FieldAccess& expr);

    // Get the type of an expression from the type map
    const Type::Type* getExprType(const Expr* expr) const;

    // Map HIR types to MIR types (usually 1:1 via TypeRegistry)
    const Type::Type* getMIRType(const Type::Type* hirType);

    // Convert a value to a target type (insert conversion instructions)
    Value convertValue(const Value& value, const Type::Type* targetType);

    // Generate unique label for basic blocks
    std::string generateLabel(const std::string& hint);

    // Calculate size in bytes of a type (for GC allocation)
    size_t getTypeSize(const Type::Type* type);
};

} // namespace MIR
