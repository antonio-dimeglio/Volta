#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Expression.hpp"

namespace volta::ast {

// Base type class
struct Type {
    virtual ~Type() = default;
};

// Primitive types: signed/unsigned integers, floats, bool, str, void
struct PrimitiveType : Type {
    enum class Kind {
        // Signed integers
        I8,
        I16,
        I32,
        I64,
        // Unsigned integers
        U8,
        U16,
        U32,
        U64,
        // Floating point
        F8,
        F16,
        F32,
        F64,
        // Other primitives
        Bool,
        Str,
        Void
    };

    Kind kind;

    PrimitiveType(Kind kind) : kind(kind) {}
};

// Compound types: Array[T], Matrix[T], Option[T], (T1, T2, ...)
struct CompoundType : Type {
    enum class Kind {
        Array,
        Matrix,
        Option,
        Tuple
    };

    Kind kind;
    std::vector<std::unique_ptr<Type>> typeArgs;

    CompoundType(Kind kind, std::vector<std::unique_ptr<Type>> typeArgs)
        : kind(kind), typeArgs(std::move(typeArgs)) {}
};

// Generic types: Identifier[T1, T2, ...]
struct GenericType : Type {
    std::unique_ptr<IdentifierExpression> identifier;
    std::vector<std::unique_ptr<Type>> typeArgs;

    GenericType(
        std::unique_ptr<IdentifierExpression> identifier,
        std::vector<std::unique_ptr<Type>> typeArgs
    ) : identifier(std::move(identifier)),
        typeArgs(std::move(typeArgs)) {}
};

// Function types: fn(T1, T2) -> T
struct FunctionType : Type {
    std::vector<std::unique_ptr<Type>> paramTypes;
    std::unique_ptr<Type> returnType;

    FunctionType(
        std::vector<std::unique_ptr<Type>> paramTypes,
        std::unique_ptr<Type> returnType
    ) : paramTypes(std::move(paramTypes)),
        returnType(std::move(returnType)) {}
};

// Named type (user-defined or identifier reference)
struct NamedType : Type {
    std::unique_ptr<IdentifierExpression> identifier;

    NamedType(std::unique_ptr<IdentifierExpression> identifier)
        : identifier(std::move(identifier)) {}
};

// Type annotation: mut? Type
struct TypeAnnotation {
    bool isMutable;
    std::unique_ptr<Type> type;

    TypeAnnotation(bool isMutable, std::unique_ptr<Type> type)
        : isMutable(isMutable), type(std::move(type)) {}
};

}