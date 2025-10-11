#pragma once

#include "vir/VIRNode.hpp"
#include <vector>
#include <string>
#include <memory>

namespace volta::vir {

// Forward declarations
class VIRBlock;

// ============================================================================
// Literals
// ============================================================================

class VIRIntLiteral : public VIRExpr {
public:
    VIRIntLiteral(int64_t value, std::shared_ptr<volta::semantic::Type> type,
                  volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    int64_t getValue() const { return value_; }
    std::shared_ptr<volta::semantic::Type> getType() const override { return type_; }

private:
    int64_t value_;
    std::shared_ptr<volta::semantic::Type> type_;
};

class VIRFloatLiteral : public VIRExpr {
public:
    VIRFloatLiteral(double value, std::shared_ptr<volta::semantic::Type> type,
                    volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    double getValue() const { return value_; }
    std::shared_ptr<volta::semantic::Type> getType() const override { return type_; }

private:
    double value_;
    std::shared_ptr<volta::semantic::Type> type_;
};

class VIRBoolLiteral : public VIRExpr {
public:
    VIRBoolLiteral(bool value, volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    bool getValue() const { return value_; }
    std::shared_ptr<volta::semantic::Type> getType() const override;

private:
    bool value_;
};

class VIRStringLiteral : public VIRExpr {
public:
    VIRStringLiteral(std::string value, std::string internedName,
                     volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const std::string& getValue() const { return value_; }
    const std::string& getInternedName() const { return internedName_; }
    std::shared_ptr<volta::semantic::Type> getType() const override;

private:
    std::string value_;
    std::string internedName_;  // e.g., ".str.0"
};

// ============================================================================
// References
// ============================================================================

class VIRLocalRef : public VIRExpr {
public:
    VIRLocalRef(std::string name, std::shared_ptr<volta::semantic::Type> type,
        volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const std::string& getName() const { return name_; }
    std::shared_ptr<volta::semantic::Type> getType() const override { return type_; }

private:
    std::string name_;
    std::shared_ptr<volta::semantic::Type> type_;
};

class VIRParamRef : public VIRExpr {
public:
    VIRParamRef(std::string name, size_t paramIndex, std::shared_ptr<volta::semantic::Type> type,
                volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const std::string& getName() const { return name_; }
    size_t getParamIndex() const { return paramIndex_; }
    std::shared_ptr<volta::semantic::Type> getType() const override { return type_; }

private:
    std::string name_;
    size_t paramIndex_;
    std::shared_ptr<volta::semantic::Type> type_;
};

class VIRGlobalRef : public VIRExpr {
public:
    VIRGlobalRef(std::string name, std::shared_ptr<volta::semantic::Type> type,
                 volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const std::string& getName() const { return name_; }
    std::shared_ptr<volta::semantic::Type> getType() const override { return type_; }

private:
    std::string name_;
    std::shared_ptr<volta::semantic::Type> type_;
};

// ============================================================================
// Binary & Unary Operations
// ============================================================================

enum class VIRBinaryOpKind {
    // Arithmetic
    Add, Sub, Mul, Div, Mod,
    // Comparisons
    Eq, Ne, Lt, Le, Gt, Ge,
    // Logical
    And, Or
};

class VIRBinaryOp : public VIRExpr {
public:
    VIRBinaryOp(VIRBinaryOpKind op, std::unique_ptr<VIRExpr> left, std::unique_ptr<VIRExpr> right,
                std::shared_ptr<volta::semantic::Type> resultType,
                volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    VIRBinaryOpKind getOp() const { return op_; }
    const VIRExpr* getLeft() const { return left_.get(); }
    const VIRExpr* getRight() const { return right_.get(); }
    std::shared_ptr<volta::semantic::Type> getType() const override { return resultType_; }

private:
    VIRBinaryOpKind op_;
    std::unique_ptr<VIRExpr> left_;
    std::unique_ptr<VIRExpr> right_;
    std::shared_ptr<volta::semantic::Type> resultType_;
};

enum class VIRUnaryOpKind {
    Neg,   // Arithmetic negation: -x
    Not    // Logical not: !x
};

class VIRUnaryOp : public VIRExpr {
public:
    VIRUnaryOp(VIRUnaryOpKind op, std::unique_ptr<VIRExpr> operand,
               std::shared_ptr<volta::semantic::Type> resultType,
               volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    VIRUnaryOpKind getOp() const { return op_; }
    const VIRExpr* getOperand() const { return operand_.get(); }
    std::shared_ptr<volta::semantic::Type> getType() const override { return resultType_; }

private:
    VIRUnaryOpKind op_;
    std::unique_ptr<VIRExpr> operand_;
    std::shared_ptr<volta::semantic::Type> resultType_;
};

// ============================================================================
// Type Operations
// ============================================================================

class VIRBox : public VIRExpr {
public:
    VIRBox(std::unique_ptr<VIRExpr> value, std::shared_ptr<volta::semantic::Type> sourceType,
           volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const VIRExpr* getValue() const { return value_.get(); }
    std::shared_ptr<volta::semantic::Type> getSourceType() const { return sourceType_; }
    std::shared_ptr<volta::semantic::Type> getType() const override;

private:
    std::unique_ptr<VIRExpr> value_;
    std::shared_ptr<volta::semantic::Type> sourceType_;
};

class VIRUnbox : public VIRExpr {
public:
    VIRUnbox(std::unique_ptr<VIRExpr> boxed, std::shared_ptr<volta::semantic::Type> targetType,
             volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const VIRExpr* getBoxed() const { return boxed_.get(); }
    std::shared_ptr<volta::semantic::Type> getType() const override { return targetType_; }

private:
    std::unique_ptr<VIRExpr> boxed_;
    std::shared_ptr<volta::semantic::Type> targetType_;
};

class VIRCast : public VIRExpr {
public:
    enum class CastKind {
        IntToInt,        // i32 -> i64 (widening), i64 -> i32 (narrowing)
        IntToFloat,      // i32 -> f32
        FloatToInt,      // f32 -> i32 (truncates)
        FloatToFloat,    // f32 -> f64 (widening), f64 -> f32 (narrowing)
        NoOp             // Same type (optimizer can remove)
    };

    VIRCast(std::unique_ptr<VIRExpr> value, std::shared_ptr<volta::semantic::Type> sourceType,
            std::shared_ptr<volta::semantic::Type> targetType, CastKind kind,
            volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const VIRExpr* getValue() const { return value_.get(); }
    std::shared_ptr<volta::semantic::Type> getSourceType() const { return sourceType_; }
    std::shared_ptr<volta::semantic::Type> getType() const override { return targetType_; }
    CastKind getCastKind() const { return kind_; }

private:
    std::unique_ptr<VIRExpr> value_;
    std::shared_ptr<volta::semantic::Type> sourceType_;
    std::shared_ptr<volta::semantic::Type> targetType_;
    CastKind kind_;
};

class VIRImplicitCast : public VIRExpr {
public:
    VIRImplicitCast(std::unique_ptr<VIRExpr> value, std::shared_ptr<volta::semantic::Type> sourceType,
                    std::shared_ptr<volta::semantic::Type> targetType, VIRCast::CastKind kind,
                    volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const VIRExpr* getValue() const { return value_.get(); }
    std::shared_ptr<volta::semantic::Type> getSourceType() const { return sourceType_; }
    std::shared_ptr<volta::semantic::Type> getType() const override { return targetType_; }
    VIRCast::CastKind getCastKind() const { return kind_; }

private:
    std::unique_ptr<VIRExpr> value_;
    std::shared_ptr<volta::semantic::Type> sourceType_;
    std::shared_ptr<volta::semantic::Type> targetType_;
    VIRCast::CastKind kind_;
};

// ============================================================================
// Function Calls
// ============================================================================

class VIRCall : public VIRExpr {
public:
    VIRCall(std::string functionName, std::vector<std::unique_ptr<VIRExpr>> args,
            std::shared_ptr<volta::semantic::Type> returnType,
            volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const std::string& getFunctionName() const { return functionName_; }
    const std::vector<std::unique_ptr<VIRExpr>>& getArgs() const { return args_; }
    std::shared_ptr<volta::semantic::Type> getType() const override { return returnType_; }

private:
    std::string functionName_;
    std::vector<std::unique_ptr<VIRExpr>> args_;
    std::shared_ptr<volta::semantic::Type> returnType_;
};

class VIRCallRuntime : public VIRExpr {
public:
    VIRCallRuntime(std::string runtimeFunc, std::vector<std::unique_ptr<VIRExpr>> args,
                   std::shared_ptr<volta::semantic::Type> returnType,
                   volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const std::string& getRuntimeFunc() const { return runtimeFunc_; }
    const std::vector<std::unique_ptr<VIRExpr>>& getArgs() const { return args_; }
    std::shared_ptr<volta::semantic::Type> getType() const override { return returnType_; }

private:
    std::string runtimeFunc_;
    std::vector<std::unique_ptr<VIRExpr>> args_;
    std::shared_ptr<volta::semantic::Type> returnType_;
};

class VIRCallIndirect : public VIRExpr {
public:
    VIRCallIndirect(std::unique_ptr<VIRExpr> functionPtr, std::vector<std::unique_ptr<VIRExpr>> args,
                    std::shared_ptr<volta::semantic::Type> returnType,
                    volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const VIRExpr* getFunctionPtr() const { return functionPtr_.get(); }
    const std::vector<std::unique_ptr<VIRExpr>>& getArgs() const { return args_; }
    std::shared_ptr<volta::semantic::Type> getType() const override { return returnType_; }

private:
    std::unique_ptr<VIRExpr> functionPtr_;
    std::vector<std::unique_ptr<VIRExpr>> args_;
    std::shared_ptr<volta::semantic::Type> returnType_;
};

// ============================================================================
// Wrapper Generation (KEY INNOVATION!)
// ============================================================================

class VIRWrapFunction : public VIRExpr {
public:
    enum class Strategy {
        UnboxCallBox,          // For map: fn(T) -> U becomes ptr(ptr) -> ptr
        UnboxCallBoolToInt,    // For filter: fn(T) -> bool becomes ptr(ptr) -> i32
        UnboxUnboxCallBox,     // For reduce: fn(Acc, T) -> Acc becomes ptr(ptr, ptr) -> ptr
    };

    VIRWrapFunction(std::string originalFunc, Strategy strategy,
                    std::shared_ptr<volta::semantic::Type> originalSig,
                    std::shared_ptr<volta::semantic::Type> targetSig,
                    volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const std::string& getOriginalFunc() const { return originalFunc_; }
    Strategy getStrategy() const { return strategy_; }
    std::shared_ptr<volta::semantic::Type> getOriginalSig() const { return originalSig_; }
    std::shared_ptr<volta::semantic::Type> getTargetSig() const { return targetSig_; }
    std::shared_ptr<volta::semantic::Type> getType() const override { return targetSig_; }

private:
    std::string originalFunc_;
    Strategy strategy_;
    std::shared_ptr<volta::semantic::Type> originalSig_;
    std::shared_ptr<volta::semantic::Type> targetSig_;
};

// ============================================================================
// Memory Operations
// ============================================================================

class VIRAlloca : public VIRExpr {
public:
    VIRAlloca(std::shared_ptr<volta::semantic::Type> allocatedType,
              volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    std::shared_ptr<volta::semantic::Type> getAllocatedType() const { return allocatedType_; }
    std::shared_ptr<volta::semantic::Type> getType() const override;

private:
    std::shared_ptr<volta::semantic::Type> allocatedType_;
};

class VIRLoad : public VIRExpr {
public:
    VIRLoad(std::unique_ptr<VIRExpr> pointer, std::shared_ptr<volta::semantic::Type> loadedType,
            volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const VIRExpr* getPointer() const { return pointer_.get(); }
    std::shared_ptr<volta::semantic::Type> getType() const override { return loadedType_; }

private:
    std::unique_ptr<VIRExpr> pointer_;
    std::shared_ptr<volta::semantic::Type> loadedType_;
};

class VIRStore : public VIRExpr {
public:
    VIRStore(std::unique_ptr<VIRExpr> pointer, std::unique_ptr<VIRExpr> value,
             volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const VIRExpr* getPointer() const { return pointer_.get(); }
    const VIRExpr* getValue() const { return value_.get(); }
    std::shared_ptr<volta::semantic::Type> getType() const override;

private:
    std::unique_ptr<VIRExpr> pointer_;
    std::unique_ptr<VIRExpr> value_;
};

// ============================================================================
// Control Flow Expressions
// ============================================================================

class VIRIfExpr : public VIRExpr {
public:
    VIRIfExpr(std::unique_ptr<VIRExpr> condition, std::unique_ptr<VIRBlock> thenBlock,
              std::unique_ptr<VIRBlock> elseBlock, std::shared_ptr<volta::semantic::Type> resultType,
              volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const VIRExpr* getCondition() const { return condition_.get(); }
    const VIRBlock* getThenBlock() const { return thenBlock_.get(); }
    const VIRBlock* getElseBlock() const { return elseBlock_.get(); }
    std::shared_ptr<volta::semantic::Type> getType() const override { return resultType_; }

private:
    std::unique_ptr<VIRExpr> condition_;
    std::unique_ptr<VIRBlock> thenBlock_;
    std::unique_ptr<VIRBlock> elseBlock_;
    std::shared_ptr<volta::semantic::Type> resultType_;
};

// ============================================================================
// Struct Operations (for later phases)
// ============================================================================

class VIRStructNew : public VIRExpr {
public:
    VIRStructNew(std::string structName, std::vector<std::unique_ptr<VIRExpr>> fieldValues,
                 std::shared_ptr<volta::semantic::Type> structType,
                 volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const std::string& getStructName() const { return structName_; }
    const std::vector<std::unique_ptr<VIRExpr>>& getFieldValues() const { return fieldValues_; }
    std::shared_ptr<volta::semantic::Type> getType() const override { return structType_; }

private:
    std::string structName_;
    std::vector<std::unique_ptr<VIRExpr>> fieldValues_;
    std::shared_ptr<volta::semantic::Type> structType_;
};

class VIRStructGet : public VIRExpr {
public:
    VIRStructGet(std::unique_ptr<VIRExpr> structPtr, std::string fieldName, size_t fieldIndex,
                 std::shared_ptr<volta::semantic::Type> fieldType,
                 volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const VIRExpr* getStructPtr() const { return structPtr_.get(); }
    const std::string& getFieldName() const { return fieldName_; }
    size_t getFieldIndex() const { return fieldIndex_; }
    std::shared_ptr<volta::semantic::Type> getType() const override { return fieldType_; }

private:
    std::unique_ptr<VIRExpr> structPtr_;
    std::string fieldName_;
    size_t fieldIndex_;
    std::shared_ptr<volta::semantic::Type> fieldType_;
};

class VIRStructSet : public VIRExpr {
public:
    VIRStructSet(std::unique_ptr<VIRExpr> structPtr, std::string fieldName, size_t fieldIndex,
                 std::unique_ptr<VIRExpr> value,
                 volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const VIRExpr* getStructPtr() const { return structPtr_.get(); }
    const std::string& getFieldName() const { return fieldName_; }
    size_t getFieldIndex() const { return fieldIndex_; }
    const VIRExpr* getValue() const { return value_.get(); }
    std::shared_ptr<volta::semantic::Type> getType() const override;

private:
    std::unique_ptr<VIRExpr> structPtr_;
    std::string fieldName_;
    size_t fieldIndex_;
    std::unique_ptr<VIRExpr> value_;
};

// ============================================================================
// Array Operations (for later phases)
// ============================================================================

class VIRArrayNew : public VIRExpr {
public:
    VIRArrayNew(std::unique_ptr<VIRExpr> capacity, std::shared_ptr<volta::semantic::Type> elementType,
                volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const VIRExpr* getCapacity() const { return capacity_.get(); }
    std::shared_ptr<volta::semantic::Type> getElementType() const { return elementType_; }
    std::shared_ptr<volta::semantic::Type> getType() const override;

private:
    std::unique_ptr<VIRExpr> capacity_;
    std::shared_ptr<volta::semantic::Type> elementType_;
};

class VIRArrayGet : public VIRExpr {
public:
    VIRArrayGet(std::unique_ptr<VIRExpr> array, std::unique_ptr<VIRExpr> index,
                std::shared_ptr<volta::semantic::Type> elementType,
                volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const VIRExpr* getArray() const { return array_.get(); }
    const VIRExpr* getIndex() const { return index_.get(); }
    std::shared_ptr<volta::semantic::Type> getType() const override { return elementType_; }

private:
    std::unique_ptr<VIRExpr> array_;
    std::unique_ptr<VIRExpr> index_;
    std::shared_ptr<volta::semantic::Type> elementType_;
};

class VIRArraySet : public VIRExpr {
public:
    VIRArraySet(std::unique_ptr<VIRExpr> array, std::unique_ptr<VIRExpr> index,
                std::unique_ptr<VIRExpr> value,
                volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const VIRExpr* getArray() const { return array_.get(); }
    const VIRExpr* getIndex() const { return index_.get(); }
    const VIRExpr* getValue() const { return value_.get(); }
    std::shared_ptr<volta::semantic::Type> getType() const override;

private:
    std::unique_ptr<VIRExpr> array_;
    std::unique_ptr<VIRExpr> index_;
    std::unique_ptr<VIRExpr> value_;
};

// ============================================================================
// Safety Operations
// ============================================================================

class VIRBoundsCheck : public VIRExpr {
public:
    VIRBoundsCheck(std::unique_ptr<VIRExpr> array, std::unique_ptr<VIRExpr> index,
                   volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast);

    const VIRExpr* getArray() const { return array_.get(); }
    const VIRExpr* getIndex() const { return index_.get(); }
    std::shared_ptr<volta::semantic::Type> getType() const override;

private:
    std::unique_ptr<VIRExpr> array_;
    std::unique_ptr<VIRExpr> index_;
};

} // namespace volta::vir
