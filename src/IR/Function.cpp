#include "IR/Function.hpp"
#include "IR/IRModule.hpp"
#include <algorithm>

namespace volta::ir {

Function::Function(
    const std::string& name,
    std::vector<std::unique_ptr<Parameter>> parameters,
    std::shared_ptr<semantic::Type> returnType
) : name_(name),
    parameters_(std::move(parameters)),
    returnType_(std::move(returnType)),
    parent_(nullptr) {
}

Function::~Function() = default;

// ============================================================================
// Parameters
// ============================================================================

Parameter* Function::getParameter(size_t index) const {
    // TODO: Return the parameter at the given index.
    // Return nullptr if index is out of bounds.
    return nullptr;
}

// ============================================================================
// Basic Block Management
// ============================================================================

BasicBlock* Function::createBasicBlock(const std::string& name) {
    // TODO: Create a new BasicBlock with the given name.
    // Add it to basicBlocks_ and return a pointer to it.
    // Set the block's parent to this function.
    return nullptr;
}

void Function::addBasicBlock(std::unique_ptr<BasicBlock> block) {
    // TODO: Add the block to basicBlocks_.
    // Set the block's parent to this function.
}

void Function::removeBasicBlock(BasicBlock* block) {
    // TODO: Remove the block from basicBlocks_.
    // Clear the block's parent.
    // Hint: Use std::remove_if with unique_ptr
}

BasicBlock* Function::getEntryBlock() const {
    // TODO: Return the first basic block (entry block).
    // Return nullptr if the function has no blocks.
    return nullptr;
}

void Function::setEntryBlock(BasicBlock* entry) {
    // TODO: Move the given block to the front of basicBlocks_.
    // This makes it the entry block (first to execute).
    // Hint: Find the block, then std::rotate it to the front
}

bool Function::isEntryBlock(const BasicBlock* block) const {
    // TODO: Return true if this block is the entry block (first block).
    return false;
}

// ============================================================================
// SSA Variable Tracking
// ============================================================================

Value* Function::getCurrentValue(const std::string& varName) const {
    // TODO: Look up the current SSA value for this variable name.
    // Return nullptr if not found.
    return nullptr;
}

void Function::setCurrentValue(const std::string& varName, Value* value) {
    // TODO: Set the current SSA value for this variable name.
    // This is used during IR generation to track which version of a variable
    // is currently "active" in the current scope.
}

// ============================================================================
// Printing/Debugging
// ============================================================================

std::string Function::toString() const {
    // TODO: Return a string representation of the entire function.
    // Format:
    //   define returnType @functionName(param1Type %param1, param2Type %param2, ...) {
    //       [basic block 1]
    //       [basic block 2]
    //       ...
    //   }
    //
    // Example:
    //   define i64 @add(i64 %a, i64 %b) {
    //   entry:
    //       %0 = add i64 %a, %b
    //       ret i64 %0
    //   }
    //
    // Hint: Loop through basicBlocks_ and call block->toString() on each
    return "";
}

} // namespace volta::ir
