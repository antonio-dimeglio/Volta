#include "../include/bytecode/Bytecode.hpp"
#include <cstring>

namespace volta::bytecode {

// ========== Chunk Implementation ==========

void Chunk::emitOpcode(Opcode opcode) {
}

void Chunk::emitInt32(int32_t value) {
}

void Chunk::emitInt64(int64_t value) {
}

void Chunk::emitFloat64(double value) {
}

void Chunk::emitBool(bool value) {
}

void Chunk::patchInt32(size_t offset, int32_t value) {
}

void Chunk::addLineNumber(size_t offset, uint32_t lineNumber) {
}

uint32_t Chunk::getLineNumber(size_t offset) const {
    return 0;
}

// ========== Utility Functions ==========

std::string getOpcodeName(Opcode opcode) {
    return "";
}

int getOpcodeOperandSize(Opcode opcode) {
    return 0;
}

} // namespace volta::bytecode
