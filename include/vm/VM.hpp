#pragma once

#include "Value.hpp"
#include "Object.hpp"
#include "Opcode.hpp"
#include "CallFrame.hpp"
#include "GC.hpp"
#include "FFI.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>

namespace volta::vm {

/**
 * @brief Volta Virtual Machine
 *
 * Register-based VM with mark-and-sweep GC and FFI support.
 *
 * Features:
 * - 256 virtual registers per call frame
 * - Automatic garbage collection
 * - Native C function calls
 * - Switch-based instruction dispatch
 */
class VM {
public:
    /**
     * @brief Construct a new VM
     */
    VM();

    /**
     * @brief Destructor (cleans up all objects)
     */
    ~VM();

    // ========================================================================
    // Execution
    // ========================================================================

    /**
     * @brief Execute bytecode
     * @param bytecode Pointer to bytecode array
     * @param length Length of bytecode
     * @return Exit value (from HALT or RETURN at top level)
     *
     * Implementation:
     * 1. Set ip_ = bytecode
     * 2. Set bytecode_end_ = bytecode + length
     * 3. Create initial call frame for top-level code
     * 4. Call run() to start execution loop
     * 5. Return final value from top-level
     */
    Value execute(uint8_t* bytecode, size_t length);

    /**
     * @brief Main execution loop
     *
     * This is the heart of the VM - the fetch-decode-execute loop.
     *
     * Implementation:
     * while (true):
     *   1. Fetch opcode: op = fetch_byte()
     *   2. Decode & execute: switch (op) { case OP_ADD: ... }
     *   3. Check for halt or return
     *
     * Performance tip: Keep this tight! Every cycle counts.
     */
    void run();

    /**
     * @brief Execute single instruction (for debugging/stepping)
     * @return True if execution should continue, false if halted
     */
    bool step();

    // ========================================================================
    // Bytecode Fetching
    // ========================================================================

    /**
     * @brief Fetch next byte and advance IP
     * @return Next byte from instruction stream
     *
     * Implementation: return *ip_++;
     */
    uint8_t fetch_byte();

    /**
     * @brief Fetch next 16-bit value (big-endian)
     * @return 16-bit value
     *
     * Implementation:
     * uint16_t value = (ip_[0] << 8) | ip_[1];
     * ip_ += 2;
     * return value;
     */
    uint16_t fetch_short();

    /**
     * @brief Fetch next 32-bit value (big-endian)
     * @return 32-bit value
     */
    uint32_t fetch_int();

    /**
     * @brief Fetch next byte without advancing IP (peek)
     */
    uint8_t peek_byte() const;

    // ========================================================================
    // Register Access
    // ========================================================================

    /**
     * @brief Get current register file
     * @return Pointer to current frame's registers
     */
    Value* registers();

    /**
     * @brief Get register value
     * @param index Register index (0-255)
     */
    Value get_register(uint8_t index);

    /**
     * @brief Set register value
     * @param index Register index (0-255)
     * @param value Value to store
     */
    void set_register(uint8_t index, const Value& value);

    // ========================================================================
    // Call Stack Management
    // ========================================================================

    /**
     * @brief Call a function
     * @param closure Function to call
     * @param args Arguments (copied to new frame's R0, R1, ...)
     * @param return_register Where to store result in caller frame
     *
     * Implementation:
     * 1. Check arity: if (args.size() != closure->function->arity) error
     * 2. Check stack overflow: if (call_stack.size() > MAX_FRAMES) error
     * 3. Create new CallFrame:
     *    - Set closure
     *    - Copy args to registers[0..arity-1]
     *    - Set return_address = current ip_
     *    - Set return_register
     * 4. Push frame to call_stack_
     * 5. Set current_frame_ = &call_stack_.back()
     * 6. Set ip_ = closure->function->bytecode
     * 7. Initialize remaining registers to none
     */
    void call_function(
        ClosureObject* closure,
        const std::vector<Value>& args,
        uint8_t return_register
    );

    /**
     * @brief Return from current function
     * @param return_value Value to return
     *
     * Implementation:
     * 1. Save return_value
     * 2. Get return_address and return_register from current frame
     * 3. Pop call_stack_ (remove current frame)
     * 4. If stack empty: halt with return_value
     * 5. Set current_frame_ = &call_stack_.back()
     * 6. Set ip_ = return_address
     * 7. Set caller's return register: registers()[return_register] = return_value
     */
    void return_from_function(const Value& return_value);

    /**
     * @brief Get current call stack depth
     */
    size_t call_depth() const;

    /**
     * @brief Get stack trace for error reporting
     * @return Vector of (function_name, line_number) pairs
     */
    std::vector<std::pair<std::string, int>> get_stack_trace() const;

    // ========================================================================
    // Global Variables
    // ========================================================================

    /**
     * @brief Get global variable
     * @param name Variable name
     * @return Variable value
     * @throws RuntimeError if undefined
     *
     * Implementation:
     * 1. Look up name in globals_
     * 2. If not found, throw error("Undefined variable: " + name)
     * 3. Return value
     */
    Value get_global(const std::string& name) const;

    /**
     * @brief Set global variable
     * @param name Variable name
     * @param value Value to store
     *
     * Implementation: globals_[name] = value
     */
    void set_global(const std::string& name, const Value& value);

    /**
     * @brief Check if global exists
     */
    bool has_global(const std::string& name) const;

    // ========================================================================
    // Constant Pool
    // ========================================================================

    /**
     * @brief Add constant to pool
     * @param value Constant value
     * @return Index in constant pool
     */
    size_t add_constant(const Value& value);

    /**
     * @brief Get constant from pool
     * @param index Constant index
     */
    Value get_constant(size_t index) const;

    /**
     * @brief Get constant pool size
     */
    size_t constant_count() const;

    // ========================================================================
    // Object Allocation
    // ========================================================================

    /**
     * @brief Allocate object on heap
     * @param size Size in bytes
     * @param type Object type
     * @return Pointer to allocated object
     *
     * Implementation:
     * 1. Check GC: if (bytes_allocated_ >= next_gc_threshold_) gc_.collect()
     * 2. Allocate: obj = (Object*)malloc(size)
     * 3. Initialize header:
     *    - obj->type = type
     *    - obj->marked = false
     *    - obj->next = objects_ (add to linked list)
     * 4. Update: objects_ = obj
     * 5. Update: bytes_allocated_ += size
     * 6. Return obj
     */
    Object* allocate_object(size_t size, ObjectType type);

    /**
     * @brief Get total bytes allocated
     */
    size_t bytes_allocated() const;

    /**
     * @brief Get object count
     */
    size_t object_count() const;

    // ========================================================================
    // Built-in Functions
    // ========================================================================

    /**
     * @brief Register all built-in functions
     *
     * Registers: print, println, len, type, str, etc.
     *
     * Implementation:
     * For each builtin:
     * 1. Create NativeFunctionObject
     * 2. Store in builtins_ map
     * 3. Optionally add to globals_ for easy access
     */
    void register_builtins();

    /**
     * @brief Register a native function
     * @param name Function name
     * @param function C++ function pointer
     * @param arity Parameter count
     */
    void register_native_function(
        const std::string& name,
        NativeFunctionObject::NativeFn function,
        uint8_t arity
    );

    // ========================================================================
    // Error Handling
    // ========================================================================

    /**
     * @brief Report runtime error and halt
     * @param message Error message
     *
     * Implementation:
     * 1. Print error message
     * 2. Print stack trace
     * 3. Set halted_ = true
     * 4. Throw exception or exit (depending on error handling mode)
     */
    [[noreturn]] void runtime_error(const std::string& message);

    /**
     * @brief Check if VM is in error state
     */
    bool has_error() const;

    // ========================================================================
    // Debugging
    // ========================================================================

    /**
     * @brief Enable debug mode (print each instruction)
     */
    void set_debug_mode(bool enabled);

    /**
     * @brief Print current VM state (registers, stack, etc.)
     */
    void print_state() const;

    /**
     * @brief Disassemble instruction at current IP
     * @return String representation of instruction
     */
    std::string disassemble_instruction() const;

    // ========================================================================
    // Friends (for GC access)
    // ========================================================================

    friend class GarbageCollector;

    // ========================================================================
    // Public accessors for GC and FFI
    // ========================================================================

    GarbageCollector& gc() { return gc_; }
    FFIManager& ffi() { return ffi_; }

private:
    // ========================================================================
    // Instruction Implementations
    // ========================================================================

    /**
     * Each opcode has a dedicated handler method.
     * These are called from the main switch in run().
     *
     * Naming convention: op_<opcode_name>()
     *
     * Example implementations follow...
     */

    void op_load_const();
    void op_load_int();
    void op_load_true();
    void op_load_false();
    void op_load_none();
    void op_move();
    void op_get_global();
    void op_set_global();

    void op_add();
    void op_sub();
    void op_mul();
    void op_div();
    void op_mod();
    void op_neg();

    void op_eq();
    void op_ne();
    void op_lt();
    void op_le();
    void op_gt();
    void op_ge();

    void op_and();
    void op_or();
    void op_not();

    void op_jmp();
    void op_jmp_if_false();
    void op_jmp_if_true();
    void op_call();
    void op_return();
    void op_return_none();

    void op_new_array();
    void op_new_struct();
    void op_get_index();
    void op_set_index();
    void op_get_field();
    void op_set_field();
    void op_get_len();

    void op_call_ffi();

    void op_print();
    void op_halt();

    // ========================================================================
    // State
    // ========================================================================

    // Execution state
    uint8_t* ip_;                   ///< Instruction pointer (program counter)
    uint8_t* bytecode_start_;       ///< Start of bytecode (for bounds checking)
    uint8_t* bytecode_end_;         ///< End of bytecode
    CallFrame* current_frame_;      ///< Current call frame
    std::vector<CallFrame> call_stack_;  ///< Function call stack
    bool halted_;                   ///< True if VM has stopped

    // Global state
    std::unordered_map<std::string, Value> globals_;  ///< Global variables

    // Constant pool
    std::vector<Value> constants_;

    // Built-in functions
    std::unordered_map<std::string, NativeFunctionObject*> builtins_;

    // Memory management
    Object* objects_;               ///< Linked list of all allocated objects
    size_t bytes_allocated_;        ///< Total bytes allocated
    size_t next_gc_threshold_;      ///< Trigger GC when bytes_allocated_ exceeds this
    GarbageCollector gc_;           ///< Garbage collector

    // FFI
    FFIManager ffi_;                ///< Foreign function interface

    // Debugging
    bool debug_mode_;               ///< Print each instruction if true
    std::string last_error_;        ///< Last error message

    // Limits
    static constexpr size_t MAX_CALL_FRAMES = 1024;  ///< Max recursion depth
};

// ============================================================================
// Built-in Function Implementations
// ============================================================================

/**
 * These are the C++ implementations of built-in functions.
 * They are registered in VM::register_builtins().
 *
 * Signature: Value builtin_name(VM* vm, const std::vector<Value>& args)
 */

/**
 * @brief print(value) -> none
 *
 * Prints value to stdout without newline.
 *
 * Implementation:
 * 1. std::cout << args[0].to_string()
 * 2. return Value::none()
 */
Value builtin_print(VM* vm, const std::vector<Value>& args);

/**
 * @brief println(value) -> none
 *
 * Prints value to stdout with newline.
 */
Value builtin_println(VM* vm, const std::vector<Value>& args);

/**
 * @brief len(collection) -> int
 *
 * Returns length of array or string.
 *
 * Implementation:
 * 1. Check args[0] is array or string
 * 2. Return appropriate length as int value
 */
Value builtin_len(VM* vm, const std::vector<Value>& args);

/**
 * @brief type(value) -> string
 *
 * Returns type name as string.
 *
 * Implementation: return Value::string(args[0].type_name())
 */
Value builtin_type(VM* vm, const std::vector<Value>& args);

/**
 * @brief str(value) -> string
 *
 * Converts value to string.
 *
 * Implementation: return Value::string(args[0].to_string())
 */
Value builtin_str(VM* vm, const std::vector<Value>& args);

/**
 * @brief int(value) -> int
 *
 * Converts value to integer.
 */
Value builtin_int(VM* vm, const std::vector<Value>& args);

/**
 * @brief float(value) -> float
 *
 * Converts value to float.
 */
Value builtin_float(VM* vm, const std::vector<Value>& args);

/**
 * @brief assert(condition, message?) -> none
 *
 * Asserts condition is true, otherwise halts with error.
 */
Value builtin_assert(VM* vm, const std::vector<Value>& args);

} // namespace volta::vm
