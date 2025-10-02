#pragma once

#include <cstdint>
#include <string>

namespace volta::vm {

// Forward declarations
struct Object;

/**
 * @brief Type tags for runtime values
 *
 * Every value in the VM carries a runtime type tag. This enables
 * dynamic typing while maintaining type safety.
 */
enum class ValueType : uint8_t {
    NONE,       ///< None/null value (represents absence of value)
    BOOL,       ///< Boolean (true/false)
    INT,        ///< 64-bit signed integer
    FLOAT,      ///< 64-bit IEEE 754 double precision float
    OBJECT      ///< Pointer to heap-allocated object
};

/**
 * @brief Tagged union representing all runtime values
 *
 * This is the core value representation in the VM. Every register,
 * every variable, every intermediate result is a Value.
 *
 * Size: 16 bytes (8-byte discriminant + 8-byte union)
 *
 * Design choice: We use a simple tagged union for clarity and debuggability.
 * Alternative (NaN boxing) would be faster but more complex.
 */
struct Value {
    ValueType type;

    union {
        bool b;          ///< Boolean value (when type == BOOL)
        int64_t i;       ///< Integer value (when type == INT)
        double f;        ///< Float value (when type == FLOAT)
        Object* obj;     ///< Object pointer (when type == OBJECT)
    } as;

    // ========================================================================
    // Factory Functions
    // ========================================================================

    /**
     * @brief Create a none/null value
     */
    static Value none();

    /**
     * @brief Create a boolean value
     * @param value The boolean value
     */
    static Value bool_value(bool value);

    /**
     * @brief Create an integer value
     * @param value The integer value
     */
    static Value int_value(int64_t value);

    /**
     * @brief Create a float value
     * @param value The float value
     */
    static Value float_value(double value);

    /**
     * @brief Create an object value
     * @param obj Pointer to heap-allocated object
     */
    static Value object_value(Object* obj);

    // Aliases for test compatibility
    static Value from_bool(bool value);
    static Value from_int(int64_t value);
    static Value from_float(double value);
    static Value from_object(Object* obj);

    // ========================================================================
    // Type Checking
    // ========================================================================

    /**
     * @brief Check if value is none
     */
    bool is_none() const;

    /**
     * @brief Check if value is boolean
     */
    bool is_bool() const;

    /**
     * @brief Check if value is integer
     */
    bool is_int() const;

    /**
     * @brief Check if value is float
     */
    bool is_float() const;

    /**
     * @brief Check if value is numeric (int or float)
     */
    bool is_number() const;

    /**
     * @brief Check if value is object
     */
    bool is_object() const;

    // ========================================================================
    // Value Extraction (with runtime checks)
    // ========================================================================

    /**
     * @brief Get boolean value (asserts if not bool)
     */
    bool as_bool() const;

    /**
     * @brief Get integer value (asserts if not int)
     */
    int64_t as_int() const;

    /**
     * @brief Get float value (asserts if not float)
     */
    double as_float() const;

    /**
     * @brief Get object pointer (asserts if not object)
     */
    Object* as_object() const;

    // ========================================================================
    // Type Conversions
    // ========================================================================

    /**
     * @brief Convert to number (int->float if needed)
     * @return Float representation of the value
     */
    double to_number() const;

    /**
     * @brief Convert to boolean (for truthiness checks)
     *
     * Truthiness rules:
     * - none → false
     * - false → false
     * - 0 (int) → false
     * - 0.0 (float) → false
     * - Everything else → true
     */
    bool to_bool() const;

    /**
     * @brief Convert to string for printing
     */
    std::string to_string() const;

    // ========================================================================
    // Comparison
    // ========================================================================

    /**
     * @brief Check equality with another value
     *
     * Rules:
     * - Different types: false (except int/float can be compared)
     * - Same type: value comparison
     * - Objects: pointer comparison (TODO: deep equality)
     */
    bool equals(const Value& other) const;

    // Operator overloads
    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const;
    bool operator<(const Value& other) const;
    bool operator<=(const Value& other) const;
    bool operator>(const Value& other) const;
    bool operator>=(const Value& other) const;
};

} // namespace volta::vm
