#pragma once

#include "Value.hpp"
#include "BytecodeModule.hpp"
#include <vector>
#include <string>
#include <cstdint>
#include "gc/GarbageCollector.hpp"
#include "gc/GCRootProvider.hpp"
#include "vm/VMTypeRegistry.hpp"


namespace volta::vm {

/**
 * @brief Activation record for a function call (stack frame)
 *
 * Each function call gets its own Frame containing:
 * - Reference to the function being executed
 * - Program counter (position in bytecode)
 * - Register file (local storage for values)
 * - Return register (where to place result in caller's frame)
 */
struct Frame {
    FunctionInfo* function;           ///< The function being executed
    size_t pc;                        ///< Program counter (bytecode offset)
    std::vector<Value> registers;    ///< Register file for this function
    uint8_t returnRegister;           ///< Destination register in caller's frame (for CALL)

    /**
     * @brief Construct a new Frame
     * @param func The function to execute
     * @param numRegisters Number of registers to allocate
     * @param retReg Destination register for return value (default 0)
     */
    Frame(FunctionInfo* func, size_t numRegisters, uint8_t retReg = 0)
        : function(func), pc(0), registers(numRegisters), returnRegister(retReg) {}
};

/**
 * @brief Virtual Machine for executing Volta bytecode
 *
 * The VM uses a register-based architecture with:
 * - Stack of activation records (call frames)
 * - Fetch-decode-execute dispatch loop
 * - Runtime function support
 *
 * ## Execution Model
 *
 * 1. Load BytecodeModule
 * 2. Find entry function by name
 * 3. Create initial frame with registers
 * 4. Execute dispatch loop:
 *    - FETCH: Read opcode from bytecode at PC
 *    - DECODE: Determine operation and operands
 *    - EXECUTE: Perform operation, update registers
 *    - ADVANCE: Move PC to next instruction (or jump)
 * 5. Return final value when done
 *
 * ## Phase Implementation
 *
 * - Phase 2a: Basic arithmetic, control flow, single function
 * - Phase 2b: Function calls, returns, call stack (CURRENT)
 * - Phase 2c: Runtime library integration
 * - Phase 3: Garbage collection integration
 * - Phase 4: Optimizations (computed goto, threaded code)
 *
 * ## Error Handling
 *
 * Currently uses exceptions for errors:
 * - Type mismatches
 * - Division by zero
 * - Invalid opcodes
 * - Stack overflow/underflow
 * - Invalid register access
 *
 * TODO: Replace exceptions with proper error values/result types
 *
 * ## Future Enhancements
 *
 * TODO: Support variadic function arguments
 * TODO: Add debugging support (breakpoints, step execution)
 * TODO: Add profiling/instrumentation hooks
 * TODO: Optimize dispatch (computed goto, direct threading)
 */
class VM : public Volta::GC::GCRootProvider  {
public:
    /**
     * @brief Construct a new VM instance
     */
    VM() : module_(nullptr), gc_(nullptr), typeRegistry_(nullptr) {
        // Create GC with default sizes (1MB nursery, 8MB old gen)
        gc_ = new Volta::GC::GarbageCollector(1024 * 1024, 8 * 1024 * 1024);
        
        // Create type registry
        typeRegistry_ = new VMTypeRegistry(nullptr);
        
        // Wire up GC
        gc_->setRootProvider(this);
        gc_->setTypeRegistry(typeRegistry_);
    }

    /**
     * @brief Destroy the VM instance
     */
    ~VM();

    /**
     * @brief Get GC roots from the VM (implements GCRootProvider)
     * @return Vector of pointers to object pointers in registers
     */
    std::vector<Volta::GC::Object**> getRoots() override;
    /**
     * @brief Execute a function from a bytecode module
     *
     * @param module The compiled bytecode module
     * @param functionName Name of the function to execute (e.g., "main")
     * @return Value The return value of the function
     * @throws std::runtime_error if function not found or execution error
     *
     * TODO Phase 2b: Add overload with std::vector<Value> args
     * TODO Phase 3: Add variadic argument support
     */
    Value execute(BytecodeModule& module, const std::string& functionName);

    /**
     * @brief Execute a function with arguments
     *
     * @param module The compiled bytecode module
     * @param functionName Name of the function to execute
     * @param args Arguments to pass to the function
     * @return Value The return value of the function
     * @throws std::runtime_error if function not found, arg count mismatch, or execution error
     *
     * TODO Phase 3: Add variadic argument support
     */
    Value execute(BytecodeModule& module, const std::string& functionName,
                  const std::vector<Value>& args);

private:
    // ========================================================================
    // STATE
    // ========================================================================

    BytecodeModule* module_;           ///< Current module being executed (valid during execute())
    std::vector<Frame> callStack;     ///< Call stack (activation records)
    Volta::GC::GarbageCollector* gc_;      ///< Garbage collector (owned by VM)
    VMTypeRegistry* typeRegistry_;         ///< Type registry (owned by VM)
    // TODO Phase 3: Add GC integration
    // GarbageCollector* gc;

    // TODO Phase 4: Add profiling/debugging state
    // bool debugMode;
    // std::vector<size_t> breakpoints;

    // ========================================================================
    // DISPATCH LOOP
    // ========================================================================

    /**
     * @brief Main dispatch loop - fetch, decode, execute cycle
     *
     * Runs until:
     * - RET/RET_VOID and call stack is empty (normal termination)
     * - HALT instruction (explicit termination)
     * - Exception thrown (error)
     *
     * @return Value The final return value
     */
    Value run();

    // ========================================================================
    // BYTECODE READING (Instruction Fetching)
    // ========================================================================

    /**
     * @brief Read one byte from bytecode and advance PC
     * @param frame The current stack frame
     * @return uint8_t The byte read
     */
    uint8_t readByte(Frame& frame);

    /**
     * @brief Read two bytes (little-endian) and advance PC
     * @param frame The current stack frame
     * @return uint16_t The 16-bit value read
     */
    uint16_t readShort(Frame& frame);

    /**
     * @brief Read four bytes (little-endian) and advance PC
     * @param frame The current stack frame
     * @return uint32_t The 32-bit value read
     */
    uint32_t readInt(Frame& frame);

    // ========================================================================
    // ARITHMETIC OPERATIONS
    // ========================================================================
    // NOTE: These are implemented as static functions in VM.cpp
    //       They operate on Values and don't need VM state
    // TODO: Move to RuntimeOps.hpp if runtime library needs them

    /**
     * @brief Add two values (int64 + int64, float64 + float64)
     * @throws std::runtime_error on type mismatch
     */
    static Value add(const Value& a, const Value& b);

    /**
     * @brief Subtract two values
     * @throws std::runtime_error on type mismatch
     */
    static Value subtract(const Value& a, const Value& b);

    /**
     * @brief Multiply two values
     * @throws std::runtime_error on type mismatch
     */
    static Value multiply(const Value& a, const Value& b);

    /**
     * @brief Divide two values
     * @throws std::runtime_error on type mismatch or division by zero
     */
    static Value divide(const Value& a, const Value& b);

    /**
     * @brief Modulo operation
     * @throws std::runtime_error on type mismatch or division by zero
     */
    static Value modulo(const Value& a, const Value& b);

    /**
     * @brief Power operation (a^b)
     * @throws std::runtime_error on type mismatch
     */
    static Value power(const Value& a, const Value& b);

    /**
     * @brief Negate a value (-a)
     * @throws std::runtime_error on invalid type
     */
    static Value negate(const Value& a);

    // ========================================================================
    // COMPARISON OPERATIONS
    // ========================================================================

    /**
     * @brief Compare two values for equality
     * Supports: int64, float64, bool, object (pointer equality)
     */
    static Value equal(const Value& a, const Value& b);

    /**
     * @brief Compare two values for inequality
     */
    static Value notEqual(const Value& a, const Value& b);

    /**
     * @brief Less than comparison
     * @throws std::runtime_error on type mismatch or uncomparable types
     */
    static Value lessThan(const Value& a, const Value& b);

    /**
     * @brief Less than or equal comparison
     * @throws std::runtime_error on type mismatch or uncomparable types
     */
    static Value lessEqual(const Value& a, const Value& b);

    /**
     * @brief Greater than comparison
     * @throws std::runtime_error on type mismatch or uncomparable types
     */
    static Value greaterThan(const Value& a, const Value& b);

    /**
     * @brief Greater than or equal comparison
     * @throws std::runtime_error on type mismatch or uncomparable types
     */
    static Value greaterEqual(const Value& a, const Value& b);

    // ========================================================================
    // LOGICAL OPERATIONS
    // ========================================================================

    /**
     * @brief Logical AND
     * @throws std::runtime_error if operands are not bool
     */
    static Value logicalAnd(const Value& a, const Value& b);

    /**
     * @brief Logical OR
     * @throws std::runtime_error if operands are not bool
     */
    static Value logicalOr(const Value& a, const Value& b);

    /**
     * @brief Logical NOT
     * @throws std::runtime_error if operand is not bool
     */
    static Value logicalNot(const Value& a);

    // ========================================================================
    // TYPE CONVERSIONS
    // ========================================================================

    /**
     * @brief Cast int64 to float64
     * @throws std::runtime_error if operand is not int64
     */
    static Value castIntToFloat(const Value& a);

    /**
     * @brief Cast float64 to int64 (truncation)
     * @throws std::runtime_error if operand is not float64
     */
    static Value castFloatToInt(const Value& a);

    // ========================================================================
    // FUNCTION CALL SUPPORT
    // ========================================================================

    /**
     * @brief Call a bytecode function
     *
     * Creates a new frame, sets up arguments in registers, pushes to call stack
     *
     * @param function The function to call
     * @param args Arguments to pass
     * @param returnReg Destination register in caller's frame for return value
     * @throws std::runtime_error if argument count mismatch
     */
    void callFunction(FunctionInfo* function, const std::vector<Value>& args, uint8_t returnReg);

    /**
     * @brief Call a native (runtime) function
     *
     * Invokes the C++ function pointer, returns result
     *
     * @param function The native function to call
     * @param args Arguments to pass
     * @return Value The return value from the native function
     * @throws std::runtime_error if native function throws or argument issues
     */
    Value callNativeFunction(RuntimeFunctionPtr function, const std::vector<Value>& args);

    // ========================================================================
    // ARRAY/STRING OPERATIONS
    // ========================================================================
    // TODO Phase 2c: Implement these when we add runtime library

    /**
     * @brief Get array element at index
     * @throws std::runtime_error if not an array or index out of bounds
     */
    static Value arrayGet(const Value& array, const Value& index);

    /**
     * @brief Set array element at index
     * @throws std::runtime_error if not an array or index out of bounds
     */
    static void arraySet(Value& array, const Value& index, const Value& value);

    /**
     * @brief Get array length
     * @throws std::runtime_error if not an array
     */
    static Value arrayLength(const Value& array);

    /**
     * @brief Get string length
     * @throws std::runtime_error if not a string
     */
    static Value stringLength(const Value& str);

    // ========================================================================
    // ERROR HANDLING HELPERS
    // ========================================================================

    /**
     * @brief Throw runtime error with context (current function, PC, etc.)
     * @param message Error message
     */
    [[noreturn]] void runtimeError(const std::string& message);

    /**
     * @brief Check stack overflow (prevent infinite recursion)
     * @throws std::runtime_error if stack too deep
     */
    void checkStackOverflow();
};

} // namespace volta