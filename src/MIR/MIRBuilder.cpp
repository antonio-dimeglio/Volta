#include "MIR/MIRBuilder.hpp"
#include "MIR/EscapeAnalysis.hpp"
#include <iostream>

namespace MIR {

void MIRBuilder::createFunction(const std::string& name,
                                 const std::vector<Value>& params,
                                 const Type::Type* returnType) {
    Function fn(name, params, returnType);
    program.addFunction(fn);

    currentFunction = &program.functions.back();
    nextValueId = 0;
    symbolTable.clear();

    for (const auto& param : params) {
        symbolTable[param.name] = param;
    }
}

void MIRBuilder::finishFunction() {
    if (currentFunction != nullptr) {
        // Run escape analysis on the function
        EscapeInfo escapeInfo(64);  // 64-byte stack size threshold
        EscapeAnalyzer analyzer(escapeInfo);
        analyzer.analyze(*currentFunction);

        // Transform Alloca -> SAlloca/HAlloca based on escape analysis
        AllocationTransformer transformer(escapeInfo);
        transformer.transform(*currentFunction);
    }

    currentFunction = nullptr;
    currentBlock = nullptr;
}

void MIRBuilder::createBlock(const std::string& label) {
    BasicBlock bb(label);
    currentFunction->addBlock(bb);
    currentBlock = &currentFunction->blocks.back();
}

void MIRBuilder::setInsertionPoint(const std::string& blockLabel) {
    BasicBlock* bb = currentFunction->getBlock(blockLabel);
    if (bb != nullptr) {
        currentBlock = bb;
    } else {
        throw std::runtime_error("Failed to set insertion point. the following block doesnt exist: " + blockLabel);
    }
}

std::string MIRBuilder::getCurrentBlockLabel() const {
    return (currentBlock != nullptr) ? currentBlock->label : "";
}

bool MIRBuilder::currentBlockTerminated() const {
    return (currentBlock != nullptr) && currentBlock->hasTerminator();
}

Value MIRBuilder::createTemporary(const Type::Type* type) {
    std::string name = std::to_string(nextValueId++);
    return Value::makeLocal(name, type);
}

Value MIRBuilder::createNamedValue(const std::string& name, const Type::Type* type) {
    std::string actual = name + std::to_string(nextValueId++);
    return Value::makeLocal(actual, type);
}

Value MIRBuilder::createConstantInt(int64_t value, const Type::Type* type) {
    return Value::makeConstantInt(value, type);
}

Value MIRBuilder::createConstantBool(bool value, const Type::Type* type) {
    return Value::makeConstantBool(value, type);
}

Value MIRBuilder::createConstantString(const std::string& value, const Type::Type* type) {
    return Value::makeConstantString(value, type);
}

Value MIRBuilder::createConstantFloat(double value, const Type::Type* type) {
    return Value::makeConstantFloat(value, type);
}

Value MIRBuilder::createConstantNull(const Type::Type* ptrType) {
    return Value::makeConstantNull(ptrType);
}


Value MIRBuilder::createIAdd(const Value& lhs, const Value& rhs) {
    auto result = createTemporary(lhs.type);
    auto instr = Instruction(Opcode::IAdd, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createISub(const Value& lhs, const Value& rhs) {
    auto result = createTemporary(lhs.type);
    auto instr = Instruction(Opcode::ISub, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createIMul(const Value& lhs, const Value& rhs) {
    auto result = createTemporary(lhs.type);
    auto instr = Instruction(Opcode::IMul, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createIDiv(const Value& lhs, const Value& rhs) {
    auto result = createTemporary(lhs.type);
    auto instr = Instruction(Opcode::IDiv, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createIRem(const Value& lhs, const Value& rhs) {
    auto result = createTemporary(lhs.type);
    auto instr = Instruction(Opcode::IRem, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createUDiv(const Value& lhs, const Value& rhs) {
    auto result = createTemporary(lhs.type);
    auto instr = Instruction(Opcode::UDiv, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createURem(const Value& lhs, const Value& rhs) {
    auto result = createTemporary(lhs.type);
    auto instr = Instruction(Opcode::URem, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createFAdd(const Value& lhs, const Value& rhs) {
    auto result = createTemporary(lhs.type);
    auto instr = Instruction(Opcode::FAdd, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createFSub(const Value& lhs, const Value& rhs) {
    auto result = createTemporary(lhs.type);
    auto instr = Instruction(Opcode::FSub, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createFMul(const Value& lhs, const Value& rhs) {
    auto result = createTemporary(lhs.type);
    auto instr = Instruction(Opcode::FMul, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createFDiv(const Value& lhs, const Value& rhs) {
    auto result = createTemporary(lhs.type);
    auto instr = Instruction(Opcode::FDiv, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createICmpEQ(const Value& lhs, const Value& rhs) {
    const Type::Type* boolType = typeRegistry.getPrimitive(Type::PrimitiveKind::Bool);
    auto result = createTemporary(boolType);
    auto instr = Instruction(Opcode::ICmpEQ, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createICmpNE(const Value& lhs, const Value& rhs) {
    const Type::Type* boolType = typeRegistry.getPrimitive(Type::PrimitiveKind::Bool);
    auto result = createTemporary(boolType);
    auto instr = Instruction(Opcode::ICmpNE, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createICmpLT(const Value& lhs, const Value& rhs) {
    const Type::Type* boolType = typeRegistry.getPrimitive(Type::PrimitiveKind::Bool);
    auto result = createTemporary(boolType);
    auto instr = Instruction(Opcode::ICmpLT, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createICmpLE(const Value& lhs, const Value& rhs) {
    const Type::Type* boolType = typeRegistry.getPrimitive(Type::PrimitiveKind::Bool);
    auto result = createTemporary(boolType);
    auto instr = Instruction(Opcode::ICmpLE, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createICmpGT(const Value& lhs, const Value& rhs) {
    const Type::Type* boolType = typeRegistry.getPrimitive(Type::PrimitiveKind::Bool);
    auto result = createTemporary(boolType);
    auto instr = Instruction(Opcode::ICmpGT, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createICmpGE(const Value& lhs, const Value& rhs) {
    const Type::Type* boolType = typeRegistry.getPrimitive(Type::PrimitiveKind::Bool);
    auto result = createTemporary(boolType);
    auto instr = Instruction(Opcode::ICmpGE, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createICmpULT(const Value& lhs, const Value& rhs) {
    const Type::Type* boolType = typeRegistry.getPrimitive(Type::PrimitiveKind::Bool);
    auto result = createTemporary(boolType);
    auto instr = Instruction(Opcode::ICmpULT, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createICmpULE(const Value& lhs, const Value& rhs) {
    const Type::Type* boolType = typeRegistry.getPrimitive(Type::PrimitiveKind::Bool);
    auto result = createTemporary(boolType);
    auto instr = Instruction(Opcode::ICmpULE, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createICmpUGT(const Value& lhs, const Value& rhs) {
    const Type::Type* boolType = typeRegistry.getPrimitive(Type::PrimitiveKind::Bool);
    auto result = createTemporary(boolType);
    auto instr = Instruction(Opcode::ICmpUGT, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createICmpUGE(const Value& lhs, const Value& rhs) {
    const Type::Type* boolType = typeRegistry.getPrimitive(Type::PrimitiveKind::Bool);
    auto result = createTemporary(boolType);
    auto instr = Instruction(Opcode::ICmpUGE, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createFCmpEQ(const Value& lhs, const Value& rhs) {
    const Type::Type* boolType = typeRegistry.getPrimitive(Type::PrimitiveKind::Bool);
    auto result = createTemporary(boolType);
    auto instr = Instruction(Opcode::FCmpEQ, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createFCmpNE(const Value& lhs, const Value& rhs) {
    const Type::Type* boolType = typeRegistry.getPrimitive(Type::PrimitiveKind::Bool);
    auto result = createTemporary(boolType);
    auto instr = Instruction(Opcode::FCmpNE, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createFCmpLT(const Value& lhs, const Value& rhs) {
    const Type::Type* boolType = typeRegistry.getPrimitive(Type::PrimitiveKind::Bool);
    auto result = createTemporary(boolType);
    auto instr = Instruction(Opcode::FCmpLT, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createFCmpLE(const Value& lhs, const Value& rhs) {
    const Type::Type* boolType = typeRegistry.getPrimitive(Type::PrimitiveKind::Bool);
    auto result = createTemporary(boolType);
    auto instr = Instruction(Opcode::FCmpLE, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createFCmpGT(const Value& lhs, const Value& rhs) {
    const Type::Type* boolType = typeRegistry.getPrimitive(Type::PrimitiveKind::Bool);
    auto result = createTemporary(boolType);
    auto instr = Instruction(Opcode::FCmpGT, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createFCmpGE(const Value& lhs, const Value& rhs) {
    const Type::Type* boolType = typeRegistry.getPrimitive(Type::PrimitiveKind::Bool);
    auto result = createTemporary(boolType);
    auto instr = Instruction(Opcode::FCmpGE, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createAnd(const Value& lhs, const Value& rhs) {
    auto result = createTemporary(lhs.type);
    auto instr = Instruction(Opcode::And, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createOr(const Value& lhs, const Value& rhs) {
    auto result = createTemporary(lhs.type);
    auto instr = Instruction(Opcode::Or, result, {lhs, rhs});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createNot(const Value& operand) {
    auto result = createTemporary(operand.type);
    auto instr = Instruction(Opcode::Not, result, {operand});
    currentBlock->addInstruction(instr);
    return result;
}


Value MIRBuilder::createConversion(Opcode op, const Value& value, const Type::Type* targetType) {
    auto result = createTemporary(targetType);
    auto instr = Instruction(op, result, {value});
    currentBlock->addInstruction(instr);
    return result;
}


Value MIRBuilder::createAlloca(const Type::Type* type) {
    const Type::Type* ptrType = typeRegistry.getPointer(type);
    Value result = createTemporary(ptrType);
    Instruction inst(Opcode::Alloca, result, {});
    currentBlock->addInstruction(inst);
    return result;
}

Value MIRBuilder::createLoad(const Value& pointer) {
    if (pointer.type == nullptr) {
        // No type information - just return the pointer as-is
        return pointer;
    }
    if (pointer.type->kind != Type::TypeKind::Pointer) {
        // If it's not a pointer (e.g., an array), just return it as-is
        // Arrays are passed by reference in Volta
        return pointer;
    }
    const auto* ptrType = dynamic_cast<const Type::PointerType*>(pointer.type);
    Value result = createTemporary(ptrType->pointeeType);
    Instruction instr(Opcode::Load, result, {pointer});
    currentBlock->addInstruction(instr);
    return result;
}

void MIRBuilder::createStore(const Value& value, const Value& pointer) {
    Instruction instr(Opcode::Store, Value(), {value, pointer});
    currentBlock->addInstruction(instr);
}

Value MIRBuilder::createGetElementPtr(const Value& arrayPtr, const Value& index) {
    const auto* ptrType = dynamic_cast<const Type::PointerType*>(arrayPtr.type);
    const auto* arrayType = dynamic_cast<const Type::ArrayType*>(ptrType->pointeeType);
    const Type::Type* elemPtrType = typeRegistry.getPointer(arrayType->elementType);

    Value result = createTemporary(elemPtrType);
    Instruction instr(Opcode::GetElementPtr, result, {arrayPtr, index});
    currentBlock->addInstruction(instr);
    return result;
}

Value MIRBuilder::createGetFieldPtr(const Value& structPtr, int fieldIndex) {
    const auto* ptrType = dynamic_cast<const Type::PointerType*>(structPtr.type);
    const auto* structType = dynamic_cast<const Type::StructType*>(ptrType->pointeeType);
    const Type::Type* fieldType = structType->fields[fieldIndex].type;
    const Type::Type* resultType = typeRegistry.getPointer(fieldType);

    Value result = createTemporary(resultType);
    Value indexValue = createConstantInt(fieldIndex, typeRegistry.getPrimitive(Type::PrimitiveKind::I32));
    
    Instruction inst(Opcode::GetFieldPtr, result, {structPtr, indexValue});
    
    if (currentBlock != nullptr) {
        currentBlock->instructions.push_back(inst);
    }

    return result;
}

Value MIRBuilder::createCall(const std::string& functionName,
                              const std::vector<Value>& args,
                              const Type::Type* returnType) {
    Value result;

    bool isVoid = false;
    if (returnType->kind == Type::TypeKind::Primitive) {
        const auto* primType = dynamic_cast<const Type::PrimitiveType*>(returnType);
        isVoid = (primType->kind == Type::PrimitiveKind::Void);
    }

    if (!isVoid) {
        result = createTemporary(returnType);
    }

    Instruction instr(Opcode::Call, result, args);
    instr.callTarget = functionName;

    currentBlock->addInstruction(instr);
    return result;
}

void MIRBuilder::createReturn(const Value& value) {
    Terminator term = Terminator::makeReturn(value);
    currentBlock->setTerminator(term);
}

void MIRBuilder::createReturnVoid() {
    Terminator term = Terminator::makeReturn(std::nullopt);
    currentBlock->setTerminator(term);
}

void MIRBuilder::createBranch(const std::string& targetLabel) {
    Terminator term = Terminator::makeBranch(targetLabel);
    currentBlock->setTerminator(term);
}

void MIRBuilder::createCondBranch(const Value& condition,
                                   const std::string& trueLabel,
                                   const std::string& falseLabel) {
    Terminator term = Terminator::makeCondBranch(condition, trueLabel, falseLabel);
    currentBlock->setTerminator(term);
}

void MIRBuilder::setVariable(const std::string& name, const Value& value) {
    symbolTable[name] = value;
}

Value MIRBuilder::getVariable(const std::string& name) const {
    auto it = symbolTable.find(name);
    
    if (it == symbolTable.end()) {
        throw std::runtime_error("Tried to access non-existant variable during MIR.");
    }

    return it->second;
}

bool MIRBuilder::hasVariable(const std::string& name) const {
    return symbolTable.count(name) > 0;
}

void MIRBuilder::clearSymbolTable() {
    symbolTable.clear();
}

} // namespace MIR
