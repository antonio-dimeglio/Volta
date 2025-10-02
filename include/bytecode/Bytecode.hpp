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

 
// ===================== X-Macro Table =====================
#define OPCODE_LIST(X) \
    /* ========== Stack Operations ========== */ \
    X(Pop, 0) /* Pop value from stack (discard top value) */ \
    X(Dup, 0) /* Duplicate top stack value */ \
    X(Swap, 0) /* Swap top two stack values */ \
    /* ========== Constants ========== */ \
    X(ConstInt, 8) /* Push constant: ConstInt <8-byte int> */ \
    X(ConstFloat, 8) /* Push constant: ConstFloat <8-byte float> */ \
    X(ConstBool, 1) /* Push constant: ConstBool <1-byte bool> */ \
    X(ConstString, 4) /* Push constant: ConstString <4-byte string-pool-index> */ \
    X(ConstNull, 0) /* Push null/void value */ \
    /* ========== Arithmetic Operations (binary) ========== */ \
    X(AddInt, 0) /* Integer addition */ \
    X(AddFloat, 0) /* Float addition */ \
    X(SubInt, 0) /* Integer subtraction */ \
    X(SubFloat, 0) /* Float subtraction */ \
    X(MulInt, 0) /* Integer multiplication */ \
    X(MulFloat, 0) /* Float multiplication */ \
    X(DivInt, 0) /* Integer division */ \
    X(DivFloat, 0) /* Float division */ \
    X(ModInt, 0) /* Integer modulo */ \
    /* ========== Unary Operations ========== */ \
    X(NegInt, 0) /* Integer negation */ \
    X(NegFloat, 0) /* Float negation */ \
    /* ========== Comparison Operations ========== */ \
    X(EqInt, 0) /* Integer equality */ \
    X(EqFloat, 0) /* Float equality */ \
    X(EqBool, 0) /* Bool equality */ \
    X(NeInt, 0) /* Integer inequality */ \
    X(NeFloat, 0) /* Float inequality */ \
    X(NeBool, 0) /* Bool inequality */ \
    X(LtInt, 0) /* Integer less than */ \
    X(LtFloat, 0) /* Float less than */ \
    X(LeInt, 0) /* Integer less than or equal */ \
    X(LeFloat, 0) /* Float less than or equal */ \
    X(GtInt, 0) /* Integer greater than */ \
    X(GtFloat, 0) /* Float greater than */ \
    X(GeInt, 0) /* Integer greater than or equal */ \
    X(GeFloat, 0) /* Float greater than or equal */ \
    /* ========== Logical Operations ========== */ \
    X(And, 0) /* Logical AND (pop 2 bools, push bool) */ \
    X(Or, 0) /* Logical OR (pop 2 bools, push bool) */ \
    X(Not, 0) /* Logical NOT (pop 1 bool, push bool) */ \
    /* ========== Local Variable Operations ========== */ \
    X(LoadLocal, 4) /* Load local variable: <4-byte local-index> */ \
    X(StoreLocal, 4) /* Store to local variable: <4-byte local-index> */ \
    /* ========== Global Variable Operations ========== */ \
    X(LoadGlobal, 4) /* Load global variable: <4-byte global-index> */ \
    X(StoreGlobal, 4) /* Store to global variable: <4-byte global-index> */ \
    /* ========== Memory Operations ========== */ \
    X(Alloc, 8) /* Allocate object: <4-byte type-id> <4-byte size-in-bytes> */ \
    X(Load, 0) /* Load from heap */ \
    X(Store, 0) /* Store to heap */ \
    /* ========== Struct/Object Operations ========== */ \
    X(GetField, 4) /* Get struct field: <4-byte field-index> */ \
    X(SetField, 4) /* Set struct field: <4-byte field-index> */ \
    /* ========== Array Operations ========== */ \
    X(NewArray, 4) /* Create array: <4-byte element-count> */ \
    X(GetElement, 0) /* Get array element */ \
    X(SetElement, 0) /* Set array element */ \
    X(ArrayLength, 0) /* Get array length */ \
    /* ========== Control Flow ========== */ \
    X(Jump, 4) /* Unconditional jump: <4-byte offset> */ \
    X(JumpIfTrue, 4) /* Conditional jump if true: <4-byte offset> */ \
    X(JumpIfFalse, 4) /* Conditional jump if false: <4-byte offset> */ \
    /* ========== Function Calls ========== */ \
    X(Call, 8) /* Call function: <4-byte function-index> <4-byte arg-count> */ \
    X(CallForeign, 8) /* Call foreign function: <4-byte foreign-index> <4-byte arg-count> */ \
    X(Return, 0) /* Return from function */ \
    X(ReturnVoid, 0) /* Return void from function */ \
    /* ========== Type Conversion ========== */ \
    X(IntToFloat, 0) /* Convert int to float */ \
    X(FloatToInt, 0) /* Convert float to int */ \
    X(IntToBool, 0) /* Convert int to bool */ \
    /* ========== Debug/Special ========== */ \
    X(Print, 0) /* Print top stack value */ \
    X(Halt, 0) /* Halt execution */

// ===================== Enum Definition =====================
enum class Opcode : uint8_t {
#define X(name, size) name,
    OPCODE_LIST(X)
#undef X
};

inline const char* opcodeNames[] = {
#define X(name, size) #name,
    OPCODE_LIST(X)
#undef X
};

inline const int opcodeOperandSizes[] = {
#define X(name, size) size,
    OPCODE_LIST(X)
#undef X
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

    // Utility function to emit bytes
    template<typename T>
    inline void emitBytes(T value) {
        for (size_t i = 0; i < sizeof(T); i++) {
            code_.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xFF));
        }
    }

    // Utility function to patch bytes at a memory location
    template<typename T>
    inline void patchBytes(size_t offset, T value) {
        for (size_t i = 0; i < sizeof(T); i++) {
            code_[offset + i] = static_cast<uint8_t>((value >> (8 * i)) & 0xFF);
        }
    }
};

/**
 * Get human-readable name for an opcode (for disassembly)
 */
inline std::string getOpcodeName(Opcode opcode) {
    return opcodeNames[static_cast<uint8_t>(opcode)];
}
/**
 * Get the size (in bytes) of an opcode's operands
 * Returns 0 for opcodes with no operands
 * Returns -1 for variable-length opcodes
 */
inline int getOpcodeOperandSize(Opcode opcode) {
    return opcodeOperandSizes[static_cast<uint8_t>(opcode)];
}

} // namespace volta::bytecode
