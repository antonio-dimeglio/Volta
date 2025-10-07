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
        Option,  // Option[T] (for null safety)

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
    bool isOption() const { return kind() == Kind::Option; }
    bool isStruct() const { return kind() == Kind::Struct; }
    bool isFunction() const { return kind() == Kind::Function; }

    // Safe casting helpers - declared here, defined after class declarations
    class IRPrimitiveType* asPrimitive();
    const class IRPrimitiveType* asPrimitive() const;
    class IRPointerType* asPointer();
    const class IRPointerType* asPointer() const;
    class IRArrayType* asArray();
    const class IRArrayType* asArray() const;
    class IROptionType* asOption();
    const class IROptionType* asOption() const;
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

// Option type (for null safety)
class IROptionType : public IRType {
public:
    explicit IROptionType(std::shared_ptr<IRType> innerType)
        : innerType_(std::move(innerType)) {}

    Kind kind() const override { return Kind::Option; }

    std::string toString() const override {
        return "Option<" + innerType_->toString() + ">";
    }

    bool equals(const IRType* other) const override {
        if (auto* opt = dynamic_cast<const IROptionType*>(other)) {
            return innerType_->equals(opt->innerType_.get());
        }
        return false;
    }

    size_t getSizeInBytes() const override {
        // Option is represented as a tagged union:
        // 1 byte for tag (Some/None) + size of inner type
        return 1 + innerType_->getSizeInBytes();
    }

    size_t getAlignment() const override {
        // Use the larger of 1 (for tag) and inner type alignment
        return std::max(size_t(1), innerType_->getAlignment());
    }

    std::shared_ptr<IRType> innerType() const { return innerType_; }

private:
    std::shared_ptr<IRType> innerType_;
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

            case semantic::Type::Kind::Option: {
                // Lower Option[T] to IR Option<T>
                auto* optType = dynamic_cast<semantic::OptionType*>(semType.get());
                if (optType) {
                    auto innerIRType = lower(optType->innerType());
                    return std::make_shared<IROptionType>(innerIRType);
                }
                return std::make_shared<IRPrimitiveType>(IRType::Kind::Void);
            }

            case semantic::Type::Kind::Array: {
                // Lower Array[T] to IR ptr<T> (arrays are heap-allocated pointers)
                auto* arrType = dynamic_cast<semantic::ArrayType*>(semType.get());
                if (arrType) {
                    auto elementIRType = lower(arrType->elementType());
                    return std::make_shared<IRPointerType>(elementIRType);
                }
                return std::make_shared<IRPrimitiveType>(IRType::Kind::Void);
            }

            // TODO: Handle Struct, etc.

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

inline IROptionType* IRType::asOption() {
    return isOption() ? static_cast<IROptionType*>(this) : nullptr;
}

inline const IROptionType* IRType::asOption() const {
    return isOption() ? static_cast<const IROptionType*>(this) : nullptr;
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
        } else if (auto* opt = type->asOption()) {
            h ^= (*this)(opt->innerType()) << 1;
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