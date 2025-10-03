#include "IR/Value.hpp"
#include "IR/Instruction.hpp"
#include <algorithm>

namespace volta::ir {

// ============================================================================
// Value Implementation
// ============================================================================

uint64_t Value::nextID_ = 0;

Value::Value(ValueKind kind, std::shared_ptr<semantic::Type> type, const std::string& name)
    : kind_(kind), type_(type), name_(name), id_(nextID_++) {
    // TODO: Implement constructor
}

void Value::replaceAllUsesWith(Value* newValue) {
    // TODO: Implement replaceAllUsesWith
    // Hint: Iterate through all uses and update them to point to newValue
}

void Value::addUse(Use* use) {
    // TODO: Implement addUse
    // Hint: Add the use to the uses_ vector
}

void Value::removeUse(Use* use) {
    // TODO: Implement removeUse
    // Hint: Remove the use from the uses_ vector using std::remove
}

bool Value::isConstant() const {
    // TODO: Implement isConstant
    // Hint: Check if kind_ is in the constant range
    return false;
}

bool Value::isInstruction() const {
    // TODO: Implement isInstruction
    return false;
}

bool Value::isArgument() const {
    // TODO: Implement isArgument
    return false;
}

bool Value::isGlobalValue() const {
    // TODO: Implement isGlobalValue
    return false;
}

// ============================================================================
// Use Implementation
// ============================================================================

Use::Use(Value* value, Instruction* user)
    : value_(value), user_(user) {
    // TODO: Implement constructor
    // Hint: Register this use with the value
}

Use::~Use() {
    // TODO: Implement destructor
    // Hint: Unregister this use from the value
}

void Use::setValue(Value* newValue) {
    // TODO: Implement setValue
    // Hint: Remove from old value's use list, add to new value's use list
}

// ============================================================================
// Constant Implementation
// ============================================================================

Constant::Constant(ValueKind kind, std::shared_ptr<semantic::Type> type, const std::string& name)
    : Value(kind, type, name) {
    // TODO: Implement constructor
}

// ============================================================================
// ConstantInt Implementation
// ============================================================================

ConstantInt::ConstantInt(int64_t value, std::shared_ptr<semantic::Type> type)
    : Constant(ValueKind::ConstantInt, type), value_(value) {
    // TODO: Implement constructor
}

ConstantInt* ConstantInt::get(int64_t value, std::shared_ptr<semantic::Type> type) {
    // TODO: Implement get (factory method)
    // Hint: For now, just create a new ConstantInt. Later, implement interning.
    return nullptr;
}

std::string ConstantInt::toString() const {
    // TODO: Implement toString
    // Hint: Return string like "i64 42" or just "42"
    return "";
}

// ============================================================================
// ConstantFloat Implementation
// ============================================================================

ConstantFloat::ConstantFloat(double value, std::shared_ptr<semantic::Type> type)
    : Constant(ValueKind::ConstantFloat, type), value_(value) {
    // TODO: Implement constructor
}

ConstantFloat* ConstantFloat::get(double value, std::shared_ptr<semantic::Type> type) {
    // TODO: Implement get (factory method)
    return nullptr;
}

std::string ConstantFloat::toString() const {
    // TODO: Implement toString
    // Hint: Return string like "f64 3.14"
    return "";
}

// ============================================================================
// ConstantBool Implementation
// ============================================================================

ConstantBool::ConstantBool(bool value, std::shared_ptr<semantic::Type> type)
    : Constant(ValueKind::ConstantBool, type), value_(value) {
    // TODO: Implement constructor
}

ConstantBool* ConstantBool::get(bool value, std::shared_ptr<semantic::Type> type) {
    // TODO: Implement get (factory method)
    return nullptr;
}

ConstantBool* ConstantBool::getTrue(std::shared_ptr<semantic::Type> type) {
    // TODO: Implement getTrue
    return nullptr;
}

ConstantBool* ConstantBool::getFalse(std::shared_ptr<semantic::Type> type) {
    // TODO: Implement getFalse
    return nullptr;
}

std::string ConstantBool::toString() const {
    // TODO: Implement toString
    // Hint: Return "true" or "false"
    return "";
}

// ============================================================================
// ConstantString Implementation
// ============================================================================

ConstantString::ConstantString(const std::string& value, std::shared_ptr<semantic::Type> type)
    : Constant(ValueKind::ConstantString, type), value_(value) {
    // TODO: Implement constructor
}

ConstantString* ConstantString::get(const std::string& value, std::shared_ptr<semantic::Type> type) {
    // TODO: Implement get (factory method)
    // Hint: Later, implement string interning for memory efficiency
    return nullptr;
}

std::string ConstantString::toString() const {
    // TODO: Implement toString
    // Hint: Return string like "\"hello world\""
    return "";
}

// ============================================================================
// ConstantNone Implementation
// ============================================================================

ConstantNone::ConstantNone(std::shared_ptr<semantic::Type> optionType)
    : Constant(ValueKind::ConstantNone, optionType) {
    // TODO: Implement constructor
}

ConstantNone* ConstantNone::get(std::shared_ptr<semantic::Type> optionType) {
    // TODO: Implement get (factory method)
    return nullptr;
}

std::string ConstantNone::toString() const {
    // TODO: Implement toString
    // Hint: Return "None"
    return "";
}

// ============================================================================
// UndefValue Implementation
// ============================================================================

UndefValue::UndefValue(std::shared_ptr<semantic::Type> type)
    : Constant(ValueKind::UndefValue, type) {
    // TODO: Implement constructor
}

UndefValue* UndefValue::get(std::shared_ptr<semantic::Type> type) {
    // TODO: Implement get (factory method)
    return nullptr;
}

std::string UndefValue::toString() const {
    // TODO: Implement toString
    // Hint: Return "undef"
    return "";
}

// ============================================================================
// Argument Implementation
// ============================================================================

Argument::Argument(std::shared_ptr<semantic::Type> type, unsigned argNo, const std::string& name)
    : Value(ValueKind::Argument, type, name), argNo_(argNo), parent_(nullptr) {
    // TODO: Implement constructor
}

std::string Argument::toString() const {
    // TODO: Implement toString
    // Hint: Return something like "%arg0" or "%name" if it has a name
    return "";
}

// ============================================================================
// GlobalVariable Implementation
// ============================================================================

GlobalVariable::GlobalVariable(std::shared_ptr<semantic::Type> type,
                               const std::string& name,
                               Constant* initializer,
                               bool isConstant)
    : Value(ValueKind::GlobalVariable, type, name),
      isConstant_(isConstant),
      initializer_(initializer) {
    // TODO: Implement constructor
}

std::string GlobalVariable::toString() const {
    // TODO: Implement toString
    // Hint: Return something like "@globalName" for globals
    return "";
}

} // namespace volta::ir
