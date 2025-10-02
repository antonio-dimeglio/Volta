#pragma once

#include <cstdint>
#include <string>

namespace volta::vm {

/**
 * @brief Bytecode opcodes for the Volta VM
 *
 * Instruction encoding formats:
 * - ABC:  [OP:8] [A:8] [B:8] [C:8]          (4 bytes, 3 register operands)
 * - ABx:  [OP:8] [A:8] [Bx:16]              (4 bytes, 1 register + 16-bit immediate)
 * - Ax:   [OP:8] [Ax:24]                    (4 bytes, 24-bit immediate)
 * - A:    [OP:8] [A:8]                      (2 bytes, 1 register operand)
 *
 * Register naming: RA, RB, RC = registers A, B, C
 * Constant naming: Kx = constant pool index
 */
enum class Opcode : uint8_t {
    // ========================================================================
    // Load/Store/Move (0x00-0x0F)
    // ========================================================================

    /**
     * LOAD_CONST RA, Kx
     * Format: ABx
     * Effect: RA = constants[Kx]
     * Loads a constant from the constant pool into register RA
     */
    LOAD_CONST = 0x00,

    /**
     * LOAD_INT RA, imm8
     * Format: AB (where B is signed int8)
     * Effect: RA = sign_extend(imm8)
     * Loads small integer directly (avoids constant pool for -128..127)
     */
    LOAD_INT = 0x01,

    /**
     * LOAD_TRUE RA
     * Format: A
     * Effect: RA = true
     */
    LOAD_TRUE = 0x02,

    /**
     * LOAD_FALSE RA
     * Format: A
     * Effect: RA = false
     */
    LOAD_FALSE = 0x03,

    /**
     * LOAD_NONE RA
     * Format: A
     * Effect: RA = none
     */
    LOAD_NONE = 0x04,

    /**
     * MOVE RA, RB
     * Format: AB
     * Effect: RA = RB
     * Copies value from RB to RA
     */
    MOVE = 0x05,

    /**
     * GET_GLOBAL RA, Kx
     * Format: ABx
     * Effect: RA = globals[constants[Kx].as_string()]
     * Loads global variable (name in constant pool)
     */
    GET_GLOBAL = 0x06,

    /**
     * SET_GLOBAL Kx, RA
     * Format: ABx (encoded as A=dest, Bx=source for consistency)
     * Effect: globals[constants[Kx].as_string()] = RA
     * Stores RA into global variable
     */
    SET_GLOBAL = 0x07,

    // ========================================================================
    // Arithmetic (0x10-0x1F)
    // ========================================================================

    /**
     * ADD RA, RB, RC
     * Format: ABC
     * Effect: RA = RB + RC
     * Adds two values (handles int+int, float+float, int+float)
     */
    ADD = 0x10,

    /**
     * SUB RA, RB, RC
     * Format: ABC
     * Effect: RA = RB - RC
     */
    SUB = 0x11,

    /**
     * MUL RA, RB, RC
     * Format: ABC
     * Effect: RA = RB * RC
     */
    MUL = 0x12,

    /**
     * DIV RA, RB, RC
     * Format: ABC
     * Effect: RA = RB / RC
     * Division by zero: throws runtime error
     */
    DIV = 0x13,

    /**
     * MOD RA, RB, RC
     * Format: ABC
     * Effect: RA = RB % RC
     * Modulo (remainder), only for integers
     */
    MOD = 0x14,

    /**
     * NEG RA, RB
     * Format: AB
     * Effect: RA = -RB
     * Unary negation
     */
    NEG = 0x15,

    // ========================================================================
    // Comparison (0x20-0x2F)
    // ========================================================================

    /**
     * EQ RA, RB, RC
     * Format: ABC
     * Effect: RA = (RB == RC)
     * Equality comparison
     */
    EQ = 0x20,

    /**
     * NE RA, RB, RC
     * Format: ABC
     * Effect: RA = (RB != RC)
     */
    NE = 0x21,

    /**
     * LT RA, RB, RC
     * Format: ABC
     * Effect: RA = (RB < RC)
     */
    LT = 0x22,

    /**
     * LE RA, RB, RC
     * Format: ABC
     * Effect: RA = (RB <= RC)
     */
    LE = 0x23,

    /**
     * GT RA, RB, RC
     * Format: ABC
     * Effect: RA = (RB > RC)
     */
    GT = 0x24,

    /**
     * GE RA, RB, RC
     * Format: ABC
     * Effect: RA = (RB >= RC)
     */
    GE = 0x25,

    // ========================================================================
    // Logical (0x30-0x3F)
    // ========================================================================

    /**
     * AND RA, RB, RC
     * Format: ABC
     * Effect: RA = RB && RC
     * Logical AND (short-circuits via jumps at compile time)
     */
    AND = 0x30,

    /**
     * OR RA, RB, RC
     * Format: ABC
     * Effect: RA = RB || RC
     * Logical OR (short-circuits via jumps at compile time)
     */
    OR = 0x31,

    /**
     * NOT RA, RB
     * Format: AB
     * Effect: RA = !RB
     * Logical NOT
     */
    NOT = 0x32,

    // ========================================================================
    // Control Flow (0x40-0x4F)
    // ========================================================================

    /**
     * JMP offset16
     * Format: Ax (where Ax is signed 16-bit offset)
     * Effect: ip += sign_extend(offset16)
     * Unconditional jump (relative to current ip)
     */
    JMP = 0x40,

    /**
     * JMP_IF_FALSE RA, offset16
     * Format: ABx (where Bx is signed 16-bit offset)
     * Effect: if (!to_bool(RA)) ip += sign_extend(offset16)
     * Conditional jump if RA is falsy
     */
    JMP_IF_FALSE = 0x41,

    /**
     * JMP_IF_TRUE RA, offset16
     * Format: ABx
     * Effect: if (to_bool(RA)) ip += sign_extend(offset16)
     * Conditional jump if RA is truthy
     */
    JMP_IF_TRUE = 0x42,

    /**
     * CALL RA, RB, nargs
     * Format: ABC
     * Effect: RA = call(closure@RB, args: R(RB+1)..R(RB+nargs))
     * Calls function in RB with nargs arguments starting at R(RB+1)
     * Result stored in RA
     *
     * Implementation:
     * 1. Get closure from RB
     * 2. Create new call frame
     * 3. Copy arguments: frame.registers[0..nargs-1] = args
     * 4. Save return address and return register (RA)
     * 5. Jump to function bytecode
     */
    CALL = 0x43,

    /**
     * RETURN RA
     * Format: A
     * Effect: return RA
     * Returns from current function with value in RA
     *
     * Implementation:
     * 1. Save return value from RA
     * 2. Pop current frame from call stack
     * 3. Restore previous frame
     * 4. Store return value in caller's return register
     * 5. Continue execution at return address
     */
    RETURN = 0x44,

    /**
     * RETURN_NONE
     * Format: None (just opcode)
     * Effect: return none
     * Returns from function with none value
     */
    RETURN_NONE = 0x45,

    // ========================================================================
    // Object Operations (0x50-0x6F)
    // ========================================================================

    /**
     * NEW_ARRAY RA, size16
     * Format: ABx
     * Effect: RA = new Array(size16)
     * Creates new array with initial capacity
     */
    NEW_ARRAY = 0x50,

    /**
     * NEW_STRUCT RA, Kx
     * Format: ABx
     * Effect: RA = new Struct(field_count from constants[Kx])
     * Creates new struct instance
     * constants[Kx] contains field count as integer
     */
    NEW_STRUCT = 0x51,

    /**
     * GET_INDEX RA, RB, RC
     * Format: ABC
     * Effect: RA = RB[RC]
     * Array/string indexing
     *
     * Implementation:
     * 1. Check RB is array or string
     * 2. Check RC is integer
     * 3. Bounds check: if (RC >= length) throw error
     * 4. Return: RA = array.elements[RC]
     */
    GET_INDEX = 0x52,

    /**
     * SET_INDEX RA, RB, RC
     * Format: ABC
     * Effect: RA[RB] = RC
     * Array element assignment
     *
     * Implementation:
     * 1. Check RA is array
     * 2. Check RB is integer
     * 3. Bounds check: if (RB >= length) throw error
     * 4. Set: array.elements[RB] = RC
     */
    SET_INDEX = 0x53,

    /**
     * GET_FIELD RA, RB, idx8
     * Format: ABC (where C is field index)
     * Effect: RA = RB.field[idx8]
     * Struct field access
     *
     * Implementation:
     * 1. Check RB is struct
     * 2. Bounds check: if (idx8 >= field_count) throw error
     * 3. Return: RA = struct.fields[idx8]
     */
    GET_FIELD = 0x54,

    /**
     * SET_FIELD RA, idx8, RB
     * Format: ABC (where B is field index, C is value)
     * Effect: RA.field[idx8] = RB
     * Struct field assignment
     *
     * Implementation:
     * 1. Check RA is struct
     * 2. Bounds check: if (idx8 >= field_count) throw error
     * 3. Set: struct.fields[idx8] = RB
     */
    SET_FIELD = 0x55,

    /**
     * GET_LEN RA, RB
     * Format: AB
     * Effect: RA = length(RB)
     * Get length of array or string
     *
     * Implementation:
     * 1. Check RB is array or string
     * 2. Return appropriate length
     */
    GET_LEN = 0x56,

    // ========================================================================
    // FFI (0x70-0x7F)
    // ========================================================================

    /**
     * CALL_FFI RA, Kx, nargs
     * Format: ABC
     * Effect: RA = call_c_function(ffi@constants[Kx], nargs)
     * Calls C function via FFI
     * Arguments in R(RA+1)..R(RA+nargs)
     *
     * Implementation:
     * 1. Get FFI function from constants[Kx]
     * 2. Collect arguments from registers
     * 3. Marshal arguments to C types
     * 4. Pause GC (prevent collection during C call)
     * 5. Call C function via FFI manager
     * 6. Resume GC
     * 7. Marshal return value to Volta value
     * 8. Store result in RA
     */
    CALL_FFI = 0x70,

    // ========================================================================
    // Debug/Meta (0xF0-0xFF)
    // ========================================================================

    /**
     * PRINT RA
     * Format: A
     * Effect: print(RA) to stdout
     * Built-in print for debugging (calls Value::to_string())
     */
    PRINT = 0xF0,

    /**
     * HALT
     * Format: None
     * Effect: Stop execution
     * Terminates the VM (used at end of top-level code)
     */
    HALT = 0xFF
};

/**
 * @brief Get human-readable name of opcode
 * @param op Opcode
 * @return String representation (e.g., "ADD")
 *
 * Used for disassembly and debugging
 */
std::string opcode_name(Opcode op);

/**
 * @brief Get number of operand bytes for opcode
 * @param op Opcode
 * @return Number of bytes following opcode (0-3)
 *
 * Used for instruction length calculation:
 * - A format: 1 byte
 * - AB format: 2 bytes
 * - ABC format: 3 bytes
 * - ABx/Ax format: 3 bytes
 */
int opcode_operand_count(Opcode op);

} // namespace volta::vm
