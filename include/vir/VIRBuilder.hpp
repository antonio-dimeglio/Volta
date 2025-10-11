#pragma once

#include "vir/VIRExpression.hpp"
#include "vir/VIRStatement.hpp"
#include "vir/VIRModule.hpp"
#include "semantic/SemanticAnalyzer.hpp"
#include <memory>

namespace volta::vir {

/**
 * VIR Builder - Helper for constructing VIR nodes
 *
 * Provides convenient factory methods for creating VIR nodes.
 * Makes lowering code cleaner and more readable.
 */
class VIRBuilder {
public:
    VIRBuilder(const volta::semantic::SemanticAnalyzer* analyzer);

    // ========================================================================
    // Literals
    // ========================================================================

    std::unique_ptr<VIRIntLiteral> createIntLiteral(
        int64_t value,
        std::shared_ptr<volta::semantic::Type> type,
        volta::errors::SourceLocation loc = {}
    );

    std::unique_ptr<VIRFloatLiteral> createFloatLiteral(
        double value,
        std::shared_ptr<volta::semantic::Type> type,
        volta::errors::SourceLocation loc = {}
    );

    std::unique_ptr<VIRBoolLiteral> createBoolLiteral(
        bool value,
        volta::errors::SourceLocation loc = {}
    );

    std::unique_ptr<VIRStringLiteral> createStringLiteral(
        std::string value,
        std::string internedName,
        volta::errors::SourceLocation loc = {}
    );

    // ========================================================================
    // References
    // ========================================================================

    std::unique_ptr<VIRLocalRef> createLocalRef(
        std::string name,
        std::shared_ptr<volta::semantic::Type> type,
        bool isMutable = false,
        volta::errors::SourceLocation loc = {}
    );

    std::unique_ptr<VIRParamRef> createParamRef(
        std::string name,
        size_t paramIndex,
        std::shared_ptr<volta::semantic::Type> type,
        volta::errors::SourceLocation loc = {}
    );

    // ========================================================================
    // Binary Operations
    // ========================================================================

    std::unique_ptr<VIRBinaryOp> createAdd(
        std::unique_ptr<VIRExpr> left,
        std::unique_ptr<VIRExpr> right,
        volta::errors::SourceLocation loc = {}
    );

    std::unique_ptr<VIRBinaryOp> createSub(
        std::unique_ptr<VIRExpr> left,
        std::unique_ptr<VIRExpr> right,
        volta::errors::SourceLocation loc = {}
    );

    std::unique_ptr<VIRBinaryOp> createMul(
        std::unique_ptr<VIRExpr> left,
        std::unique_ptr<VIRExpr> right,
        volta::errors::SourceLocation loc = {}
    );

    std::unique_ptr<VIRBinaryOp> createLt(
        std::unique_ptr<VIRExpr> left,
        std::unique_ptr<VIRExpr> right,
        volta::errors::SourceLocation loc = {}
    );

    // Generic binary op creator
    std::unique_ptr<VIRBinaryOp> createBinaryOp(
        VIRBinaryOpKind op,
        std::unique_ptr<VIRExpr> left,
        std::unique_ptr<VIRExpr> right,
        volta::errors::SourceLocation loc = {}
    );

    // ========================================================================
    // Function Calls
    // ========================================================================

    std::unique_ptr<VIRCall> createCall(
        const std::string& functionName,
        std::vector<std::unique_ptr<VIRExpr>> args,
        std::shared_ptr<volta::semantic::Type> returnType,
        volta::errors::SourceLocation loc = {}
    );

    std::unique_ptr<VIRCallRuntime> createRuntimeCall(
        const std::string& runtimeFunc,
        std::vector<std::unique_ptr<VIRExpr>> args,
        std::shared_ptr<volta::semantic::Type> returnType,
        volta::errors::SourceLocation loc = {}
    );

    // ========================================================================
    // Wrapper Generation
    // ========================================================================

    std::unique_ptr<VIRWrapFunction> createMapWrapper(
        const std::string& functionName,
        std::shared_ptr<volta::semantic::Type> originalSig,
        volta::errors::SourceLocation loc = {}
    );

    std::unique_ptr<VIRWrapFunction> createFilterWrapper(
        const std::string& functionName,
        std::shared_ptr<volta::semantic::Type> originalSig,
        volta::errors::SourceLocation loc = {}
    );

    std::unique_ptr<VIRWrapFunction> createReduceWrapper(
        const std::string& functionName,
        std::shared_ptr<volta::semantic::Type> originalSig,
        volta::errors::SourceLocation loc = {}
    );

    // ========================================================================
    // Type Operations
    // ========================================================================

    std::unique_ptr<VIRBox> createBox(
        std::unique_ptr<VIRExpr> value,
        volta::errors::SourceLocation loc = {}
    );

    std::unique_ptr<VIRUnbox> createUnbox(
        std::unique_ptr<VIRExpr> boxed,
        std::shared_ptr<volta::semantic::Type> targetType,
        volta::errors::SourceLocation loc = {}
    );

    std::unique_ptr<VIRCast> createCast(
        std::unique_ptr<VIRExpr> value,
        std::shared_ptr<volta::semantic::Type> targetType,
        volta::errors::SourceLocation loc = {}
    );

    // ========================================================================
    // Control Flow
    // ========================================================================

    std::unique_ptr<VIRIfExpr> createIfExpr(
        std::unique_ptr<VIRExpr> condition,
        std::unique_ptr<VIRBlock> thenBlock,
        std::unique_ptr<VIRBlock> elseBlock,
        std::shared_ptr<volta::semantic::Type> resultType,
        volta::errors::SourceLocation loc = {}
    );

    std::unique_ptr<VIRIfStmt> createIfStmt(
        std::unique_ptr<VIRExpr> condition,
        std::unique_ptr<VIRBlock> thenBlock,
        std::unique_ptr<VIRBlock> elseBlock,
        volta::errors::SourceLocation loc = {}
    );

    std::unique_ptr<VIRWhile> createWhile(
        std::unique_ptr<VIRExpr> condition,
        std::unique_ptr<VIRBlock> body,
        volta::errors::SourceLocation loc = {}
    );

    // ========================================================================
    // Statements
    // ========================================================================

    std::unique_ptr<VIRBlock> createBlock(volta::errors::SourceLocation loc = {});

    std::unique_ptr<VIRVarDecl> createVarDecl(
        std::string name,
        std::unique_ptr<VIRExpr> initializer,
        bool isMutable,
        std::shared_ptr<volta::semantic::Type> type,
        volta::errors::SourceLocation loc = {}
    );

    std::unique_ptr<VIRReturn> createReturn(
        std::unique_ptr<VIRExpr> value,
        volta::errors::SourceLocation loc = {}
    );

    std::unique_ptr<VIRBreak> createBreak(volta::errors::SourceLocation loc = {});
    std::unique_ptr<VIRContinue> createContinue(volta::errors::SourceLocation loc = {});

    std::unique_ptr<VIRExprStmt> createExprStmt(
        std::unique_ptr<VIRExpr> expr,
        volta::errors::SourceLocation loc = {}
    );

    // ========================================================================
    // Arrays
    // ========================================================================

    std::unique_ptr<VIRArrayNew> createArrayNew(
        std::unique_ptr<VIRExpr> capacity,
        std::shared_ptr<volta::semantic::Type> elementType,
        volta::errors::SourceLocation loc = {}
    );

    std::unique_ptr<VIRBoundsCheck> createBoundsCheck(
        std::unique_ptr<VIRExpr> array,
        std::unique_ptr<VIRExpr> index,
        volta::errors::SourceLocation loc = {}
    );

    // ========================================================================
    // Structs
    // ========================================================================

    std::unique_ptr<VIRStructNew> createStructNew(
        const std::string& structName,
        std::vector<std::unique_ptr<VIRExpr>> fieldValues,
        std::shared_ptr<volta::semantic::Type> structType,
        volta::errors::SourceLocation loc = {}
    );

    std::unique_ptr<VIRStructGet> createStructGet(
        std::unique_ptr<VIRExpr> structPtr,
        const std::string& fieldName,
        size_t fieldIndex,
        std::shared_ptr<volta::semantic::Type> fieldType,
        volta::errors::SourceLocation loc = {}
    );

private:
    const volta::semantic::SemanticAnalyzer* analyzer_;
};

} // namespace volta::vir
