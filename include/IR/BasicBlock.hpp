#pragma once

#include "Instruction.hpp"
#include <memory>
#include <vector>
#include <string>

namespace volta::ir {

// Forward declarations
class Function;

// ============================================================================
// BasicBlock - A sequence of instructions with single entry/exit
// ============================================================================

/**
 * BasicBlock represents a straight-line sequence of instructions.
 *
 * Properties of a BasicBlock:
 * 1. Single entry point (only the first instruction can be jumped to)
 * 2. Single exit point (only the last instruction can jump elsewhere)
 * 3. No branches in the middle (except the terminator at the end)
 *
 * Control Flow Graph (CFG):
 * - BasicBlocks are nodes
 * - Branches are edges
 * - Every path through a function is a sequence of BasicBlocks
 *
 * Example Volta code:
 *   fn test(x: int) -> int {
 *       if x > 10 {
 *           return x * 2
 *       } else {
 *           return x + 5
 *       }
 *   }
 *
 * IR with BasicBlocks:
 *   entry:                      <- BasicBlock 1
 *       %cond = icmp gt %x, 10
 *       br i1 %cond, label %then, label %else
 *
 *   then:                       <- BasicBlock 2
 *       %mul = mul i64 %x, 2
 *       ret i64 %mul
 *
 *   else:                       <- BasicBlock 3
 *       %add = add i64 %x, 5
 *       ret i64 %add
 *
 * CFG structure:
 *   entry -> then
 *   entry -> else
 */
class BasicBlock {
public:
    explicit BasicBlock(const std::string& name, Function* parent = nullptr);
    ~BasicBlock();

    // Identity
    const std::string& getName() const { return name_; }
    Function* getParent() const { return parent_; }
    void setParent(Function* parent) { parent_ = parent; }

    // Instruction management
    void addInstruction(std::unique_ptr<Instruction> inst);
    void insertInstruction(size_t index, std::unique_ptr<Instruction> inst);
    void removeInstruction(Instruction* inst);

    const std::vector<std::unique_ptr<Instruction>>& getInstructions() const {
        return instructions_;
    }

    size_t size() const { return instructions_.size(); }
    bool empty() const { return instructions_.empty(); }

    // Get first/last instruction
    Instruction* getFirstInstruction() const;
    Instruction* getTerminator() const;  // Last instruction (must be Br/CondBr/Ret)

    bool hasTerminator() const;

    // CFG - Predecessor/Successor relationships
    void addPredecessor(BasicBlock* pred);
    void removePredecessor(BasicBlock* pred);
    const std::vector<BasicBlock*>& getPredecessors() const { return predecessors_; }

    void addSuccessor(BasicBlock* succ);
    void removeSuccessor(BasicBlock* succ);
    const std::vector<BasicBlock*>& getSuccessors() const { return successors_; }

    // CFG queries
    bool hasSinglePredecessor() const { return predecessors_.size() == 1; }
    bool hasSingleSuccessor() const { return successors_.size() == 1; }

    BasicBlock* getSinglePredecessor() const;
    BasicBlock* getSingleSuccessor() const;

    // SSA - Dominance (for phi node placement)
    // "Block A dominates Block B" means: every path to B must go through A
    bool dominates(const BasicBlock* other) const;
    void setImmediateDominator(BasicBlock* dom) { immediateDominator_ = dom; }
    BasicBlock* getImmediateDominator() const { return immediateDominator_; }

    const std::vector<BasicBlock*>& getDominanceFrontier() const {
        return dominanceFrontier_;
    }
    void addToDominanceFrontier(BasicBlock* block);

    // For printing/debugging
    std::string toString() const;

private:
    std::string name_;
    Function* parent_;

    // Instructions in this block (sequential)
    std::vector<std::unique_ptr<Instruction>> instructions_;

    // CFG edges
    std::vector<BasicBlock*> predecessors_;  // Blocks that jump to this block
    std::vector<BasicBlock*> successors_;    // Blocks this block jumps to

    // Dominance information (computed by analysis passes)
    BasicBlock* immediateDominator_;              // Closest dominator
    std::vector<BasicBlock*> dominanceFrontier_;  // Where to place PHI nodes
};

} // namespace volta::ir
