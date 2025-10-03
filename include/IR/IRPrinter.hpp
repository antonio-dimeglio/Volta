#pragma once

#include "IRModule.hpp"
#include <ostream>
#include <string>

namespace volta::ir {

// ============================================================================
// IRPrinter - Pretty-prints IR in human-readable format
// ============================================================================

/**
 * IRPrinter formats IR for debugging and visualization.
 *
 * Output format is similar to LLVM IR:
 *   ; ModuleID = 'test.volta'
 *
 *   @global = global i64 42
 *
 *   define i64 @add(i64 %a, i64 %b) {
 *   entry:
 *       %0 = add i64 %a, %b
 *       ret i64 %0
 *   }
 *
 * Usage:
 *   IRPrinter printer;
 *   printer.print(module, std::cout);
 */
class IRPrinter {
public:
    IRPrinter();

    /**
     * Print entire module to output stream.
     */
    void print(const IRModule& module, std::ostream& out);

    /**
     * Print single function.
     */
    void printFunction(const Function& function, std::ostream& out);

    /**
     * Print single basic block.
     */
    void printBasicBlock(const BasicBlock& block, std::ostream& out);

    /**
     * Print single instruction.
     */
    void printInstruction(const Instruction& instruction, std::ostream& out);

    /**
     * Configuration: indent level (for pretty-printing).
     */
    void setIndentLevel(int level) { indentLevel_ = level; }

private:
    int indentLevel_;

    // Helper: Get indent string
    std::string getIndent() const;
};

} // namespace volta::ir
