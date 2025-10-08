#pragma once

#include <cstdint>
#include <cstddef>

namespace Volta {
namespace GC {

/**
 * Object type tags for heap-allocated objects
 */
enum ObjectType : uint8_t {
    OBJ_STRING = 0,
    OBJ_ARRAY  = 1,
    OBJ_STRUCT = 2,
    OBJ_ENUM   = 3,
};

/**
 * Generation tags for generational GC
 */
enum Generation : uint8_t {
    NURSERY = 0,  // Young generation
    OLD     = 1,  // Old generation (tenured)
};

/**
 * Object header - prefix for all heap-allocated objects
 *
 * Every heap object (string, array, struct) begins with this header.
 * The GC uses this metadata for tracing, copying, and compaction.
 *
 * Layout (16 bytes, 8-byte aligned):
 * - type: Object type tag (STRING/ARRAY/STRUCT)
 * - generation: Which generation this object lives in (NURSERY/OLD)
 * - age: Number of GC cycles survived (0-255)
 * - marked: Mark bit for tracing (0 or 1)
 * - size: Total object size in bytes (including header)
 * - forwarding: Forwarding pointer (used during copying GC)
 */
struct ObjectHeader {
    ObjectType  type;        // 1 byte
    Generation  generation;  // 1 byte
    uint8_t     age;         // 1 byte
    uint8_t     marked;      // 1 byte (0 or 1)
    uint32_t    size;        // 4 bytes
    void*       forwarding;  // 8 bytes

    // Total: 16 bytes (well-aligned)

    ObjectHeader()
        : type(OBJ_STRING)
        , generation(NURSERY)
        , age(0)
        , marked(0)
        , size(0)
        , forwarding(nullptr)
    {}
};

// Ensure header is the expected size
static_assert(sizeof(ObjectHeader) == 16, "ObjectHeader must be 16 bytes");

/**
 * Base class for all heap objects (conceptual - not for polymorphism)
 *
 * All heap objects follow this layout:
 *   [ObjectHeader | Object-specific data]
 *
 * To get the header from an object pointer:
 *   ObjectHeader* header = (ObjectHeader*)obj;
 *
 * To get the data section:
 *   void* data = (char*)obj + sizeof(ObjectHeader);
 */
struct Object {
    ObjectHeader header;

    // Object-specific data follows in derived types
};

/**
 * String object layout
 *
 * Layout:
 *   [ObjectHeader | length (8 bytes) | char data (null-terminated)]
 */
struct StringObject {
    ObjectHeader header;
    size_t       length;  // Number of characters (excluding null terminator)
    char         data[];  // Flexible array member (actual chars follow)

    // Total size: sizeof(ObjectHeader) + sizeof(size_t) + length + 1
};

/**
 * Array object layout
 *
 * Layout:
 *   [ObjectHeader | length (8 bytes) | element_type_id (4 bytes) | padding (4 bytes) | elements...]
 *
 * Elements are stored inline. For object-typed arrays, elements are pointers.
 * For primitive arrays, elements are values.
 */
struct ArrayObject {
    ObjectHeader header;
    size_t       length;         // Number of elements
    uint32_t     elementTypeId;  // Type ID of elements (for tracing)
    uint32_t     padding;        // Keep 8-byte alignment
    char         elements[];     // Flexible array member (actual elements follow)

    // Total size: sizeof(ObjectHeader) + 16 + (length * element_size)
};

/**
 * Struct object layout
 *
 * Layout:
 *   [ObjectHeader | struct_type_id (4 bytes) | padding (4 bytes) | field data...]
 *
 * Field data is laid out according to the struct type definition.
 * The GC uses struct_type_id to look up which fields are pointers.
 */
struct StructObject {
    ObjectHeader header;
    uint32_t     structTypeId;  // Type ID (for looking up field layout)
    uint32_t     padding;       // Keep 8-byte alignment
    char         fields[];      // Flexible array member (actual fields follow)

    // Total size: sizeof(ObjectHeader) + 8 + (sum of field sizes)
};

/**
 * Enum object layout
 *
 * Layout:
 *   [ObjectHeader | variant_tag (4 bytes) | padding (4 bytes) | variant data...]
 *
 * Variant data is laid out according to the specific variant's fields.
 * The tag identifies which variant this enum value represents.
 */
struct EnumObject {
    ObjectHeader header;
    uint32_t     variantTag;   // Which variant (0, 1, 2, ...)
    uint32_t     padding;      // Keep 8-byte alignment
    char         data[];       // Flexible array member (variant data follows)

    // Total size: sizeof(ObjectHeader) + 8 + (variant data size)
};

/**
 * Alignment requirement for all heap objects
 */
constexpr size_t OBJECT_ALIGNMENT = 8;

/**
 * Align a size to object alignment
 */
inline size_t alignSize(size_t size) {
    return (size + OBJECT_ALIGNMENT - 1) & ~(OBJECT_ALIGNMENT - 1);
}

/**
 * Calculate total size for a string object
 */
inline size_t stringObjectSize(size_t length) {
    size_t size = sizeof(ObjectHeader) + sizeof(size_t) + length + 1; // +1 for null terminator
    return alignSize(size);
}

/**
 * Calculate total size for an array object
 */
inline size_t arrayObjectSize(size_t length, size_t elementSize) {
    size_t size = sizeof(ObjectHeader) + 16 + (length * elementSize);
    return alignSize(size);
}

/**
 * Calculate total size for a struct object
 */
inline size_t structObjectSize(size_t fieldDataSize) {
    size_t size = sizeof(ObjectHeader) + 8 + fieldDataSize;
    return alignSize(size);
}

/**
 * Calculate total size for an enum object
 */
inline size_t enumObjectSize(size_t variantDataSize) {
    size_t size = sizeof(ObjectHeader) + 8 + variantDataSize;
    return alignSize(size);
}

} // namespace GC
} // namespace Volta