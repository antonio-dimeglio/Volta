#pragma once

#include "Value.hpp"

namespace volta::vm {

// Forward declaration (VM is defined elsewhere)
class VM;

/**
 * RuntimeFunctionPtr - Type signature for native runtime functions
 *
 * Native functions are C++ functions callable from Volta bytecode.
 * They handle operations like heap allocation, I/O, and complex type operations.
 *
 * Signature:
 *   Value function_name(VM* vm, Value* args, int arg_count)
 *
 * Parameters:
 *   - vm: Pointer to the VM (for GC allocation, error handling)
 *   - args: Array of arguments (args[0], args[1], ...)
 *   - arg_count: Number of arguments passed
 *
 * Returns:
 *   A Value (int, float, bool, or object pointer)
 *
 * Example:
 *   Value runtime_print_int(VM* vm, Value* args, int arg_count) {
 *       std::cout << args[0].as.as_i64 << std::endl;
 *       return Value{};  // void return
 *   }
 */
using RuntimeFunctionPtr = Value (*)(VM* vm, Value* args, int arg_count);

// ============================================================================
// Built-in Runtime Functions
// ============================================================================

/**
 * runtime_print - Print a value to stdout
 * Parameters: 1 (value to print)
 * Returns: void
 */
Value runtime_print(VM* vm, Value* args, int arg_count);

} // namespace volta::vm
