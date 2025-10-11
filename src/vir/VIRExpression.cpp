#include "vir/VIRExpression.hpp"
#include "vir/VIRStatement.hpp"  // For VIRBlock complete type
#include "semantic/Type.hpp"     // For TypeCache

namespace volta::vir {

// ============================================================================
// Literals - Constructors
// ============================================================================

VIRIntLiteral::VIRIntLiteral(int64_t value, std::shared_ptr<volta::semantic::Type> type,
                              volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), value_(value), type_(type) {}

VIRFloatLiteral::VIRFloatLiteral(double value, std::shared_ptr<volta::semantic::Type> type,
                                  volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), value_(value), type_(type) {}

VIRBoolLiteral::VIRBoolLiteral(bool value, volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), value_(value) {}

std::shared_ptr<volta::semantic::Type> VIRBoolLiteral::getType() const {
    // Return bool type from type cache
    static volta::semantic::TypeCache typeCache;
    return typeCache.getBool();
}

VIRStringLiteral::VIRStringLiteral(std::string value, std::string internedName,
                                    volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), value_(std::move(value)), internedName_(std::move(internedName)) {}

std::shared_ptr<volta::semantic::Type> VIRStringLiteral::getType() const {
    // Return string type
    // TODO: Get from semantic::Type
    return nullptr;  // Placeholder
}

// ============================================================================
// References - Constructors
// ============================================================================

VIRLocalRef::VIRLocalRef(std::string name, std::shared_ptr<volta::semantic::Type> type,
                          volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), name_(std::move(name)), type_(type) {}

VIRParamRef::VIRParamRef(std::string name, size_t paramIndex, std::shared_ptr<volta::semantic::Type> type,
                          volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), name_(std::move(name)), paramIndex_(paramIndex), type_(type) {}

VIRGlobalRef::VIRGlobalRef(std::string name, std::shared_ptr<volta::semantic::Type> type,
                            volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), name_(std::move(name)), type_(type) {}

// ============================================================================
// Binary & Unary Operations - Constructors
// ============================================================================

VIRBinaryOp::VIRBinaryOp(VIRBinaryOpKind op, std::unique_ptr<VIRExpr> left, std::unique_ptr<VIRExpr> right,
                          std::shared_ptr<volta::semantic::Type> resultType,
                          volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), op_(op), left_(std::move(left)), right_(std::move(right)), resultType_(resultType) {}

VIRUnaryOp::VIRUnaryOp(VIRUnaryOpKind op, std::unique_ptr<VIRExpr> operand,
                        std::shared_ptr<volta::semantic::Type> resultType,
                        volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), op_(op), operand_(std::move(operand)), resultType_(resultType) {}

// ============================================================================
// Type Operations - Constructors
// ============================================================================

VIRBox::VIRBox(std::unique_ptr<VIRExpr> value, std::shared_ptr<volta::semantic::Type> sourceType,
                volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), value_(std::move(value)), sourceType_(sourceType) {}

std::shared_ptr<volta::semantic::Type> VIRBox::getType() const {
    // Box always produces ptr
    // TODO: Get pointer type from semantic::Type
    return nullptr;  // Placeholder
}

VIRUnbox::VIRUnbox(std::unique_ptr<VIRExpr> boxed, std::shared_ptr<volta::semantic::Type> targetType,
                    volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), boxed_(std::move(boxed)), targetType_(targetType) {}

VIRCast::VIRCast(std::unique_ptr<VIRExpr> value, std::shared_ptr<volta::semantic::Type> sourceType,
                  std::shared_ptr<volta::semantic::Type> targetType, CastKind kind,
                  volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), value_(std::move(value)), sourceType_(sourceType), targetType_(targetType), kind_(kind) {}

VIRImplicitCast::VIRImplicitCast(std::unique_ptr<VIRExpr> value, std::shared_ptr<volta::semantic::Type> sourceType,
                                  std::shared_ptr<volta::semantic::Type> targetType, VIRCast::CastKind kind,
                                  volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), value_(std::move(value)), sourceType_(sourceType), targetType_(targetType), kind_(kind) {}

// ============================================================================
// Function Calls - Constructors
// ============================================================================

VIRCall::VIRCall(std::string functionName, std::vector<std::unique_ptr<VIRExpr>> args,
                  std::shared_ptr<volta::semantic::Type> returnType,
                  volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), functionName_(std::move(functionName)), args_(std::move(args)), returnType_(returnType) {}

VIRCallRuntime::VIRCallRuntime(std::string runtimeFunc, std::vector<std::unique_ptr<VIRExpr>> args,
                                std::shared_ptr<volta::semantic::Type> returnType,
                                volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), runtimeFunc_(std::move(runtimeFunc)), args_(std::move(args)), returnType_(returnType) {}

VIRCallIndirect::VIRCallIndirect(std::unique_ptr<VIRExpr> functionPtr, std::vector<std::unique_ptr<VIRExpr>> args,
                                  std::shared_ptr<volta::semantic::Type> returnType,
                                  volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), functionPtr_(std::move(functionPtr)), args_(std::move(args)), returnType_(returnType) {}

// ============================================================================
// Wrapper Generation - Constructors
// ============================================================================

VIRWrapFunction::VIRWrapFunction(std::string originalFunc, Strategy strategy,
                                  std::shared_ptr<volta::semantic::Type> originalSig,
                                  std::shared_ptr<volta::semantic::Type> targetSig,
                                  volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), originalFunc_(std::move(originalFunc)), strategy_(strategy),
      originalSig_(originalSig), targetSig_(targetSig) {}

// ============================================================================
// Memory Operations - Constructors
// ============================================================================

VIRAlloca::VIRAlloca(std::shared_ptr<volta::semantic::Type> allocatedType,
                      volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), allocatedType_(allocatedType) {}

std::shared_ptr<volta::semantic::Type> VIRAlloca::getType() const {
    // Returns pointer to allocated type
    // TODO: Get pointer type from semantic::Type
    return nullptr;  // Placeholder
}

VIRLoad::VIRLoad(std::unique_ptr<VIRExpr> pointer, std::shared_ptr<volta::semantic::Type> loadedType,
                  volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), pointer_(std::move(pointer)), loadedType_(loadedType) {}

VIRStore::VIRStore(std::unique_ptr<VIRExpr> pointer, std::unique_ptr<VIRExpr> value,
                    volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), pointer_(std::move(pointer)), value_(std::move(value)) {}

std::shared_ptr<volta::semantic::Type> VIRStore::getType() const {
    // Store returns void/unit
    // TODO: Get unit type from semantic::Type
    return nullptr;  // Placeholder
}

// ============================================================================
// Control Flow Expressions - Constructors
// ============================================================================

VIRIfExpr::VIRIfExpr(std::unique_ptr<VIRExpr> condition, std::unique_ptr<VIRBlock> thenBlock,
                      std::unique_ptr<VIRBlock> elseBlock, std::shared_ptr<volta::semantic::Type> resultType,
                      volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), condition_(std::move(condition)), thenBlock_(std::move(thenBlock)),
      elseBlock_(std::move(elseBlock)), resultType_(resultType) {}

// ============================================================================
// Struct Operations - Constructors
// ============================================================================

VIRStructNew::VIRStructNew(std::string structName, std::vector<std::unique_ptr<VIRExpr>> fieldValues,
                            std::shared_ptr<volta::semantic::Type> structType,
                            volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), structName_(std::move(structName)), fieldValues_(std::move(fieldValues)), structType_(structType) {}

VIRStructGet::VIRStructGet(std::unique_ptr<VIRExpr> structPtr, std::string fieldName, size_t fieldIndex,
                            std::shared_ptr<volta::semantic::Type> fieldType,
                            volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), structPtr_(std::move(structPtr)), fieldName_(std::move(fieldName)),
      fieldIndex_(fieldIndex), fieldType_(fieldType) {}

VIRStructSet::VIRStructSet(std::unique_ptr<VIRExpr> structPtr, std::string fieldName, size_t fieldIndex,
                            std::unique_ptr<VIRExpr> value,
                            volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), structPtr_(std::move(structPtr)), fieldName_(std::move(fieldName)),
      fieldIndex_(fieldIndex), value_(std::move(value)) {}

std::shared_ptr<volta::semantic::Type> VIRStructSet::getType() const {
    // Returns void/unit
    // TODO: Get unit type from semantic::Type
    return nullptr;  // Placeholder
}

// ============================================================================
// Fixed Array Operations - Constructors
// ============================================================================

VIRFixedArrayNew::VIRFixedArrayNew(std::vector<std::unique_ptr<VIRExpr>> elements,
                                     size_t size,
                                     std::shared_ptr<volta::semantic::Type> elementType,
                                     bool isStackAllocated,
                                     volta::errors::SourceLocation loc,
                                     const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), elements_(std::move(elements)), size_(size),
      elementType_(elementType), isStackAllocated_(isStackAllocated) {}

std::shared_ptr<volta::semantic::Type> VIRFixedArrayNew::getType() const {
    // Returns FixedArrayType(elementType, size)
    // TODO: Get fixed array type from semantic::Type
    return nullptr;  // Placeholder for now
}

VIRFixedArrayGet::VIRFixedArrayGet(std::unique_ptr<VIRExpr> array,
                                     std::unique_ptr<VIRExpr> index,
                                     std::shared_ptr<volta::semantic::Type> elementType,
                                     volta::errors::SourceLocation loc,
                                     const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), array_(std::move(array)), index_(std::move(index)), elementType_(elementType) {}

VIRFixedArraySet::VIRFixedArraySet(std::unique_ptr<VIRExpr> array,
                                     std::unique_ptr<VIRExpr> index,
                                     std::unique_ptr<VIRExpr> value,
                                     volta::errors::SourceLocation loc,
                                     const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), array_(std::move(array)), index_(std::move(index)), value_(std::move(value)) {}

std::shared_ptr<volta::semantic::Type> VIRFixedArraySet::getType() const {
    // Returns void/unit type
    // TODO: Get unit type from semantic::Type
    return nullptr;  // Placeholder for now
}

// ============================================================================
// Dynamic Array Operations - Constructors
// ============================================================================

VIRArrayNew::VIRArrayNew(std::unique_ptr<VIRExpr> capacity, std::shared_ptr<volta::semantic::Type> elementType,
                          volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), capacity_(std::move(capacity)), elementType_(elementType) {}

std::shared_ptr<volta::semantic::Type> VIRArrayNew::getType() const {
    // Returns Array[elementType]
    // TODO: Get array type from semantic::Type
    return nullptr;  // Placeholder
}

VIRArrayGet::VIRArrayGet(std::unique_ptr<VIRExpr> array, std::unique_ptr<VIRExpr> index,
                          std::shared_ptr<volta::semantic::Type> elementType,
                          volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), array_(std::move(array)), index_(std::move(index)), elementType_(elementType) {}

VIRArraySet::VIRArraySet(std::unique_ptr<VIRExpr> array, std::unique_ptr<VIRExpr> index,
                          std::unique_ptr<VIRExpr> value,
                          volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), array_(std::move(array)), index_(std::move(index)), value_(std::move(value)) {}

std::shared_ptr<volta::semantic::Type> VIRArraySet::getType() const {
    // Returns void/unit
    // TODO: Get unit type from semantic::Type
    return nullptr;  // Placeholder
}

// ============================================================================
// Safety Operations - Constructors
// ============================================================================

VIRBoundsCheck::VIRBoundsCheck(std::unique_ptr<VIRExpr> array, std::unique_ptr<VIRExpr> index,
                                volta::errors::SourceLocation loc, const volta::ast::ASTNode* ast)
    : VIRExpr(loc, ast), array_(std::move(array)), index_(std::move(index)) {}

std::shared_ptr<volta::semantic::Type> VIRBoundsCheck::getType() const {
    // Returns the index if valid
    return index_->getType();
}

} // namespace volta::vir
