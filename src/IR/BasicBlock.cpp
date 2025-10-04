#include "IR/BasicBlock.hpp"
#include "IR/Module.hpp"
#include "IR/Instruction.hpp"
#include <algorithm>
#include <sstream>
#include <assert.h>
#include <queue>
#include <unordered_set>


namespace volta::ir {

// ============================================================================
// BasicBlock Implementation
// ============================================================================

BasicBlock::BasicBlock(const std::string& name, Function* parent)
    : Value(ValueKind::BasicBlock,
            std::make_shared<IRPrimitiveType>(IRType::Kind::Void),
            name),
      parent_(parent) {
}

// ========================================================================
// Instruction Management
// ========================================================================

Instruction* BasicBlock::getFirstInstruction() const {
    if (instructions_.size() == 0) {
        return nullptr;
    }
    return instructions_[0];
}

Instruction* BasicBlock::getTerminator() const {
    if (instructions_.size() == 0) return nullptr;
    return instructions_.back()->isTerminator() ? instructions_.back() : nullptr;
}

bool BasicBlock::hasTerminator() const {
    return getTerminator() != nullptr;
}

void BasicBlock::addInstruction(Instruction* inst) {
    if (hasTerminator()) {
        return;
    }

    inst->setParent(this);
    instructions_.push_back(inst);
}

void BasicBlock::insertBefore(Instruction* inst, Instruction* before) {
    auto it = std::find(instructions_.begin(), instructions_.end(), before);
    assert((it != instructions_.end()) && "Tried to insert before non existant instruction.");
    instructions_.insert(it, inst);
    inst->setParent(this);
}

void BasicBlock::removeInstruction(Instruction* inst) {
    auto it = std::find(instructions_.begin(), instructions_.end(), inst);
    assert((it != instructions_.end()) && "Tried to delete instruction not in block.");
    instructions_.erase(it);
    inst->setParent(nullptr);
}

void BasicBlock::eraseInstruction(Instruction* inst) {
    // Just remove from list - arena owns the memory
    removeInstruction(inst);
    // No delete! Arena will free this when Module is destroyed
}

// ========================================================================
// CFG Navigation
// ========================================================================

std::vector<BasicBlock*> BasicBlock::getSuccessors() const {
    std::vector<BasicBlock*> successors;
    
    Instruction* term = getTerminator();
    if (!term) {
        return successors; // No terminator = no successors
    }

    if (auto* ret = dynamic_cast<ReturnInst*>(term)) {
        return successors; // Empty vector
    }
    
    if (auto* br = dynamic_cast<BranchInst*>(term)) {
        successors.push_back(br->getDestination());
        return successors;
    }

    if (auto* condBr = dynamic_cast<CondBranchInst*>(term)) {
        successors.push_back(condBr->getTrueDest());
        successors.push_back(condBr->getFalseDest());
        return successors;
    }
    
    if (auto* sw = dynamic_cast<SwitchInst*>(term)) {
        // Default case
        successors.push_back(sw->getDefaultDest());
        
        // All the case destinations
        for (const auto& caseEntry : sw->getCases()) {
            successors.push_back(caseEntry.dest);
        }
        return successors;
    }
    
    return successors;
}

size_t BasicBlock::getNumSuccessors() const {
    return getSuccessors().size();
}

void BasicBlock::addPredecessor(BasicBlock* pred) {
    auto it = std::find(predecessors_.begin(), predecessors_.end(), pred);

    // Only add if not already present (silently ignore duplicates)
    if (it == predecessors_.end()) {
        predecessors_.push_back(pred);
    }
}

void BasicBlock::removePredecessor(BasicBlock* pred) {
    auto it = std::find(predecessors_.begin(), predecessors_.end(), pred);
    predecessors_.erase(it);
}

bool BasicBlock::isPredecessor(BasicBlock* block) const {
    return std::find(predecessors_.begin(), predecessors_.end(), block) != predecessors_.end();
}

bool BasicBlock::isSuccessor(BasicBlock* block) const {
    auto succ = getSuccessors();
    return std::find(succ.begin(), succ.end(), block) != succ.end();
}

// ========================================================================
// CFG Utilities
// ========================================================================

BasicBlock* BasicBlock::splitAt(Module& module, Instruction* splitPoint, const std::string& newBlockName) {
    // STEP 1: Validate
    auto it = std::find(instructions_.begin(), instructions_.end(), splitPoint);
    if (it == instructions_.end()) return nullptr; // Not in this block
    if (it == instructions_.begin()) return nullptr; // Can't split before first

    // STEP 2: Create new block using arena allocation
    auto* newBlock = module.createBasicBlock(newBlockName, parent_);

    // STEP 3: Move instructions [splitPoint, end) to new block
    while (it != instructions_.end()) {
        auto* inst = *it;
        it = instructions_.erase(it); // Remove from this block
        inst->setParent(newBlock);
        newBlock->instructions_.push_back(inst);
    }

    // STEP 4: Update CFG edges
    // Get old successors from new block's terminator
    auto oldSuccessors = newBlock->getSuccessors();

    // Update predecessor lists in old successors
    for (auto* succ : oldSuccessors) {
        succ->removePredecessor(this);
        succ->addPredecessor(newBlock);
    }

    // STEP 5: Create branch from this block to new block
    auto* br = module.createBranch(newBlock);
    addInstruction(br);
    newBlock->addPredecessor(this);

    return newBlock;
}

void BasicBlock::replaceAllUsesWith(BasicBlock* newBlock) {
    // TODO: Replace all uses of this block with newBlock
    // This means:
    // 1. Update all predecessor's terminators to point to newBlock instead of this
    // 2. Update all phi nodes that reference this block to reference newBlock

    // This is ADVANCED. Come back to this after you implement simpler methods.
}

bool BasicBlock::isReachable() const {
    // A block is reachable if it has at least one predecessor
    // (Entry blocks will be handled when we add Function class)
    return !predecessors_.empty();
}

// ========================================================================
// Phi Node Utilities
// ========================================================================

std::vector<PhiNode*> BasicBlock::getPhiNodes() const {
    std::vector<PhiNode*> phis;

    // Phi nodes are always at the beginning, stop at first non-phi
    for (auto* inst : instructions_) {
        if (auto* phi = dynamic_cast<PhiNode*>(inst)) {
            phis.push_back(phi);
        } else {
            break; // First non-phi, stop
        }
    }

    return phis;
}

bool BasicBlock::hasPhiNodes() const {
    if (instructions_.empty()) return false;
    return dynamic_cast<PhiNode*>(instructions_[0]) != nullptr;
}

void BasicBlock::addPhiNode(PhiNode* phi) {
    // Find position: after existing phis, before other instructions
    auto it = instructions_.begin();

    while (it != instructions_.end() && dynamic_cast<PhiNode*>(*it)) {
        ++it;
    }

    instructions_.insert(it, phi);
    phi->setParent(this);
}

// ========================================================================
// Pretty Printing
// ========================================================================

std::string BasicBlock::toString() const {
    std::stringstream ss;

    // Print label
    ss << getName() << ":";

    // Print predecessors
    if (!predecessors_.empty()) {
        ss << "                     ; preds = ";
        for (size_t i = 0; i < predecessors_.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << "%" << predecessors_[i]->getName();
        }
    }
    ss << "\n";

    // Print instructions with indentation
    for (auto* inst : instructions_) {
        ss << "  " << inst->toString() << "\n";
    }

    return ss.str();
}

std::string BasicBlock::toStringDetailed() const {
    std::stringstream ss;

    ss << "BasicBlock '" << getName() << "':\n";
    ss << "  Instructions: " << getNumInstructions() << "\n";
    ss << "  Has Terminator: " << (hasTerminator() ? "yes" : "no") << "\n";
    ss << "  Has Phi Nodes: " << (hasPhiNodes() ? "yes" : "no") << "\n";
    ss << "  Predecessors: " << getNumPredecessors() << "\n";
    ss << "  Successors: " << getNumSuccessors() << "\n";

    return ss.str();
}

// ============================================================================
// CFG Builder Utilities
// ============================================================================

namespace cfg {

void connectBlocks(Module& module, BasicBlock* from, BasicBlock* to) {
    auto* br = module.createBranch(to);
    from->addInstruction(br);
    to->addPredecessor(from);
}

void connectBlocksConditional(Module& module, BasicBlock* from, Value* condition,
                              BasicBlock* trueBlock, BasicBlock* falseBlock) {
    auto* condBr = module.createCondBranch(condition, trueBlock, falseBlock);
    from->addInstruction(condBr);
    trueBlock->addPredecessor(from);
    falseBlock->addPredecessor(from);
}

void removeBlockFromCFG(BasicBlock* block) {
    // TODO: Remove block from CFG (advanced)
    // STEPS:
    // 1. For each predecessor: update their terminators to not point to this block
    // 2. For each successor: remove this block from their predecessor list
    // 3. Clear this block's predecessor list

    // This is COMPLEX. Come back to this later.
}

bool hasPath(BasicBlock* from, BasicBlock* to) {

    if (from == to) return true;

    std::queue<BasicBlock*> queue;
    std::unordered_set<BasicBlock*> visited;

    queue.push(from);
    visited.insert(from);

    while (!queue.empty()) {
        BasicBlock* current = queue.front();
        queue.pop();
        
        // Check all successors of current block
        for (BasicBlock* succ : current->getSuccessors()) {
            // Found it!
            if (succ == to) {
                return true;
            }
            
            // Haven't visited this block yet? Add to queue
            if (visited.find(succ) == visited.end()) {
                visited.insert(succ);
                queue.push(succ);
            }
        }
    }
    
    // Exhausted all paths, didn't find 'to'
    return false;
}

// DFS helper
static void dfsHelper(BasicBlock* block,
               std::unordered_set<BasicBlock*>& visited,
               std::vector<BasicBlock*>& result) {
    if (visited.find(block) != visited.end()) return;

    visited.insert(block);
    result.push_back(block);

    for (BasicBlock* succ : block->getSuccessors()) {
        dfsHelper(succ, visited, result);
    }
}

std::vector<BasicBlock*> getReachableBlocks(BasicBlock* start) {
    std::vector<BasicBlock*> result;
    std::unordered_set<BasicBlock*> visited;
    dfsHelper(start, visited, result);
    return result;
}

// Postorder DFS helper
static void postorderHelper(BasicBlock* block,
                      std::unordered_set<BasicBlock*>& visited,
                      std::vector<BasicBlock*>& postorder) {
    if (visited.find(block) != visited.end()) return;

    visited.insert(block);

    // Visit children first (preorder part)
    for (BasicBlock* succ : block->getSuccessors()) {
        postorderHelper(succ, visited, postorder);
    }

    // Add to postorder AFTER visiting children
    postorder.push_back(block);
}

std::vector<BasicBlock*> reversePostorder(BasicBlock* entry) {
    std::vector<BasicBlock*> postorder;
    std::unordered_set<BasicBlock*> visited;

    postorderHelper(entry, visited, postorder);

    // Reverse to get reverse postorder
    std::reverse(postorder.begin(), postorder.end());
    return postorder;
}

} // namespace cfg

} // namespace volta::ir
