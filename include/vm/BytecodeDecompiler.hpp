#pragma once
#include "BytecodeModule.hpp"
#include "OPCode.hpp"
#include <ostream>

namespace volta::vm {

class BytecodeDecompiler {
private:
    const BytecodeModule& module_;  // Use const - we're only reading
    std::ostream& out_;
    
    // Disassemble one function
    void disassembleFunction(const FunctionInfo& fn);
    
    // Disassemble one instruction at offset, returns bytes consumed
    size_t disassembleInstruction(uint32_t offset);
    
    // Helper: get opcode name as string
    const char* getOpcodeName(Opcode op);
    
public:
    BytecodeDecompiler(const BytecodeModule& module, std::ostream& out)
        : module_(module), out_(out) {}
    
    // Disassemble entire module
    void disassemble();
};

}