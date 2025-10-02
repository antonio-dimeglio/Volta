#pragma once

#include "Value.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace volta::vm {

/**
 * @brief Types of heap-allocated objects
 */
enum class ObjectType : uint8_t {
    STRING,             ///< Immutable string
    ARRAY,              ///< Dynamic array of values
    STRUCT,             ///< User-defined struct
    CLOSURE,            ///< Function closure
    NATIVE_FUNCTION     ///< Built-in native function
};

/**
 * @brief Base object header for all heap-allocated objects
 *
 * Every heap object begins with this header, which provides:
 * - Type information for safe casting
 * - GC mark bit for mark-and-sweep
 * - Linked list pointer for GC tracking
 */
struct Object {
    ObjectType type;    ///< What kind of object this is
    bool marked;        ///< GC mark bit (true = reachable)
    Object* next;       ///< Next object in VM's linked list

    /**
     * @brief Check if this object is a string
     */
    bool is_string() const;

    /**
     * @brief Check if this object is an array
     */
    bool is_array() const;

    /**
     * @brief Check if this object is a struct
     */
    bool is_struct() const;

    /**
     * @brief Check if this object is a closure
     */
    bool is_closure() const;

    /**
     * @brief Check if this object is a native function
     */
    bool is_native_function() const;

    /**
     * @brief Get string representation for debugging
     */
    std::string to_string() const;
};

// ============================================================================
// String Object
// ============================================================================

/**
 * @brief Immutable string object
 *
 * Strings are immutable and null-terminated for C interop.
 * We cache the hash for fast comparison and hash table lookups.
 *
 * Layout in memory:
 * [Object header][length][hash][char data...]
 */
struct StringObject : Object {
    size_t length;      ///< String length (excluding null terminator)
    size_t hash;        ///< Cached hash value
    char data[];        ///< Flexible array member (null-terminated)

    /**
     * @brief Create a new string object
     * @param vm VM instance (for allocation)
     * @param str Source string
     * @param length String length
     * @return Pointer to new StringObject
     *
     * TODO: Implement string interning for deduplication
     */
    static StringObject* create(class VM* vm, const char* str, size_t length);

    /**
     * @brief Compute hash of string data
     *
     * Uses FNV-1a hash algorithm for speed and good distribution.
     * Implementation:
     * 1. Start with FNV offset basis (2166136261u)
     * 2. For each byte: hash = (hash ^ byte) * FNV prime (16777619u)
     * 3. Return final hash
     */
    static size_t compute_hash(const char* str, size_t length);
};

// ============================================================================
// Array Object
// ============================================================================

/**
 * @brief Dynamic array of values
 *
 * Arrays can grow dynamically and hold any Value type.
 * All elements are stored inline for cache efficiency.
 *
 * Layout in memory:
 * [Object header][length][capacity][Value array...]
 */
struct ArrayObject : Object {
    size_t length;      ///< Current number of elements
    size_t capacity;    ///< Allocated capacity
    Value elements[];   ///< Flexible array member

    /**
     * @brief Create a new array with initial capacity
     * @param vm VM instance
     * @param capacity Initial capacity
     */
    static ArrayObject* create(class VM* vm, size_t capacity);

    /**
     * @brief Get element at index (with bounds checking)
     * @param index Array index
     * @return Value at index
     * @throws RuntimeError if index out of bounds
     *
     * Implementation:
     * 1. Check: if (index >= length) throw error
     * 2. Return: elements[index]
     */
    Value get(size_t index) const;

    /**
     * @brief Set element at index (with bounds checking)
     * @param index Array index
     * @param value Value to store
     *
     * Implementation:
     * 1. Check: if (index >= length) throw error
     * 2. Set: elements[index] = value
     */
    void set(size_t index, const Value& value);

    /**
     * @brief Append element (grows if necessary)
     * @param vm VM instance (for reallocation if needed)
     * @param value Value to append
     *
     * Implementation:
     * 1. If length == capacity, grow array (capacity * 2)
     * 2. elements[length++] = value
     */
    void append(class VM* vm, const Value& value);
};

// ============================================================================
// Struct Object
// ============================================================================

/**
 * @brief User-defined struct instance
 *
 * Stores field values in order defined by struct type.
 * Field access uses index (resolved at compile time).
 *
 * Layout in memory:
 * [Object header][field_count][Value fields...]
 */
struct StructObject : Object {
    const char* struct_name;  ///< Struct type name
    size_t field_count;       ///< Number of fields
    Value fields[];           ///< Field values (indexed by field order)

    /**
     * @brief Create new struct instance with name
     * @param vm VM instance
     * @param struct_name Name of the struct type
     * @param field_count Number of fields
     */
    static StructObject* create(class VM* vm, const char* struct_name, size_t field_count);

    /**
     * @brief Create new struct instance (without name)
     * @param vm VM instance
     * @param field_count Number of fields
     */
    static StructObject* create(class VM* vm, size_t field_count);

    /**
     * @brief Get field value by index
     * @param index Field index
     * @return Value of field
     *
     * Implementation:
     * 1. Check: if (index >= field_count) throw error
     * 2. Return: fields[index]
     */
    Value get_field(size_t index) const;

    /**
     * @brief Set field value by index
     * @param index Field index
     * @param value New value
     *
     * Implementation:
     * 1. Check: if (index >= field_count) throw error
     * 2. Set: fields[index] = value
     */
    void set_field(size_t index, const Value& value);

    /**
     * @brief Convenience methods for compatibility
     */
    Value get(size_t index) const { return get_field(index); }
    void set(size_t index, const Value& value) { set_field(index, value); }
};

// ============================================================================
// Closure Object
// ============================================================================

/**
 * @brief Function closure (function + captured environment)
 *
 * Represents a function that can be called. For now, closures
 * don't capture variables (upvalues = 0), but the structure
 * is ready for future closure support.
 */
struct ClosureObject : Object {
    /**
     * @brief Function metadata
     */
    struct Function {
        std::string name;           ///< Function name (for debugging)
        uint8_t* bytecode;          ///< Pointer to bytecode
        size_t bytecode_length;     ///< Length of bytecode
        uint8_t arity;              ///< Number of parameters
        uint8_t register_count;     ///< Total registers needed
    };

    Function* function;         ///< Function metadata
    size_t upvalue_count;       ///< Number of captured variables (future)
    Value upvalues[];           ///< Captured variables (flexible array)

    /**
     * @brief Create a closure
     * @param vm VM instance
     * @param function Function metadata
     * @param upvalue_count Number of upvalues (0 for now)
     */
    static ClosureObject* create(class VM* vm, Function* function, size_t upvalue_count);
};

// ============================================================================
// Native Function Object
// ============================================================================

/**
 * @brief Built-in native function (implemented in C++)
 *
 * Represents built-in functions like print, len, etc.
 */
struct NativeFunctionObject : Object {
    /**
     * @brief Native function pointer type
     *
     * Takes array of arguments and VM context, returns result
     */
    using NativeFn = Value (*)(class VM* vm, const std::vector<Value>& args);

    std::string name;       ///< Function name
    NativeFn function;      ///< C++ function pointer
    uint8_t arity;          ///< Number of parameters (-1 for variadic)

    /**
     * @brief Create a native function object
     * @param vm VM instance
     * @param name Function name
     * @param function C++ function pointer
     * @param arity Number of parameters
     */
    static NativeFunctionObject* create(
        class VM* vm,
        const std::string& name,
        NativeFn function,
        uint8_t arity
    );

    /**
     * @brief Call the native function
     * @param vm VM instance
     * @param args Arguments
     * @return Result value
     *
     * Implementation:
     * 1. Check arity: if (arity >= 0 && args.size() != arity) throw error
     * 2. Call: return function(vm, args)
     */
    Value call(class VM* vm, const std::vector<Value>& args);
};

// ============================================================================
// Object Size Calculation
// ============================================================================

/**
 * @brief Calculate size of object in bytes (for GC)
 * @param obj Object pointer
 * @return Size in bytes
 *
 * Implementation: Use sizeof + flexible array sizes
 * - String: sizeof(StringObject) + length + 1
 * - Array: sizeof(ArrayObject) + capacity * sizeof(Value)
 * - Struct: sizeof(StructObject) + field_count * sizeof(Value)
 * - Closure: sizeof(ClosureObject) + upvalue_count * sizeof(Value)
 * - Native: sizeof(NativeFunctionObject)
 */
size_t object_size(const Object* obj);

} // namespace volta::vm
