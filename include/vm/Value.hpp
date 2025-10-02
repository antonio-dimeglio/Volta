#pragma once

#include "VM.hpp"
#include <string>
#include <ostream>

namespace volta::vm {

// ============================================================================
// Value Utilities
// ============================================================================

/**
 * Get a string representation of a value type (for debugging/error messages)
 */
std::string valueTypeToString(ValueType type);

/**
 * Get a string representation of a value (for debugging/printing)
 * Requires string pool to resolve string references
 */
std::string valueToString(const Value& value, const std::vector<std::string>* stringPool = nullptr);

/**
 * Print a value to an output stream
 */
void printValue(std::ostream& out, const Value& value, const std::vector<std::string>* stringPool = nullptr);

/**
 * Compare two values for equality
 * Returns true if values are equal, false otherwise
 * Note: For objects, compares references (pointer equality), not structural equality
 */
bool valuesEqual(const Value& a, const Value& b);

/**
 * Check if two values have the same type
 */
bool sameType(const Value& a, const Value& b);

// ============================================================================
// Object Utilities
// ============================================================================

/**
 * Get the size of a struct object in bytes (including header)
 */
size_t getStructSize(uint32_t fieldCount);

/**
 * Get the size of an array object in bytes (including header)
 */
size_t getArraySize(uint32_t elementCount);

/**
 * Cast a void* object pointer to StructObject*
 * Includes type checking (verifies kind is Struct)
 * Returns nullptr if not a struct
 */
StructObject* asStruct(void* obj);

/**
 * Cast a void* object pointer to ArrayObject*
 * Includes type checking (verifies kind is Array)
 * Returns nullptr if not an array
 */
ArrayObject* asArray(void* obj);

/**
 * Get a field from a struct object by index
 * Returns the field value
 * Throws if index out of bounds
 */
Value getStructField(StructObject* obj, uint32_t fieldIndex);

/**
 * Set a field in a struct object by index
 * Throws if index out of bounds
 */
void setStructField(StructObject* obj, uint32_t fieldIndex, const Value& value);

/**
 * Get an element from an array object by index
 * Returns the element value
 * Throws if index out of bounds
 */
Value getArrayElement(ArrayObject* arr, uint32_t index);

/**
 * Set an element in an array object by index
 * Throws if index out of bounds
 */
void setArrayElement(ArrayObject* arr, uint32_t index, const Value& value);

/**
 * Print a struct object (for debugging)
 * Format: Struct<typeId>{ field0, field1, ... }
 */
void printStruct(std::ostream& out, StructObject* obj, const std::vector<std::string>* stringPool = nullptr);

/**
 * Print an array object (for debugging)
 * Format: [elem0, elem1, elem2, ...]
 */
void printArray(std::ostream& out, ArrayObject* arr, const std::vector<std::string>* stringPool = nullptr);

} // namespace volta::vm
