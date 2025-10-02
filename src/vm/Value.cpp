#include "../include/vm/Value.hpp"
#include <sstream>

namespace volta::vm {

// ========== Value Utilities ==========

std::string valueTypeToString(ValueType type) {
    return "";
}

std::string valueToString(const Value& value, const std::vector<std::string>* stringPool) {
    return "";
}

void printValue(std::ostream& out, const Value& value, const std::vector<std::string>* stringPool) {
}

bool valuesEqual(const Value& a, const Value& b) {
    return false;
}

bool sameType(const Value& a, const Value& b) {
    return false;
}

// ========== Object Utilities ==========

size_t getStructSize(uint32_t fieldCount) {
    return 0;
}

size_t getArraySize(uint32_t elementCount) {
    return 0;
}

StructObject* asStruct(void* obj) {
    return nullptr;
}

ArrayObject* asArray(void* obj) {
    return nullptr;
}

Value getStructField(StructObject* obj, uint32_t fieldIndex) {
    return Value::makeNull();
}

void setStructField(StructObject* obj, uint32_t fieldIndex, const Value& value) {
}

Value getArrayElement(ArrayObject* arr, uint32_t index) {
    return Value::makeNull();
}

void setArrayElement(ArrayObject* arr, uint32_t index, const Value& value) {
}

void printStruct(std::ostream& out, StructObject* obj, const std::vector<std::string>* stringPool) {
}

void printArray(std::ostream& out, ArrayObject* arr, const std::vector<std::string>* stringPool) {
}

} // namespace volta::vm
