#include "IR/Value.hpp"
#include "IR/Instruction.hpp"
#include <algorithm>
#include <stdexcept>

namespace volta::ir {

// ============================================================================
// Value Implementation
// ============================================================================

uint64_t Value::nextID_ = 0;

Value::Value(ValueKind kind, std::shared_ptr<IRType> type, const std::string& name)
    : kind_(kind), type_(type), name_(name), id_(nextID_++) {
}

void Value::replaceAllUsesWith(Value* newValue) {
    if (newValue == nullptr) return;
    // Make a copy since setValue will modify uses_
    auto usesCopy = uses_;
    for (auto use: usesCopy) {
        use->setValue(newValue);
    }
}

void Value::addUse(Use* use) {
    if (use == nullptr) return;

    uses_.push_back(use);
}

void Value::removeUse(Use* use) {
    if (use == nullptr) return;

    uses_.erase(std::remove(uses_.begin(), uses_.end(), use), uses_.end());
}

bool Value::isConstant() const {
    switch (kind_) {
        case ValueKind::ConstantInt:
        case ValueKind::ConstantFloat:
        case ValueKind::ConstantBool:
        case ValueKind::ConstantString:
        case ValueKind::ConstantNone:
            return true;
        default: 
            return false;
    }
}

bool Value::isInstruction() const {
    return kind_ == ValueKind::Instruction;
}

bool Value::isArgument() const {
    return kind_ == ValueKind::Argument;
}

bool Value::isGlobalValue() const {
    return kind_ == ValueKind::GlobalVariable;
}

// ============================================================================
// Use Implementation
// ============================================================================

Use::Use(Value* value, Instruction* user)
    : value_(value), user_(user) {
    if (value_) {
        value_->addUse(this);
    }
}

Use::Use(Use&& other) noexcept
    : value_(other.value_), user_(other.user_) {
    // Transfer ownership - remove from old location, add to new
    if (value_) {
        value_->removeUse(&other);
        value_->addUse(this);
    }
    other.value_ = nullptr;
    other.user_ = nullptr;
}

Use& Use::operator=(Use&& other) noexcept {
    if (this != &other) {
        // Remove old use
        if (value_) {
            value_->removeUse(this);
        }

        // Transfer from other
        value_ = other.value_;
        user_ = other.user_;

        if (value_) {
            value_->removeUse(&other);
            value_->addUse(this);
        }

        other.value_ = nullptr;
        other.user_ = nullptr;
    }
    return *this;
}

void Use::setValue(Value* newValue) {
    if (value_) {
        value_->removeUse(this);
    }
    value_ = newValue;
    if (value_) {
        value_->addUse(this);
    }
}

// ============================================================================
// Constant Implementation
// ============================================================================

Constant::Constant(ValueKind kind, std::shared_ptr<IRType> type, const std::string& name)
    : Value(kind, type, name) {
}

// ============================================================================
// ConstantInt Implementation
// ============================================================================

ConstantInt::ConstantInt(int64_t value, std::shared_ptr<IRType> type)
    : Constant(ValueKind::ConstantInt, type), value_(value) {
}

ConstantInt* ConstantInt::get(int64_t value, std::shared_ptr<IRType> type) {
    // TODO: Implement interning.
    return new ConstantInt(value, type);
}

std::string ConstantInt::toString() const {
    return "i64 " + std::to_string(value_);
}

// ============================================================================
// ConstantFloat Implementation
// ============================================================================

ConstantFloat::ConstantFloat(double value, std::shared_ptr<IRType> type)
    : Constant(ValueKind::ConstantFloat, type), value_(value) {
}

ConstantFloat* ConstantFloat::get(double value, std::shared_ptr<IRType> type) {
    // TODO: Implement interning.
    return new ConstantFloat(value, type);
}

std::string ConstantFloat::toString() const {
    return "f64 " + std::to_string(value_);
}

// ============================================================================
// ConstantBool Implementation
// ============================================================================

ConstantBool::ConstantBool(bool value, std::shared_ptr<IRType> type)
    : Constant(ValueKind::ConstantBool, type), value_(value) {
}

ConstantBool* ConstantBool::get(bool value, std::shared_ptr<IRType> type) {
    // TODO: Implement interning.
    return new ConstantBool(value, type);
}

ConstantBool* ConstantBool::getTrue(std::shared_ptr<IRType> type) {
    return new ConstantBool(true, type);
}

ConstantBool* ConstantBool::getFalse(std::shared_ptr<IRType> type) {
    return new ConstantBool(false, type);
}

std::string ConstantBool::toString() const {
    return value_ ? "true" : "false";
}

// ============================================================================
// ConstantString Implementation
// ============================================================================

ConstantString::ConstantString(const std::string& value, std::shared_ptr<IRType> type)
    : Constant(ValueKind::ConstantString, type), value_(value) {
}

ConstantString* ConstantString::get(const std::string& value, std::shared_ptr<IRType> type) {
    // TODO: Later, implement string interning for memory efficiency
    return new ConstantString(value, type);
}

std::string ConstantString::toString() const {
    return "\"" + value_ + "\"";
}

// ============================================================================
// ConstantNone Implementation
// ============================================================================

ConstantNone::ConstantNone(std::shared_ptr<IRType> optionType)
    : Constant(ValueKind::ConstantNone, optionType) {
}

ConstantNone* ConstantNone::get(std::shared_ptr<IRType> optionType) {
    // TODO: Implement interning.
    return new ConstantNone(optionType);
}

std::string ConstantNone::toString() const {
    return "None";
}

// ============================================================================
// UndefValue Implementation
// ============================================================================

UndefValue::UndefValue(std::shared_ptr<IRType> type)
    : Constant(ValueKind::UndefValue, type) {
}

UndefValue* UndefValue::get(std::shared_ptr<IRType> type) {
    // TODO: Implement interning
    return new UndefValue(type);
}

std::string UndefValue::toString() const {
    return "undef";
}

// ============================================================================
// Argument Implementation
// ============================================================================

Argument::Argument(std::shared_ptr<IRType> type, unsigned argNo, const std::string& name)
    : Value(ValueKind::Argument, type, name), argNo_(argNo), parent_(nullptr) {
    // TODO: Implement constructor
}

std::string Argument::toString() const {
    return "%" + (hasName() ? getName() : ("arg" + std::to_string(argNo_)));
}

// ============================================================================
// GlobalVariable Implementation
// ============================================================================

GlobalVariable::GlobalVariable(std::shared_ptr<IRType> type,
                               const std::string& name,
                               Constant* initializer,
                               bool isConstant)
    : Value(ValueKind::GlobalVariable, type, name),
      isConstant_(isConstant),
      initializer_(initializer) {
}

std::string GlobalVariable::toString() const {
    return "@" + getName();
}

} // namespace volta::ir
