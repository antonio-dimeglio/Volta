#include "IR/Function.hpp"
#include "IR/Instruction.hpp"
#include "IR/BasicBlock.hpp"
#include <algorithm>
#include <sstream>
#include <queue>
#include <unordered_set>

namespace volta::ir {

// ============================================================================
// Constructor and Factory
// ============================================================================

Function::Function(const std::string& name,
                   std::shared_ptr<IRType> returnType,
                   const std::vector<std::shared_ptr<IRType>>& paramTypes)
    : Value(ValueKind::Function, returnType, name),
      returnType_(returnType),
      hasVarArgs_(false),
      parent_(nullptr),
      entryBlock_(nullptr) {
    // NOTE: When created via Module, arguments are created separately using arena
    // When created standalone (tests), we create them here with new
    // This is a transitional pattern - eventually all creation should go through Module
}

Function::~Function() {
    // NOTE: This handles both arena-allocated and standalone (test) functions
    // For arena-allocated: Module owns this, don't call destructor manually
    // For standalone: Need to clean up

    // Clean up blocks (which clean up their instructions)
    for (auto* block : blocks_) {
        delete block;
    }

    // Clean up arguments
    for (auto* arg : arguments_) {
        delete arg;
    }
}

Function* Function::create(const std::string& name,
                           std::shared_ptr<IRType> returnType,
                           const std::vector<std::shared_ptr<IRType>>& paramTypes) {
    auto* func = new Function(name, returnType, paramTypes);

    // Create arguments for standalone function creation (mainly for tests)
    // NOTE: This uses 'new' - when using Module, use Module::createFunction() instead
    for (size_t i = 0; i < paramTypes.size(); ++i) {
        auto* arg = new Argument(paramTypes[i], i, "arg" + std::to_string(i));
        arg->setParent(func);
        func->arguments_.push_back(arg);
    }

    return func;
}

// ============================================================================
// Basic Properties
// ============================================================================

bool Function::isDeclaration() const {
    return blocks_.empty();
}

bool Function::isDefinition() const {
    return !isDeclaration();
}

// ============================================================================
// Parameter Management
// ============================================================================

size_t Function::getNumParams() const {

    return arguments_.size();
}

Argument* Function::getParam(size_t idx) const {
    if (idx >= arguments_.size()) return nullptr;
    return arguments_[idx];
}

// ============================================================================
// Basic Block Management
// ============================================================================

size_t Function::getNumBlocks() const {
    return blocks_.size();
}

bool Function::hasBlocks() const {
    return getNumBlocks() > 0;
}

void Function::addBasicBlock(BasicBlock* block) {
    blocks_.push_back(block);
    block->setParent(this);
    if (blocks_.size() == 1) entryBlock_ = block;
}

void Function::removeBasicBlock(BasicBlock* block) {
    auto it = std::find(blocks_.begin(), blocks_.end(), block);
    if (it == blocks_.end()) return;
    blocks_.erase(it);
    block->setParent(nullptr);

    // Update entry block if we removed it
    if (entryBlock_ == block) {
        entryBlock_ = blocks_.empty() ? nullptr : blocks_[0];
    }
}

void Function::eraseBasicBlock(BasicBlock* block) {
    // Just remove from list - arena owns the memory
    removeBasicBlock(block);
    // No delete! Arena will free this when Module is destroyed
}

void Function::setEntryBlock(BasicBlock* block) {
    auto it = std::find(blocks_.begin(), blocks_.end(), block);
    if (it == blocks_.end()) return;
    entryBlock_ = block;
}

BasicBlock* Function::findBlock(const std::string& name) const {
    for (auto block : blocks_) {
        if (block->getName() == name) return block;
    }
    return nullptr;
}

bool Function::hasSingleBlock() const {
    return blocks_.size() == 1;
}

// ============================================================================
// Instruction Access
// ============================================================================

std::vector<Instruction*> Function::getAllInstructions() const {
    std::vector<Instruction*> instructions;

    for (auto block : blocks_) {
        for (auto inst : block->getInstructions()) {
            instructions.push_back(inst);
        }
    }
    return instructions;
}

size_t Function::getNumInstructions() const {
    size_t count = 0;
    for (auto* block : blocks_) {
        count += block->getNumInstructions();
    }
    return count;
}

// ============================================================================
// CFG Analysis
// ============================================================================

std::vector<BasicBlock*> Function::getExitBlocks() const {
    std::vector<BasicBlock*> exits;

    for (auto* block : blocks_) {
        auto* terminator = block->getTerminator();
        if (terminator && dynamic_cast<ReturnInst*>(terminator)) {
            exits.push_back(block);
        }
    }
    return exits;
}

std::vector<BasicBlock*> Function::getUnreachableBlocks() const {
    // If no entry block, all blocks are unreachable
    if (!entryBlock_) {
        return blocks_;
    }

    // Get all reachable blocks from entry
    auto reachable = cfg::getReachableBlocks(entryBlock_);

    // Find blocks that are NOT in the reachable set
    std::vector<BasicBlock*> unreachable;
    for (auto* block : blocks_) {
        if (std::find(reachable.begin(), reachable.end(), block) == reachable.end()) {
            unreachable.push_back(block);
        }
    }

    return unreachable;
}

size_t Function::removeUnreachableBlocks() {
    auto unreachable = getUnreachableBlocks();
    size_t count = unreachable.size();

    for (auto* block : unreachable) {
        eraseBasicBlock(block);
    }

    return count;
}

bool Function::canReturn() const {
    // Check if any block has a return instruction
    for (auto* block : blocks_) {
        auto* term = block->getTerminator();
        if (term && dynamic_cast<ReturnInst*>(term)) {
            return true;
        }
    }
    return false;
}

bool Function::doesNotReturn() const {
    // Empty function (declaration) is not noreturn
    if (blocks_.empty()) {
        return false;
    }
    // Function doesn't return if it has blocks but can't return
    return !canReturn();
}

bool Function::hasReturnValue() const {
    return returnType_->kind() != IRType::Kind::Void;
}

// ============================================================================
// Verification
// ============================================================================

bool Function::verify(std::string* error) const {
    // 1. Check entry block exists (if function has body)
    if (!isDeclaration() && !entryBlock_) {
        if (error) *error = "Function has no entry block";
        return false;
    }

    // 2. Check entry block is in blocks_ list
    if (entryBlock_ && std::find(blocks_.begin(), blocks_.end(), entryBlock_) == blocks_.end()) {
        if (error) *error = "Entry block not in function's block list";
        return false;
    }

    // 3. Check all blocks have terminators
    for (auto* block : blocks_) {
        if (!block->hasTerminator()) {
            if (error) *error = "Block missing terminator: " + block->getName();
            return false;
        }
    }

    // 4. Check CFG consistency
    for (auto* block : blocks_) {
        for (auto* succ : block->getSuccessors()) {
            if (succ->getParent() != this) {
                if (error) *error = "Successor from different function";
                return false;
            }
            if (!succ->isPredecessor(block)) {
                if (error) *error = "CFG edge inconsistency";
                return false;
            }
        }
    }

    // 5. Check arguments
    for (size_t i = 0; i < arguments_.size(); ++i) {
        if (arguments_[i]->getArgNo() != i) {
            if (error) *error = "Argument index mismatch";
            return false;
        }
        if (arguments_[i]->getParent() != this) {
            if (error) *error = "Argument parent mismatch";
            return false;
        }
    }

    // 6. Check return statements match return type
    if (hasReturnValue()) {
        // All returns must have a value
        for (auto* block : blocks_) {
            auto* term = block->getTerminator();
            if (auto* ret = dynamic_cast<ReturnInst*>(term)) {
                if (!ret->hasReturnValue()) {
                    if (error) *error = "Function expects return value but has void return";
                    return false;
                }
            }
        }
    }

    return true;
}

// ============================================================================
// Pretty Printing
// ============================================================================

std::string Function::toString() const {
    std::stringstream ss;

    // Function signature
    ss << "function @" << getName() << "(";

    for (size_t i = 0; i < arguments_.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << "%" << arguments_[i]->getName() << ": "
           << arguments_[i]->getType()->toString();
    }

    ss << ") -> " << returnType_->toString();

    // If declaration only
    if (isDeclaration()) {
        ss << " declare\n";
        return ss.str();
    }

    ss << " {\n";

    // Print all blocks
    for (auto* block : blocks_) {
        ss << block->toString();
        if (block != blocks_.back()) {
            ss << "\n";
        }
    }

    ss << "}\n";

    return ss.str();
}

std::string Function::toStringDetailed() const {
    std::stringstream ss;

    ss << "Function: @" << getName() << "\n";
    ss << "  Return Type: " << returnType_->toString() << "\n";
    ss << "  Parameters: " << arguments_.size() << "\n";
    ss << "  Basic Blocks: " << blocks_.size() << "\n";
    ss << "  Instructions: " << getNumInstructions() << "\n";

    if (entryBlock_) {
        ss << "  Entry Block: " << entryBlock_->getName() << "\n";
    }

    auto exitBlocks = getExitBlocks();
    ss << "  Exit Blocks: " << exitBlocks.size() << "\n";

    if (isDeclaration()) {
        ss << "  Status: Declaration\n";
    } else {
        ss << "  Status: Definition\n";
    }

    return ss.str();
}

std::string Function::printCFG() const {
    std::stringstream ss;

    ss << "digraph \"CFG for '" << getName() << "' function\" {\n";
    ss << "  label=\"" << getName() << "\";\n\n";

    // Print nodes (blocks)
    for (auto* block : blocks_) {
        ss << "  \"" << block->getName() << "\"";

        // Highlight entry block
        if (block == entryBlock_) {
            ss << " [shape=box,style=filled,fillcolor=lightblue]";
        }

        ss << ";\n";
    }

    ss << "\n";

    // Print edges
    for (auto* block : blocks_) {
        for (auto* succ : block->getSuccessors()) {
            ss << "  \"" << block->getName() << "\" -> \""
               << succ->getName() << "\";\n";
        }
    }

    ss << "}\n";

    return ss.str();
}

// ============================================================================
// Advanced (Optional)
// ============================================================================

Function* Function::clone(const std::string& newName) const {
    // TODO: Deep copy of function
    // This is VERY HARD! Skip for now.
    return nullptr;
}

void Function::inlineInto(CallInst* callSite) {
    // TODO: Inline function into call site
    // This is VERY HARD! Skip for now.
}

} // namespace volta::ir
