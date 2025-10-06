#include "vm/RuntimeFunction.hpp"
#include "vm/Value.hpp"
#include <iostream>

namespace volta::vm {

/**
 * runtime_print - Built-in print function
 *
 * Prints a value to stdout followed by a newline.
 * Currently supports: int, float, bool
 */
Value runtime_print(VM* vm, Value* args, int arg_count) {
    (void)vm;  // VM not needed for print
    if (arg_count != 1) {
        std::cerr << "Error: print() expects exactly 1 argument, got " << arg_count << std::endl;
        return Value{};  // Return void
    }

    Value arg = args[0];

    // Check value type and print accordingly
    switch (arg.type) {
        case ValueType::INT64:
            std::cout << arg.as.as_i64 << std::endl;
            break;
        case ValueType::FLOAT64:
            std::cout << arg.as.as_f64 << std::endl;
            break;
        case ValueType::BOOL:
            std::cout << (arg.as.as_b ? "true" : "false") << std::endl;
            break;
        case ValueType::OBJECT:
            std::cout << "<object>" << std::endl;
            break;
        case ValueType::NONE:
            std::cout << "none" << std::endl;
            break;
    }

    return Value{};  // void return
}

} // namespace volta::vm