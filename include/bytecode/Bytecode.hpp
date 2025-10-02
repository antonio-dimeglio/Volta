#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace volta::bytecode {

/**
 * Bytecode instruction set for the Volta VM
 * Stack-based architecture with support for:
 * - Arithmetic and logical operations
 * - Memory operations (local, global, heap)
 * - Control flow (branches, calls)
 * - Type-specific operations
 */
enum class Opcode : uint8_t {
    // ========== Stack Operations ==========

    /// Pop value from stack (discard top value)
    Pop,

    /// Duplicate top stack value
    Dup,

    /// Swap top two stack values
    Swap,

    // ========== Constants ==========

    /// Push constant: ConstInt <8-byte int>
    ConstInt,

    /// Push constant: ConstFloat <8-byte float>
    ConstFloat,

    /// Push constant: ConstBool <1-byte bool>
    ConstBool,

    /// Push constant: ConstString <4-byte string-pool-index>
    ConstString,

    /// Push null/void value
    ConstNull,

    // ========== Arithmetic Operations (binary) ==========
    // All pop two values, push result

    AddInt,     ///< Integer addition
    AddFloat,   ///< Float addition
    SubInt,     ///< Integer subtraction
    SubFloat,   ///< Float subtraction
    MulInt,     ///< Integer multiplication
    MulFloat,   ///< Float multiplication
    DivInt,     ///< Integer division
    DivFloat,   ///< Float division
    ModInt,     ///< Integer modulo

    // ========== Unary Operations ==========

    NegInt,     ///< Integer negation
    NegFloat,   ///< Float negation

    // ========== Comparison Operations ==========
    // All pop two values, push bool result

    EqInt,      ///< Integer equality
    EqFloat,    ///< Float equality
    EqBool,     ///< Bool equality
    NeInt,      ///< Integer inequality
    NeFloat,    ///< Float inequality
    NeBool,     ///< Bool inequality
    LtInt,      ///< Integer less than
    LtFloat,    ///< Float less than
    LeInt,      ///< Integer less than or equal
    LeFloat,    ///< Float less than or equal
    GtInt,      ///< Integer greater than
    GtFloat,    ///< Float greater than
    GeInt,      ///< Integer greater than or equal
    GeFloat,    ///< Float greater than or equal

    // ========== Logical Operations ==========

    And,        ///< Logical AND (pop 2 bools, push bool)
    Or,         ///< Logical OR (pop 2 bools, push bool)
    Not,        ///< Logical NOT (pop 1 bool, push bool)

    // ========== Local Variable Operations ==========

    /// Load local variable: LoadLocal <4-byte local-index>
    LoadLocal,

    /// Store to local variable: StoreLocal <4-byte local-index>
    StoreLocal,

    // ========== Global Variable Operations ==========

    /// Load global variable: LoadGlobal <4-byte global-index>
    LoadGlobal,

    /// Store to global variable: StoreGlobal <4-byte global-index>
    StoreGlobal,

    // ========== Memory Operations ==========

    /// Allocate object on heap: Alloc <4-byte type-id> <4-byte size-in-bytes>
    Alloc,

    /// Load from heap: Load (pops address, pushes value)
    Load,

    /// Store to heap: Store (pops value, pops address)
    Store,

    // ========== Struct/Object Operations ==========

    /// Get struct field: GetField <4-byte field-index>
    /// Pops object reference, pushes field value
    GetField,

    /// Set struct field: SetField <4-byte field-index>
    /// Pops value, pops object reference
    SetField,

    // ========== Array Operations ==========

    /// Create array: NewArray <4-byte element-count>
    /// Pops N elements from stack, pushes array reference
    NewArray,

    /// Get array element: GetElement
    /// Pops index, pops array reference, pushes element
    GetElement,

    /// Set array element: SetElement
    /// Pops value, pops index, pops array reference
    SetElement,

    /// Get array length: ArrayLength
    /// Pops array reference, pushes length as int
    ArrayLength,

    // ========== Control Flow ==========

    /// Unconditional jump: Jump <4-byte offset>
    Jump,

    /// Conditional jump if true: JumpIfTrue <4-byte offset>
    /// Pops bool value, jumps if true
    JumpIfTrue,

    /// Conditional jump if false: JumpIfFalse <4-byte offset>
    /// Pops bool value, jumps if false
    JumpIfFalse,

    // ========== Function Calls ==========

    /// Call function: Call <4-byte function-index> <4-byte arg-count>
    /// Pops N arguments in reverse order, pushes return value
    Call,

    /// Call foreign (C) function: CallForeign <4-byte foreign-index> <4-byte arg-count>
    /// Pops N arguments, calls C function, pushes return value
    CallForeign,

    /// Return from function: Return
    /// Pops return value (if any), returns to caller
    Return,

    /// Return void from function: ReturnVoid
    /// Returns to caller without popping a value
    ReturnVoid,

    // ========== Type Conversion ==========

    IntToFloat,   ///< Convert int to float
    FloatToInt,   ///< Convert float to int (truncate)
    IntToBool,    ///< Convert int to bool (0 = false, else true)

    // ========== Debug/Special ==========

    /// Print top stack value for debugging (does not pop)
    Print,

    /// Halt execution (error/panic)
    Halt,
};

/**
 * Bytecode chunk - represents compiled bytecode for a function or module
 * Contains:
 * - Raw bytecode instructions
 * - Constant pool (strings, large constants)
 * - Debug information (line numbers, source locations)
 */
class Chunk {
public:
    Chunk() = default;

    /// Emit a single-byte opcode
    void emitOpcode(Opcode opcode);
    
    /// Emit a 4-byte integer operand (for indices, offsets)
    void emitInt32(int32_t value);

    /// Emit an 8-byte integer constant
    void emitInt64(int64_t value);

    /// Emit an 8-byte float constant
    void emitFloat64(double value);

    /// Emit a 1-byte boolean constant
    void emitBool(bool value);

    /// Get the current bytecode offset (for jump patching)
    size_t currentOffset() const { return code_.size(); }

    /// Patch a 4-byte integer at the given offset (for forward jumps)
    void patchInt32(size_t offset, int32_t value);

    /// Get the raw bytecode
    const std::vector<uint8_t>& code() const { return code_; }

    /// Add debug line number information
    void addLineNumber(size_t offset, uint32_t lineNumber);

    /// Get line number for a bytecode offset (for error reporting)
    uint32_t getLineNumber(size_t offset) const;

    /// Raw bytecode instructions and operands
    std::vector<uint8_t> code_;
private:
    /// Debug info: bytecode offset -> source line number
    std::vector<std::pair<size_t, uint32_t>> lineNumbers_;
};

/**
 * Get human-readable name for an opcode (for disassembly)
 */
std::string getOpcodeName(Opcode opcode);

/**
 * Get the size (in bytes) of an opcode's operands
 * Returns 0 for opcodes with no operands
 * Returns -1 for variable-length opcodes
 */
int getOpcodeOperandSize(Opcode opcode);

} // namespace volta::bytecode
