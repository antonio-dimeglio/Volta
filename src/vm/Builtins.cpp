#include "vm/Builtins.hpp"
#include "vm/VM.hpp"
#include "vm/Object.hpp"
#include <iostream>
#include <sstream>

namespace volta::vm {

// ============================================================================
// Built-in Function Implementations
// ============================================================================

Value builtin_print(VM* vm, const std::vector<Value>& args) {
    // Todo: change this to take varargs
    if (args.size() != 1) throw std::runtime_error("print() takes exactly 1 argument");
    const Value& val = args[0];

    std::cout << val.to_string();
    return Value::none();
}

Value builtin_println(VM* vm, const std::vector<Value>& args) {
    // Todo: change this to take varargs
    if (args.size() != 1) throw std::runtime_error("print() takes exactly 1 argument");
    const Value& val = args[0];

    std::cout << val.to_string() << "\n";
    return Value::none();
}

Value builtin_len(VM* vm, const std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("len() takes exactly 1 argument");
    const Value& val = args[0];

    if (val.is_object()) {
        Object* obj = val.as_object();
        if (obj->is_string()) {
            auto length = ((StringObject*)obj)->length;
            return Value::int_value(length);
        } else if (obj->is_array()) {
            auto length = ((ArrayObject*)obj)->length;
            return Value::int_value(length);
        } else {
            throw std::runtime_error("len() requires string or array");
        }
    } else {
        throw std::runtime_error("len() requires string or array");
    }
}

Value builtin_type(VM* vm, const std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("type() takes exactly 1 argument");
    const Value& val = args[0];

    std::string typeName;

    switch (val.type) {
        case ValueType::NONE: typeName = "none"; break;
        case ValueType::BOOL: typeName = "bool"; break;
        case ValueType::INT: typeName = "int"; break;
        case ValueType::FLOAT: typeName = "float"; break;
        case ValueType::OBJECT:
            Object* obj = val.as_object();
            switch (obj->type) {
                case ObjectType::STRING: typeName = "string"; break;
                case ObjectType::ARRAY: typeName = "array"; break;
                case ObjectType::STRUCT: typeName = "struct"; break;
                case ObjectType::CLOSURE: typeName = "function"; break;
                case ObjectType::NATIVE_FUNCTION: typeName = "function"; break;
            }
            break;
    }

    StringObject* strObj = StringObject::create(vm, typeName.c_str(), typeName.length());
    return Value::object_value(strObj);
}

Value builtin_str(VM* vm, const std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("str() takes exactly 1 argument");
    const Value& val = args[0];

    if (val.is_object() && val.as_object()->is_string()) {
        return val;
    }

    std::string str = val.to_string();
    StringObject* strObj = StringObject::create(vm, str.c_str(), str.length());
    return Value::object_value(strObj);
}

Value builtin_int(VM* vm, const std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("int() takes exactly 1 argument");
    const Value& val = args[0];
    if (val.is_int()) {
        return val;
    } else if (val.is_float()) {
        return Value::int_value((int64_t)val.as_float());
    } else if (val.is_bool()) {
        return Value::int_value(val.as_bool() ? 1 : 0);
    } else if (val.is_object() && val.as_object()->is_string()) {
        StringObject* str = (StringObject*)val.as_object();
        try {
            int64_t result = std::stoll(str->data);
            return Value::int_value(result);
        } catch (...) {
            throw std::runtime_error("Cannot convert string to int");
        }
       } else {
        throw std::runtime_error("Cannot convert to int");
    }
}

Value builtin_float(VM* vm, const std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("float() takes exactly 1 argument");
    const Value& val = args[0];
    
    if (val.is_float()) {
        return val;
    } else if (val.is_int()) {
        return Value::float_value((double)val.as_int());
    } else if (val.is_bool()) {
        return Value::float_value(val.as_bool() ? 1.0 : 0.0);
    } else if (val.is_object() && val.as_object()->is_string()) {
        StringObject* str = (StringObject*)val.as_object();
        try {
            double result = std::stod(str->data);
            return Value::float_value(result);
        } catch (...) {
            throw std::runtime_error("Cannot convert string to float");
        }
    } else {
        throw std::runtime_error("Cannot convert to float");
    }
}

Value builtin_assert(VM* vm, const std::vector<Value>& args) {
    // Note: This is variadic (1-2 args):
    //       assert(x > 0)          - Simple assertion
    //       assert(x > 0, "x must be positive") - With message
    if (args.size() < 1 || args.size() > 2) throw std::runtime_error("assert() takes 1-2 arguments");
    const Value& condition = args[0];
    bool result = condition.to_bool();
    
    if (!result) {
        std::string message = "Assertion failed";
        if (args.size() == 2 && args[1].is_object() && args[1].as_object()->is_string()) {
            StringObject* str = (StringObject*)args[1].as_object();
            message = "Assertion failed: " + std::string(str->data);
        }
        throw std::runtime_error(message);
    }
    
    return Value::none();
}

} 
