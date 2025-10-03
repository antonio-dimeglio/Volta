#include "IR/Instruction.hpp"
#include "IR/BasicBlock.hpp"
#include <sstream>

namespace volta::ir {

// ============================================================================
// Instruction Base Class
// ============================================================================

Instruction::Instruction(
    Opcode opcode,
    std::shared_ptr<semantic::Type> type,
    const std::string& name,
    BasicBlock* parent
) : Value(Kind::Instruction, std::move(type), name),
    opcode_(opcode),
    parent_(parent) {
}

bool Instruction::isTerminator() const {
    // TODO: Return true if this instruction is a terminator (Br, CondBr, Ret).
    // Terminators are instructions that end a basic block.
    return false;
}

bool Instruction::isBinaryOp() const {
    // TODO: Return true if this is a binary arithmetic operation
    // (Add, Sub, Mul, Div, Mod, FAdd, FSub, FMul, FDiv)
    return false;
}

bool Instruction::isCompare() const {
    // TODO: Return true if this is a comparison operation
    // (ICmpEQ, ICmpNE, ..., FCmpEQ, FCmpNE, ...)
    return false;
}

bool Instruction::isMemoryOp() const {
    // TODO: Return true if this is a memory operation (Alloca, Load, Store)
    return false;
}

const char* Instruction::getOpcodeName(Opcode opcode) {
    // TODO: Return a string name for each opcode.
    // Examples: "add", "sub", "load", "store", "br", "phi", etc.
    // Hint: Use a switch statement or static array
    return "unknown";
}

// ============================================================================
// BinaryInstruction
// ============================================================================

BinaryInstruction::BinaryInstruction(
    Opcode opcode,
    Value* lhs,
    Value* rhs,
    const std::string& name
) : Instruction(opcode, lhs->getType(), name),
    lhs_(lhs),
    rhs_(rhs) {
    // TODO: Register this instruction as a user of lhs and rhs
    // (call lhs->addUse(this) and rhs->addUse(this))
}

Value* BinaryInstruction::getOperand(size_t index) const {
    // TODO: Return lhs if index == 0, rhs if index == 1, nullptr otherwise
    return nullptr;
}

void BinaryInstruction::setOperand(size_t index, Value* value) {
    // TODO: Set lhs or rhs based on index.
    // Remember to update use-def chains:
    //   1. Remove old use (old_value->removeUse(this))
    //   2. Add new use (new_value->addUse(this))
}

std::string BinaryInstruction::toString() const {
    // TODO: Format as "%name = opcode type %lhs, %rhs"
    // Example: "%2 = add i64 %0, %1"
    return "";
}

// ============================================================================
// CompareInstruction
// ============================================================================

CompareInstruction::CompareInstruction(
    Opcode opcode,
    Value* lhs,
    Value* rhs,
    const std::string& name
) : Instruction(opcode, semantic::Type::getBool(), name),  // Comparisons return bool
    lhs_(lhs),
    rhs_(rhs) {
    // TODO: Register uses (same as BinaryInstruction)
}

Value* CompareInstruction::getOperand(size_t index) const {
    // TODO: Same as BinaryInstruction
    return nullptr;
}

void CompareInstruction::setOperand(size_t index, Value* value) {
    // TODO: Same as BinaryInstruction
}

std::string CompareInstruction::toString() const {
    // TODO: Format as "%name = icmp/fcmp pred %lhs, %rhs"
    // Example: "%0 = icmp eq i64 %x, 42"
    return "";
}

// ============================================================================
// AllocaInstruction
// ============================================================================

AllocaInstruction::AllocaInstruction(
    std::shared_ptr<semantic::Type> allocatedType,
    const std::string& name
) : Instruction(Opcode::Alloca, semantic::Type::getPointer(allocatedType), name),
    allocatedType_(std::move(allocatedType)) {
}

std::string AllocaInstruction::toString() const {
    // TODO: Format as "%name = alloca type"
    // Example: "%x = alloca i64"
    return "";
}

// ============================================================================
// LoadInstruction
// ============================================================================

LoadInstruction::LoadInstruction(
    Value* pointer,
    const std::string& name
) : Instruction(Opcode::Load, pointer->getType(), name),  // TODO: extract pointee type
    pointer_(pointer) {
    // TODO: Register use
}

Value* LoadInstruction::getOperand(size_t index) const {
    // TODO: Return pointer if index == 0
    return nullptr;
}

void LoadInstruction::setOperand(size_t index, Value* value) {
    // TODO: Update pointer, maintain use-def chains
}

std::string LoadInstruction::toString() const {
    // TODO: Format as "%name = load type, ptr %pointer"
    // Example: "%0 = load i64, ptr %x"
    return "";
}

// ============================================================================
// StoreInstruction
// ============================================================================

StoreInstruction::StoreInstruction(Value* value, Value* pointer)
    : Instruction(Opcode::Store, semantic::Type::getVoid(), ""),
      value_(value),
      pointer_(pointer) {
    // TODO: Register uses for both value and pointer
}

Value* StoreInstruction::getOperand(size_t index) const {
    // TODO: Return value if index == 0, pointer if index == 1
    return nullptr;
}

void StoreInstruction::setOperand(size_t index, Value* value) {
    // TODO: Update value or pointer based on index, maintain use-def chains
}

std::string StoreInstruction::toString() const {
    // TODO: Format as "store type %value, ptr %pointer"
    // Example: "store i64 42, ptr %x"
    return "";
}

// ============================================================================
// BranchInstruction
// ============================================================================

BranchInstruction::BranchInstruction(BasicBlock* target)
    : Instruction(Opcode::Br, semantic::Type::getVoid(), ""),
      target_(target) {
}

std::string BranchInstruction::toString() const {
    // TODO: Format as "br label %target"
    // Example: "br label %loop_body"
    // Hint: You'll need to get the target block's name
    return "";
}

// ============================================================================
// CondBranchInstruction
// ============================================================================

CondBranchInstruction::CondBranchInstruction(
    Value* condition,
    BasicBlock* thenBlock,
    BasicBlock* elseBlock
) : Instruction(Opcode::CondBr, semantic::Type::getVoid(), ""),
    condition_(condition),
    thenBlock_(thenBlock),
    elseBlock_(elseBlock) {
    // TODO: Register use for condition
}

Value* CondBranchInstruction::getOperand(size_t index) const {
    // TODO: Return condition if index == 0
    return nullptr;
}

void CondBranchInstruction::setOperand(size_t index, Value* value) {
    // TODO: Update condition, maintain use-def chains
}

std::string CondBranchInstruction::toString() const {
    // TODO: Format as "br i1 %cond, label %then, label %else"
    // Example: "br i1 %cond, label %if_true, label %if_false"
    return "";
}

// ============================================================================
// ReturnInstruction
// ============================================================================

ReturnInstruction::ReturnInstruction(Value* returnValue)
    : Instruction(Opcode::Ret, semantic::Type::getVoid(), ""),
      returnValue_(returnValue) {
    // TODO: Register use for returnValue (if not nullptr)
}

Value* ReturnInstruction::getOperand(size_t index) const {
    // TODO: Return returnValue if index == 0 and returnValue is not nullptr
    return nullptr;
}

void ReturnInstruction::setOperand(size_t index, Value* value) {
    // TODO: Update returnValue, maintain use-def chains
}

std::string ReturnInstruction::toString() const {
    // TODO: Format as:
    //   "ret void" (if returnValue is nullptr)
    //   "ret type %value" (otherwise)
    // Example: "ret i64 %0"
    return "";
}

// ============================================================================
// CallInstruction
// ============================================================================

CallInstruction::CallInstruction(
    Value* callee,
    std::vector<Value*> arguments,
    const std::string& name
) : Instruction(Opcode::Call, callee->getType(), name),  // TODO: extract return type
    callee_(callee),
    arguments_(std::move(arguments)) {
    // TODO: Register uses for callee and all arguments
}

size_t CallInstruction::getNumOperands() const {
    // TODO: Return 1 (callee) + number of arguments
    return 0;
}

Value* CallInstruction::getOperand(size_t index) const {
    // TODO: Return callee if index == 0, arguments[index-1] otherwise
    return nullptr;
}

void CallInstruction::setOperand(size_t index, Value* value) {
    // TODO: Update callee or argument, maintain use-def chains
}

std::string CallInstruction::toString() const {
    // TODO: Format as "%name = call returnType @callee(argType %arg, ...)"
    // Example: "%ret = call i64 @add(i64 %a, i64 %b)"
    return "";
}

// ============================================================================
// PhiInstruction - THE HEART OF SSA!
// ============================================================================

PhiInstruction::PhiInstruction(
    std::shared_ptr<semantic::Type> type,
    const std::string& name
) : Instruction(Opcode::Phi, std::move(type), name) {
}

Value* PhiInstruction::getOperand(size_t index) const {
    // TODO: Return the value from incomingValues_[index]
    return nullptr;
}

void PhiInstruction::setOperand(size_t index, Value* value) {
    // TODO: Update the value in incomingValues_[index], maintain use-def chains
}

void PhiInstruction::addIncoming(Value* value, BasicBlock* block) {
    // TODO: Add a new incoming value from the given block.
    // Store in incomingValues_ as {value, block}
    // Register use for the value (value->addUse(this))
}

void PhiInstruction::removeIncoming(BasicBlock* block) {
    // TODO: Remove the incoming value from the given block.
    // Remember to unregister the use (value->removeUse(this))
}

Value* PhiInstruction::getIncomingValueForBlock(BasicBlock* block) const {
    // TODO: Find and return the value that comes from the given block.
    // Return nullptr if not found.
    return nullptr;
}

std::string PhiInstruction::toString() const {
    // TODO: Format as "%name = phi type [ %val1, %block1 ], [ %val2, %block2 ], ..."
    // Example: "%x.merged = phi i64 [ %x.1, %then ], [ %x.2, %else ]"
    return "";
}

} // namespace volta::ir
