#include "IR/Instruction.hpp"
#include "IR/Value.hpp"
#include "IR/Function.hpp"
#include <stdexcept>
#include <cassert>

namespace volta::ir {

// ============================================================================
// Instruction Base Class
// ============================================================================

Instruction::Instruction(Opcode opcode,
                         std::shared_ptr<IRType> type,
                         const std::vector<Value*>& operands,
                         const std::string& name)
    : Value(ValueKind::Instruction, type, name),
      opcode_(opcode),
      parent_(nullptr) {

    for (auto opr : operands) {
        addOperand(opr);
    }
}

void Instruction::addOperand(Value* value) {
    if (value == nullptr) return;
    operands_.emplace_back(value, this);
}

Value* Instruction::getOperand(unsigned idx) const {
    if (idx >= operands_.size()) return nullptr;
    return operands_[idx].getValue();
}

void Instruction::setOperand(unsigned idx, Value* value) {
    if (value == nullptr || idx > operands_.size()) return;
    operands_[idx].setValue(value);
}

bool Instruction::isTerminator() const {
    return opcode_ == Opcode::Ret || opcode_ == Opcode::Br ||
            opcode_ == Opcode::CondBr || opcode_ == Opcode::Switch;
}

bool Instruction::isBinaryOp() const {
    switch (opcode_) {
        case Opcode::Add:
        case Opcode::Sub:
        case Opcode::Mul:
        case Opcode::Div:
        case Opcode::Rem:
        case Opcode::Pow:
        case Opcode::And:
        case Opcode::Or:
            return true;
        default:
            return false;
    }
}

bool Instruction::isUnaryOp() const {
    return opcode_ == Opcode::Neg || opcode_ == Opcode::Not;
}

bool Instruction::isComparison() const {
    switch (opcode_) {
        case Opcode::Eq:
        case Opcode::Ne:
        case Opcode::Lt:
        case Opcode::Le:
        case Opcode::Gt:
        case Opcode::Ge:
            return true;
        default:
            return false;
    }
}

bool Instruction::isArithmetic() const {
    switch (opcode_) {
        case Opcode::Add:
        case Opcode::Sub:
        case Opcode::Mul:
        case Opcode::Div:
        case Opcode::Rem:
        case Opcode::Pow:
        case Opcode::Neg:  // Unary negation is also arithmetic
            return true;
        default:
            return false;
    }
}

bool Instruction::isMemoryOp() const {
    return opcode_ == Opcode::Alloca || opcode_ == Opcode::Load ||
            opcode_ == Opcode::Store || opcode_ == Opcode::GCAlloc;
}

bool Instruction::isArrayOp() const {
    switch (opcode_) {
        case Opcode::ArrayNew:
        case Opcode::ArrayGet:
        case Opcode::ArraySet:
        case Opcode::ArrayLen:
        case Opcode::ArraySlice:
            return true;
        default:
            return false;
    }
}

bool Instruction::isMatrixOp() const {
    switch (opcode_) {
        case Opcode::MatrixNew:
        case Opcode::MatrixGet:
        case Opcode::MatrixSet:
        case Opcode::MatrixMul:
        case Opcode::MatrixTranspose:
            return true;
        default:
            return false;
    }
}

bool Instruction::isCallInst() const {
    return opcode_ == Opcode::Call || opcode_ == Opcode::CallIndirect;
}

const char* Instruction::getOpcodeName(Opcode op) {
    switch (op) {
        // Arithmetic (binary)
        case Opcode::Add: return "add";
        case Opcode::Sub: return "sub";
        case Opcode::Mul: return "mul";
        case Opcode::Div: return "div";
        case Opcode::Rem: return "rem";
        case Opcode::Pow: return "pow";

        // Unary arithmetic
        case Opcode::Neg: return "neg";

        // Comparison
        case Opcode::Eq: return "eq";
        case Opcode::Ne: return "ne";
        case Opcode::Lt: return "lt";
        case Opcode::Le: return "le";
        case Opcode::Gt: return "gt";
        case Opcode::Ge: return "ge";

        // Logical
        case Opcode::And: return "and";
        case Opcode::Or: return "or";
        case Opcode::Not: return "not";

        // Memory
        case Opcode::Alloca: return "alloca";
        case Opcode::Load: return "load";
        case Opcode::Store: return "store";
        case Opcode::GCAlloc: return "gcalloc";

        // Array
        case Opcode::ArrayNew: return "array.new";
        case Opcode::ArrayGet: return "array.get";
        case Opcode::ArraySet: return "array.set";
        case Opcode::ArrayLen: return "array.len";
        case Opcode::ArraySlice: return "array.slice";

        // Matrix
        case Opcode::MatrixNew: return "matrix_new";
        case Opcode::MatrixGet: return "matrix_get";
        case Opcode::MatrixSet: return "matrix_set";
        case Opcode::MatrixMul: return "matrix_mul";
        case Opcode::MatrixTranspose: return "matrix_transpose";

        // Broadcasting
        case Opcode::Broadcast: return "broadcast";

        // Type
        case Opcode::Cast: return "cast";
        case Opcode::OptionWrap: return "option_wrap";
        case Opcode::OptionUnwrap: return "option_unwrap";
        case Opcode::OptionCheck: return "option_check";

        // Control flow
        case Opcode::Ret: return "ret";
        case Opcode::Br: return "br";
        case Opcode::CondBr: return "condbr";
        case Opcode::Switch: return "switch";

        // Function calls
        case Opcode::Call: return "call";
        case Opcode::CallIndirect: return "call.indirect";

        // Aggregate
        case Opcode::ExtractValue: return "extract_value";
        case Opcode::InsertValue: return "insert_value";

        // SSA
        case Opcode::Phi: return "phi";

        // Default
        default: return "unknown";
    }
}

std::string Instruction::toString() const {
    // Format: "%name = opcode operands"
    auto str = "% " + getName() + " = " + getOpcodeName(opcode_);
    for (const auto& op : operands_) {
        str += op.getValue()->toString() + " ";
    }
    return str;
}

// ============================================================================
// Type Computation Helpers
// ============================================================================

namespace {

    /**
     * Computes the result type for a binary operation
     * Handles type promotion and result type inference
     */
    std::shared_ptr<IRType> computeBinaryOpType(Instruction::Opcode op,
                                                Value* lhs, Value* rhs) {
        auto lhsType = lhs->getType();
        auto rhsType = rhs->getType();

        // Arithmetic operations: apply type promotion
        if (op == Instruction::Opcode::Add ||
            op == Instruction::Opcode::Sub ||
            op == Instruction::Opcode::Mul ||
            op == Instruction::Opcode::Div ||
            op == Instruction::Opcode::Rem ||
            op == Instruction::Opcode::Pow) {
            auto promoted = TypePromotion::promote(lhsType, rhsType);
            assert(promoted && "Type mismatch in arithmetic operation");
            return promoted;
        }

        // Comparison operations: return i1 (bool)
        if (op == Instruction::Opcode::Eq ||
            op == Instruction::Opcode::Ne ||
            op == Instruction::Opcode::Lt ||
            op == Instruction::Opcode::Le ||
            op == Instruction::Opcode::Gt ||
            op == Instruction::Opcode::Ge) {
            // Operands should have compatible types (promotion allowed)
            auto promoted = TypePromotion::promote(lhsType, rhsType);
            assert(promoted && "Type mismatch in comparison operation");
            return std::make_shared<IRPrimitiveType>(IRType::Kind::I1);
        }

        // Logical operations: both operands must be i1, result is i1
        if (op == Instruction::Opcode::And ||
            op == Instruction::Opcode::Or) {
            auto* lhsPrim = lhsType->asPrimitive();
            auto* rhsPrim = rhsType->asPrimitive();
            assert(lhsPrim && rhsPrim &&
                   lhsPrim->kind() == IRType::Kind::I1 &&
                   rhsPrim->kind() == IRType::Kind::I1 &&
                   "Logical operations require bool operands");
            return std::make_shared<IRPrimitiveType>(IRType::Kind::I1);
        }

        // Default: return promoted type
        auto promoted = TypePromotion::promote(lhsType, rhsType);
        assert(promoted && "Type mismatch in binary operation");
        return promoted;
    }
}

// ============================================================================
// BinaryOperator
// ============================================================================

BinaryOperator::BinaryOperator(Opcode op, Value* lhs, Value* rhs, const std::string& name)
    : Instruction(op, computeBinaryOpType(op, lhs, rhs), {lhs, rhs}, name) {
    assert(op >= Opcode::Add && op <= Opcode::Or);
}

bool BinaryOperator::classof(const Instruction* I) {
    return I->isBinaryOp();
}

// ============================================================================
// UnaryOperator
// ============================================================================

UnaryOperator::UnaryOperator(Opcode op, Value* operand, const std::string& name)
    : Instruction(op,
                  op == Opcode::Not ?
                      std::make_shared<IRPrimitiveType>(IRType::Kind::I1) :
                      operand->getType(),
                  {operand},
                  name) {
    // For Neg: result type = operand type (negating int gives int, float gives float)
    // For Not: result type = bool (i1)
}

bool UnaryOperator::classof(const Instruction* I) {
    return I->isUnaryOp();
}

// ============================================================================
// CmpInst
// ============================================================================

CmpInst::CmpInst(Opcode op, Value* lhs, Value* rhs, const std::string& name)
    : Instruction(op, std::make_shared<IRPrimitiveType>(IRType::Kind::I1), {lhs, rhs}, name) {
    // Comparison always returns bool (i1)
}


bool CmpInst::classof(const Instruction* I) {
    return I->isComparison();
}

// ============================================================================
// AllocaInst
// ============================================================================

AllocaInst::AllocaInst(std::shared_ptr<IRType> allocatedType,
                       const std::string& name)
    : Instruction(Opcode::Alloca,
                  std::make_shared<IRPointerType>(allocatedType),
                  {},
                  name) {
}

// ============================================================================
// LoadInst
// ============================================================================

LoadInst::LoadInst(Value* ptr, const std::string& name)
    : Instruction(Opcode::Load,
                  ptr->getType()->asPointer()->pointeeType(),
                  {ptr},
                  name) {
}


// ============================================================================
// StoreInst
// ============================================================================

StoreInst::StoreInst(Value* value, Value* ptr)
    : Instruction(Opcode::Store,
                  std::make_shared<IRPrimitiveType>(IRType::Kind::Void),
                  {value, ptr}) {
}


// ============================================================================
// GCAllocInst
// ============================================================================

GCAllocInst::GCAllocInst(std::shared_ptr<IRType> allocatedType,
                         const std::string& name)
    : Instruction(Opcode::GCAlloc,
                  std::make_shared<IRPointerType>(allocatedType),
                  {},
                  name) {
}


// ============================================================================
// ArrayNewInst
// ============================================================================

ArrayNewInst::ArrayNewInst(std::shared_ptr<IRType> elementType,
                           Value* size,
                           const std::string& name)
    : Instruction(Opcode::ArrayNew,
                  std::make_shared<IRPointerType>(elementType),
                  {size},
                  name) {
}

// ============================================================================
// ArrayGetInst
// ============================================================================

ArrayGetInst::ArrayGetInst(Value* array, Value* index, const std::string& name)
    : Instruction(Opcode::ArrayGet,
                  array->getType()->asPointer()->pointeeType(),
                  {array, index},
                  name) {
}


// ============================================================================
// ArraySetInst
// ============================================================================

ArraySetInst::ArraySetInst(Value* array, Value* index, Value* value)
    : Instruction(Opcode::ArraySet,
                  std::make_shared<IRPrimitiveType>(IRType::Kind::Void),
                  {array, index, value}) {
}


// ============================================================================
// ArrayLenInst
// ============================================================================

ArrayLenInst::ArrayLenInst(Value* array, const std::string& name)
    : Instruction(Opcode::ArrayLen,
                  std::make_shared<IRPrimitiveType>(IRType::Kind::I64),
                  {array},
                  name) {
}

// ============================================================================
// ArraySliceInst
// ============================================================================

ArraySliceInst::ArraySliceInst(Value* array, Value* start, Value* end,
                               const std::string& name)
    : Instruction(Opcode::ArraySlice,
                  array->getType(),
                  {array, start, end},
                  name) {
    // Returns same type as input (ptr<T>)
}


// ============================================================================
// CastInst
// ============================================================================

CastInst::CastInst(Value* value, std::shared_ptr<IRType> destType,
                   const std::string& name)
    : Instruction(Opcode::Cast, destType, {value}, name) {
}



// ============================================================================
// OptionWrapInst
// ============================================================================

OptionWrapInst::OptionWrapInst(Value* value,
                               std::shared_ptr<IRType> optionType,
                               const std::string& name)
    : Instruction(Opcode::OptionWrap, optionType, {value}, name) {
}


// ============================================================================
// OptionUnwrapInst
// ============================================================================

OptionUnwrapInst::OptionUnwrapInst(Value* option, const std::string& name)
    : Instruction(Opcode::OptionUnwrap,
                  option->getType()->asOption()->innerType(),
                  {option},
                  name) {
}


// ============================================================================
// OptionCheckInst
// ============================================================================

OptionCheckInst::OptionCheckInst(Value* option, const std::string& name)
    : Instruction(Opcode::OptionCheck,
                  std::make_shared<IRPrimitiveType>(IRType::Kind::I1),
                  {option},
                  name) {
}

// ============================================================================
// ReturnInst
// ============================================================================

ReturnInst::ReturnInst(Value* returnValue)
    : Instruction(Opcode::Ret,
                  std::make_shared<IRPrimitiveType>(IRType::Kind::Void),
                  returnValue ? std::vector<Value*>{returnValue} : std::vector<Value*>{}) {
}

// ============================================================================
// BranchInst
// ============================================================================

BranchInst::BranchInst(BasicBlock* dest)
    : Instruction(Opcode::Br,
                  std::make_shared<IRPrimitiveType>(IRType::Kind::Void),
                  {}),
      destination_(dest) {
    // CFG edges will be set when instruction is added to a block
}

void BranchInst::setDestination(BasicBlock* dest) {
    // Remove old edge
    if (destination_ && getParent()) {
        destination_->removePredecessor(getParent());
    }

    destination_ = dest;

    // Add new edge
    if (destination_ && getParent()) {
        destination_->addPredecessor(getParent());
    }
}


// ============================================================================
// CondBranchInst
// ============================================================================

CondBranchInst::CondBranchInst(Value* condition, BasicBlock* trueDest,
                               BasicBlock* falseDest)
    : Instruction(Opcode::CondBr,
                  std::make_shared<IRPrimitiveType>(IRType::Kind::Void),
                  {condition}),
      trueDest_(trueDest),
      falseDest_(falseDest) {
    // CFG edges will be set when instruction is added to a block
}

void CondBranchInst::setTrueDest(BasicBlock* dest) {
    // Remove old edge
    if (trueDest_ && getParent()) {
        trueDest_->removePredecessor(getParent());
    }

    trueDest_ = dest;

    // Add new edge
    if (trueDest_ && getParent()) {
        trueDest_->addPredecessor(getParent());
    }
}

void CondBranchInst::setFalseDest(BasicBlock* dest) {
    // Remove old edge
    if (falseDest_ && getParent()) {
        falseDest_->removePredecessor(getParent());
    }

    falseDest_ = dest;

    // Add new edge
    if (falseDest_ && getParent()) {
        falseDest_->addPredecessor(getParent());
    }
}

// ============================================================================
// SwitchInst
// ============================================================================

SwitchInst::SwitchInst(Value* value, BasicBlock* defaultDest,
                       const std::vector<CaseEntry>& cases)
    : Instruction(Opcode::Switch,
                  std::make_shared<IRPrimitiveType>(IRType::Kind::Void),
                  {value}),
      defaultDest_(defaultDest),
      cases_(cases) {
}

void SwitchInst::addCase(Constant* value, BasicBlock* dest) {
    cases_.push_back({value, dest});

    // Update CFG edge if this instruction is already in a block
    if (getParent() && dest) {
        dest->addPredecessor(getParent());
    }
}

// ============================================================================
// CallInst
// ============================================================================

CallInst::CallInst(Function* callee, const std::vector<Value*>& args,
                   const std::string& name)
    : Instruction(Opcode::Call,
                  callee->getReturnType(),
                  args,
                  name),
      callee_(callee) {
}

// ============================================================================
// CallIndirectInst
// ============================================================================

CallIndirectInst::CallIndirectInst(Value* callee, const std::vector<Value*>& args,
                                   const std::string& name)
    : Instruction(Opcode::CallIndirect,
                  /* return type - will be extracted from callee's function type */ nullptr,
                  {},
                  name) {
    // Add callee as first operand
    addOperand(callee);
    // Add remaining args
    for (auto* arg : args) {
        addOperand(arg);
    }
    // TODO: Once IRFunctionType is implemented, set type:
    //   auto* fnType = callee->getType()->asFunctionType();
    //   type_ = fnType->getReturnType();
}


// ============================================================================
// PhiNode
// ============================================================================

PhiNode::PhiNode(std::shared_ptr<IRType> type,
                 const std::vector<IncomingValue>& incomingValues,
                 const std::string& name)
    : Instruction(Opcode::Phi, type, {}, name),
      incomingValues_(incomingValues) {
    // Add each incoming value as an operand
    for (const auto& incoming : incomingValues) {
        addOperand(incoming.value);
    }
}

Value* PhiNode::getIncomingValue(unsigned idx) const {
    if (idx >= incomingValues_.size()) return nullptr;
    return incomingValues_[idx].value;
}

BasicBlock* PhiNode::getIncomingBlock(unsigned idx) const {
    if (idx >= incomingValues_.size()) return nullptr;
    return incomingValues_[idx].block;
}

void PhiNode::addIncoming(Value* value, BasicBlock* block) {
    incomingValues_.push_back({value, block});
    addOperand(value);
}

void PhiNode::removeIncomingValue(unsigned idx) {
    if (idx >= incomingValues_.size()) return;

    // Remove from incoming values
    incomingValues_.erase(incomingValues_.begin() + idx);

    // Remove just the operand at this index
    operands_.erase(operands_.begin() + idx);
}

} // namespace volta::ir
