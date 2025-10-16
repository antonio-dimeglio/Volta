#pragma once

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <functional>
#include "../Parser/Type.hpp"
#include "../Lexer/TokenType.hpp"

// LLVM forward declaration
namespace llvm {
    class Type;
    class LLVMContext;
}

// Forward declarations
struct Literal;

namespace Semantic {

// ============================================================================
// Interned Type System
// ============================================================================
// The TypeRegistry owns all types and returns const pointers.
// Semantic analysis uses pointer equality for fast type comparison.
// AST parsing still uses unique_ptr<Type>, but TypeRegistry interns them.

/**
 * Hash and equality functors for interning array types
 */
struct ArrayTypeHash {
    size_t operator()(const std::unique_ptr<ArrayType>& arr) const {
        // Hash based on element type VALUE (using toString) and size
        // We can't use pointer hash because element types are clones
        size_t h1 = std::hash<std::string>{}(arr->element_type->toString());
        size_t h2 = std::hash<int>{}(arr->size);
        return h1 ^ (h2 << 1);
    }
};

struct ArrayTypeEqual {
    bool operator()(const std::unique_ptr<ArrayType>& a,
                   const std::unique_ptr<ArrayType>& b) const {
        // For interned types, we need to compare element types by VALUE
        // because the element types are clones, not interned pointers
        return a->size == b->size &&
               a->element_type->equals(b->element_type.get());
    }
};

/**
 * Hash and equality functors for interning generic types
 */
struct GenericTypeHash {
    size_t operator()(const std::unique_ptr<GenericType>& gen) const {
        size_t h = std::hash<std::string>{}(gen->name);
        for (const auto& param : gen->type_params) {
            h ^= std::hash<const Type*>{}(param.get()) << 1;
        }
        return h;
    }
};

struct GenericTypeEqual {
    bool operator()(const std::unique_ptr<GenericType>& a,
                   const std::unique_ptr<GenericType>& b) const {
        if (a->name != b->name || a->type_params.size() != b->type_params.size()) {
            return false;
        }
        for (size_t i = 0; i < a->type_params.size(); ++i) {
            if (!a->type_params[i]->equals(b->type_params[i].get())) {
                return false;
            }
        }
        return true;
    }
};

/**
 * TypeRegistry: Central type system with interning
 *
 * Responsibilities:
 * 1. Type Interning: Each unique type exists once, enabling fast pointer comparison
 * 2. Type Validation: Check if types are valid
 * 3. Type Compatibility: Check if types can be implicitly converted
 * 4. Operator Type Inference: Determine result types of operations
 * 5. Literal Type Inference: Determine types of literals
 */
class TypeRegistry {
private:
    // ========== Primitive Types (Pre-interned) ==========
    std::unique_ptr<PrimitiveType> i8_type;
    std::unique_ptr<PrimitiveType> i16_type;
    std::unique_ptr<PrimitiveType> i32_type;
    std::unique_ptr<PrimitiveType> i64_type;
    std::unique_ptr<PrimitiveType> u8_type;
    std::unique_ptr<PrimitiveType> u16_type;
    std::unique_ptr<PrimitiveType> u32_type;
    std::unique_ptr<PrimitiveType> u64_type;
    std::unique_ptr<PrimitiveType> f32_type;
    std::unique_ptr<PrimitiveType> f64_type;
    std::unique_ptr<PrimitiveType> bool_type;
    std::unique_ptr<PrimitiveType> char_type;
    std::unique_ptr<PrimitiveType> string_type;
    std::unique_ptr<PrimitiveType> void_type;

    // ========== Compound Types (Interned on demand) ==========
    std::unordered_set<std::unique_ptr<ArrayType>, ArrayTypeHash, ArrayTypeEqual> array_types;
    std::unordered_set<std::unique_ptr<GenericType>, GenericTypeHash, GenericTypeEqual> generic_types;

    // ========== Type Width Information ==========
    std::unordered_map<PrimitiveTypeKind, int> type_widths;

public:
    TypeRegistry();

    // ========== Type Interning ==========

    /**
     * Get interned primitive type (always succeeds)
     */
    const PrimitiveType* getPrimitive(PrimitiveTypeKind kind) const;

    /**
     * Intern array type (creates if doesn't exist)
     * Returns interned type pointer
     */
    const ArrayType* internArray(const Type* element_type, int size);

    /**
     * Intern generic type (creates if doesn't exist)
     * Returns interned type pointer
     */
    const GenericType* internGeneric(const std::string& name,
                                     const std::vector<const Type*>& type_params);

    /**
     * Intern type from AST representation
     * This is the main entry point for converting parser types to interned types
     */
    const Type* internFromAST(const Type* ast_type);

    // ========== Type Validation ==========

    /**
     * Check if a type is valid (exists in registry)
     */
    bool isValidType(const Type* type) const;

    // ========== Type Compatibility ==========

    /**
     * Check if source type can be assigned to target type.
     * Returns true if:
     * - Types are identical (pointer equality for interned types!)
     * - Valid implicit cast exists (e.g., i32 -> i64)
     */
    bool areTypesCompatible(const Type* target, const Type* source) const;

    /**
     * Check if explicit cast is valid (for future 'as' operator)
     * Examples: i64 -> i32 (narrowing), f32 -> i32 (conversion)
     */
    bool canCastExplicitly(const Type* from_type, const Type* to_type) const;

    // ========== Operator Type Inference ==========

    /**
     * Infer result type of binary operation
     * Returns nullptr if operation is invalid
     *
     * Examples:
     * - (i32, +, i32) -> i32
     * - (i32, +, f32) -> nullptr (type mismatch)
     * - (i32, ==, i32) -> bool
     */
    const Type* inferBinaryOp(const Type* left_type, TokenType op, const Type* right_type);

    /**
     * Infer result type of unary operation
     * Returns nullptr if operation is invalid
     *
     * Examples:
     * - (-, i32) -> i32
     * - (!, bool) -> bool
     */
    const Type* inferUnaryOp(TokenType op, const Type* operand_type);

    // ========== Literal Type Inference ==========

    /**
     * Determine type of a literal from AST node
     * Uses Token type to identify literal kind
     */
    const Type* inferLiteral(const Literal* literal);

    /**
     * Validate that a literal value fits within target type bounds
     * Returns true if valid, false otherwise
     *
     * Examples:
     * - Literal(32), i8 -> true (32 fits in -128 to 127)
     * - Literal(300), i8 -> false (300 > 127)
     */
    bool validateLiteralBounds(const Literal* literal, const Type* target_type);

    // ========== Type Category Checks ==========

    bool isInteger(const Type* type) const;
    bool isSigned(const Type* type) const;
    bool isUnsigned(const Type* type) const;
    bool isFloat(const Type* type) const;
    bool isNumeric(const Type* type) const;
    bool isArray(const Type* type) const;
    bool isGeneric(const Type* type) const;

    // ========== Array Type Utilities ==========

    /**
     * Get element type of an array (returns nullptr if not array)
     */
    const Type* getArrayElementType(const Type* type) const;

    /**
     * Get size of an array (returns -1 if not array)
     */
    int getArraySize(const Type* type) const;

    // ========== LLVM Type Conversion ==========

    /**
     * Convert Volta type to LLVM type for codegen
     * Handles primitives, arrays, and future compound types
     */
    llvm::Type* toLLVMType(const Type* type, llvm::LLVMContext& ctx) const;

private:
    // Helper to get type width for compatibility checks
    int getTypeWidth(PrimitiveTypeKind kind) const;

    // Helper to recursively intern types
    const Type* internTypeRecursive(const Type* ast_type);
};

} // namespace Semantic
