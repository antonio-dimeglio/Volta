#pragma once

#include "gc/TypeRegistry.hpp"
#include <unordered_map>

using namespace Volta::GC;

/**
 * Simple TypeRegistry implementation for testing
 *
 * Convention:
 * - Type ID 999 = Object type (for arrays/structs with object pointers)
 * - Any other ID = Primitive type (no pointers)
 */
class TestTypeRegistry : public TypeRegistry {
public:
    TestTypeRegistry() {
        // Object type (for arrays holding objects)
        TypeInfo objType;
        objType.kind = TypeInfo::OBJECT;
        objType.elementTypeId = 0;
        types[999] = objType;

        // Primitive types (int, float, etc.)
        TypeInfo primType;
        primType.kind = TypeInfo::PRIMITIVE;
        primType.elementTypeId = 0;

        types[1] = primType;  // TYPE_INT64
        types[2] = primType;  // TYPE_FLOAT64
        types[3] = primType;  // TYPE_BOOL
    }

    const TypeInfo* getTypeInfo(uint32_t typeId) const override {
        auto it = types.find(typeId);
        if (it != types.end()) {
            return &it->second;
        }

        // Default: treat unknown types as primitive (safe)
        static TypeInfo defaultPrim = {TypeInfo::PRIMITIVE, 0, {}};
        return &defaultPrim;
    }

    /**
     * Add a struct type with specific pointer field offsets
     */
    void addStructType(uint32_t typeId, const std::vector<size_t>& pointerOffsets) {
        TypeInfo structType;
        structType.kind = TypeInfo::OBJECT;
        structType.elementTypeId = 0;
        structType.pointerFieldOffsets = pointerOffsets;
        types[typeId] = structType;
    }

private:
    std::unordered_map<uint32_t, TypeInfo> types;
};