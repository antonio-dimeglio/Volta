#include "../include/bytecode/Bytecode.hpp"
#include <cstring>

namespace volta::bytecode {

// ========== Chunk Implementation ==========

void Chunk::emitOpcode(Opcode opcode) {
    code_.push_back(static_cast<uint8_t>(opcode));
}

void Chunk::emitInt32(int32_t value) {
    emitBytes(value);
}

void Chunk::emitInt64(int64_t value) {
    emitBytes(value);
}

void Chunk::emitFloat64(double value) {
    uint64_t asInt;
    std::memcpy(&asInt, &value, sizeof(double)); // copy bits
    emitBytes(asInt);
}

void Chunk::emitBool(bool value) {
    code_.push_back(value ? 1 : 0);
}

void Chunk::patchInt32(size_t offset, int32_t value) {
    patchBytes(offset, value);
}

void Chunk::addLineNumber(size_t offset, uint32_t lineNumber) {
    lineNumbers_.emplace_back(offset, lineNumber);
}

uint32_t Chunk::getLineNumber(size_t offset) const {
    if (lineNumbers_.empty()) return 0;

    uint32_t lastLine = lineNumbers_[0].second;
    for (const auto& pair : lineNumbers_) {
        if (pair.first > offset) break;
        lastLine = pair.second;
    }
    return lastLine;
}

} // namespace volta::bytecode
