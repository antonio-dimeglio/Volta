#pragma once

#include "MIR/MIR.hpp"
#include "Error/Error.hpp"
#include <unordered_set>
#include <string>

namespace MIR {
// Validates MIR correctness after construction.
// Catches common errors before codegen
class MIRVerifier {
private:
    DiagnosticManager& diag;
    bool hasErrors = false;

    // Track defined values per function
    std::unordered_set<std::string> definedValues;

    // Track block labels per function
    std::unordered_set<std::string> blockLabels;

public:
    explicit MIRVerifier(DiagnosticManager& d) : diag(d) {}

    // Verify entire program
    // Returns: true if valid, false if errors found
    bool verify(const Program& program);

    // Verify a single function
    bool verifyFunction(const Function& function);

    // Verify a basic block
    bool verifyBasicBlock(const BasicBlock& block);

    // Verify an instruction
    bool verifyInstruction(const Instruction& inst);

    // Verify a terminator
    bool verifyTerminator(const Terminator& term);

private:
    void error(const std::string& message);
    void warning(const std::string& message);

};

} // namespace MIR
