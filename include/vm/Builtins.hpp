#pragma once

#include "vm/Value.hpp"
#include <vector>

namespace volta::vm {

// Forward declaration
class VM;

// ============================================================================
// Built-in Function Signatures
// ============================================================================
// All built-in functions follow this signature:
//   Value builtin_name(VM* vm, const std::vector<Value>& args)
//
// They are called from the VM when CALL_NATIVE opcode is executed.
// Each function should:
//   1. Check argument count and types
//   2. Perform the operation
//   3. Return a Value (or throw runtime_error on failure)
//
// These functions are registered with the VM during initialization via
// VM::register_builtins(), which calls VM::register_native_function() for each.

/**
 * print(value) -> none
 * Prints the string representation of a value to stdout without a newline.
 */
Value builtin_print(VM* vm, const std::vector<Value>& args);

/**
 * println(value) -> none
 * Prints the string representation of a value to stdout with a newline.
 */
Value builtin_println(VM* vm, const std::vector<Value>& args);

/**
 * len(array|string) -> int
 * Returns the length of an array or string.
 * Throws runtime_error if argument is not an array or string.
 */
Value builtin_len(VM* vm, const std::vector<Value>& args);

/**
 * type(value) -> string
 * Returns the type name of a value as a string.
 * Possible return values: "none", "bool", "int", "float", "string", "array", "struct", "function"
 */
Value builtin_type(VM* vm, const std::vector<Value>& args);

/**
 * str(value) -> string
 * Converts a value to its string representation.
 * Returns a string object.
 */
Value builtin_str(VM* vm, const std::vector<Value>& args);

/**
 * int(value) -> int
 * Converts a value to an integer.
 * - int -> returns as-is
 * - float -> truncates to integer
 * - bool -> 1 for true, 0 for false
 * - string -> parses string to integer (throws on failure)
 * - other types throw runtime_error
 */
Value builtin_int(VM* vm, const std::vector<Value>& args);

/**
 * float(value) -> float
 * Converts a value to a floating-point number.
 * - float -> returns as-is
 * - int -> converts to float
 * - bool -> 1.0 for true, 0.0 for false
 * - string -> parses string to float (throws on failure)
 * - other types throw runtime_error
 */
Value builtin_float(VM* vm, const std::vector<Value>& args);

/**
 * assert(condition, [message]) -> none
 * Asserts that a condition is true. If false, throws runtime_error.
 * Optional second argument provides a custom error message.
 *
 * Examples:
 *   assert(x > 0)
 *   assert(x > 0, "x must be positive")
 */
Value builtin_assert(VM* vm, const std::vector<Value>& args);

} // namespace volta::vm
