#pragma once

#include "vm/Opcode.hpp"
#include "bytecode/ConstantPool.hpp"
#include <vector>
#include <cstdint>
#include <unordered_map>

namespace volta::bytecode {

/**
 * @brief Bytecode emitter with label management
 *
 * Emits bytecode instructions and handles forward/backward jumps.
 * Supports two-pass assembly:
 * 1. First pass: Emit instructions, record label positions
 * 2. Second pass: Patch jump offsets
 */
class BytecodeEmitter {
public:
    /**
     * @brief Construct emitter
     * @param constants Constant pool builder
     */
    explicit BytecodeEmitter(ConstantPoolBuilder& constants);

    // ========================================================================
    // Low-level emission
    // ========================================================================

    /**
     * @brief Emit single opcode byte
     */
    void emit_opcode(vm::Opcode op);

    /**
     * @brief Emit single byte
     */
    void emit_byte(uint8_t byte);

    /**
     * @brief Emit 16-bit value (big-endian)
     * @param value Value to emit
     *
     * Implementation:
     * 1. Emit high byte: (value >> 8) & 0xFF
     * 2. Emit low byte: value & 0xFF
     */
    void emit_short(uint16_t value);

    /**
     * @brief Emit 24-bit signed value (big-endian)
     * @param value Signed offset (-8388608 to 8388607)
     *
     * Implementation:
     * 1. Emit high byte: (value >> 16) & 0xFF
     * 2. Emit mid byte: (value >> 8) & 0xFF
     * 3. Emit low byte: value & 0xFF
     */
    void emit_int24(int32_t value);

    // ========================================================================
    // High-level instruction emission
    // ========================================================================

    /**
     * @brief Emit A format instruction (1 operand)
     * @param op Opcode
     * @param ra Register A
     *
     * Format: [opcode][ra]
     * Examples: LOAD_TRUE ra, RETURN ra, PRINT ra
     */
    void emit_a(vm::Opcode op, uint8_t ra);

    /**
     * @brief Emit AB format instruction (2 operands)
     * @param op Opcode
     * @param ra Register A
     * @param rb Register B
     *
     * Format: [opcode][ra][rb]
     * Examples: MOVE ra, rb; NEG ra, rb; NOT ra, rb
     */
    void emit_ab(vm::Opcode op, uint8_t ra, uint8_t rb);

    /**
     * @brief Emit ABC format instruction (3 operands)
     * @param op Opcode
     * @param ra Register A (destination)
     * @param rb Register B (source 1)
     * @param rc Register C (source 2)
     *
     * Format: [opcode][ra][rb][rc]
     * Examples: ADD ra, rb, rc; SUB ra, rb, rc; CALL ra, rb, rc
     */
    void emit_abc(vm::Opcode op, uint8_t ra, uint8_t rb, uint8_t rc);

    /**
     * @brief Emit ABx format instruction (register + 16-bit immediate)
     * @param op Opcode
     * @param ra Register A
     * @param bx 16-bit immediate value
     *
     * Format: [opcode][ra][bx_hi][bx_lo]
     * Examples: LOAD_CONST ra, kx; GET_GLOBAL ra, kx
     */
    void emit_abx(vm::Opcode op, uint8_t ra, uint16_t bx);

    /**
     * @brief Emit Ax format instruction (24-bit immediate)
     * @param op Opcode
     * @param offset 24-bit signed offset
     *
     * Format: [opcode][ax_hi][ax_mid][ax_lo]
     * Examples: JMP offset
     * Note: Used for jumps, offset is relative to next instruction
     */
    void emit_ax(vm::Opcode op, int32_t offset);

    // ========================================================================
    // Label management
    // ========================================================================

    /**
     * @brief Create a new label
     * @return Label ID
     *
     * Implementation:
     * 1. Generate unique label ID (next_label_++)
     * 2. Return ID
     */
    uint32_t create_label();

    /**
     * @brief Mark current position with label
     * @param label Label ID
     *
     * Implementation:
     * 1. Record label_positions_[label] = current_offset()
     */
    void mark_label(uint32_t label);

    /**
     * @brief Emit unconditional jump to label
     * @param label Target label
     *
     * Implementation (first pass):
     * 1. Emit JMP opcode
     * 2. Emit placeholder offset (0x000000)
     * 3. Record jump_patches_.push_back({current_offset - 3, label})
     */
    void emit_jump(uint32_t label);

    /**
     * @brief Emit conditional jump if register is false
     * @param reg Register to test
     * @param label Target label if false
     *
     * Implementation:
     * 1. Emit JMP_IF_FALSE opcode
     * 2. Emit register
     * 3. Emit placeholder offset (0x0000)
     * 4. Record jump patch
     */
    void emit_jump_if_false(uint8_t reg, uint32_t label);

    /**
     * @brief Emit conditional jump if register is true
     * @param reg Register to test
     * @param label Target label if true
     */
    void emit_jump_if_true(uint8_t reg, uint32_t label);

    /**
     * @brief Resolve all jump offsets (call after all code emitted)
     *
     * Implementation:
     * For each (patch_offset, label_id) in jump_patches_:
     * 1. Get target = label_positions_[label_id]
     * 2. Calculate offset = target - (patch_offset + 4)
     *    - +4 because offset is from next instruction
     * 3. Check offset fits in 24 bits
     * 4. Patch bytecode at patch_offset with offset
     */
    void resolve_jumps();

    // ========================================================================
    // Access
    // ========================================================================

    /**
     * @brief Get current bytecode offset
     */
    size_t current_offset() const { return code_.size(); }

    /**
     * @brief Get final bytecode
     */
    const std::vector<uint8_t>& bytecode() const { return code_; }

    /**
     * @brief Get mutable bytecode (for patching)
     */
    std::vector<uint8_t>& bytecode() { return code_; }

    /**
     * @brief Clear bytecode (for reuse)
     */
    void clear();

private:
    ConstantPoolBuilder& constants_;
    std::vector<uint8_t> code_;

    // Label management
    uint32_t next_label_ = 0;
    std::unordered_map<uint32_t, uint32_t> label_positions_;  // label_id -> bytecode_offset

    // Jump patching
    struct JumpPatch {
        uint32_t code_offset;  // Where to patch
        uint32_t label_id;     // Target label
        bool is_short;         // true = 16-bit offset, false = 24-bit offset
    };
    std::vector<JumpPatch> jump_patches_;
};

} // namespace volta::bytecode
