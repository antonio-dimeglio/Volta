#pragma once

#include "IR.hpp"
#include "IRModule.hpp"
#include <ostream>
#include <string>

namespace volta::ir {

/**
 * IRPrinter - Pretty-print IR in human-readable format
 *
 * Outputs IR in a format similar to LLVM IR:
 *
 * function @factorial(%n: int) -> int:
 *   bb0:
 *     %0 = le %n, 1
 *     br_if %0, bb1, bb2
 *
 *   bb1:
 *     ret 1
 *
 *   bb2:
 *     %1 = sub %n, 1
 *     %2 = call @factorial, %1
 *     %3 = mul %n, %2
 *     ret %3
 */
class IRPrinter {
public:
    explicit IRPrinter(std::ostream& out) : out_(out), indentLevel_(0) {}

    /**
     * Print entire module
     */
    void print(const IRModule& module);

    /**
     * Print a function
     */
    void printFunction(const Function& function);

    /**
     * Print a basic block
     */
    void printBasicBlock(const BasicBlock& block);

    /**
     * Print an instruction
     */
    void printInstruction(const Instruction& inst);

    /**
     * Print a global variable
     */
    void printGlobal(const GlobalVariable& global);

    /**
     * Print a value (for use in operands)
     */
    std::string valueToString(const Value* value);

    /**
     * Print a type
     */
    std::string typeToString(const semantic::Type* type);

private:
    std::ostream& out_;
    int indentLevel_;

    void indent();
    void increaseIndent() { indentLevel_++; }
    void decreaseIndent() { indentLevel_--; }
};

} // namespace volta::ir
