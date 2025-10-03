#include "IR/IRPrinter.hpp"
#include <sstream>

namespace volta::ir {

IRPrinter::IRPrinter() : indentLevel_(0) {
}

// ============================================================================
// Module Printing
// ============================================================================

void IRPrinter::print(const IRModule& module, std::ostream& out) {
    // TODO: Print the entire module.
    // Format:
    //   ; ModuleID = 'module_name'
    //
    //   [global variables]
    //
    //   [functions]
    //
    // You can use module.toString() or manually iterate and format.
}

// ============================================================================
// Function Printing
// ============================================================================

void IRPrinter::printFunction(const Function& function, std::ostream& out) {
    // TODO: Print a function.
    // Format:
    //   define returnType @functionName(paramType %param1, ...) {
    //       [basic blocks]
    //   }
    //
    // Hint: Loop through function.getBasicBlocks() and call printBasicBlock()
}

// ============================================================================
// BasicBlock Printing
// ============================================================================

void IRPrinter::printBasicBlock(const BasicBlock& block, std::ostream& out) {
    // TODO: Print a basic block.
    // Format:
    //   label_name:
    //       instruction1
    //       instruction2
    //       ...
    //
    // Use getIndent() for proper indentation.
}

// ============================================================================
// Instruction Printing
// ============================================================================

void IRPrinter::printInstruction(const Instruction& instruction, std::ostream& out) {
    // TODO: Print an instruction.
    // You can use instruction.toString() or format manually.
    // Add proper indentation.
}

// ============================================================================
// Helpers
// ============================================================================

std::string IRPrinter::getIndent() const {
    // TODO: Return indent string based on indentLevel_.
    // Example: indentLevel_ = 2 -> "        " (4 spaces per level)
    return "";
}

} // namespace volta::ir
