#include "bytecode/BytecodeEmitter.hpp"
#include <stdexcept>

namespace volta::bytecode {

BytecodeEmitter::BytecodeEmitter(ConstantPoolBuilder& constants)
    : constants_(constants) {
}

void BytecodeEmitter::emit_opcode(vm::Opcode op) {
    code_.push_back(static_cast<uint8_t>(op));
}

void BytecodeEmitter::emit_byte(uint8_t byte) {
    code_.push_back(byte);
}

void BytecodeEmitter::emit_short(uint16_t value) {
    code_.push_back((value >> 8) & 0xFF);
    code_.push_back(value & 0xFF);
}

void BytecodeEmitter::emit_int24(int32_t value) {
    if (value < -8388608 || value > 8388607) throw std::runtime_error("tried to emit value out of range");
    code_.push_back((value >> 16) & 0xFF);
    code_.push_back((value >> 8) & 0xFF);
    code_.push_back(value & 0xFF);
}

void BytecodeEmitter::emit_a(vm::Opcode op, uint8_t ra) {
    emit_opcode(op);
    emit_byte(ra);
}

void BytecodeEmitter::emit_ab(vm::Opcode op, uint8_t ra, uint8_t rb) {
    emit_opcode(op);
    emit_byte(ra);
    emit_byte(rb);
}

void BytecodeEmitter::emit_abc(vm::Opcode op, uint8_t ra, uint8_t rb, uint8_t rc) {
    emit_opcode(op);
    emit_byte(ra);
    emit_byte(rb);
    emit_byte(rc);
}

void BytecodeEmitter::emit_abx(vm::Opcode op, uint8_t ra, uint16_t bx) {
    emit_opcode(op);
    emit_byte(ra);
    emit_short(bx);
}

void BytecodeEmitter::emit_ax(vm::Opcode op, int32_t offset) {
    emit_opcode(op);
    emit_int24(offset);
}

uint32_t BytecodeEmitter::create_label() {
    return next_label_++;
}

void BytecodeEmitter::mark_label(uint32_t label) {
    label_positions_[label] = current_offset();
}

void BytecodeEmitter::emit_jump(uint32_t label) {
    emit_opcode(vm::Opcode::JMP);
    uint32_t patch_offset = current_offset();
    emit_int24(0);  // Placeholder
    jump_patches_.push_back({patch_offset, label, false});
}

void BytecodeEmitter::emit_jump_if_false(uint8_t reg, uint32_t label) {
    emit_opcode(vm::Opcode::JMP_IF_FALSE);
    emit_byte(reg);
    uint32_t patch_offset = current_offset();
    emit_short(0);  // Placeholder (16-bit for conditional jumps)
    jump_patches_.push_back({patch_offset, label, true});
}

void BytecodeEmitter::emit_jump_if_true(uint8_t reg, uint32_t label) {
    emit_opcode(vm::Opcode::JMP_IF_TRUE);
    emit_byte(reg);
    uint32_t patch_offset = current_offset();
    emit_short(0);  // Placeholder (16-bit for conditional jumps)
    jump_patches_.push_back({patch_offset, label, true});
}

void BytecodeEmitter::resolve_jumps() {
    for (const auto& patch : jump_patches_) {
        auto it = label_positions_.find(patch.label_id);
        if (it == label_positions_.end()) {
            throw std::runtime_error("Undefined label: " + std::to_string(patch.label_id));
        }

        uint32_t target = it->second;
        int32_t offset;

        if (patch.is_short) {
            // 16-bit offset for conditional jumps
            offset = static_cast<int32_t>(target) - static_cast<int32_t>(patch.code_offset + 2);
            if (offset < -32768 || offset > 32767) {
                throw std::runtime_error("Jump offset out of range for 16-bit: " + std::to_string(offset));
            }
            code_[patch.code_offset] = (offset >> 8) & 0xFF;
            code_[patch.code_offset + 1] = offset & 0xFF;
        } else {
            // 24-bit offset for unconditional jumps
            offset = static_cast<int32_t>(target) - static_cast<int32_t>(patch.code_offset + 3);
            if (offset < -8388608 || offset > 8388607) {
                throw std::runtime_error("Jump offset out of range for 24-bit: " + std::to_string(offset));
            }
            code_[patch.code_offset] = (offset >> 16) & 0xFF;
            code_[patch.code_offset + 1] = (offset >> 8) & 0xFF;
            code_[patch.code_offset + 2] = offset & 0xFF;
        }
    }
}

void BytecodeEmitter::clear() {
    code_.clear();
    label_positions_.clear();
    jump_patches_.clear();
    next_label_ = 0;
}

} 
