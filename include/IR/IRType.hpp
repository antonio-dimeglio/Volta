#pragma once
#include <memory>
#include <string>
#include "semantic/Type.hpp"

namespace volta::ir {

/**
 * IR-level type system
 *
 * This is separate from semantic::Type because IR needs implementation
 * details (pointers, exact sizes, etc.) that don't exist at source level.
 *
 * Design:
 * - Primitive types are sized (i64, f64, i1)
 * - Pointers exist (ptr<T>)
 * - Can be constructed from semantic::Type via lowering
 */
class IRType {
public:
    enum class Kind {
        // Primitives (sized)
        I1,      // 1-bit integer (bool)
        I64,     // 64-bit integer
        F64,     // 64-bit float
        String,  // String type (GC-managed)

        // Composite
        Pointer, // ptr<T>
        Array,   // array<T>
        Struct,  // struct with fields
        Function,// function type
        Enum,

        Void     // void (no value)
    };

    virtual ~IRType() = default;

    virtual Kind kind() const = 0;
    virtual std::string toString() const = 0;
    virtual bool equals(const IRType* other) const = 0;

    // Size in bytes (for allocation)
    virtual size_t getSizeInBytes() const = 0;

    // Alignment requirements
    virtual size_t getAlignment() const = 0;

    // Type checking helpers
    bool isPrimitive() const {
        return kind() == Kind::I1 || kind() == Kind::I64 ||
               kind() == Kind::F64 || kind() == Kind::String ||
               kind() == Kind::Void;
    }
    bool isPointer() const { return kind() == Kind::Pointer; }
    bool isArray() const { return kind() == Kind::Array; }
    bool isStruct() const { return kind() == Kind::Struct; }
    bool isFunction() const { return kind() == Kind::Function; }
    bool isEnum() const { return kind() == Kind::Enum; }

    // Safe casting helpers - declared here, defined after class declarations
    class IRPrimitiveType* asPrimitive();
    const class IRPrimitiveType* asPrimitive() const;
    class IRPointerType* asPointer();
    const class IRPointerType* asPointer() const;
    class IRArrayType* asArray();
    const class IRArrayType* asArray() const;
    class IRStructType* asStruct();
    const class IRStructType* asStruct() const;
    class IREnumType* asEnum();
    const class IREnumType* asEnum() const;
};

// Primitive types
class IRPrimitiveType : public IRType {
public:
    explicit IRPrimitiveType(Kind kind) : kind_(kind) {}

    Kind kind() const override { return kind_; }

    std::string toString() const override {
        switch (kind_) {
            case Kind::I1: return "i1";
            case Kind::I64: return "i64";
            case Kind::F64: return "f64";
            case Kind::String: return "str";
            case Kind::Void: return "void";
            default: return "unknown";
        }
    }

    bool equals(const IRType* other) const override {
        if (auto* prim = dynamic_cast<const IRPrimitiveType*>(other)) {
            return kind_ == prim->kind_;
        }
        return false;
    }

    size_t getSizeInBytes() const override {
        switch (kind_) {
            case Kind::I1: return 1;
            case Kind::I64: return 8;
            case Kind::F64: return 8;
            case Kind::String: return 8;  // Pointer to GC-managed string
            case Kind::Void: return 0;
            default: return 0;
        }
    }

    size_t getAlignment() const override {
        return getSizeInBytes();
    }

    bool isInt() const { return kind_ == Kind::I64 ;}
    bool isNumeric() const { return kind_ == Kind::I64 || kind_ == Kind::F64; }

private:
    Kind kind_;
};

// Pointer type
class IRPointerType : public IRType {
public:
    explicit IRPointerType(std::shared_ptr<IRType> pointeeType)
        : pointeeType_(std::move(pointeeType)) {}

    Kind kind() const override { return Kind::Pointer; }

    std::string toString() const override {
        return "ptr<" + pointeeType_->toString() + ">";
    }

    bool equals(const IRType* other) const override {
        if (auto* ptr = dynamic_cast<const IRPointerType*>(other)) {
            return pointeeType_->equals(ptr->pointeeType_.get());
        }
        return false;
    }

    size_t getSizeInBytes() const override {
        return 8; // 64-bit pointers
    }

    size_t getAlignment() const override {
        return 8;
    }

    std::shared_ptr<IRType> pointeeType() const { return pointeeType_; }

private:
    std::shared_ptr<IRType> pointeeType_;
};

// Array type
class IRArrayType : public IRType {
public:
    IRArrayType(std::shared_ptr<IRType> elementType, size_t size)
        : elementType_(std::move(elementType)), size_(size) {}

    Kind kind() const override { return Kind::Array; }

    std::string toString() const override {
        return "[" + std::to_string(size_) + " x " + elementType_->toString() + "]";
    }

    bool equals(const IRType* other) const override {
        if (auto* arr = dynamic_cast<const IRArrayType*>(other)) {
            return size_ == arr->size_ &&
                   elementType_->equals(arr->elementType_.get());
        }
        return false;
    }

    size_t getSizeInBytes() const override {
        return size_ * elementType_->getSizeInBytes();
    }

    size_t getAlignment() const override {
        return elementType_->getAlignment();
    }

    std::shared_ptr<IRType> elementType() const { return elementType_; }
    size_t getSize() const { return size_; }

private:
    std::shared_ptr<IRType> elementType_;
    size_t size_;
};

class IRStructType : public IRType {
public:
    IRStructType(
    std::vector<std::shared_ptr<IRType>> fieldTypes,
    std::vector<std::string> fieldNames = {}  // Optional names for debugging
    ) : fieldTypes_(std::move(fieldTypes)), fieldNames_(std::move(fieldNames)) {}


    Kind kind() const override { return Kind::Struct; }
    
    std::string toString() const override {    
        std::string result = "{ ";
        for (size_t i = 0; i < fieldTypes_.size(); i++) {
            if (i > 0) result += ", ";
            
            // If you have field names, include them:
            if (!fieldNames_.empty() && i < fieldNames_.size()) {
                result += fieldNames_[i] + ": ";
            }
            
            result += fieldTypes_[i]->toString();
        }
        result += " }";
        return result;
    }

    size_t getSizeInBytes() const override {
        size_t total = 0;
        for (const auto& fieldType : fieldTypes_) {
            total += fieldType->getSizeInBytes();
        }
        return total;
    }

    size_t getAlignment() const override {
        size_t maxAlign = 1;
        for (const auto& fieldType : fieldTypes_) {
            maxAlign = std::max(maxAlign, fieldType->getAlignment());
        }
        return maxAlign;
    }

    bool equals(const IRType* other) const override {
        if (auto* structType = dynamic_cast<const IRStructType*>(other)) {
            if (fieldTypes_.size() != structType->fieldTypes_.size()) return false;
            for (size_t i = 0; i < fieldTypes_.size(); i++) {
                if (!fieldTypes_[i]->equals(structType->fieldTypes_[i].get())) return false;
            }
            return true;
        }
        return false;
    }

    unsigned getNumFields() const { return fieldTypes_.size(); }
    std::shared_ptr<IRType> getFieldTypeAtIdx(unsigned idx) const { return fieldTypes_.at(idx);}
    
private:
    std::vector<std::shared_ptr<IRType>> fieldTypes_;
    std::vector<std::string> fieldNames_;
};

class IREnumType : public IRType {
public:
    IREnumType(
        std::vector<std::vector<std::shared_ptr<IRType>>> variantTypes,
        std::vector<std::string> variantNames
    ) : variantTypes_(variantTypes), variantNames_(variantNames) {}

    Kind kind() const override { return Kind::Enum; }

    std::string toString() const override {    
        std::string result = "enum { ";  // Add space after {
        
        for (size_t i = 0; i < variantTypes_.size(); i++) {
            const auto& varType = variantTypes_[i];
            result += variantNames_[i] + "(";
            
            // Inner loop: iterate through fields in THIS variant
            for (size_t j = 0; j < varType.size(); j++) {
                result += varType[j]->toString();
                if (j < varType.size() - 1) {  // Not last field?
                    result += ", ";
                }
            }
            
            result += ")";
            
            // Add comma between variants (but not after last variant)
            if (i < variantTypes_.size() - 1) {
                result += ", ";
            }
        }
        
        result += " }";
        return result;
    }

    size_t getSizeInBytes() const override {
        const size_t TAG_SIZE = 4;  
        size_t maxVariantSize = 0;
        
        // For each variant, calculate its total size
        for (const auto& varType : variantTypes_) {
            size_t variantSize = 0;
            for (const auto& fieldType : varType) {
                variantSize += fieldType->getSizeInBytes();
            }
            maxVariantSize = std::max(maxVariantSize, variantSize);
        }
        
        return TAG_SIZE + maxVariantSize;
    }

    size_t getAlignment() const override {
        size_t maxAlign = 4;  // At least 4 for the tag
    
        for (const auto& varType : variantTypes_) {
            for (const auto& fieldType : varType) {
                maxAlign = std::max(maxAlign, fieldType->getAlignment());
            }
        }
        
        return maxAlign;
    }

    bool equals(const IRType* other) const override {
        if (auto* enumType = dynamic_cast<const IREnumType*>(other)) {
            if (variantTypes_.size() != enumType->variantTypes_.size()) 
                return false;
                
            for (size_t i = 0; i < variantTypes_.size(); i++) {
                if (variantTypes_[i].size() != enumType->variantTypes_[i].size())
                    return false;
                    
                for (size_t j = 0; j < variantTypes_[i].size(); j++) {
                    if (!variantTypes_[i][j]->equals(enumType->variantTypes_[i][j].get()))
                        return false;
                }
            }

            return true;
        }
        return false;
    }

    size_t getNumVariants() const { return variantTypes_.size();}
    std::vector<std::shared_ptr<IRType>> getVariantTypes(size_t idx) { return variantTypes_.at(idx); }

private:
    std::vector<std::vector<std::shared_ptr<IRType>>> variantTypes_;
    std::vector<std::string> variantNames_; 
};


/**
 * Type promotion for binary operations
 * Handles implicit conversions (int → float, etc.)
 */
class TypePromotion {
public:
    /**
     * Promotes types for binary operations
     * Rules:
     * - float + int → float (promote int to float)
     * - int + float → float (promote int to float)
     * - int + int → int
     * - float + float → float
     */
    static std::shared_ptr<IRType> promote(std::shared_ptr<IRType> lhsType,
                                           std::shared_ptr<IRType> rhsType) {
        auto* lhsPrim = lhsType->asPrimitive();
        auto* rhsPrim = rhsType->asPrimitive();

        if (!lhsPrim || !rhsPrim) {
            // Non-primitive types must match exactly
            if (!lhsType->equals(rhsType.get())) {
                return nullptr; // Type mismatch
            }
            return lhsType;
        }

        // If either is float, promote to float
        if (lhsPrim->kind() == IRType::Kind::F64 ||
            rhsPrim->kind() == IRType::Kind::F64) {
            return std::make_shared<IRPrimitiveType>(IRType::Kind::F64);
        }

        // If either is i64, promote to i64
        if (lhsPrim->kind() == IRType::Kind::I64 ||
            rhsPrim->kind() == IRType::Kind::I64) {
            return std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
        }

        // Both are i1 (bool)
        if (lhsPrim->kind() == IRType::Kind::I1 &&
            rhsPrim->kind() == IRType::Kind::I1) {
            return std::make_shared<IRPrimitiveType>(IRType::Kind::I1);
        }

        // Default: return lhs type
        return lhsType;
    }
};

/**
 * Type lowering: semantic::Type → ir::IRType
 *
 * This converts source-level types to IR types.
 * Example:
 *   semantic int → ir::i64
 *   semantic mut int → ir::ptr<i64> (pointer for mutability)
 */
class TypeLowering {
public:
    // Convert semantic type to IR type
    static std::shared_ptr<IRType> lower(std::shared_ptr<semantic::Type> semType) {
        switch (semType->kind()) {
            case semantic::Type::Kind::Int:
                return std::make_shared<IRPrimitiveType>(IRType::Kind::I64);

            case semantic::Type::Kind::Float:
                return std::make_shared<IRPrimitiveType>(IRType::Kind::F64);

            case semantic::Type::Kind::Bool:
                return std::make_shared<IRPrimitiveType>(IRType::Kind::I1);

            case semantic::Type::Kind::String:
                return std::make_shared<IRPrimitiveType>(IRType::Kind::String);

            case semantic::Type::Kind::Void:
                return std::make_shared<IRPrimitiveType>(IRType::Kind::Void);

            case semantic::Type::Kind::Array: {
                // Lower Array[T] to IR ptr<T> (arrays are heap-allocated pointers)
                auto* arrType = dynamic_cast<semantic::ArrayType*>(semType.get());
                if (arrType) {
                    auto elementIRType = lower(arrType->elementType());
                    return std::make_shared<IRPointerType>(elementIRType);
                }
                return std::make_shared<IRPrimitiveType>(IRType::Kind::Void);
            }

            case semantic::Type::Kind::Struct: {
                // Lower Struct to IR struct type
                auto* structType = dynamic_cast<semantic::StructType*>(semType.get());
                if (structType) {
                    std::vector<std::shared_ptr<IRType>> fieldTypes;
                    for (const auto& field : structType->fields()) {
                        fieldTypes.push_back(lower(field.type));
                    }
                    return std::make_shared<IRStructType>(fieldTypes);
                }
                return std::make_shared<IRPrimitiveType>(IRType::Kind::Void);
            }

            case semantic::Type::Kind::Enum: {
                auto* enumType = dynamic_cast<semantic::EnumType*>(semType.get());
                if (enumType) {
                    std::vector<std::vector<std::shared_ptr<IRType>>> variantTypes;
                    std::vector<std::string> variantNames;
                    
                    for (const auto& variant : enumType->variants()) {
                        std::vector<std::shared_ptr<IRType>> fieldTypes;
                        for (const auto& fieldType : variant.associatedTypes) {
                            fieldTypes.push_back(lower(fieldType));
                        }
                        variantTypes.push_back(fieldTypes);
                        variantNames.push_back(variant.name);
                    }
                    
                    return std::make_shared<IREnumType>(variantTypes, variantNames);
                }
                return std::make_shared<IRPrimitiveType>(IRType::Kind::Void);
            }

            default:
                // Fallback
                return std::make_shared<IRPrimitiveType>(IRType::Kind::Void);
        }
    }

    // Get pointer to a type (for alloca, mutable variables)
    static std::shared_ptr<IRType> pointerTo(std::shared_ptr<IRType> type) {
        return std::make_shared<IRPointerType>(type);
    }

    // Get pointer to a semantic type (shorthand)
    static std::shared_ptr<IRType> pointerTo(std::shared_ptr<semantic::Type> semType) {
        return pointerTo(lower(semType));
    }
};

// ============================================================================
// IRType casting helper implementations
// ============================================================================

inline IRPrimitiveType* IRType::asPrimitive() {
    return isPrimitive() ? static_cast<IRPrimitiveType*>(this) : nullptr;
}

inline const IRPrimitiveType* IRType::asPrimitive() const {
    return isPrimitive() ? static_cast<const IRPrimitiveType*>(this) : nullptr;
}

inline IRPointerType* IRType::asPointer() {
    return isPointer() ? static_cast<IRPointerType*>(this) : nullptr;
}

inline const IRPointerType* IRType::asPointer() const {
    return isPointer() ? static_cast<const IRPointerType*>(this) : nullptr;
}

inline IRArrayType* IRType::asArray() {
    return isArray() ? static_cast<IRArrayType*>(this) : nullptr;
}

inline const IRArrayType* IRType::asArray() const {
    return isArray() ? static_cast<const IRArrayType*>(this) : nullptr;
}

inline class IRStructType* IRType::asStruct() {
    return isStruct() ? static_cast<IRStructType*>(this) : nullptr;
}

inline const class IRStructType* IRType::asStruct() const {
    return isStruct() ? static_cast<const IRStructType*>(this) : nullptr;
}


inline IREnumType* IRType::asEnum() {
    return isEnum() ? static_cast<IREnumType*>(this) : nullptr;
}

inline const IREnumType* IRType::asEnum() const {
    return isEnum() ? static_cast<const IREnumType*>(this) : nullptr;
}

struct IRTypeHash {
    size_t operator()(const std::shared_ptr<IRType>& type) const {
        // Hash based on kind and structural properties
        size_t h = std::hash<int>{}(static_cast<int>(type->kind()));

        if (auto* ptr = type->asPointer()) {
            h ^= (*this)(ptr->pointeeType()) << 1;
        } else if (auto* arr = type->asArray()) {
            h ^= (*this)(arr->elementType()) << 1;
            h ^= std::hash<size_t>{}(arr->getSize()) << 2;
        } 

        return h;
    }
};

struct IRTypeEqual {
    bool operator()(const std::shared_ptr<IRType>& a,
                   const std::shared_ptr<IRType>& b) const {
        return a->equals(b.get());
    }
};

} // namespace volta::ir