#pragma once

#include <string>
#include <sstream>
#include <unordered_map>
#include "IR/Module.hpp"
#include "IR/Function.hpp"
#include "IR/BasicBlock.hpp"
#include "IR/Instruction.hpp"
#include "IR/Value.hpp"

namespace volta::ir {

/**
 * IRPrinter - Converts IR to human-readable text
 *
 * Design Philosophy:
 * - LLVM-like syntax: Readable, unambiguous
 * - SSA numbering: Automatic %0, %1, %2, etc.
 * - Type annotations: Show types for clarity
 * - CFG visualization: Show predecessors
 *
 * Key Concepts:
 * - Value naming: Give each unnamed value a unique %N identifier
 * - Block labels: Show basic block names and predecessors
 * - Type signatures: Show function signatures clearly
 * - Indentation: Proper formatting for readability
 *
 * Output Format:
 *   ; Module: my_program
 *
 *   @global: i64 = 42
 *
 *   function @add(x: i64, y: i64) -> i64 {
 *   entry:                           ; preds =
 *     %0 = add i64 %x, %y
 *     ret i64 %0
 *   }
 *
 * Learning Goals:
 * - Understand IR structure deeply
 * - Practice visitor pattern
 * - Learn SSA naming conventions
 * - Debug your IR generator output
 */
class IRPrinter {
public:
    /**
     * Create IR printer
     */
    IRPrinter();

    // ========================================================================
    // Top-Level Printing
    // ========================================================================

    /**
     * Print entire module
     * @param module The module to print
     * @return String representation
     */
    std::string printModule(const Module& module);

    /**
     * Print a function
     * @param func The function to print
     * @return String representation
     */
    std::string printFunction(const Function* func);

    /**
     * Print a basic block
     * @param block The block to print
     * @return String representation
     */
    std::string printBasicBlock(const BasicBlock* block);

    /**
     * Print an instruction
     * @param inst The instruction to print
     * @return String representation
     */
    std::string printInstruction(const Instruction* inst);

    /**
     * Print a value
     * @param value The value to print
     * @return String representation (e.g., "%0", "@global", "42")
     *
     * LEARNING TIP: Values can be:
     * - Constants: Print literal value (42, 3.14, true)
     * - Arguments: Print argument name (%x, %y)
     * - Instructions: Print SSA number (%0, %1)
     * - Globals: Print global name (@counter)
     */
    std::string printValue(const Value* value);

    /**
     * Print a type
     * @param type The type to print
     * @return String representation (i64, f64, ptr<i64>, etc.)
     */
    std::string printType(const std::shared_ptr<IRType>& type);

    // ========================================================================
    // Control
    // ========================================================================

    /**
     * Enable/disable type annotations
     * @param enable If true, show types in output
     *
     * Example with types:    %0 = add i64 %x, %y
     * Example without types: %0 = add %x, %y
     */
    void setShowTypes(bool enable) { showTypes_ = enable; }

    /**
     * Enable/disable CFG information (predecessors)
     * @param enable If true, show predecessors for each block
     */
    void setShowCFG(bool enable) { showCFG_ = enable; }

    /**
     * Enable/disable use-def information
     * @param enable If true, show number of uses for each value
     */
    void setShowUses(bool enable) { showUses_ = enable; }

    /**
     * Reset printer state (clears value numbering)
     * Call this before printing a new function
     */
    void reset();

private:
    // ========================================================================
    // Helper Methods
    // ========================================================================

    /**
     * Get or assign SSA number for a value
     * @param value The value
     * @return SSA string (e.g., "%0", "%1")
     *
     * LEARNING TIP: This maintains a map of Value* → number.
     * Each unnamed value gets a unique sequential number.
     */
    std::string getValueName(const Value* value);

    /**
     * Print global variable
     */
    std::string printGlobal(const GlobalVariable* global);

    /**
     * Print function signature
     * Example: function @add(x: i64, y: i64) -> i64
     */
    std::string printFunctionSignature(const Function* func);

    /**
     * Print instruction operands
     * Helper for printing instructions
     */
    std::string printOperands(const Instruction* inst);

    /**
     * Print phi node (special formatting)
     * Example: %0 = phi i64 [%a, %entry], [%b, %loop]
     */
    std::string printPhi(const PhiNode* phi);

    /**
     * Print call instruction (special formatting)
     * Example: %0 = call @func(i64 %x, i64 %y)
     */
    std::string printCall(const CallInst* call);

    /**
     * Print branch instruction
     * Example: br label %block
     */
    std::string printBranch(const Instruction* inst);

    /**
     * Print return instruction
     * Example: ret i64 %0 or ret void
     */
    std::string printReturn(const ReturnInst* ret);

    /**
     * Get indentation string
     * @param level Indentation level
     * @return Spaces for indentation
     */
    std::string indent(int level = 1) const;

    // ========================================================================
    // State
    // ========================================================================

    // SSA value numbering
    std::unordered_map<const Value*, int> valueNumbers_;
    int nextValueNumber_;

    // Printing options
    bool showTypes_;     // Show type annotations
    bool showCFG_;       // Show CFG information (predecessors)
    bool showUses_;      // Show use counts

    // Output stream
    std::stringstream output_;
};

} // namespace volta::ir
