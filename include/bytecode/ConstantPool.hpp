#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <variant>

namespace volta::bytecode {

/**
 * @brief Constant types in bytecode
 */
enum class ConstantType : uint8_t {
    NONE = 0,
    BOOL = 1,
    INT = 2,
    FLOAT = 3,
    STRING = 4
};

/**
 * @brief A constant value
 */
struct Constant {
    ConstantType type;
    std::variant<std::monostate, bool, int64_t, double, std::string> value;

    Constant() : type(ConstantType::NONE), value(std::monostate{}) {}

    static Constant none() {
        return Constant{ConstantType::NONE, std::monostate{}};
    }

    static Constant from_bool(bool b) {
        return Constant{ConstantType::BOOL, b};
    }

    static Constant from_int(int64_t i) {
        return Constant{ConstantType::INT, i};
    }

    static Constant from_float(double f) {
        return Constant{ConstantType::FLOAT, f};
    }

    static Constant from_string(const std::string& s) {
        return Constant{ConstantType::STRING, s};
    }

private:
    template<typename T>
    Constant(ConstantType t, T v) : type(t), value(v) {}
};

/**
 * @brief Constant pool builder with deduplication
 *
 * Manages all constants used in the module. Automatically
 * deduplicates identical constants to save space.
 */
class ConstantPoolBuilder {
public:
    ConstantPoolBuilder() = default;

    /**
     * @brief Add integer constant
     * @param value Integer value
     * @return Index in constant pool
     *
     * Implementation:
     * 1. Check if value already in int_map_
     * 2. If found, return existing index
     * 3. Else:
     *    - Create Constant::from_int(value)
     *    - Add to constants_
     *    - Store index in int_map_
     *    - Return new index
     */
    uint32_t add_int(int64_t value);

    /**
     * @brief Add float constant
     * @param value Float value
     * @return Index in constant pool
     *
     * Implementation: Similar to add_int
     */
    uint32_t add_float(double value);

    /**
     * @brief Add string constant
     * @param value String value
     * @return Index in constant pool
     *
     * Implementation:
     * 1. Check if value already in string_map_
     * 2. If found, return existing index
     * 3. Else:
     *    - Create Constant::from_string(value)
     *    - Add to constants_
     *    - Store index in string_map_
     *    - Return new index
     */
    uint32_t add_string(const std::string& value);

    /**
     * @brief Add boolean constant
     * @param value Boolean value
     * @return Index in constant pool
     *
     * Implementation: Similar to add_int
     * Note: Could optimize with fixed indices 0=false, 1=true
     */
    uint32_t add_bool(bool value);

    /**
     * @brief Add none constant
     * @return Index in constant pool
     *
     * Implementation: Similar to add_int
     * Note: Could optimize with fixed index 0=none
     */
    uint32_t add_none();

    /**
     * @brief Get constant by index
     * @param index Constant pool index
     * @return Constant reference
     */
    const Constant& get(uint32_t index) const;

    /**
     * @brief Get total number of constants
     */
    size_t size() const { return constants_.size(); }

    /**
     * @brief Get all constants
     */
    const std::vector<Constant>& constants() const { return constants_; }

    /**
     * @brief Serialize constant pool to bytes
     * @return Byte array
     *
     * Format:
     * [count:4]
     * For each constant:
     *   [type:1]
     *   [value:variable]
     *
     * Value encoding:
     * - NONE: no data
     * - BOOL: [value:1]
     * - INT: [value:8]
     * - FLOAT: [value:8]
     * - STRING: [length:4][bytes...]
     *
     * Implementation:
     * 1. Write count as uint32
     * 2. For each constant:
     *    - Write type as uint8
     *    - Switch on type:
     *      - NONE: write nothing
     *      - BOOL: write 1 byte
     *      - INT: write 8 bytes (big-endian)
     *      - FLOAT: write 8 bytes (IEEE 754)
     *      - STRING: write length + bytes
     */
    std::vector<uint8_t> serialize() const;

private:
    std::vector<Constant> constants_;

    // Deduplication maps
    std::unordered_map<int64_t, uint32_t> int_map_;
    std::unordered_map<double, uint32_t> float_map_;
    std::unordered_map<std::string, uint32_t> string_map_;
    std::unordered_map<bool, uint32_t> bool_map_;
    uint32_t none_index_ = 0xFFFFFFFF;  // Invalid until set
};

} // namespace volta::bytecode
