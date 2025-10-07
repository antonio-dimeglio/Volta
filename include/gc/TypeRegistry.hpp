#pragma once

#include <cstdint>
#include <vector>

namespace Volta {
namespace GC {

/**
 * Type information for GC tracing
 *
 * Tells the GC which fields/elements contain object pointers
 */
struct TypeInfo {
    enum Kind {
        PRIMITIVE,  // int, float, bool - no pointers
        OBJECT,     // string, array, struct - is a pointer
    };

    Kind kind;

    // For arrays: element type
    uint32_t elementTypeId;  // 0 if not array

    // For structs: which field offsets contain object pointers
    std::vector<size_t> pointerFieldOffsets;
};

/**
 * Interface for looking up type information
 *
 * The VM/Runtime implements this to provide type metadata to the GC
 */
class TypeRegistry {
public:
    virtual ~TypeRegistry() = default;

    /**
     * Get type information for a type ID
     *
     * @param typeId Type ID (from object header)
     * @return Type information, or nullptr if not found
     */
    virtual const TypeInfo* getTypeInfo(uint32_t typeId) const = 0;
};

} // namespace GC
} // namespace Volta
