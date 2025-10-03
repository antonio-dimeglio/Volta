#include "IR/IRBuilder.hpp"
#include <sstream>

namespace volta::ir {

IRBuilder::IRBuilder() : insertBlock_(nullptr), valueCounter_(0) {
}

// ============================================================================
// Insert Point Management
// ============================================================================

void IRBuilder::setInsertPoint(BasicBlock* block) {
    // TODO: Set insertBlock_ to the given block.
    // All subsequent createXXX calls will insert instructions into this block.
}

// ============================================================================
// Arithmetic Operations
// ============================================================================

Value* IRBuilder::createAdd(Value* lhs, Value* rhs, const std::string& name) {
    // TODO: Create an Add instruction and insert it into the current block.
    // Steps:
    // 1. Generate a unique name if 'name' is empty (use getUniqueName())
    // 2. Create a BinaryInstruction with Opcode::Add
    // 3. Insert it into insertBlock_ (use insertInstruction helper)
    // 4. Return the instruction (it's also a Value)
    return nullptr;
}

Value* IRBuilder::createSub(Value* lhs, Value* rhs, const std::string& name) {
    // TODO: Same as createAdd, but with Opcode::Sub
    return nullptr;
}

Value* IRBuilder::createMul(Value* lhs, Value* rhs, const std::string& name) {
    // TODO: Same as createAdd, but with Opcode::Mul
    return nullptr;
}

Value* IRBuilder::createDiv(Value* lhs, Value* rhs, const std::string& name) {
    // TODO: Same as createAdd, but with Opcode::Div
    return nullptr;
}

Value* IRBuilder::createMod(Value* lhs, Value* rhs, const std::string& name) {
    // TODO: Same as createAdd, but with Opcode::Mod
    return nullptr;
}

Value* IRBuilder::createFAdd(Value* lhs, Value* rhs, const std::string& name) {
    // TODO: Floating-point add
    return nullptr;
}

Value* IRBuilder::createFSub(Value* lhs, Value* rhs, const std::string& name) {
    // TODO: Floating-point subtract
    return nullptr;
}

Value* IRBuilder::createFMul(Value* lhs, Value* rhs, const std::string& name) {
    // TODO: Floating-point multiply
    return nullptr;
}

Value* IRBuilder::createFDiv(Value* lhs, Value* rhs, const std::string& name) {
    // TODO: Floating-point divide
    return nullptr;
}

// ============================================================================
// Comparison Operations
// ============================================================================

Value* IRBuilder::createICmpEQ(Value* lhs, Value* rhs, const std::string& name) {
    // TODO: Create a CompareInstruction with Opcode::ICmpEQ
    // Note: Result type is always bool (not the operand type)
    return nullptr;
}

Value* IRBuilder::createICmpNE(Value* lhs, Value* rhs, const std::string& name) {
    // TODO: Integer not-equal comparison
    return nullptr;
}

Value* IRBuilder::createICmpLT(Value* lhs, Value* rhs, const std::string& name) {
    // TODO: Integer less-than comparison
    return nullptr;
}

Value* IRBuilder::createICmpLE(Value* lhs, Value* rhs, const std::string& name) {
    // TODO: Integer less-than-or-equal comparison
    return nullptr;
}

Value* IRBuilder::createICmpGT(Value* lhs, Value* rhs, const std::string& name) {
    // TODO: Integer greater-than comparison
    return nullptr;
}

Value* IRBuilder::createICmpGE(Value* lhs, Value* rhs, const std::string& name) {
    // TODO: Integer greater-than-or-equal comparison
    return nullptr;
}

Value* IRBuilder::createFCmpEQ(Value* lhs, Value* rhs, const std::string& name) {
    // TODO: Float equal comparison
    return nullptr;
}

Value* IRBuilder::createFCmpNE(Value* lhs, Value* rhs, const std::string& name) {
    // TODO: Float not-equal comparison
    return nullptr;
}

Value* IRBuilder::createFCmpLT(Value* lhs, Value* rhs, const std::string& name) {
    // TODO: Float less-than comparison
    return nullptr;
}

Value* IRBuilder::createFCmpLE(Value* lhs, Value* rhs, const std::string& name) {
    // TODO: Float less-than-or-equal comparison
    return nullptr;
}

Value* IRBuilder::createFCmpGT(Value* lhs, Value* rhs, const std::string& name) {
    // TODO: Float greater-than comparison
    return nullptr;
}

Value* IRBuilder::createFCmpGE(Value* lhs, Value* rhs, const std::string& name) {
    // TODO: Float greater-than-or-equal comparison
    return nullptr;
}

// ============================================================================
// Logical Operations
// ============================================================================

Value* IRBuilder::createAnd(Value* lhs, Value* rhs, const std::string& name) {
    // TODO: Logical AND
    return nullptr;
}

Value* IRBuilder::createOr(Value* lhs, Value* rhs, const std::string& name) {
    // TODO: Logical OR
    return nullptr;
}

Value* IRBuilder::createNot(Value* operand, const std::string& name) {
    // TODO: Logical NOT (unary operation)
    // Hint: You'll need to create a UnaryInstruction (not implemented yet - add it!)
    return nullptr;
}

// ============================================================================
// Memory Operations
// ============================================================================

Value* IRBuilder::createAlloca(std::shared_ptr<semantic::Type> type, const std::string& name) {
    // TODO: Create an AllocaInstruction.
    // This allocates stack space for a local variable.
    // Returns a pointer to the allocated space.
    return nullptr;
}

Value* IRBuilder::createLoad(Value* pointer, const std::string& name) {
    // TODO: Create a LoadInstruction.
    // Loads the value pointed to by 'pointer'.
    return nullptr;
}

void IRBuilder::createStore(Value* value, Value* pointer) {
    // TODO: Create a StoreInstruction.
    // Stores 'value' into the memory location pointed to by 'pointer'.
    // Note: Store doesn't return a value (void)
}

// ============================================================================
// Control Flow
// ============================================================================

void IRBuilder::createBr(BasicBlock* target) {
    // TODO: Create an unconditional BranchInstruction.
    // Also update CFG:
    //   - Add target to insertBlock_'s successors
    //   - Add insertBlock_ to target's predecessors
}

void IRBuilder::createCondBr(Value* condition, BasicBlock* thenBlock, BasicBlock* elseBlock) {
    // TODO: Create a CondBranchInstruction.
    // Update CFG for both thenBlock and elseBlock.
}

void IRBuilder::createRet(Value* value) {
    // TODO: Create a ReturnInstruction with the given return value.
}

void IRBuilder::createRetVoid() {
    // TODO: Create a ReturnInstruction with nullptr (void return).
}

// ============================================================================
// Function Calls
// ============================================================================

Value* IRBuilder::createCall(Function* callee, const std::vector<Value*>& arguments, const std::string& name) {
    // TODO: Create a CallInstruction.
    // The callee should be converted to a Value (functions are first-class values).
    // For now, you can cast callee or create a function pointer value.
    return nullptr;
}

// ============================================================================
// PHI Nodes
// ============================================================================

PhiInstruction* IRBuilder::createPhi(std::shared_ptr<semantic::Type> type, const std::string& name) {
    // TODO: Create a PhiInstruction.
    // Caller will add incoming values via phi->addIncoming(value, block).
    //
    // Important: PHI nodes must be at the START of a basic block!
    // Insert it before any other instructions.
    return nullptr;
}

// ============================================================================
// Helpers
// ============================================================================

std::string IRBuilder::getUniqueName() {
    // TODO: Generate a unique SSA name like "%0", "%1", "%2", etc.
    // Increment valueCounter_ each time.
    // Format: "%" + std::to_string(valueCounter_++)
    return "";
}

void IRBuilder::insertInstruction(std::unique_ptr<Instruction> inst) {
    // TODO: Insert the instruction into insertBlock_.
    // Check that insertBlock_ is not nullptr!
    // Call insertBlock_->addInstruction(std::move(inst))
}

} // namespace volta::ir
