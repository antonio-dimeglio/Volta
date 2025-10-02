#include "vm/Object.hpp"
#include "vm/VM.hpp"
#include <cstring>
#include <cassert>
#include <stdexcept>

namespace volta::vm {

// ============================================================================
// Object Base
// ============================================================================

bool Object::is_string() const {
    return type == ObjectType::STRING;
}

bool Object::is_array() const {
    return type == ObjectType::ARRAY;
}

bool Object::is_struct() const {
    return type == ObjectType::STRUCT;
}

bool Object::is_closure() const {
    return type == ObjectType::CLOSURE;
}

bool Object::is_native_function() const {
    return type == ObjectType::NATIVE_FUNCTION;
}

std::string Object::to_string() const {
    switch (type) {
        case ObjectType::STRING:
            return std::string(((StringObject*)this)->data);
        case ObjectType::ARRAY:
            return "[Array length =" + std::to_string(((ArrayObject*)this)->length) + "]";
        case ObjectType::STRUCT:
            return "[Struct length =" + std::to_string(((StructObject*)this)->field_count) + "]";
        case ObjectType::CLOSURE:
            return "[Function name =" + ((ClosureObject*)this)->function->name + "]";
        case ObjectType::NATIVE_FUNCTION:
            return "[Function name =" + ((NativeFunctionObject*)this)->name + "]";
    }
    return "undefined_object";
}

// ============================================================================
// String Object
// ============================================================================

StringObject* StringObject::create(VM* vm, const char* str, size_t length) {
    size_t objSize = sizeof(StringObject) + length + 1;
    StringObject* obj = (StringObject*)vm->allocate_object(objSize, ObjectType::STRING);
    obj->length = length;
    obj->hash = compute_hash(str, length);
    std::memcpy(obj->data, str, length);
    obj->data[length] = '\0';
    return obj;
}

size_t StringObject::compute_hash(const char* str, size_t length) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < length; i++) {
        hash ^= (uint8_t)str[i];
        hash *= 16777619u;
    }
    return hash;
}

// ============================================================================
// Array Object
// ============================================================================

ArrayObject* ArrayObject::create(VM* vm, size_t capacity) {
    size_t objSize = sizeof(ArrayObject) + capacity * sizeof(Value);
    ArrayObject* obj = (ArrayObject*)vm->allocate_object(objSize, ObjectType::ARRAY);
    obj->length = capacity;  // Initialize length to capacity
    obj->capacity = capacity;
    for (size_t i = 0; i < capacity; i++) obj->elements[i] = Value::none();
    return obj;
}

Value ArrayObject::get(size_t index) const {

    if (index >= length) throw std::runtime_error("Array index out of bounds");
    return elements[index];
}

void ArrayObject::set(size_t index, const Value& value) {
    if (index >= length) throw std::runtime_error("Array index out of bounds");
    elements[index] = value;
}

void ArrayObject::append(VM* vm, const Value& value) {
    if (length == capacity) {
        auto new_cap = capacity == 0 ? 8 : capacity* 2;
        ArrayObject* new_array = create(vm, new_cap);
        for (size_t i = 0; i < length; i++) new_array->elements[i] = elements[i];
        new_array->length = length;
        //   Replace this array's data (tricky - need to handle in VM)
        //   For now: throw error if capacity reached (implement growth later)
    }
    elements[length] = value;
    length++;
}

// ============================================================================
// Struct Object
// ============================================================================

StructObject* StructObject::create(VM* vm, const char* struct_name, size_t field_count) {
    size_t objSize = sizeof(StructObject) + field_count * sizeof(Value);
    StructObject* obj = (StructObject*)vm->allocate_object(objSize, ObjectType::STRUCT);
    obj->struct_name = struct_name;
    obj->field_count = field_count;
    for (size_t i = 0; i < field_count; i++) obj->fields[i] = Value::none();
    return obj;
}

StructObject* StructObject::create(VM* vm, size_t field_count) {
    return create(vm, nullptr, field_count);
}

Value StructObject::get_field(size_t index) const {
    if (index >= field_count) throw std::runtime_error("Field index out of bounds");
    return fields[index];
}

void StructObject::set_field(size_t index, const Value& value) {
    if (index >= field_count) throw std::runtime_error("Field index out of bounds");
    fields[index] = value;
}

// ============================================================================
// Closure Object
// ============================================================================

ClosureObject* ClosureObject::create(VM* vm, Function* function, size_t upvalue_count) {
    // Note: For now, upvalue_count is always 0 (no closures yet)
    size_t objSize = sizeof(ClosureObject) + upvalue_count * sizeof(Value);
    ClosureObject* obj = (ClosureObject*)vm->allocate_object(objSize, ObjectType::CLOSURE);
    obj->function = function;
    obj->upvalue_count = upvalue_count;
    
    for (size_t i = 0; i < upvalue_count; i++) obj->upvalues[i] = Value::none();
    return obj;
}

// ============================================================================
// Native Function Object
// ============================================================================

NativeFunctionObject* NativeFunctionObject::create(
    VM* vm,
    const std::string& name,
    NativeFn function,
    uint8_t arity
) {
    size_t objSize = sizeof(NativeFunctionObject);
    NativeFunctionObject* obj = (NativeFunctionObject*)vm->allocate_object(objSize, ObjectType::NATIVE_FUNCTION);

    // Manually initialize the std::string member using placement new
    // Don't construct the whole object - the Object header is already initialized
    new (&obj->name) std::string(name);
    obj->function = function;
    obj->arity = arity;
    return obj;
}

Value NativeFunctionObject::call(VM* vm, const std::vector<Value>& args) {
    // TODO: Add a way of checking if function is variadic, and if it is, avoid arity check.
        // Note: arity == 255 means variadic (any number of args)
    
    
    if (arity != 255 && args.size() != arity) {
        throw std::runtime_error("Wrong number of arguments to " + name);
    }
    return function(vm, args);
}

// ============================================================================
// Object Size Calculation
// ============================================================================

size_t object_size(const Object* obj) {

    switch (obj->type) {
        case ObjectType::STRING: {
            StringObject* str = (StringObject*)obj;
            return sizeof(StringObject) + str->length + 1;
        }
        case ObjectType::ARRAY: {
            ArrayObject* arr = (ArrayObject*)obj;
            return sizeof(ArrayObject) + arr->capacity * sizeof(Value);
        }
        case ObjectType::STRUCT: {
            StructObject* strc = (StructObject*)obj;
            return sizeof(StructObject) + strc->field_count * sizeof(Value);
        }
        case ObjectType::CLOSURE: {
            ClosureObject* clos = (ClosureObject*)obj;
            return sizeof(ClosureObject) + clos->upvalue_count * sizeof(Value);
        }
        case ObjectType::NATIVE_FUNCTION: {
            NativeFunctionObject* nat = (NativeFunctionObject*)obj;
            return sizeof(NativeFunctionObject);
        }
    }
}

} // namespace volta::vm
