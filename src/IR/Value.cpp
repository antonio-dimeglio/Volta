#include "IR/Value.hpp"
#include "IR/Instruction.hpp"
#include <sstream>

namespace volta::ir {

// ============================================================================
// Value Implementation
// ============================================================================

Value::Value(Kind kind, std::shared_ptr<semantic::Type> type, const std::string& name)
    : kind_(kind), type_(std::move(type)), name_(name) {
}

void Value::addUse(Instruction* user) {
    // TODO: Add this instruction to the uses_ vector.
    // This tracks which instructions use this value (for def-use chains).
}

void Value::removeUse(Instruction* user) {
    // TODO: Remove this instruction from the uses_ vector.
    // Hint: std::remove + erase idiom
}

std::string Value::toString() const {
    // TODO: Return a string representation of this value.
    // Format: just return the name_ for now (e.g., "%0", "%x", "@global")
    return "";
}

// ============================================================================
// Constant Implementation
// ============================================================================

Constant::Constant(std::shared_ptr<semantic::Type> type, ConstantValue value, const std::string& name)
    : Value(Kind::Constant, std::move(type), name), value_(std::move(value)) {
}

std::shared_ptr<Constant> Constant::getInt(int64_t value) {
    // TODO: Create and return a Constant representing an integer.
    // - Type should be semantic::Type::getInt()
    // - Name can be empty (constants don't need SSA names)
    // - Value should be stored in the ConstantValue variant
    return nullptr;
}

std::shared_ptr<Constant> Constant::getFloat(double value) {
    // TODO: Create and return a Constant representing a float.
    // Similar to getInt but for floats.
    return nullptr;
}

std::shared_ptr<Constant> Constant::getBool(bool value) {
    // TODO: Create and return a Constant representing a boolean.
    return nullptr;
}

std::shared_ptr<Constant> Constant::getString(const std::string& value) {
    // TODO: Create and return a Constant representing a string.
    return nullptr;
}

std::shared_ptr<Constant> Constant::getNone() {
    // TODO: Create and return a Constant representing None (null/unit type).
    // Use std::monostate in the variant.
    return nullptr;
}

std::string Constant::toString() const {
    // TODO: Return a string representation of this constant.
    // Format examples:
    //   Integer: "i64 42"
    //   Float: "float 3.14"
    //   Bool: "bool true"
    //   String: "str \"hello\""
    //   None: "none"
    // Hint: Use std::visit on value_ variant
    return "";
}

// ============================================================================
// Parameter Implementation
// ============================================================================

Parameter::Parameter(std::shared_ptr<semantic::Type> type, const std::string& name, size_t index)
    : Value(Kind::Parameter, std::move(type), name), index_(index) {
}

std::string Parameter::toString() const {
    // TODO: Return a string representation of this parameter.
    // Format: "i64 %param_name" or just use base Value::toString()
    return "";
}

// ============================================================================
// GlobalVariable Implementation
// ============================================================================

GlobalVariable::GlobalVariable(
    std::shared_ptr<semantic::Type> type,
    const std::string& name,
    std::shared_ptr<Constant> initializer,
    bool isMutable
) : Value(Kind::GlobalVariable, std::move(type), "@" + name),
    initializer_(std::move(initializer)),
    isMutable_(isMutable) {
}

std::string GlobalVariable::toString() const {
    // TODO: Return a string representation of this global variable.
    // Format: "@global_name = global i64 42" (if has initializer)
    //         "@global_name = global i64 undef" (if no initializer)
    return "";
}

} // namespace volta::ir
