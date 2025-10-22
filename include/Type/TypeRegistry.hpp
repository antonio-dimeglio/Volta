#pragma once

#include "Type.hpp"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <vector>
#include <deque>
#include <string>

namespace Type {

// Hash function for array types (based on element type + size)
struct ArrayTypeKey {
    const Type* elementType;
    int size;

    bool operator==(const ArrayTypeKey& other) const {
        return elementType == other.elementType && size == other.size;
    }
};

struct ArrayTypeHash {
    size_t operator()(const ArrayTypeKey& key) const {
        return std::hash<const Type*>{}(key.elementType) ^ (std::hash<int>{}(key.size) << 1);
    }
};

// Hash function for generic types (based on name + type params)
struct GenericTypeKey {
    std::string name;
    std::vector<const Type*> typeParams;

    bool operator==(const GenericTypeKey& other) const {
        return name == other.name && typeParams == other.typeParams;
    }
};

struct GenericTypeHash {
    size_t operator()(const GenericTypeKey& key) const {
        size_t h = std::hash<std::string>{}(key.name);
        for (const Type* param : key.typeParams) {
            h ^= (std::hash<const Type*>{}(param) << 1);
        }
        return h;
    }
};

class TypeRegistry {
private:
    std::deque<std::unique_ptr<Type>> ownedTypes;
    std::unordered_map<PrimitiveKind, PrimitiveType*> primitiveCache;
    std::unordered_map<ArrayTypeKey, ArrayType*, ArrayTypeHash> arrayCache;
    std::unordered_map<const Type*, PointerType*> pointerCache;
    std::unordered_map<GenericTypeKey, GenericType*, GenericTypeHash> genericCache;
    std::unordered_map<std::string, StructType*> structCache;
    std::unordered_map<std::string, UnresolvedType*> unresolvedCache;
    OpaqueType* opaqueType = nullptr;

public:
    TypeRegistry();

    // Returns the primitive type associated with that primitive kind.
    const PrimitiveType* getPrimitive(PrimitiveKind kind);

    // Get or create an array type of the specific format
    const ArrayType* getArray(const Type* elementType, int size);

    // Get or creates a generic type
    const GenericType* getGeneric(const std::string& name, const std::vector<const Type*>& typeParams);

    // Get or creates a pointer to the type of the pointee.
    const PointerType* getPointer(const Type* pointeeType);

    // Get opaque type
    const OpaqueType* getOpaque();

    // Get or create an unresolved type (for forward references)
    const UnresolvedType* getUnresolved(const std::string& name);

    // Register a new struct type (called during semantic analysis)
    // Returns nullptr if a struct with this name already exists
    StructType* registerStruct(const std::string& name,
                               const std::vector<FieldInfo>& fields);

    // Register a struct name as a placeholder (for HIR desugaring before semantic analysis)
    // This allows HIR lowering to check if "Point" is a type name for Point.new() desugaring
    // Returns the stub struct type, or existing type if already registered
    StructType* registerStructStub(const std::string& name);

    // Get a registered struct type by name
    // Returns nullptr if not found
    StructType* getStruct(const std::string& name);

    // Check if a struct type is registered
    [[nodiscard]] bool hasStruct(const std::string& name) const;

    // Utility function that converts a string to a type
    const Type* parseTypeName(const std::string& name);
};

}