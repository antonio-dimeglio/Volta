#pragma once

#include <string>
#include <vector>
#include "IR/Module.hpp"
#include "IR/Function.hpp"
#include "IR/BasicBlock.hpp"
#include "IR/Instruction.hpp"

namespace volta::ir {

/**
 * Verifier - Validates IR correctness
 *
 * Design Philosophy:
 * - Defensive programming: Catch bugs early
 * - Comprehensive checks: Verify all invariants
 * - Clear error messages: Help debug issues
 * - Fast failure: Stop at first error
 *
 * What We Verify:
 * 1. **SSA Form**: Each value defined once
 * 2. **Type Safety**: Operations match types
 * 3. **CFG Consistency**: Edges are bidirectional
 * 4. **Dominance**: Uses dominated by definitions
 * 5. **Terminator Rules**: Blocks have exactly one terminator at end
 * 6. **Phi Placement**: Phi nodes at block start
 * 7. **Return Types**: Match function signature
 *
 * Usage:
 *   Verifier verifier;
 *   if (!verifier.verify(module)) {
 *     for (auto& error : verifier.getErrors()) {
 *       std::cerr << error << std::endl;
 *     }
 *   }
 *
 * Learning Goals:
 * - Understand IR invariants deeply
 * - Practice defensive programming
 * - Learn CFG and dominance concepts
 */
class Verifier {
public:
    Verifier();

    // ========================================================================
    // Verification Entry Points
    // ========================================================================

    /**
     * Verify entire module
     * @param module The module to verify
     * @return true if valid, false if errors found
     */
    bool verify(const Module& module);

    /**
     * Verify a single function
     * @param func The function to verify
     * @return true if valid
     */
    bool verifyFunction(const Function* func);

    /**
     * Verify a basic block
     * @param block The block to verify
     * @return true if valid
     */
    bool verifyBasicBlock(const BasicBlock* block);

    /**
     * Verify an instruction
     * @param inst The instruction to verify
     * @return true if valid
     */
    bool verifyInstruction(const Instruction* inst);

    // ========================================================================
    // Error Reporting
    // ========================================================================

    /**
     * Get all error messages
     */
    const std::vector<std::string>& getErrors() const { return errors_; }

    /**
     * Check if any errors occurred
     */
    bool hasErrors() const { return !errors_.empty(); }

    /**
     * Clear errors (for reuse)
     */
    void clearErrors() { errors_.clear(); }

private:
    // ========================================================================
    // Verification Helpers
    // ========================================================================

    /**
     * Report verification error
     */
    void reportError(const std::string& message);

    /**
     * Verify CFG consistency
     * Check that predecessors/successors are bidirectional
     */
    bool verifyCFG(const Function* func);

    /**
     * Verify terminator placement
     * Each block must have exactly one terminator at the end
     */
    bool verifyTerminators(const Function* func);

    /**
     * Verify phi node placement
     * Phi nodes must be at the beginning of blocks
     */
    bool verifyPhiNodes(const BasicBlock* block);

    /**
     * Verify return statements match function signature
     */
    bool verifyReturns(const Function* func);

    /**
     * Verify instruction types are correct
     */
    bool verifyInstructionTypes(const Instruction* inst);

    /**
     * Verify use-def chains
     * All uses must be reachable from definitions
     */
    bool verifyUseDefChains(const Function* func);

    /**
     * Verify function parameters match calls
     */
    bool verifyCallSites(const Function* func);

    // State
    std::vector<std::string> errors_;
};

} // namespace volta::ir
