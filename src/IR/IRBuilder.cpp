#include "IR/IRBuilder.hpp"
#include <stdexcept>

namespace volta::ir {

// ============================================================================
// Function & Basic Block Creation
// ============================================================================

std::unique_ptr<Function> IRBuilder::createFunction(
    const std::string& name,
    std::shared_ptr<semantic::FunctionType> type) {
    std::vector<Parameter*> parameters;

    for (size_t i = 0; i < type->paramTypes().size(); i++) {
        // Create parameter via module (ownership stays in module)
        Parameter* param = createParameter(
            type->paramTypes()[i],
            "%param" + std::to_string(i),
            i
        );
        parameters.push_back(param);
    }

    return std::make_unique<Function>(name, type, std::move(parameters));
}

BasicBlock* IRBuilder::createBasicBlock(
    const std::string& name,
    Function* parent) {

    return module_->createBasicBlock(name, parent);
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
    auto* inst = module_->createInstruction<AllocInst>(type, name);
    return insert(inst);
}

Value* IRBuilder::createLoad(std::shared_ptr<semantic::Type> type, Value* address) {
    std::string name = getUniqueTempName();
    auto* inst = module_->createInstruction<LoadInst>(type, name, address);
    return insert(inst);
}

void IRBuilder::createStore(Value* value, Value* address) {
    auto* inst = module_->createInstruction<StoreInst>(value, address);
    insert(inst);
}

Value* IRBuilder::createGetField(Value* object, size_t fieldIndex,
                                 std::shared_ptr<semantic::Type> fieldType) {
    std::string name = getUniqueTempName();
    auto* inst = module_->createInstruction<GetFieldInst>(fieldType, name, object, fieldIndex);
    return insert(inst);
}

void IRBuilder::createSetField(Value* object, size_t fieldIndex, Value* value) {
    auto* inst = module_->createInstruction<SetFieldInst>(object, fieldIndex, value);
    insert(inst);
}

Value* IRBuilder::createGetElement(Value* array, Value* index,
                                  std::shared_ptr<semantic::Type> elementType) {
    std::string name = getUniqueTempName();
    auto* inst = module_->createInstruction<GetElementInst>(elementType, name, array, index);
    return insert(inst);
}

void IRBuilder::createSetElement(Value* array, Value* index, Value* value) {
    auto* inst = module_->createInstruction<SetElementInst>(array, index, value);
    insert(inst);
}

Value* IRBuilder::createNewArray(std::shared_ptr<semantic::Type> arrayType,
                                 const std::vector<Value*>& elements) {
    std::string name = getUniqueTempName();
    auto* inst = module_->createInstruction<NewArrayInst>(arrayType, name, elements);
    return insert(inst);
}

// ============================================================================
// Control Flow Instructions
// ============================================================================

void IRBuilder::createBr(BasicBlock* target) {
    auto* inst = module_->createInstruction<BranchInst>(target);
    insert(inst);
}

void IRBuilder::createBrIf(Value* condition, BasicBlock* thenBlock, BasicBlock* elseBlock) {
    auto* inst = module_->createInstruction<BranchIfInst>(condition, thenBlock, elseBlock);
    insert(inst);
}

void IRBuilder::createRetVoid() {
    auto* inst = module_->createInstruction<ReturnInst>(nullptr);
    insert(inst);
}

void IRBuilder::createRet(Value* value) {
    auto* inst = module_->createInstruction<ReturnInst>(value);
    insert(inst);
}

Value* IRBuilder::createCall(Function* callee, const std::vector<Value*>& arguments) {
    auto retType = callee->type()->returnType();
    CallInst* inst;
    if (retType->kind() == semantic::PrimitiveType::Kind::Void) {
        inst = module_->createInstruction<CallInst>(retType, "", callee, arguments);
    } else {
        std::string name = getUniqueTempName();
        inst = module_->createInstruction<CallInst>(retType, name, callee, arguments);
    }

    return insert(inst);
}

Value* IRBuilder::createCallForeign(const std::string& foreignName,
                                   std::shared_ptr<semantic::Type> returnType,
                                   const std::vector<Value*>& arguments) {
    std::string name = getUniqueTempName();
    auto* inst = module_->createInstruction<CallForeignInst>(returnType, name, foreignName, arguments);
    return insert(inst);
}

// ============================================================================
// Constant Creation
// ============================================================================

Constant* IRBuilder::getInt(int64_t value) {
    auto intType = std::make_shared<semantic::PrimitiveType>(
        semantic::PrimitiveType::PrimitiveKind::Int);
    return module_->createConstant(intType, value);
}

Constant* IRBuilder::getFloat(double value) {
    auto floatType = std::make_shared<semantic::PrimitiveType>(
        semantic::PrimitiveType::PrimitiveKind::Float);
    return module_->createConstant(floatType, value);
}

Constant* IRBuilder::getBool(bool value) {
    auto boolType = std::make_shared<semantic::PrimitiveType>(
        semantic::PrimitiveType::PrimitiveKind::Bool);
    return module_->createConstant(boolType, value);
}

Constant* IRBuilder::getString(const std::string& value) {
    auto stringType = std::make_shared<semantic::PrimitiveType>(
        semantic::PrimitiveType::PrimitiveKind::String);
    return module_->createConstant(stringType, value);
}

Constant* IRBuilder::getNone() {
    auto noneType = std::make_shared<semantic::PrimitiveType>(
        semantic::PrimitiveType::PrimitiveKind::Void);
    return module_->createConstant(noneType, std::monostate{});
}

// ============================================================================
// Parameter Creation
// ============================================================================

Parameter* IRBuilder::createParameter(
    std::shared_ptr<semantic::Type> type,
    const std::string& name,
    size_t index) {
    return module_->createParameter(type, name, index);
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

    auto* inst = module_->createInstruction<BinaryInst>(opcode, type, name, left, right);
    return insert(inst);
}

Value* IRBuilder::createUnaryOp(Instruction::Opcode opcode, Value* operand) {
    std::string name = getUniqueTempName();
    std::shared_ptr<semantic::Type> type = operand->type();
    auto* inst = module_->createInstruction<UnaryInst>(opcode, type, name, operand);
    return insert(inst);
}

}
