#include "IR/IRBuilder.hpp"
#include <stdexcept>

namespace volta::ir {

// ============================================================================
// Function & Basic Block Creation
// ============================================================================

std::unique_ptr<Function> IRBuilder::createFunction(
    const std::string& name,
    std::shared_ptr<semantic::FunctionType> type) {
    std::vector<std::unique_ptr<Parameter>> parameters;
    
    for (size_t i = 0; i < type->paramTypes().size(); i++) {
        parameters.push_back(
            std::move(createParameter(
                type->paramTypes()[i],
                "%param" + std::to_string(i),
                i   
            ))
        );
    }

    return std::make_unique<Function>(name, type, std::move(parameters));
}

std::unique_ptr<BasicBlock> IRBuilder::createBasicBlock(
    const std::string& name,
    Function* parent) {

    return std::make_unique<BasicBlock>(name, parent);
}

// ============================================================================
// Arithmetic Instructions
// ============================================================================

Value* IRBuilder::createAdd(Value* left, Value* right) {
    return createBinaryOp(Instruction::Opcode::Add, left, right);
}

Value* IRBuilder::createSub(Value* left, Value* right) {
    return createBinaryOp(Instruction::Opcode::Sub, left, right);
}

Value* IRBuilder::createMul(Value* left, Value* right) {
    return createBinaryOp(Instruction::Opcode::Mul, left, right);
}

Value* IRBuilder::createDiv(Value* left, Value* right) {
    return createBinaryOp(Instruction::Opcode::Div, left, right);
}

Value* IRBuilder::createMod(Value* left, Value* right) {
    return createBinaryOp(Instruction::Opcode::Mod, left, right);
}

Value* IRBuilder::createNeg(Value* operand) {
    return createUnaryOp(Instruction::Opcode::Neg, operand);
}

// ============================================================================
// Comparison Instructions
// ============================================================================

Value* IRBuilder::createEq(Value* left, Value* right) {
    return createBinaryOp(Instruction::Opcode::Eq, left, right);
}

Value* IRBuilder::createNe(Value* left, Value* right) {
    return createBinaryOp(Instruction::Opcode::Ne, left, right);
}

Value* IRBuilder::createLt(Value* left, Value* right) {
    return createBinaryOp(Instruction::Opcode::Lt, left, right);
}

Value* IRBuilder::createLe(Value* left, Value* right) {
    return createBinaryOp(Instruction::Opcode::Le, left, right);
}

Value* IRBuilder::createGt(Value* left, Value* right) {
    return createBinaryOp(Instruction::Opcode::Gt, left, right);
}

Value* IRBuilder::createGe(Value* left, Value* right) {
    return createBinaryOp(Instruction::Opcode::Ge, left, right);
}

// ============================================================================
// Logical Instructions
// ============================================================================

Value* IRBuilder::createAnd(Value* left, Value* right) {
    return createBinaryOp(Instruction::Opcode::And, left, right);
}

Value* IRBuilder::createOr(Value* left, Value* right) {
    return createBinaryOp(Instruction::Opcode::Or, left, right);
}

Value* IRBuilder::createNot(Value* operand) {
    return createUnaryOp(Instruction::Opcode::Not, operand);
}

// ============================================================================
// Memory Instructions
// ============================================================================

Value* IRBuilder::createAlloc(std::shared_ptr<semantic::Type> type) {
    std::string name = getUniqueTempName();
    auto inst = std::make_unique<AllocInst>(type, name);
    return insert(std::move(inst));
}

Value* IRBuilder::createLoad(std::shared_ptr<semantic::Type> type, Value* address) {
    std::string name = getUniqueTempName();
    auto inst = std::make_unique<LoadInst>(type, name, address);
    return insert(std::move(inst));
}

void IRBuilder::createStore(Value* value, Value* address) {
    auto inst = std::make_unique<StoreInst>(value, address);
    insert(std::move(inst));
}

Value* IRBuilder::createGetField(Value* object, size_t fieldIndex,
                                 std::shared_ptr<semantic::Type> fieldType) {
    std::string name = getUniqueTempName();
    auto inst = std::make_unique<GetFieldInst>(fieldType, name, object, fieldIndex);
    return insert(std::move(inst));
}

void IRBuilder::createSetField(Value* object, size_t fieldIndex, Value* value) {
    auto inst = std::make_unique<SetFieldInst>(object, fieldIndex, value);
    insert(std::move(inst));
}

Value* IRBuilder::createGetElement(Value* array, Value* index,
                                  std::shared_ptr<semantic::Type> elementType) {
    std::string name = getUniqueTempName();
    auto inst = std::make_unique<GetElementInst>(elementType, name, array, index);
    return insert(std::move(inst));
}

void IRBuilder::createSetElement(Value* array, Value* index, Value* value) {
    auto inst = std::make_unique<SetElementInst>(array, index, value);
    insert(std::move(inst));
}

Value* IRBuilder::createNewArray(std::shared_ptr<semantic::Type> arrayType,
                                 const std::vector<Value*>& elements) {
    std::string name = getUniqueTempName();
    auto inst = std::make_unique<NewArrayInst>(arrayType, name, elements);
    return insert(std::move(inst));
}

// ============================================================================
// Control Flow Instructions
// ============================================================================

void IRBuilder::createBr(BasicBlock* target) {
    // TODO: Create an unconditional branch
    //
    // Steps:
    // 1. Create BranchInst: auto inst = std::make_unique<BranchInst>(target)
    // 2. Insert: insert(std::move(inst))

    auto inst = std::make_unique<BranchInst>(target);
    insert(std::move(inst));
}

void IRBuilder::createBrIf(Value* condition, BasicBlock* thenBlock, BasicBlock* elseBlock) {
    auto inst = std::make_unique<BranchIfInst>(condition, thenBlock, elseBlock);
    insert(std::move(inst));
}

void IRBuilder::createRetVoid() {
    auto inst = std::make_unique<ReturnInst>(nullptr);
    insert(std::move(inst));
}

void IRBuilder::createRet(Value* value) {
    auto inst = std::make_unique<ReturnInst>(value);
    insert(std::move(inst));
}

Value* IRBuilder::createCall(Function* callee, const std::vector<Value*>& arguments) {
    auto retType = callee->type()->returnType();
    std::unique_ptr<CallInst> inst;
    if (retType->kind() == semantic::PrimitiveType::Kind::Void) {
        inst = std::make_unique<CallInst>(retType, "", callee, arguments);
    } else {
        std::string name = getUniqueTempName();
        inst = std::make_unique<CallInst>(retType, name, callee, arguments);
    }

    return insert(std::move(inst));
}

Value* IRBuilder::createCallForeign(const std::string& foreignName,
                                   std::shared_ptr<semantic::Type> returnType,
                                   const std::vector<Value*>& arguments) {
    std::string name = getUniqueTempName();
    auto inst = std::make_unique<CallForeignInst>(returnType, name, foreignName, arguments);
    return insert(std::move(inst));
}

// ============================================================================
// Constant Creation
// ============================================================================

Constant* IRBuilder::getInt(int64_t value) {
    auto it = intConstants_.find(value);

    if (it != intConstants_.end()) {
        return it->second.get();
    } else {
        auto intType = std::make_shared<semantic::PrimitiveType>(
            semantic::PrimitiveType::PrimitiveKind::Int);
        auto c = std::make_unique<Constant>(intType, value);
        Constant* ptr = c.get();
        intConstants_[value] = std::move(c);
        return ptr; 
    }
}

Constant* IRBuilder::getFloat(double value) {
    auto it = floatConstants_.find(value);

    if (it != floatConstants_.end()) {
        return it->second.get();
    } else {
        auto floatType = std::make_shared<semantic::PrimitiveType>(
            semantic::PrimitiveType::PrimitiveKind::Float);
        auto c = std::make_unique<Constant>(floatType, value);
        Constant* ptr = c.get();
        floatConstants_[value] = std::move(c);
        return ptr; 
    }
}

Constant* IRBuilder::getBool(bool value) {
    auto it = boolConstants_.find(value);

    if (it != boolConstants_.end()) {
        return it->second.get();
    } else {
        auto boolType = std::make_shared<semantic::PrimitiveType>(
            semantic::PrimitiveType::PrimitiveKind::Bool);
        auto c = std::make_unique<Constant>(boolType, value);
        Constant* ptr = c.get();
        boolConstants_[value] = std::move(c);
        return ptr; 
    }
}

Constant* IRBuilder::getString(const std::string& value) {
    auto it = stringConstants_.find(value);

    if (it != stringConstants_.end()) {
        return it->second.get();
    } else {
        auto stringType = std::make_shared<semantic::PrimitiveType>(
            semantic::PrimitiveType::PrimitiveKind::String);
        auto c = std::make_unique<Constant>(stringType, value);
        Constant* ptr = c.get();
        stringConstants_[value] = std::move(c);
        return ptr; 
    }
}

Constant* IRBuilder::getNone() {
    if (noneConstant_ == nullptr) {
        auto noneType = std::make_shared<semantic::PrimitiveType>(
            semantic::PrimitiveType::PrimitiveKind::Void);
        noneConstant_ = std::make_unique<Constant>(noneType, std::monostate{});
    }
    return noneConstant_.get();
}

// ============================================================================
// Parameter Creation
// ============================================================================

std::unique_ptr<Parameter> IRBuilder::createParameter(
    std::shared_ptr<semantic::Type> type,
    const std::string& name,
    size_t index) {
    return std::make_unique<Parameter>(type, name, index);
}

// ============================================================================
// Helper Methods
// ============================================================================

std::string IRBuilder::getUniqueTempName() {
    std::string name = "%" + std::to_string(nextTempId_);
    nextTempId_++;
    return name;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

Value* IRBuilder::createBinaryOp(Instruction::Opcode opcode, Value* left, Value* right) {
    std::string name = getUniqueTempName();
    std::shared_ptr<semantic::Type> type;

    switch (opcode) {
        case Instruction::Opcode::Eq:
        case Instruction::Opcode::Ne:
        case Instruction::Opcode::Lt:
        case Instruction::Opcode::Le:
        case Instruction::Opcode::Gt:
        case Instruction::Opcode::Ge:
            type = std::make_shared<semantic::PrimitiveType>(
                semantic::PrimitiveType::PrimitiveKind::Bool);
            break;
        default:
            type = left->type();
            break;
    }

    auto inst = std::make_unique<BinaryInst>(opcode, type, name, left, right);
    return insert(std::move(inst));
}

Value* IRBuilder::createUnaryOp(Instruction::Opcode opcode, Value* operand) {
    std::string name = getUniqueTempName();
    std::shared_ptr<semantic::Type> type = operand->type();
    auto inst = std::make_unique<UnaryInst>(opcode, type, name, operand);
    return insert(std::move(inst));
}

}
