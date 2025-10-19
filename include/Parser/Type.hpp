#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

// Forward declarations
struct Type;
struct PrimitiveType;
struct ArrayType;
struct GenericType;

// Enum for primitive type kinds
enum class PrimitiveTypeKind {
    I8, I16, I32, I64,
    U8, U16, U32, U64,
    F32, F64,
    Bool,
    Char,
    String,
    Void
};

// Base class for all type representations
struct Type {
    virtual ~Type() = default;

    // Get string representation of the type
    virtual std::string toString() const = 0;

    // Optional: Type equality check
    virtual bool equals(const Type* other) const = 0;
};

// Simple types: i32, bool, etc.
struct PrimitiveType : Type {
    PrimitiveTypeKind kind;

    explicit PrimitiveType(PrimitiveTypeKind kind) : kind(kind) {}

    std::string toString() const override {
        switch (kind) {
            case PrimitiveTypeKind::I8:     return "i8";
            case PrimitiveTypeKind::I16:    return "i16";
            case PrimitiveTypeKind::I32:    return "i32";
            case PrimitiveTypeKind::I64:    return "i64";
            case PrimitiveTypeKind::U8:     return "u8";
            case PrimitiveTypeKind::U16:    return "u16";
            case PrimitiveTypeKind::U32:    return "u32";
            case PrimitiveTypeKind::U64:    return "u64";
            case PrimitiveTypeKind::F32:    return "f32";
            case PrimitiveTypeKind::F64:    return "f64";
            case PrimitiveTypeKind::Bool:   return "bool";
            case PrimitiveTypeKind::Char:   return "char";
            case PrimitiveTypeKind::String: return "string";
            case PrimitiveTypeKind::Void:   return "void";
        }
        return "unknown";
    }

    bool equals(const Type* other) const override {
        if (auto* prim = dynamic_cast<const PrimitiveType*>(other)) {
            return kind == prim->kind;
        }
        return false;
    }
};

// Array type: [T; size]
struct ArrayType : Type {
    std::unique_ptr<Type> element_type;
    int size;

    ArrayType(std::unique_ptr<Type> element_type, int size)
        : element_type(std::move(element_type)), size(size) {}

    std::string toString() const override {
        return "[" + element_type->toString() + "; " + std::to_string(size) + "]";
    }

    bool equals(const Type* other) const override {
        if (auto* arr = dynamic_cast<const ArrayType*>(other)) {
            return size == arr->size && element_type->equals(arr->element_type.get());
        }
        return false;
    }
};

// Generic types: Array<T>, Result<T, E>
struct GenericType : Type {
    std::string name;  // "Array", "Result"
    std::vector<std::unique_ptr<Type>> type_params;

    GenericType(const std::string& name, std::vector<std::unique_ptr<Type>> type_params)
        : name(name), type_params(std::move(type_params)) {}

    std::string toString() const override {
        std::string result = name + "<";
        for (size_t i = 0; i < type_params.size(); ++i) {
            if (i > 0) result += ", ";
            result += type_params[i]->toString();
        }
        result += ">";
        return result;
    }

    bool equals(const Type* other) const override {
        if (auto* gen = dynamic_cast<const GenericType*>(other)) {
            if (name != gen->name || type_params.size() != gen->type_params.size()) {
                return false;
            }
            for (size_t i = 0; i < type_params.size(); ++i) {
                if (!type_params[i]->equals(gen->type_params[i].get())) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }
};

// Only for ptrs.
struct OpaqueType : Type {
    std::string toString() const override {
        return "opaque";
    }

    bool equals(const Type* other) const override {
        return dynamic_cast<const OpaqueType*>(other) != nullptr;
    }
};

// Helper function to convert string to PrimitiveTypeKind (useful for parsing)
// Returns std::nullopt if the string doesn't match any primitive type
inline std::optional<PrimitiveTypeKind> stringToPrimitiveTypeKind(const std::string& str) {
    if (str == "i8")     return PrimitiveTypeKind::I8;
    if (str == "i16")    return PrimitiveTypeKind::I16;
    if (str == "i32")    return PrimitiveTypeKind::I32;
    if (str == "i64")    return PrimitiveTypeKind::I64;
    if (str == "u8")     return PrimitiveTypeKind::U8;
    if (str == "u16")    return PrimitiveTypeKind::U16;
    if (str == "u32")    return PrimitiveTypeKind::U32;
    if (str == "u64")    return PrimitiveTypeKind::U64;
    if (str == "f32")    return PrimitiveTypeKind::F32;
    if (str == "f64")    return PrimitiveTypeKind::F64;
    if (str == "bool")   return PrimitiveTypeKind::Bool;
    if (str == "char")   return PrimitiveTypeKind::Char;
    if (str == "string") return PrimitiveTypeKind::String;
    if (str == "void")   return PrimitiveTypeKind::Void;

    return std::nullopt;
}

// Helper to check if a string is a valid primitive type
inline bool isPrimitiveType(const std::string& str) {
    return stringToPrimitiveTypeKind(str).has_value();
}