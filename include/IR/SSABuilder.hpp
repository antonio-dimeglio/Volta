#pragma once

#include "Function.hpp"
#include "BasicBlock.hpp"
#include "Instruction.hpp"
#include "Value.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace volta::ir {

// ============================================================================
// SSABuilder - Automatic SSA Form Construction
// ============================================================================

/**
 * SSABuilder automates the construction of SSA form.
 *
 * This is Phase 3 - THE HEART OF IR GENERATION!
 *
 * Problem: In Volta, variables can be assigned multiple times:
 *   x: mut int = 10
 *   if condition {
 *       x = 20
 *   } else {
 *       x = 30
 *   }
 *   print(x)  <- Which value of x?
 *
 * SSA Solution: Each assignment creates a new SSA value:
 *   %x.0 = 10
 *   br %cond, %then, %else
 * then:
 *   %x.1 = 20
 *   br %merge
 * else:
 *   %x.2 = 30
 *   br %merge
 * merge:
 *   %x.3 = phi [%x.1, %then], [%x.2, %else]  <- PHI merges values!
 *   call @print(%x.3)
 *
 * SSABuilder handles:
 * 1. Variable versioning (x -> x.0, x.1, x.2)
 * 2. Phi node placement (where do we need phi nodes?)
 * 3. Phi node population (which values come from which blocks?)
 *
 * Algorithm: Braun et al. "Simple and Efficient Construction of SSA Form"
 * Paper: https://pp.info.uni-karlsruhe.de/uploads/publikationen/braun13cc.pdf
 *
 * Usage:
 *   SSABuilder ssa;
 *   ssa.setCurrentBlock(entryBlock);
 *
 *   // Declare variable
 *   ssa.declareVariable("x", entryBlock);
 *
 *   // Write to variable
 *   Value* val = builder.createAdd(...);
 *   ssa.writeVariable("x", currentBlock, val);
 *
 *   // Read from variable (automatically inserts phi if needed!)
 *   Value* x = ssa.readVariable("x", currentBlock);
 */
class SSABuilder {
public:
    SSABuilder();

    // ========== Variable Declaration ==========

    /**
     * Declare a variable (allocate tracking structures).
     * Called when a variable is first introduced in scope.
     */
    void declareVariable(const std::string& name, BasicBlock* block);

    // ========== Variable Read/Write ==========

    /**
     * Write a value to a variable in the current block.
     * Creates a new SSA version.
     *
     * Example:
     *   Value* ten = Constant::getInt(10);
     *   ssa.writeVariable("x", currentBlock, ten);
     *   // Now x.0 = 10
     */
    void writeVariable(const std::string& name, BasicBlock* block, Value* value);

    /**
     * Read a variable's current value in the given block.
     * Automatically inserts PHI nodes if needed!
     *
     * Example:
     *   Value* x = ssa.readVariable("x", currentBlock);
     *   // Returns %x.0, or creates PHI if multiple definitions reach here
     */
    Value* readVariable(const std::string& name, BasicBlock* block);

    // ========== PHI Node Management ==========

    /**
     * Seal a block (indicate that no more predecessors will be added).
     * This triggers phi node completion for any incomplete phis.
     *
     * Call this after all predecessors of a block are known.
     */
    void sealBlock(BasicBlock* block);

    /**
     * Check if a block is sealed.
     */
    bool isSealed(BasicBlock* block) const;

    // ========== Dominance Analysis ==========

    /**
     * Compute dominance information for a function.
     * Required for phi node placement.
     *
     * Call this once per function before generating IR body.
     */
    void computeDominance(Function* function);

    /**
     * Compute dominance frontiers.
     * Tells us where to place phi nodes.
     */
    void computeDominanceFrontiers(Function* function);

private:
    // ========== Internal: SSA Construction Algorithm ==========

    /**
     * Read variable recursively.
     * If value is not found in current block, search predecessors
     * and create phi nodes as needed.
     */
    Value* readVariableRecursive(const std::string& name, BasicBlock* block);

    /**
     * Add phi operands after phi node is created.
     * Recursively reads variable from all predecessor blocks.
     */
    Value* addPhiOperands(const std::string& name, PhiInstruction* phi);

    /**
     * Try to remove trivial phi nodes (optimization).
     * A phi is trivial if all inputs are the same value.
     */
    Value* tryRemoveTrivialPhi(PhiInstruction* phi);

    // ========== Internal: Dominance Helpers ==========

    /**
     * Compute immediate dominators using Lengauer-Tarjan algorithm.
     */
    void computeImmediateDominators(Function* function);

    /**
     * Check if block1 dominates block2.
     */
    bool dominates(BasicBlock* block1, BasicBlock* block2) const;

private:
    // Variable definitions: maps (block, variable name) -> SSA value
    // This tracks which value a variable has in each block
    std::unordered_map<BasicBlock*, std::unordered_map<std::string, Value*>> currentDef_;

    // Incomplete phi nodes: created before all operands are known
    // Maps (block, variable name) -> phi instruction
    std::unordered_map<BasicBlock*, std::unordered_map<std::string, PhiInstruction*>> incompletePhis_;

    // Sealed blocks: set of blocks where all predecessors are known
    std::unordered_set<BasicBlock*> sealedBlocks_;

    // Dominance information (computed once per function)
    std::unordered_map<BasicBlock*, BasicBlock*> immediateDominators_;
    std::unordered_map<BasicBlock*, std::vector<BasicBlock*>> dominanceFrontiers_;
};

} // namespace volta::ir
