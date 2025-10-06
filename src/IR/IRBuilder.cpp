#include "IR/IRBuilder.hpp"
#include <algorithm>

namespace volta::ir {

// ============================================================================
// Constructor
// ============================================================================

IRBuilder::IRBuilder(Module& module)
    : module_(module), insertionBlock_(nullptr), insertionPoint_(nullptr) {
}

// ============================================================================
// Insertion Point Management
// ============================================================================

void IRBuilder::setInsertionPoint(BasicBlock* block) {
    insertionBlock_ = block;
    insertionPoint_ = nullptr;
}

void IRBuilder::setInsertionPointBefore(Instruction* inst) {
    if (!inst) return;
    insertionBlock_ = inst->getParent();
    insertionPoint_ = inst;
}

void IRBuilder::setInsertionPointAfter(Instruction* inst) {
    if (!inst) return;
    insertionBlock_ = inst->getParent();

    // Find the instruction after this one
    auto& instructions = insertionBlock_->getInstructions();
    auto it = std::find(instructions.begin(), instructions.end(), inst);

    if (it != instructions.end()) {
        ++it;
        if (it != instructions.end()) {
            insertionPoint_ = *it;
        } else {
            insertionPoint_ = nullptr;  // Insert at end
        }
    }
}

void IRBuilder::clearInsertionPoint() {
    insertionBlock_ = nullptr;
    insertionPoint_ = nullptr;
}

// ============================================================================
// Type Helpers
// ============================================================================

std::shared_ptr<IRType> IRBuilder::getIntType() {
    return module_.getIntType();
}

std::shared_ptr<IRType> IRBuilder::getFloatType() {
    return module_.getFloatType();
}

std::shared_ptr<IRType> IRBuilder::getBoolType() {
    return module_.getBoolType();
}

std::shared_ptr<IRType> IRBuilder::getStringType() {
    return module_.getStringType();
}

std::shared_ptr<IRType> IRBuilder::getVoidType() {
    return module_.getVoidType();
}

std::shared_ptr<IRType> IRBuilder::getPointerType(std::shared_ptr<IRType> pointeeType) {
    return module_.getPointerType(pointeeType);
}

std::shared_ptr<IRType> IRBuilder::getArrayType(std::shared_ptr<IRType> elementType, size_t size) {
    return module_.getArrayType(elementType, size);
}

std::shared_ptr<IRType> IRBuilder::getOptionType(std::shared_ptr<IRType> innerType) {
    return module_.getOptionType(innerType);
}

// ============================================================================
// Function and Block Creation
// ============================================================================

Function* IRBuilder::createFunction(const std::string& name,
                                   std::shared_ptr<IRType> returnType,
                                   const std::vector<std::shared_ptr<IRType>>& paramTypes,
                                   bool setAsInsertionPoint) {
    auto fn = module_.createFunction(name, returnType, paramTypes);

    BasicBlock* entry = module_.createBasicBlock("entry", fn);
    // Note: createBasicBlock already adds block to function, no need to call addBasicBlock
    fn->setEntryBlock(entry);

    if (setAsInsertionPoint) {
        setInsertionPoint(entry);
    }

    return fn;
}

BasicBlock* IRBuilder::createBasicBlock(const std::string& name,
                                       bool insertIntoFunction) {
    BasicBlock* block = module_.createBasicBlock(name);

    if (insertIntoFunction && insertionBlock_ != nullptr) {
        Function* func = insertionBlock_->getParent();
        if (func) {
            func->addBasicBlock(block);
        }
    }

    return block;
}

// ============================================================================
// Constant Creation
// ============================================================================

ConstantInt* IRBuilder::getInt(int64_t value) {
    return module_.getConstantInt(value, getIntType());
}

ConstantFloat* IRBuilder::getFloat(double value) {
    return module_.getConstantFloat(value, getFloatType());
}

ConstantBool* IRBuilder::getBool(bool value) {
    return module_.getConstantBool(value, getBoolType());
}

ConstantBool* IRBuilder::getTrue() {
    return module_.getConstantBool(true, getBoolType());
}

ConstantBool* IRBuilder::getFalse() {
    return module_.getConstantBool(false, getBoolType());
}

ConstantString* IRBuilder::getString(const std::string& value) {
    return module_.getConstantString(value, getStringType());
}

ConstantNone* IRBuilder::getNone(std::shared_ptr<IRType> optionType) {
    return module_.getArena().allocate<ConstantNone>(optionType);
}

UndefValue* IRBuilder::getUndef(std::shared_ptr<IRType> type) {
    return module_.getArena().allocate<UndefValue>(type);
}

// ============================================================================
// Arithmetic Instructions
// ============================================================================

Value* IRBuilder::createAdd(Value* lhs, Value* rhs, const std::string& name) {
    auto promotedType = TypePromotion::promote(lhs->getType(), rhs->getType());
    if (!promotedType) return nullptr;

    // Insert casts if needed (compare types by value, not pointer)
    if (!lhs->getType()->equals(promotedType.get())) {
        lhs = createCast(lhs, promotedType);
    }
    if (!rhs->getType()->equals(promotedType.get())) {
        rhs = createCast(rhs, promotedType);
    }

    BinaryOperator* opr = module_.createBinaryOp(Instruction::Opcode::Add, lhs, rhs, name);
    insert(opr);

    return opr;
}

Value* IRBuilder::createSub(Value* lhs, Value* rhs, const std::string& name) {
    auto promotedType = TypePromotion::promote(lhs->getType(), rhs->getType());
    if (!promotedType) return nullptr;

    // Insert casts if needed (compare types by value, not pointer)
    if (!lhs->getType()->equals(promotedType.get())) {
        lhs = createCast(lhs, promotedType);
    }
    if (!rhs->getType()->equals(promotedType.get())) {
        rhs = createCast(rhs, promotedType);
    }

    BinaryOperator* opr = module_.createBinaryOp(Instruction::Opcode::Sub, lhs, rhs, name);
    insert(opr);

    return opr;
}

Value* IRBuilder::createMul(Value* lhs, Value* rhs, const std::string& name) {
    auto promotedType = TypePromotion::promote(lhs->getType(), rhs->getType());
    if (!promotedType) return nullptr;

    // Insert casts if needed (compare types by value, not pointer)
    if (!lhs->getType()->equals(promotedType.get())) {
        lhs = createCast(lhs, promotedType);
    }
    if (!rhs->getType()->equals(promotedType.get())) {
        rhs = createCast(rhs, promotedType);
    }

    BinaryOperator* opr = module_.createBinaryOp(Instruction::Opcode::Mul, lhs, rhs, name);
    insert(opr);

    return opr;
}

Value* IRBuilder::createDiv(Value* lhs, Value* rhs, const std::string& name) {
    auto promotedType = TypePromotion::promote(lhs->getType(), rhs->getType());
    if (!promotedType) return nullptr;

    // Insert casts if needed (compare types by value, not pointer)
    if (!lhs->getType()->equals(promotedType.get())) {
        lhs = createCast(lhs, promotedType);
    }
    if (!rhs->getType()->equals(promotedType.get())) {
        rhs = createCast(rhs, promotedType);
    }

    BinaryOperator* opr = module_.createBinaryOp(Instruction::Opcode::Div, lhs, rhs, name);
    insert(opr);

    return opr;
}

Value* IRBuilder::createRem(Value* lhs, Value* rhs, const std::string& name) {
    auto promotedType = TypePromotion::promote(lhs->getType(), rhs->getType());
    if (!promotedType) return nullptr;

    // Insert casts if needed (compare types by value, not pointer)
    if (!lhs->getType()->equals(promotedType.get())) {
        lhs = createCast(lhs, promotedType);
    }
    if (!rhs->getType()->equals(promotedType.get())) {
        rhs = createCast(rhs, promotedType);
    }

    BinaryOperator* opr = module_.createBinaryOp(Instruction::Opcode::Rem, lhs, rhs, name);
    insert(opr);

    return opr;
}

Value* IRBuilder::createPow(Value* lhs, Value* rhs, const std::string& name) {
    auto promotedType = TypePromotion::promote(lhs->getType(), rhs->getType());
    if (!promotedType) return nullptr;

    // Insert casts if needed (compare types by value, not pointer)
    if (!lhs->getType()->equals(promotedType.get())) {
        lhs = createCast(lhs, promotedType);
    }
    if (!rhs->getType()->equals(promotedType.get())) {
        rhs = createCast(rhs, promotedType);
    }

    BinaryOperator* opr = module_.createBinaryOp(Instruction::Opcode::Pow, lhs, rhs, name);
    insert(opr);

    return opr;
}

Value* IRBuilder::createNeg(Value* operand, const std::string& name) {
    UnaryOperator* opr = module_.createUnaryOp(Instruction::Opcode::Neg, operand, name);
    insert(opr);

    return opr;
}

// ============================================================================
// Comparison Instructions
// ============================================================================

Value* IRBuilder::createEq(Value* lhs, Value* rhs, const std::string& name) {
    if (!lhs->getType()->equals(rhs->getType().get())) {
        return nullptr;
    }

    CmpInst* opr = module_.createCmp(Instruction::Opcode::Eq, lhs, rhs, name);
    insert(opr);

    return opr;
}

Value* IRBuilder::createNe(Value* lhs, Value* rhs, const std::string& name) {
    if (!lhs->getType()->equals(rhs->getType().get())) {
        return nullptr;
    }

    CmpInst* opr = module_.createCmp(Instruction::Opcode::Ne, lhs, rhs, name);
    insert(opr);

    return opr;
}

Value* IRBuilder::createLt(Value* lhs, Value* rhs, const std::string& name) {
    if (!lhs->getType()->equals(rhs->getType().get())) {
        return nullptr;
    }

    CmpInst* opr = module_.createCmp(Instruction::Opcode::Lt, lhs, rhs, name);
    insert(opr);

    return opr;
}

Value* IRBuilder::createLe(Value* lhs, Value* rhs, const std::string& name) {
    if (!lhs->getType()->equals(rhs->getType().get())) {
        return nullptr;
    }

    CmpInst* opr = module_.createCmp(Instruction::Opcode::Le, lhs, rhs, name);
    insert(opr);

    return opr;
}

Value* IRBuilder::createGt(Value* lhs, Value* rhs, const std::string& name) {
    if (!lhs->getType()->equals(rhs->getType().get())) {
        return nullptr;
    }

    CmpInst* opr = module_.createCmp(Instruction::Opcode::Gt, lhs, rhs, name);
    insert(opr);

    return opr;
}

Value* IRBuilder::createGe(Value* lhs, Value* rhs, const std::string& name) {
    if (!lhs->getType()->equals(rhs->getType().get())) {
        return nullptr;
    }

    CmpInst* opr = module_.createCmp(Instruction::Opcode::Ge, lhs, rhs, name);
    insert(opr);

    return opr;
}

// ============================================================================
// Logical Instructions
// ============================================================================

Value* IRBuilder::createAnd(Value* lhs, Value* rhs, const std::string& name) {
    if (!lhs->getType()->equals(rhs->getType().get())) {
        return nullptr;
    }

    BinaryOperator* opr = module_.createBinaryOp(Instruction::Opcode::And, lhs, rhs, name);
    insert(opr);

    return opr;
}

Value* IRBuilder::createOr(Value* lhs, Value* rhs, const std::string& name) {
    if (!lhs->getType()->equals(rhs->getType().get())) {
        return nullptr;
    }

    BinaryOperator* opr = module_.createBinaryOp(Instruction::Opcode::Or, lhs, rhs, name);
    insert(opr);

    return opr;
}

Value* IRBuilder::createNot(Value* operand, const std::string& name) {
    UnaryOperator* opr = module_.createUnaryOp(Instruction::Opcode::Not, operand, name);
    insert(opr);

    return opr;
}

// ============================================================================
// Memory Instructions
// ============================================================================

Value* IRBuilder::createAlloca(std::shared_ptr<IRType> type, const std::string& name) {
    auto* alloca = module_.createAlloca(type, name);
    insert(alloca);
    return alloca;
}

Value* IRBuilder::createLoad(Value* ptr, const std::string& name) {
    // TODO: Validate ptr is actually a pointer type
    auto* load = module_.createLoad(ptr, name);
    insert(load);
    return load;
}

void IRBuilder::createStore(Value* value, Value* ptr) {
    // TODO: Validate types match
    auto* store = module_.createStore(value, ptr);
    insert(store);
}

Value* IRBuilder::createGCAlloc(std::shared_ptr<IRType> type, const std::string& name) {
    auto* alloc = module_.createGCAlloc(type, name);
    insert(alloc);
    return alloc;
}

// ============================================================================
// Array Instructions
// ============================================================================

Value* IRBuilder::createArrayNew(std::shared_ptr<IRType> elementType, Value* size,
                                const std::string& name) {
    auto* arr = module_.createArrayNew(elementType, size, name);
    insert(arr);
    return arr;
}

Value* IRBuilder::createArrayGet(Value* array, Value* index, const std::string& name) {
    auto* arr = module_.createArrayGet(array, index, name);
    insert(arr);
    return arr;
}

void IRBuilder::createArraySet(Value* array, Value* index, Value* value) {
    auto* arr = module_.createArraySet(array, index, value);
    insert(arr);
}

Value* IRBuilder::createArrayLen(Value* array, const std::string& name) {
    auto* arr = module_.createArrayLen(array, name);
    insert(arr);
    return arr;
}

Value* IRBuilder::createArraySlice(Value* array, Value* start, Value* end,
                                  const std::string& name) {
    auto* arr = module_.createArraySlice(array, start, end, name);
    insert(arr);
    return arr;
}

// ============================================================================
// Type Conversion Instructions
// ============================================================================

Value* IRBuilder::createCast(Value* value, std::shared_ptr<IRType> destType,
                            const std::string& name) {
    auto* cast = module_.createCast(value, destType, name);
    insert(cast);
    return cast;
}

Value* IRBuilder::createOptionWrap(Value* value, std::shared_ptr<IRType> optionType,
                                  const std::string& name) {
    (void)optionType;  // Unused - module infers type from value
    // Module.createOptionWrap infers the option type from value's type
    // The optionType parameter here is unused but kept for backward compatibility
    auto* op = module_.createOptionWrap(value, name);
    insert(op);
    return op;
}

Value* IRBuilder::createOptionUnwrap(Value* option, const std::string& name) {
    auto* op = module_.createOptionUnwrap(option, name);
    insert(op);
    return op;
}

Value* IRBuilder::createOptionCheck(Value* option, const std::string& name) {
    auto* op = module_.createOptionCheck(option, name);
    insert(op);
    return op;
}

// ============================================================================
// Control Flow Instructions
// ============================================================================

void IRBuilder::createRet(Value* value) {
    auto* ret = module_.createReturn(value);
    insert(ret);
    clearInsertionPoint();
}

void IRBuilder::createBr(BasicBlock* dest) {
    auto* br = module_.createBranch(dest);
    insert(br);
    // CFG edges are automatically added when the branch is inserted
    clearInsertionPoint();
}

void IRBuilder::createCondBr(Value* condition, BasicBlock* trueBlock, BasicBlock* falseBlock) {
    auto* condBr = module_.createCondBranch(condition, trueBlock, falseBlock);

    insert(condBr);

    // CFG edges are automatically added when the conditional branch is inserted

    clearInsertionPoint();
}

Value* IRBuilder::createSwitch(Value* value, BasicBlock* defaultDest,
                               const std::vector<SwitchInst::CaseEntry>& cases) {
    (void)value; (void)defaultDest; (void)cases;  // TODO: Implement
    return nullptr;
}

// ============================================================================
// Function Call Instructions
// ============================================================================

Value* IRBuilder::createCall(Function* callee, const std::vector<Value*>& args,
                            const std::string& name) {
    if (args.size() != callee->getNumParams()) {
        // Error: wrong number of arguments
        return nullptr;
    }

    // 2. Validate argument types
    for (size_t i = 0; i < args.size(); i++) {
        // Compare types by value, not by shared_ptr address
        if (!args[i]->getType()->equals(callee->getParam(i)->getType().get())) {
            // Error: wrong type for argument i
            return nullptr;
        }
    }

    // 3. Create call instruction
    auto* call = module_.createCall(callee, args, name);

    // 4. Insert it
    insert(call);

    return call;
}

Value* IRBuilder::createCallIndirect(Value* callee, const std::vector<Value*>& args,
                                    const std::string& name) {
    (void)callee; (void)args; (void)name;  // TODO: Implement
    return nullptr;
}

// ============================================================================
// SSA Phi Node
// ============================================================================

Value* IRBuilder::createPhi(std::shared_ptr<IRType> type,
                           const std::vector<PhiNode::IncomingValue>& incomingValues,
                           const std::string& name) {
    auto* phi = module_.createPhi(type, incomingValues, name);

    // Phi nodes must be inserted at the beginning of the block, before any non-phi instructions
    if (insertionBlock_) {
        auto* savedPoint = insertionPoint_;

        Instruction* firstNonPhi = nullptr;
        for (auto* inst : insertionBlock_->getInstructions()) {
            if (!isa<PhiNode>(inst)) {
                firstNonPhi = inst;
                break;
            }
        }

        insertionPoint_ = firstNonPhi;
        insert(phi);

        insertionPoint_ = savedPoint;
    }

    return phi;
}

// ============================================================================
// High-Level Control Flow Helpers
// ============================================================================

IRBuilder::IfThenElseBlocks IRBuilder::createIfThenElse(Value* condition,
                                                        const std::string& thenName,
                                                        const std::string& elseName,
                                                        const std::string& mergeName) {
    (void)insertionBlock_;  // Not needed for this operation

    BasicBlock* thenBlock = createBasicBlock(thenName);
    BasicBlock* elseBlock = createBasicBlock(elseName);
    BasicBlock* mergeBlock = createBasicBlock(mergeName);

    createCondBr(condition, thenBlock, elseBlock);

    return {thenBlock, elseBlock, mergeBlock};
}

IRBuilder::LoopBlocks IRBuilder::createLoop(const std::string& headerName,
                                           const std::string& bodyName,
                                           const std::string& exitName) {
    BasicBlock* headerBlock = createBasicBlock(headerName);
    BasicBlock* bodyBlock = createBasicBlock(bodyName);
    BasicBlock* exitBlock = createBasicBlock(exitName);

    createBr(headerBlock);

    return {headerBlock, bodyBlock, exitBlock};
}

// ============================================================================
// SSA Variable Management
// ============================================================================

Value* IRBuilder::createVariable(const std::string& name, std::shared_ptr<IRType> type,
                                Value* initialValue) {
    auto* var = createAlloca(type, name);

    // 2. If initial value provided, store it
    if (initialValue) {
        createStore(initialValue, var);
    }

    return var;
}

Value* IRBuilder::readVariable(Value* varPtr, const std::string& name) {
    return createLoad(varPtr, name);
}

void IRBuilder::writeVariable(Value* varPtr, Value* value) {
    createStore(value, varPtr);
}

// ============================================================================
// Named Value Tracking
// ============================================================================

void IRBuilder::setNamedValue(const std::string& name, Value* value) {
    namedValues_[name] = value;
}

Value* IRBuilder::getNamedValue(const std::string& name) const {
    auto it = namedValues_.find(name);
    return (it != namedValues_.end()) ? it->second : nullptr;
}

void IRBuilder::clearNamedValues() {
    namedValues_.clear();
}

void IRBuilder::removeNamedValue(const std::string& name) {
    namedValues_.erase(name);
}

// ============================================================================
// Validation and Debugging
// ============================================================================

Function* IRBuilder::getCurrentFunction() const {
    if (insertionBlock_) {
        return insertionBlock_->getParent();
    }
    return nullptr;
}

bool IRBuilder::validate(std::string* error) const {
    (void)error;  // TODO: Implement
    return true;
}

// ============================================================================
// Helper Methods
// ============================================================================

void IRBuilder::insert(Instruction* inst) {
    if (insertionBlock_ == nullptr) return;

    inst->setParent(insertionBlock_);

    if (insertionPoint_ == nullptr) insertionBlock_->addInstruction(inst);
    else insertionBlock_->insertBefore(inst, insertionPoint_);
}

bool IRBuilder::validateBinaryOp(Value* lhs, Value* rhs, const std::string& opName,
                                std::string* error) const {
    (void)lhs; (void)rhs; (void)opName; (void)error;  // TODO: Implement
    return true;
}

bool IRBuilder::validatePointerOp(Value* ptr, const std::string& opName,
                                 std::string* error) const {
    (void)ptr; (void)opName; (void)error;  // TODO: Implement
    return true;
}

} // namespace volta::ir
