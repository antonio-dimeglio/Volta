#include "../include/vm/Value.hpp"
#include <sstream>

namespace volta::vm {

// ========== Value Utilities ==========

std::string valueTypeToString(ValueType type) {
    switch (type) {
        case ValueType::Null: return "Null";
        case ValueType::Int: return "Int";
        case ValueType::Float: return "Float";
        case ValueType::Bool: return "Bool";
        case ValueType::String: return "String";
        case ValueType::Object: return "Object";
    }
    return "unimplemented type";
}

std::string valueToString(const Value& value, const std::vector<std::string>* stringPool) {
    switch (value.type) {
        case ValueType::Int: return std::to_string(value.asInt);
        case ValueType::Float: return std::to_string(value.asFloat);
        case ValueType::Bool: return value.asBool ? "true" : "false";
        case ValueType::String: return "\"" + stringPool->at(value.asStringIndex) + "\"";
        case ValueType::Object: {
            if (!value.asObject) return "null";

            ObjectHeader* header = static_cast<ObjectHeader*>(value.asObject);
            std::ostringstream oss;

            if (header->kind == ObjectHeader::ObjectKind::Struct) {
                StructObject* obj = static_cast<StructObject*>(value.asObject);
                oss << "Struct{typeId=" << obj->header.typeId << " @" << value.asObject << "}";
            } else if (header->kind == ObjectHeader::ObjectKind::Array) {
                ArrayObject* arr = static_cast<ArrayObject*>(value.asObject);
                oss << "[";
                for (uint32_t i = 0; i < arr->length; i++) {
                    if (i > 0) oss << ", ";
                    oss << valueToString(arr->elements[i], stringPool);
                }
                oss << "]";
            } else {
                oss << "Object{@" << value.asObject << "}";
            }
            return oss.str();
        }
        case ValueType::Null:
            return "null";
    }
    return "";
}

void printValue(std::ostream& out, const Value& value, const std::vector<std::string>* stringPool) {
    out << valueToString(value, stringPool);
}

bool valuesEqual(const Value& a, const Value& b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case ValueType::Int:
            return a.asInt == b.asInt;
        case ValueType::Float:
            return a.asFloat == b.asFloat;
        case ValueType::Bool:
            return a.asBool == b.asBool;
        case ValueType::String:
            return a.asStringIndex == b.asStringIndex;
        case ValueType::Object:
            return a.asObject == b.asObject;  // Pointer equality
        case ValueType::Null:
            return true;  // null == null
    }
    return false;
}

bool sameType(const Value& a, const Value& b) {
    return a.type == b.type;  // Fixed: should be == not !=
}

// ========== Object Utilities ==========

size_t getStructSize(uint32_t fieldCount) {
    return sizeof(ObjectHeader) + sizeof(Value) * fieldCount;
}

size_t getArraySize(uint32_t elementCount) {
    return sizeof(ObjectHeader) + sizeof(uint32_t) + sizeof(Value) * elementCount;
}

StructObject* asStruct(void* obj) {
    if (!obj) return nullptr;

    auto* header = reinterpret_cast<ObjectHeader*>(obj);
    if (header->kind == ObjectHeader::ObjectKind::Struct) {
        return reinterpret_cast<StructObject*>(obj);
    }
    return nullptr;
}

ArrayObject* asArray(void* obj) {
    if (!obj) return nullptr;

    auto* header = reinterpret_cast<ObjectHeader*>(obj);
    if (header->kind == ObjectHeader::ObjectKind::Array) {
        return reinterpret_cast<ArrayObject*>(obj);
    }

    return nullptr;
}

Value getStructField(StructObject* obj, uint32_t fieldIndex) {
    // Unsafe
    return obj->fields[fieldIndex];
}

void setStructField(StructObject* obj, uint32_t fieldIndex, const Value& value) {
    obj->fields[fieldIndex] = value;
}

Value getArrayElement(ArrayObject* arr, uint32_t index) {
    if (index < arr->length) return arr->elements[index];
    return Value::makeNull();  // Out of bounds returns null
}

void setArrayElement(ArrayObject* arr, uint32_t index, const Value& value) {
    if (index < arr->length) arr->elements[index] = value;
}

void printStruct(std::ostream& out, StructObject* obj, const std::vector<std::string>* stringPool) {
    // Note: We can't determine field count from the object itself without storing it
    // This is a limitation of the flexible array member approach
    // For now, print the type ID and address
    out << "Struct{typeId=" << obj->header.typeId
        << ", size=" << obj->header.size
        << " @" << static_cast<void*>(obj) << "}";
}

void printArray(std::ostream& out, ArrayObject* arr, const std::vector<std::string>* stringPool) {
    out << "[";

    for (uint32_t i = 0; i < arr->length; i++) {
        if (i > 0) out << ", ";
        printValue(out, arr->elements[i], stringPool);
    }

    out << "]";
}

} // namespace volta::vm
