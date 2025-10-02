#pragma once

#include "Value.hpp"
#include "Object.hpp"
#include <cstdint>
#include <string>

namespace volta::vm {

/**
 * @brief Call frame for function execution
 *
 * Each function call creates a new CallFrame on the call stack.
 * The frame contains all execution state for that function:
 * - Register file (256 registers)
 * - Return information (where to jump back, where to store result)
 * - Function metadata
 * - Debug information
 */
struct CallFrame {
    // ========================================================================
    // Register File
    // ========================================================================

    /**
     * @brief Register file (256 virtual registers)
     *
     * Register allocation:
     * - R0..R(n-1): Function parameters (n = arity)
     * - R(n)..R(m): Local variables
     * - R(m+1)..R255: Temporary values
     */
    Value registers[256];

    // ========================================================================
    // Return Information
    // ========================================================================

    /**
     * @brief Return address (instruction pointer to resume at)
     *
     * When this function returns, execution continues at this address
     */
    uint8_t* return_address;

    /**
     * @brief Return register (where to store return value)
     *
     * When this function returns, the result is stored in
     * caller_frame.registers[return_register]
     */
    uint8_t return_register;

    // ========================================================================
    // Function Metadata
    // ========================================================================

    /**
     * @brief Function being executed
     *
     * Contains bytecode, name, arity, etc.
     */
    ClosureObject* closure;

    // ========================================================================
    // Debug Information
    // ========================================================================

    /**
     * @brief Function name (for stack traces)
     */
    std::string function_name;

    /**
     * @brief Current line number (for stack traces)
     *
     * Updated by LINE debug instructions (optional)
     */
    int line_number;

    // ========================================================================
    // Constructor
    // ========================================================================

    /**
     * @brief Create a new call frame
     * @param closure Function to execute
     * @param return_addr Where to return to
     * @param return_reg Where to store result
     *
     * Implementation:
     * 1. Initialize all registers to none
     * 2. Set return_address = return_addr
     * 3. Set return_register = return_reg
     * 4. Set closure = closure
     * 5. Set function_name = closure->function->name
     * 6. Set line_number = 0
     */
    CallFrame(
        ClosureObject* closure,
        uint8_t* return_addr,
        uint8_t return_reg
    );

    /**
     * @brief Default constructor (for std::vector)
     */
    CallFrame() = default;
};

} // namespace volta::vm
