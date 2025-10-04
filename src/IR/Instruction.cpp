#include "IR/Instruction.hpp"
#include "IR/Value.hpp"
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

Instruction::~Instruction() {
    // The Use-Def chain cleanup is handled automatically:
    // 1. operands_ vector destroys its Use objects
    // 2. Each Use destructor calls value_->removeUse(this)
    // The issue is when other instructions use THIS instruction - they need to be updated first
    //
    // Solution: BasicBlock should delete instructions in reverse order
    // This ensures that if inst A uses inst B, and B is defined before A,
    // then A is deleted before B, so A's uses of B are cleaned up first
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
    auto str = "%" + getName() + "= " + getOpcodeName(opcode_);
    for (const auto& op : operands_) {
        str += op.getValue()->toString();
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

BinaryOperator::BinaryOperator(Opcode op, Value* lhs, Value* rhs,
                               const std::string& name)
    : Instruction(op, computeBinaryOpType(op, lhs, rhs), {lhs, rhs}, name) {
}

BinaryOperator* BinaryOperator::create(Opcode op, Value* lhs, Value* rhs,
                                       const std::string& name) {

    assert(op >= Opcode::Add && op <= Opcode::Or);
    return new BinaryOperator(op, lhs, rhs, name);
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

UnaryOperator* UnaryOperator::create(Opcode op, Value* operand,
                                     const std::string& name) {
    assert(op == Opcode::Not || op == Opcode::Neg);
    return new UnaryOperator(op, operand, name);
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

CmpInst* CmpInst::create(Opcode op, Value* lhs, Value* rhs,
                        const std::string& name) {
    assert(op >= Opcode::Eq || op <= Opcode::Or);
    return new CmpInst(op, lhs, rhs, name);
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

AllocaInst* AllocaInst::create(std::shared_ptr<IRType> allocatedType,
                               const std::string& name) {
    assert(allocatedType && "AllocaInst requires valid type");
    return new AllocaInst(allocatedType, name);
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

LoadInst* LoadInst::create(Value* ptr, const std::string& name) {
    auto* ptrType = ptr->getType()->asPointer();
    assert(ptrType && "Load instruction requires pointer operand");
    return new LoadInst(ptr, name);
}

// ============================================================================
// StoreInst
// ============================================================================

StoreInst::StoreInst(Value* value, Value* ptr)
    : Instruction(Opcode::Store,
                  std::make_shared<IRPrimitiveType>(IRType::Kind::Void),
                  {value, ptr}) {
}

StoreInst* StoreInst::create(Value* value, Value* ptr) {
    auto* ptrType = ptr->getType()->asPointer();
    assert(ptrType && "Store instruction requires pointer operand");
    assert(value->getType()->equals(ptrType->pointeeType().get()) &&
           "Store value type must match pointer pointee type");
    return new StoreInst(value, ptr);
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

GCAllocInst* GCAllocInst::create(std::shared_ptr<IRType> allocatedType,
                                 const std::string& name) {
    assert(allocatedType && "GCAllocInst requires valid type");
    return new GCAllocInst(allocatedType, name);
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

ArrayNewInst* ArrayNewInst::create(std::shared_ptr<IRType> elementType,
                                   Value* size,
                                   const std::string& name) {
    auto* sizeType = size->getType()->asPrimitive();
    assert(sizeType && sizeType->kind() == IRType::Kind::I64 &&
           "ArrayNew size must be i64");
    return new ArrayNewInst(elementType, size, name);
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

ArrayGetInst* ArrayGetInst::create(Value* array, Value* index,
                                   const std::string& name) {
    auto* arrayPtrType = array->getType()->asPointer();
    assert(arrayPtrType && "ArrayGet requires pointer type (dynamic array)");

    auto* indexType = index->getType()->asPrimitive();
    assert(indexType && indexType->kind() == IRType::Kind::I64 &&
           "ArrayGet index must be i64");

    return new ArrayGetInst(array, index, name);
}

// ============================================================================
// ArraySetInst
// ============================================================================

ArraySetInst::ArraySetInst(Value* array, Value* index, Value* value)
    : Instruction(Opcode::ArraySet,
                  std::make_shared<IRPrimitiveType>(IRType::Kind::Void),
                  {array, index, value}) {
}

ArraySetInst* ArraySetInst::create(Value* array, Value* index, Value* value) {
    auto* arrayPtrType = array->getType()->asPointer();
    assert(arrayPtrType && "ArraySet requires pointer type (dynamic array)");

    auto* indexType = index->getType()->asPrimitive();
    assert(indexType && indexType->kind() == IRType::Kind::I64 &&
           "ArraySet index must be i64");

    assert(value->getType()->equals(arrayPtrType->pointeeType().get()) &&
           "ArraySet value type must match array element type");

    return new ArraySetInst(array, index, value);
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

ArrayLenInst* ArrayLenInst::create(Value* array, const std::string& name) {
    auto* arrayPtrType = array->getType()->asPointer();
    assert(arrayPtrType && "ArrayLen requires pointer type (dynamic array)");
    return new ArrayLenInst(array, name);
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

ArraySliceInst* ArraySliceInst::create(Value* array, Value* start, Value* end,
                                       const std::string& name) {
    auto* arrayPtrType = array->getType()->asPointer();
    assert(arrayPtrType && "ArraySlice requires pointer type (dynamic array)");

    auto* startType = start->getType()->asPrimitive();
    assert(startType && startType->kind() == IRType::Kind::I64 &&
           "ArraySlice start must be i64");

    auto* endType = end->getType()->asPrimitive();
    assert(endType && endType->kind() == IRType::Kind::I64 &&
           "ArraySlice end must be i64");

    return new ArraySliceInst(array, start, end, name);
}

// ============================================================================
// CastInst
// ============================================================================

CastInst::CastInst(Value* value, std::shared_ptr<IRType> destType,
                   const std::string& name)
    : Instruction(Opcode::Cast, destType, {value}, name) {
}

CastInst* CastInst::create(Value* value, std::shared_ptr<IRType> destType,
                          const std::string& name) {
    auto* srcPrim = value->getType()->asPrimitive();
    auto* destPrim = destType->asPrimitive();

    // Validate cast is legal (numeric types can be cast to each other)
    assert(srcPrim && destPrim && srcPrim->isNumeric() && destPrim->isNumeric() &&
           "Cast only supports numeric type conversions");

    return new CastInst(value, destType, name);
}

// ============================================================================
// OptionWrapInst
// ============================================================================

OptionWrapInst::OptionWrapInst(Value* value,
                               std::shared_ptr<IRType> optionType,
                               const std::string& name)
    : Instruction(Opcode::OptionWrap, optionType, {value}, name) {
}

OptionWrapInst* OptionWrapInst::create(Value* value,
                                       std::shared_ptr<IRType> optionType,
                                       const std::string& name) {
    auto* optType = optionType->asOption();
    assert(optType && "OptionWrap requires Option type");
    assert(value->getType()->equals(optType->innerType().get()) &&
           "OptionWrap value type must match Option inner type");
    return new OptionWrapInst(value, optionType, name);
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

OptionUnwrapInst* OptionUnwrapInst::create(Value* option, const std::string& name) {
    auto* optType = option->getType()->asOption();
    assert(optType && "OptionUnwrap requires Option type");
    return new OptionUnwrapInst(option, name);
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

OptionCheckInst* OptionCheckInst::create(Value* option, const std::string& name) {
    auto* optType = option->getType()->asOption();
    assert(optType && "OptionCheck requires Option type");
    return new OptionCheckInst(option, name);
}

// ============================================================================
// ReturnInst
// ============================================================================

ReturnInst::ReturnInst(Value* returnValue)
    : Instruction(Opcode::Ret,
                  std::make_shared<IRPrimitiveType>(IRType::Kind::Void),
                  returnValue ? std::vector<Value*>{returnValue} : std::vector<Value*>{}) {
}

ReturnInst* ReturnInst::create(Value* returnValue) {
    return new ReturnInst(returnValue);
}

// ============================================================================
// BranchInst
// ============================================================================

BranchInst::BranchInst(BasicBlock* dest)
    : Instruction(Opcode::Br,
                  std::make_shared<IRPrimitiveType>(IRType::Kind::Void),
                  {}),
      destination_(dest) {
}

BranchInst* BranchInst::create(BasicBlock* dest) {
    assert(dest && "BranchInst requires valid destination");
    return new BranchInst(dest);
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
}

CondBranchInst* CondBranchInst::create(Value* condition, BasicBlock* trueDest,
                                       BasicBlock* falseDest) {
    auto* condType = condition->getType()->asPrimitive();
    assert(condType && condType->kind() == IRType::Kind::I1 &&
           "CondBranch condition must be i1 (bool)");
    assert(trueDest && falseDest && "CondBranch requires valid destinations");
    return new CondBranchInst(condition, trueDest, falseDest);
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

SwitchInst* SwitchInst::create(Value* value, BasicBlock* defaultDest,
                               const std::vector<CaseEntry>& cases) {
    assert(defaultDest && "SwitchInst requires valid default destination");
    return new SwitchInst(value, defaultDest, cases);
}

void SwitchInst::addCase(Constant* value, BasicBlock* dest) {
    cases_.push_back({value, dest});
}

// ============================================================================
// CallInst
// ============================================================================

CallInst::CallInst(Function* callee, const std::vector<Value*>& args,
                   const std::string& name)
    : Instruction(Opcode::Call,
                  /* return type - will be set from callee->getReturnType() */ nullptr,
                  args,
                  name),
      callee_(callee) {
    // TODO: Once Function class is implemented, set type:
    //   type_ = callee->getReturnType();
}

CallInst* CallInst::create(Function* callee, const std::vector<Value*>& args,
                           const std::string& name) {
    assert(callee && "CallInst requires valid callee");
    // TODO: Once Function class is implemented, validate:
    //   - callee->getNumParams() == args.size()
    //   - Each arg type matches corresponding param type
    return new CallInst(callee, args, name);
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

CallIndirectInst* CallIndirectInst::create(Value* callee,
                                           const std::vector<Value*>& args,
                                           const std::string& name) {
    assert(callee && "CallIndirectInst requires valid callee");
    // TODO: Once IRFunctionType is implemented, validate:
    //   auto* fnType = callee->getType()->asFunctionType();
    //   assert(fnType && "CallIndirect callee must have function type");
    //   assert(fnType->getNumParams() == args.size());
    return new CallIndirectInst(callee, args, name);
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

PhiNode* PhiNode::create(std::shared_ptr<IRType> type,
                         const std::vector<IncomingValue>& incomingValues,
                         const std::string& name) {
    assert(type && "PhiNode requires valid type");
    return new PhiNode(type, incomingValues, name);
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

    // Rebuild operands from remaining incoming values
    operands_.clear();
    for (const auto& incoming : incomingValues_) {
        addOperand(incoming.value);
    }
}

} // namespace volta::ir
