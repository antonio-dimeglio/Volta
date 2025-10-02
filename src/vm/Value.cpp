#include "vm/Value.hpp"
#include "vm/Object.hpp"
#include <cassert>
#include <sstream>

namespace volta::vm {

// ============================================================================
// Factory Functions
// ============================================================================

Value Value::none() {
    Value v;
    v.type = ValueType::NONE;
    return v;
}

Value Value::bool_value(bool value) {
    Value v;
    v.type = ValueType::BOOL;
    v.as.b = value;
    return v;
}

Value Value::int_value(int64_t value) {
    Value v;
    v.type = ValueType::INT;
    v.as.i = value;
    return v;
}

Value Value::float_value(double value) {
    Value v;
    v.type = ValueType::FLOAT;
    v.as.f = value;
    return v;
}

Value Value::object_value(Object* obj) {
    Value v;
    v.type = ValueType::OBJECT;
    v.as.obj = obj;
    return v;
}

// Aliases for test compatibility
Value Value::from_bool(bool value) { return bool_value(value); }
Value Value::from_int(int64_t value) { return int_value(value); }
Value Value::from_float(double value) { return float_value(value); }
Value Value::from_object(Object* obj) { return object_value(obj); }

// ============================================================================
// Type Checking
// ============================================================================

bool Value::is_none() const {
    return type == ValueType::NONE;
}

bool Value::is_bool() const {
    return type == ValueType::BOOL;
}

bool Value::is_int() const {
    return type == ValueType::INT;
}

bool Value::is_float() const {
    return type == ValueType::FLOAT;
}

bool Value::is_number() const {
    return is_int() || is_float();
}

bool Value::is_object() const {
    return type == ValueType::OBJECT;
}

// ============================================================================
// Value Extraction
// ============================================================================

bool Value::as_bool() const {
    assert(is_bool() && "Value is not a boolean");
    return as.b;
}

int64_t Value::as_int() const {
    assert(is_int() && "Value is not an integer");
    return as.i;
}

double Value::as_float() const {
    assert(is_float() && "Value is not a float");
    return as.f;
}

Object* Value::as_object() const {
    assert(is_object() && "Value is not an object");
    return as.obj;
}

// ============================================================================
// Type Conversions
// ============================================================================

double Value::to_number() const {
    if (is_int()) return (double)as.i;
    if (is_float()) return as.f;
    throw std::runtime_error("Cannot convert non number type to number.");
}

bool Value::to_bool() const {
    if (is_none()) return false;
    if (is_bool()) return as.b;
    if (is_int()) return as.i != 0;
    if (is_float()) return as.f != 0.0;
    if (is_object()) return as.obj != nullptr;
    return false;
}

std::string Value::to_string() const {
    std::ostringstream oss;

    switch (type) {
        case ValueType::INT:
            oss << as.i;
            break;
        case ValueType::FLOAT:
            oss << as.f;
            break;
        case ValueType::BOOL:
            oss << (as.b ? "true" : "false");
            break;
        case ValueType::NONE:
            oss << "none";
            break;
        case ValueType::OBJECT:
            oss << as.obj->to_string();
            break;
    }

    return oss.str();

}

// ============================================================================
// Comparison
// ============================================================================

bool Value::equals(const Value& other) const {
    if (this->type == other.type) {
        switch (other.type) {
            case ValueType::NONE: return true;
            case ValueType::BOOL: return this->as.b == other.as.b;
            case ValueType::INT: return this->as.i == other.as.i;
            case ValueType::FLOAT: return this->as.f == other.as.f;
            case ValueType::OBJECT: return this->as.obj == other.as.obj;
        }
    } else if ((this->type == ValueType::INT && other.type == ValueType::FLOAT) ||
        (this->type == ValueType::FLOAT && other.type == ValueType::INT)) {
            return this->to_number() == other.to_number();
    }
    return false;
}

// Operator overloads
bool Value::operator==(const Value& other) const {
    return equals(other);
}

bool Value::operator!=(const Value& other) const {
    return !equals(other);
}

bool Value::operator<(const Value& other) const {
    // Both numbers - compare numerically
    if (is_number() && other.is_number()) {
        return to_number() < other.to_number();
    }
    // Both strings - lexicographic comparison
    if (is_object() && other.is_object()) {
        Object* obj1 = as_object();
        Object* obj2 = other.as_object();
        if (obj1->is_string() && obj2->is_string()) {
            StringObject* str1 = (StringObject*)obj1;
            StringObject* str2 = (StringObject*)obj2;
            return std::string(str1->data) < std::string(str2->data);
        }
    }
    throw std::runtime_error("Cannot compare non-numeric/non-string values");
}

bool Value::operator<=(const Value& other) const {
    return *this < other || *this == other;
}

bool Value::operator>(const Value& other) const {
    return !(*this <= other);
}

bool Value::operator>=(const Value& other) const {
    return !(*this < other);
}

} // namespace volta::vm
