#pragma once

#include <vector>
#include <string>
#include <memory>
#include "IR/Instruction.hpp"
#include "IR/Value.hpp"

namespace volta::ir {

// Forward declarations
class Function;
class Module;

namespace cfg {
    void connectBlocks(Module& module, BasicBlock* from, BasicBlock* to);
    void connectBlocksConditional(Module& module, BasicBlock* from, Value* condition,
                                  BasicBlock* trueBlock, BasicBlock* falseBlock);
}

/**
 * BasicBlock - A sequence of instructions with a single entry and single exit
 *
 * Design Philosophy:
 * - Control Flow Graph (CFG): Basic blocks form nodes in the CFG
 * - Single Entry: Control enters only at the first instruction
 * - Single Exit: Control leaves only via terminator (last instruction)
 * - SSA Form: Each block can have phi nodes at the beginning
 * - Predecessor/Successor tracking: Maintain CFG edges explicitly
 *
 * Key Concepts:
 * - Instructions: Owned list of instructions in this block
 * - Terminator: Last instruction (Ret, Br, CondBr, Switch)
 * - Predecessors: Blocks that can jump to this block
 * - Successors: Blocks this block can jump to
 * - Parent Function: The function containing this block
 *
 * SSA Properties:
 * - Phi nodes appear at the beginning of blocks with multiple predecessors
 * - Each phi merges values from different incoming edges
 *
 * Learning Goals:
 * - Understand CFG construction and navigation
 * - Practice intrusive linked list patterns (instruction list management)
 * - Implement bidirectional graph edges (predecessor/successor tracking)
 * - Handle terminator instruction constraints
 */
class BasicBlock : public Value {
    friend class Arena;
    friend class Module;
public:

    // ========================================================================
    // Instruction Management
    // ========================================================================

    /**
     * Get all instructions in this block (includes terminators)
     * Instructions are ordered from first to last
     */
    const std::vector<Instruction*>& getInstructions() const { return instructions_; }

    /**
     * Get number of instructions (including terminator)
     */
    size_t getNumInstructions() const { return instructions_.size(); }

    /**
     * Check if block is empty (has no instructions)
     */
    bool empty() const { return instructions_.empty(); }

    /**
     * Get the first instruction in the block (or nullptr if empty)
     * This is often a phi node in blocks with multiple predecessors
     */
    Instruction* getFirstInstruction() const;

    /**
     * Get the terminator instruction (last instruction)
     * Returns nullptr if block has no terminator yet
     */
    Instruction* getTerminator() const;

    /**
     * Check if this block has a terminator
     */
    bool hasTerminator() const;

    /**
     * Add an instruction to the end of this block
     * @param inst The instruction to add
     *
     * IMPORTANT: You can only add ONE terminator, and it must be last.
     * Adding a terminator when one exists should fail (or replace).
     * Adding a non-terminator after a terminator should fail.
     */
    void addInstruction(Instruction* inst);

    /**
     * Insert an instruction before another instruction
     * @param inst The instruction to insert
     * @param before The instruction to insert before (must be in this block)
     *
     * LEARNING TIP: This requires finding 'before' in the instructions_ vector
     * and inserting 'inst' at that position. Don't forget to set inst's parent!
     */
    void insertBefore(Instruction* inst, Instruction* before);

    /**
     * Remove an instruction from this block
     * @param inst The instruction to remove
     *
     * IMPORTANT: Does NOT delete the instruction. Caller owns the pointer.
     * Updates the instruction's parent to nullptr.
     *
     * LEARNING TIP: Use std::find + std::vector::erase
     */
    void removeInstruction(Instruction* inst);

    /**
     * Erase an instruction from this block (removes AND deletes)
     * @param inst The instruction to erase
     */
    void eraseInstruction(Instruction* inst);

    // ========================================================================
    // CFG Navigation (Control Flow Graph)
    // ========================================================================

    /**
     * Get the parent function containing this block
     */
    Function* getParent() const { return parent_; }

    /**
     * Set the parent function
     * LEARNING TIP: This is typically called by Function::addBasicBlock
     */
    void setParent(Function* parent) { parent_ = parent; }

    /**
     * Get all predecessor blocks (blocks that can jump to this block)
     */
    const std::vector<BasicBlock*>& getPredecessors() const { return predecessors_; }

    /**
     * Get all successor blocks (blocks this block can jump to)
     * LEARNING TIP: Successors are determined by the terminator instruction:
     * - Ret: no successors
     * - Br: one successor
     * - CondBr: two successors (true and false)
     * - Switch: multiple successors
     */
    std::vector<BasicBlock*> getSuccessors() const;

    /**
     * Get number of predecessors
     */
    size_t getNumPredecessors() const { return predecessors_.size(); }

    /**
     * Get number of successors
     */
    size_t getNumSuccessors() const;

    /**
     * Check if this block has multiple predecessors
     * LEARNING TIP: Blocks with multiple predecessors need phi nodes!
     */
    bool hasMultiplePredecessors() const { return predecessors_.size() > 1; }

    /**
     * Remove a predecessor block
     * LEARNING TIP: Use std::find + std::vector::erase
     */
    void removePredecessor(BasicBlock* pred);

    /**
     * Check if a block is a predecessor of this block
     */
    bool isPredecessor(BasicBlock* block) const;

    /**
     * Check if a block is a successor of this block
     */
    bool isSuccessor(BasicBlock* block) const;

    // ========================================================================
    // CFG Utilities
    // ========================================================================

    /**
     * Split this block at the given instruction
     * Creates a new block containing [splitPoint, end) and returns it
     * This block keeps [begin, splitPoint)
     *
     * Example:
     *   Before:  BB1: [inst1, inst2, inst3, terminator]
     *   After:   BB1: [inst1, inst2, br new_block]
     *            new_block: [inst3, terminator]
     *
     * @param module Module for arena allocation
     * @param splitPoint The first instruction that goes to the new block
     * @param newBlockName Name for the new block
     * @return The newly created block
     *
     * LEARNING TIPS:
     * 1. Create new block with newBlockName
     * 2. Move instructions [splitPoint, end) to new block
     * 3. Update predecessor/successor relationships:
     *    - Old successors become new block's successors
     *    - This block's only successor is new block
     *    - New block's only predecessor is this block
     * 4. Add unconditional branch from this block to new block
     * 5. Update phi nodes in old successors (change this -> new block)
     */
    BasicBlock* splitAt(Module& module, Instruction* splitPoint, const std::string& newBlockName = "");

    /**
     * Replace all uses of this block (in terminators and phis) with another block
     * LEARNING TIP: This is like Value::replaceAllUsesWith but for CFG edges
     */
    void replaceAllUsesWith(BasicBlock* newBlock);

    /**
     * Check if this block is reachable from entry (has predecessors or is entry)
     */
    bool isReachable() const;

    // ========================================================================
    // Phi Node Utilities
    // ========================================================================

    /**
     * Get all phi nodes in this block
     * LEARNING TIP: Phi nodes are always at the beginning of the block
     */
    std::vector<PhiNode*> getPhiNodes() const;

    /**
     * Check if this block has any phi nodes
     */
    bool hasPhiNodes() const;

    /**
     * Add a phi node at the beginning of the block
     * LEARNING TIP: Phi nodes must come before non-phi instructions
     */
    void addPhiNode(PhiNode* phi);

    // ========================================================================
    // Pretty Printing
    // ========================================================================

    /**
     * Convert to string representation
     * Format:
     *   label:                          ; preds = %pred1, %pred2
     *     %1 = phi i64 [%a, %pred1], [%b, %pred2]
     *     %2 = add i64 %1, 42
     *     br %next_block
     */
    std::string toString() const override;

    /**
     * Print with detailed CFG information
     */
    std::string toStringDetailed() const;

    // ========================================================================
    // LLVM-style RTTI
    // ========================================================================

    static bool classof(const Value* V) {
        return V->getKind() == ValueKind::BasicBlock;
    }

private:
    BasicBlock(const std::string& name, Function* parent);

    /**
     * Add a predecessor block
     * PRIVATE: Only callable by terminator instructions to maintain CFG consistency
     * This ensures that predecessor lists are always in sync with terminator destinations
     */
    void addPredecessor(BasicBlock* pred);

    // Allow terminators to manage CFG edges
    friend class BranchInst;
    friend class CondBranchInst;
    friend class SwitchInst;

    // Allow cfg utility functions to manage edges
    friend void cfg::connectBlocks(Module& module, BasicBlock* from, BasicBlock* to);
    friend void cfg::connectBlocksConditional(Module& module, BasicBlock* from, Value* condition,
                                             BasicBlock* trueBlock, BasicBlock* falseBlock);

    // Instruction list (owned pointers)
    std::vector<Instruction*> instructions_;

    // CFG edges
    Function* parent_;
    std::vector<BasicBlock*> predecessors_;  // Blocks that jump to this block

    // LEARNING NOTE: Successors are NOT stored explicitly!
    // They are computed on-demand from the terminator instruction.
    // This avoids redundancy and keeps the CFG consistent.
};

/**
 * CFG Builder Utilities
 *
 * These helper functions make CFG construction easier and less error-prone.
 */
namespace cfg {

/**
 * Connect two basic blocks with a branch
 * Creates a BranchInst in 'from' targeting 'to'
 * Updates predecessor/successor relationships
 *
 * LEARNING TIP: This is safer than manually creating branches
 * because it updates the CFG edges automatically.
 */
void connectBlocks(Module& module, BasicBlock* from, BasicBlock* to);

/**
 * Connect with a conditional branch
 * Creates a CondBranchInst in 'from'
 * Updates CFG edges for both true and false branches
 */
void connectBlocksConditional(Module& module, BasicBlock* from, Value* condition,
                              BasicBlock* trueBlock, BasicBlock* falseBlock);

/**
 * Remove a block from the CFG
 * - Removes all incoming edges (from predecessors)
 * - Removes all outgoing edges (to successors)
 * - Does NOT delete the block (caller owns it)
 *
 * LEARNING TIP: Must update predecessor lists in successors
 * and terminator instructions in predecessors.
 */
void removeBlockFromCFG(BasicBlock* block);

/**
 * Check if there's a path from 'from' to 'to' in the CFG
 * Uses BFS/DFS traversal
 *
 * LEARNING TIP: This is useful for dominance analysis
 */
bool hasPath(BasicBlock* from, BasicBlock* to);

/**
 * Get all blocks reachable from a given block
 * Returns blocks in DFS order
 */
std::vector<BasicBlock*> getReachableBlocks(BasicBlock* start);

/**
 * Compute reverse postorder traversal
 * LEARNING TIP: Reverse postorder is often used for dataflow analysis
 * because it processes predecessors before successors
 */
std::vector<BasicBlock*> reversePostorder(BasicBlock* entry);

} // namespace cfg

} // namespace volta::ir
