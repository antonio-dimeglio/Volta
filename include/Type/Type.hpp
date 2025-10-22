#pragma once

#include "TypeKind.hpp"
#include <string>
#include <vector>

namespace Type {

struct Type {
    TypeKind kind;

    explicit Type(TypeKind kind) : kind(kind) {}
    virtual ~Type() = default;

    [[nodiscard]] virtual std::string toString() const = 0;
    virtual bool equals(const Type* other) const = 0;
};

struct PrimitiveType : Type {
    PrimitiveKind kind;

    explicit PrimitiveType(PrimitiveKind kind) : Type(TypeKind::Primitive), kind(kind) {}

    // Check if this primitive type is unsigned
    [[nodiscard]] bool isUnsigned() const {
        return kind == PrimitiveKind::U8 ||
               kind == PrimitiveKind::U16 ||
               kind == PrimitiveKind::U32 ||
               kind == PrimitiveKind::U64;
    }

    // Check if this primitive type is signed integer
    [[nodiscard]] bool isSigned() const {
        return kind == PrimitiveKind::I8 ||
               kind == PrimitiveKind::I16 ||
               kind == PrimitiveKind::I32 ||
               kind == PrimitiveKind::I64;
    }

    // Check if this is an integer type (signed or unsigned)
    [[nodiscard]] bool isInteger() const {
        return isSigned() || isUnsigned();
    }

    [[nodiscard]] std::string toString() const override {
        switch (kind) {
            case PrimitiveKind::I8:     return "i8";
            case PrimitiveKind::I16:    return "i16";
            case PrimitiveKind::I32:    return "i32";
            case PrimitiveKind::I64:    return "i64";
            case PrimitiveKind::U8:     return "u8";
            case PrimitiveKind::U16:    return "u16";
            case PrimitiveKind::U32:    return "u32";
            case PrimitiveKind::U64:    return "u64";
            case PrimitiveKind::F32:    return "f32";
            case PrimitiveKind::F64:    return "f64";
            case PrimitiveKind::Bool:   return "bool";
            case PrimitiveKind::String: return "str";
            case PrimitiveKind::Void:   return "void";
        }
        return "unknown";
    }

    bool equals(const Type* other) const override {
        if (const auto* prim = dynamic_cast<const PrimitiveType*>(other)) {
            return kind == prim->kind;
        }
        return false;
    }
};

struct ArrayType : Type {
    const Type* elementType;
    int size;

    ArrayType(const Type* elementType, int size)
        : Type(TypeKind::Array), elementType(elementType), size(size) {}

    [[nodiscard]] std::string toString() const override {
        return "[" + elementType->toString() + "; " + std::to_string(size) + "]";
    }

    bool equals(const Type* other) const override {
        if (const auto* arr = dynamic_cast<const ArrayType*>(other)) {
            return size == arr->size && elementType->equals(arr->elementType);
        }
        return false;
    }
};

struct MethodSignature {
    std::string name;
    std::vector<const Type*> paramTypes;
    const Type* returnType;
    bool hasSelf;        // true for instance methods
    bool hasMutSelf;     // true for mutable instance methods
    bool isPublic;

    MethodSignature(const std::string& name,
                    const std::vector<const Type*>& params,
                    const Type* retType,
                    bool self = false,
                    bool mutSelf = false,
                    bool pub = false)
        : name(name), paramTypes(params), returnType(retType),
          hasSelf(self), hasMutSelf(mutSelf), isPublic(pub) {}
};

struct FieldInfo {
    std::string name;
    const Type* type;
    bool isPublic;

    FieldInfo(const std::string& n, const Type* t, bool pub)
        : name(n), type(t), isPublic(pub) {}
};

struct StructType : Type {
    std::string name;
    std::vector<FieldInfo> fields;
    std::vector<MethodSignature> methods;

    StructType(const std::string& name,
               const std::vector<FieldInfo>& fields)
        : Type(TypeKind::Struct), name(name), fields(fields) {}

    [[nodiscard]] std::string toString() const override {
        return name;
    }

    bool equals(const Type* other) const override {
        if (const auto* st = dynamic_cast<const StructType*>(other)) {
            return name == st->name;
        }
        return false;
    }

    [[nodiscard]] const Type* getFieldType(const std::string& fieldName) const {
        for (const auto& field : fields) {
            if (field.name == fieldName) { return field.type;
}
        }
        return nullptr;
    }

    [[nodiscard]] bool isFieldPublic(const std::string& fieldName) const {
        for (const auto& field : fields) {
            if (field.name == fieldName) { return field.isPublic;
}
        }
        return false;  // Field not found, treat as private
    }

    [[nodiscard]] const MethodSignature* getMethod(const std::string& methodName) const {
        for (const auto& method : methods) {
            if (method.name == methodName) { return &method;
}
        }
        return nullptr;
    }

    void addMethod(const MethodSignature& method) {
        methods.push_back(method);
    }
};

struct GenericType : Type {
    std::string name;
    std::vector<const Type*> typeParams;

    GenericType(const std::string& name, std::vector<const Type*> typeParams)
        : Type(TypeKind::Generic), name(name), typeParams(typeParams) {}

    [[nodiscard]] std::string toString() const override {
        std::string result = name + "<";
        for (size_t i = 0; i < typeParams.size(); ++i) {
            if (i > 0) { result += ", ";
}
            result += typeParams[i]->toString();
        }
        result += ">";
        return result;
    }

    bool equals(const Type* other) const override {
        if (const auto* gen = dynamic_cast<const GenericType*>(other)) {
            if (name != gen->name || typeParams.size() != gen->typeParams.size()) {
                return false;
            }
            for (size_t i = 0; i < typeParams.size(); ++i) {
                if (!typeParams[i]->equals(gen->typeParams[i])) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }
};

struct OpaqueType : Type {
    OpaqueType() : Type(TypeKind::Opaque) {}

    [[nodiscard]] std::string toString() const override {
        return "opaque";
    }

    bool equals(const Type* other) const override {
        return other->kind == TypeKind::Opaque;
    }
};

struct PointerType : Type {
    const Type* pointeeType;

    explicit PointerType(const Type* pointee)
        : Type(TypeKind::Pointer), pointeeType(pointee) {}

    [[nodiscard]] std::string toString() const override {
        return "ptr " + pointeeType->toString();
    }

    bool equals(const Type* other) const override {
        if (const auto* ptr = dynamic_cast<const PointerType*>(other)) {
            return pointeeType->equals(ptr->pointeeType);
        }
        return false;
    }
};

// UnresolvedType represents a type name that hasn't been resolved yet
// (e.g., struct names encountered during parsing before semantic analysis)
// This will be resolved to the actual type during semantic analysis
struct UnresolvedType : Type {
    std::string name;

    explicit UnresolvedType(const std::string& name)
        : Type(TypeKind::Unresolved), name(name) {}

    [[nodiscard]] std::string toString() const override {
        return name + " (unresolved)";
    }

    bool equals(const Type* other) const override {
        if (const auto* unresolved = dynamic_cast<const UnresolvedType*>(other)) {
            return name == unresolved->name;
        }
        return false;
    }
};

};