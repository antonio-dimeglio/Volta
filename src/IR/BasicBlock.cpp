#include "IR/BasicBlock.hpp"
#include "IR/Function.hpp"
#include <algorithm>
#include <sstream>

namespace volta::ir {

BasicBlock::BasicBlock(const std::string& name, Function* parent)
    : name_(name), parent_(parent), immediateDominator_(nullptr) {
}

BasicBlock::~BasicBlock() = default;

// ============================================================================
// Instruction Management
// ============================================================================

void BasicBlock::addInstruction(std::unique_ptr<Instruction> inst) {
    // TODO: Add the instruction to the end of instructions_ vector.
    // Set the instruction's parent to this block (inst->setParent(this))
}

void BasicBlock::insertInstruction(size_t index, std::unique_ptr<Instruction> inst) {
    // TODO: Insert the instruction at the given index in instructions_.
    // Set the instruction's parent to this block.
    // Hint: Use vector::insert()
}

void BasicBlock::removeInstruction(Instruction* inst) {
    // TODO: Find and remove the instruction from instructions_.
    // Remember to clear the instruction's parent (inst->setParent(nullptr))
    // Hint: Use std::remove_if with std::unique_ptr
}

Instruction* BasicBlock::getFirstInstruction() const {
    // TODO: Return the first instruction in the block, or nullptr if empty
    return nullptr;
}

Instruction* BasicBlock::getTerminator() const {
    // TODO: Return the last instruction in the block (should be a terminator: Br/CondBr/Ret)
    // Return nullptr if the block is empty or last instruction is not a terminator
    return nullptr;
}

bool BasicBlock::hasTerminator() const {
    // TODO: Return true if the block has a terminator instruction at the end
    // Hint: Use getTerminator() and check if it's not nullptr
    return false;
}

// ============================================================================
// CFG - Predecessor/Successor Management
// ============================================================================

void BasicBlock::addPredecessor(BasicBlock* pred) {
    // TODO: Add pred to the predecessors_ vector (if not already present)
    // Hint: Check for duplicates before adding
}

void BasicBlock::removePredecessor(BasicBlock* pred) {
    // TODO: Remove pred from the predecessors_ vector
    // Hint: Use std::remove + erase
}

void BasicBlock::addSuccessor(BasicBlock* succ) {
    // TODO: Add succ to the successors_ vector (if not already present)
    // This should be called when adding a branch instruction
}

void BasicBlock::removeSuccessor(BasicBlock* succ) {
    // TODO: Remove succ from the successors_ vector
}

BasicBlock* BasicBlock::getSinglePredecessor() const {
    // TODO: Return the single predecessor if there's exactly one, nullptr otherwise
    return nullptr;
}

BasicBlock* BasicBlock::getSingleSuccessor() const {
    // TODO: Return the single successor if there's exactly one, nullptr otherwise
    return nullptr;
}

// ============================================================================
// Dominance Analysis (for SSA construction)
// ============================================================================

bool BasicBlock::dominates(const BasicBlock* other) const {
    // TODO: Check if this block dominates 'other'.
    // A block dominates another if every path to 'other' must go through this block.
    //
    // Simple algorithm:
    // - Walk up the dominator tree from 'other' to the entry block
    // - If we encounter 'this', then 'this' dominates 'other'
    //
    // Hint: Follow the immediateDominator_ chain from 'other'
    return false;
}

void BasicBlock::addToDominanceFrontier(BasicBlock* block) {
    // TODO: Add block to dominanceFrontier_ (if not already present)
    // The dominance frontier is where PHI nodes need to be placed.
    //
    // Definition: Block X is in the dominance frontier of block A if:
    // - A dominates a predecessor of X
    // - A does not strictly dominate X
    //
    // This is computed by a separate analysis pass, you just need to store it here.
}

// ============================================================================
// Printing/Debugging
// ============================================================================

std::string BasicBlock::toString() const {
    // TODO: Return a string representation of this basic block.
    // Format:
    //   label_name:
    //       instruction1
    //       instruction2
    //       ...
    //
    // Example:
    //   entry:
    //       %0 = add i64 %a, %b
    //       ret i64 %0
    //
    // Hint: Loop through instructions_ and call inst->toString() on each
    return "";
}

} // namespace volta::ir
