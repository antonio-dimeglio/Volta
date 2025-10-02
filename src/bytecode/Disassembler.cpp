#include "../include/bytecode/Disassembler.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace volta::bytecode {

/*
Utility function do disassemble value of a specific size.
*/
template<typename T>
inline T readLE(const std::vector<uint8_t>& code, size_t offset) {
    T value = 0;
    for (size_t i = 0; i < sizeof(T); i++) {
        value |= static_cast<T>(code[offset + i]) << (8 * i);
    }
    return value;
}

// ========== Module Disassembly ==========

std::string Disassembler::disassembleModule(const CompiledModule& module) {
    std::stringstream ss;
    disassembleModule(module, ss);
    return ss.str();
}

void Disassembler::disassembleModule(const CompiledModule& module, std::ostream& out) {
    setStringPool(&module.stringPool());

    out << "Module: " << module.name() << "\n";
    out << "========================================\n\n";

    if (!module.stringPool().empty()) {
        out << "String Pool:\n";
        for (size_t i = 0; i < module.stringPool().size(); i++) {
            out << "  @str" << i << ": \"" << module.stringPool()[i] << "\"\n";
        }
        out << "\n";
    }

    if (module.globalCount() > 0) {
        out << "Globals: " << module.globalCount() << "\n\n";
    }

    if (!module.foreignFunctions().empty()) {
        out << "Foreign Functions:\n";
        for (size_t i = 0; i < module.foreignFunctions().size(); i++) {
            out << "  [" << i << "] " << module.foreignFunctions()[i] << "\n";
        }
        out << "\n";
    }

    for (const auto& function : module.functions()) {
        disassembleFunction(function, out);
        out << "\n"; 
    }
}

// ========== Function Disassembly ==========

std::string Disassembler::disassembleFunction(const CompiledFunction& function) {
    std::stringstream ss;
    disassembleFunction(function, ss);
    return ss.str();
}

void Disassembler::disassembleFunction(const CompiledFunction& function, std::ostream& out) {
    out << "Function: " << function.name;

    if (function.isForeign) {
        out << " [FOREIGN]";
    } else {
        out << " (params=" << function.parameterCount
            << ", locals=" << function.localCount << ")";
    }

    out << "\n";

    if (function.isForeign) {
        return;
    }

    std::stringstream chunkStream;
    disassembleChunk(function.chunk, chunkStream);

    std::string line;
    while (std::getline(chunkStream, line)) {
        out << "  " << line << "\n"; 
    }
}

// ========== Chunk Disassembly ==========

std::string Disassembler::disassembleChunk(const Chunk& chunk) {
    std::stringstream ss;
    size_t offset = 0;

    while (offset < chunk.code_.size()) {
        offset = disassembleInstruction(chunk, offset, ss);
    }

    return ss.str();
}

void Disassembler::disassembleChunk(const Chunk& chunk, std::ostream& out) {
    std::string disasm = disassembleChunk(chunk); 
    out << disasm;
}

// ========== Instruction Disassembly ==========

size_t Disassembler::disassembleInstruction(const Chunk& chunk, size_t offset, std::ostream& out) {
    Opcode op = static_cast<Opcode>(chunk.code_[offset]);

    out << std::hex << std::setw(4) << std::setfill('0') << offset << " ";
    out << std::dec << getOpcodeName(op);

    int operandSize = getOpcodeOperandSize(op);

    if (operandSize == 0) {
        out << "\n";
        return offset + 1;  // Return next offset (current + opcode byte)
    }
    else if (operandSize == 1) {
        uint8_t value = chunk.code_[offset + 1];
        out << " " << static_cast<int>(value) << "\n";
        return offset + 1 + 1; // current + opcode + 1 byte operand
    }
    else if (operandSize == 4) {
        int32_t value = readInt32(chunk, offset + 1);

        if (op == Opcode::ConstString) {
            out << " " << formatStringConstant(value) << "\n";
        } else {
            out << " " << value << "\n";
        }

        return offset + 1 + 4; // current + opcode + 4 byte operand
    }
    else if (operandSize == 8) {
        if (op == Opcode::Call || op == Opcode::CallForeign) {
            int32_t funcIndex = readInt32(chunk, offset + 1);
            int32_t argCount = readInt32(chunk, offset + 5);
            out << " func=" << funcIndex << " args=" << argCount << "\n";
            return offset + 1 + 8;
        }
        else if (op == Opcode::Alloc) {
            int32_t typeId = readInt32(chunk, offset + 1);
            int32_t size = readInt32(chunk, offset + 5);
            out << " type=" << typeId << " size=" << size << "\n";
            return offset + 1 + 8;
        }
        else if (op == Opcode::ConstFloat) {
            double value = readFloat64(chunk, offset + 1);
            out << " " << value << "\n";
            return offset + 1 + 8;
        }
        else {
            int64_t value = readInt64(chunk, offset + 1);
            out << " " << value << "\n";
            return offset + 1 + 8;
        }
    }

    // Unreachable unless something is implemented
    out << " [UNKNOWN OPERAND SIZE]\n";
    return offset + 1;
}

// ========== Operand Decoding Helpers ==========

int32_t Disassembler::readInt32(const Chunk& chunk, size_t offset) const {
    return readLE<int32_t>(chunk.code_, offset);
}

int64_t Disassembler::readInt64(const Chunk& chunk, size_t offset) const {
    return readLE<int64_t>(chunk.code_, offset);
}

double Disassembler::readFloat64(const Chunk& chunk, size_t offset) const {
    int64_t asInt = readLE<int64_t>(chunk.code_, offset);
    double value;
    std::memcpy(&value, &asInt, sizeof(double));
    return value;
}

bool Disassembler::readBool(const Chunk& chunk, size_t offset) const {
    return readLE<bool>(chunk.code_, offset);
}

// ========== Formatting Helpers ==========

std::string Disassembler::formatOffset(size_t offset) const {
    std::stringstream ss;
    ss << std::hex << std::setw(4) << std::setfill('0') << offset;
    return ss.str();
}

std::string Disassembler::formatRawBytes(const Chunk& chunk, size_t offset, size_t length) const {
    // Format raw bytes as hex: "05 2A 00 00 00" for a 5-byte instruction
    std::stringstream ss;
    for (size_t i = 0; i < length; i++) {
        if (i > 0) ss << " ";
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(chunk.code_[offset + i]);
    }
    return ss.str();
}

std::string Disassembler::formatStringConstant(uint32_t index) const {
    // Format string reference, look up actual string if pool is available
    std::stringstream ss;
    ss << "@str" << index;

    if (stringPool_ && index < stringPool_->size()) {
        // Show the actual string value: @str0 ("hello")
        ss << " (\"" << (*stringPool_)[index] << "\")";
    }

    return ss.str();
}

// ========== Utility Functions ==========

void dumpModule(const CompiledModule& module) {
    Disassembler disasm;
    disasm.disassembleModule(module, std::cout);
}

void dumpFunction(const CompiledFunction& function) {
    Disassembler disasm;
    disasm.disassembleFunction(function, std::cout);
}

void dumpChunk(const Chunk& chunk) {
    Disassembler disasm;
    disasm.disassembleChunk(chunk, std::cout);
}

} // namespace volta::bytecode
