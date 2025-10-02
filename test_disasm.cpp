#include "include/bytecode/Bytecode.hpp"
#include "include/bytecode/BytecodeCompiler.hpp"
#include "include/bytecode/Disassembler.hpp"
#include <iostream>

int main() {
    using namespace volta::bytecode;

    CompiledFunction func;
    func.name = "test_func";
    func.parameterCount = 2;
    func.localCount = 5;
    func.chunk.emitOpcode(Opcode::ReturnVoid);

    Disassembler disasm;
    std::string output = disasm.disassembleFunction(func);

    std::cout << "=== Output ===\n";
    std::cout << output;
    std::cout << "\n=== End ===\n";

    std::cout << "Looking for 'ReturnVoid': " << (output.find("ReturnVoid") != std::string::npos ? "FOUND" : "NOT FOUND") << "\n";
    std::cout << "Looking for 'test_func': " << (output.find("test_func") != std::string::npos ? "FOUND" : "NOT FOUND") << "\n";

    return 0;
}
