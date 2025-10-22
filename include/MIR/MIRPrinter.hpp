#pragma once

#include "MIR/MIR.hpp"
#include <ostream>
#include <string>

namespace MIR {
// Provides human-readable textual output for MIR.
// Useful for debugging and visualization.
class MIRPrinter {
private:
    std::ostream& out;
    int indentLevel = 0;

    // Helper for indentation
    void indent();
    void increaseIndent();
    void decreaseIndent();

public:
    explicit MIRPrinter(std::ostream& os) : out(os) {}

    // Print entire program
    void print(const Program& program);

    // Print a single function
    void printFunction(const Function& function);

    // Print a basic block
    void printBasicBlock(const BasicBlock& block);

    // Print an instruction
    void printInstruction(const Instruction& inst);

    // Print a terminator
    void printTerminator(const Terminator& term);

    // Print a value (with % or @ prefix)
    void printValue(const Value& value);

    // Print a type
    void printType(const Type::Type* type);

    // Print an opcode name
    [[nodiscard]] std::string opcodeToString(Opcode opcode) const;

    // Print a terminator kind name
    [[nodiscard]] std::string terminatorKindToString(TerminatorKind kind) const;
};

} // namespace MIR
