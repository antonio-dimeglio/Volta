#include "vm/BytecodeCompiler.hpp"
#include "vm/RuntimeFunction.hpp"
#include <cassert>
#include <stdexcept>
#include <iostream>

namespace volta::vm {
std::unique_ptr<BytecodeModule> BytecodeCompiler::compile(
    std::unique_ptr<ir::Module> mod
) {
    irModule_ = std::move(mod);
    module_ = std::make_unique<BytecodeModule>();

    registerRuntimeFunctions();

    // PASS 1: Register all function declarations (names + param counts)
    // This allows forward references (function A can call function B even if B is defined later)
    // Skip declarations (native functions) - they're already registered via registerRuntimeFunctions()
    for (auto* func : irModule_->getFunctions()) {
        // Skip function declarations (native/external functions with no body)
        if (func->isDeclaration()) {
            continue;
        }

        size_t numParams = func->getNumParams();
        if (numParams > 255) {
            throw std::runtime_error("Function has too many parameters (max 255)");
        }
        module_->addFunctionDeclaration(func->getName(), numParams);
    }

    // PASS 2: Compile the actual bytecode for each function
    for (auto* func : irModule_->getFunctions()) {
        // Skip function declarations (native/external functions with no body)
        if (func->isDeclaration()) {
            continue;
        }
        compileFunction(func);
    }

    std::string verifyError;
    if (!module_->verify(&verifyError)) {
        throw std::runtime_error("Generated invalid bytecode module: " + verifyError);
    }

    return std::move(module_);
}

void BytecodeCompiler::compileFunction(ir::Function* func) {
    currentFunction_ = std::make_unique<FunctionCompilationContext>(
        func,
        module_->getCurrentOffset()
    );

    // Emit moves to copy incoming parameters from r0, r1, ... to their allocated registers
    // This prevents parameters from being clobbered when we use r0, r1, ... for function calls
    for (size_t i = 0; i < func->getNumParams(); i++) {
        auto* param = func->getParam(i);
        byte paramReg = i;                                    // Incoming param in r0, r1, r2, ...
        byte allocatedReg = currentFunction_->valueToRegister[param];  // Allocated safe register

        if (paramReg != allocatedReg) {
            emitMove(allocatedReg, paramReg);
        }
    }

    for (auto* block : func->getBlocks()) {
        compileBasicBlock(block);
    }

    resolveFixups();

    // Get the function index (already registered in Pass 1)
    index funcIdx = module_->getFunctionIndex(func->getName());

    // Update the function with the compiled bytecode info
    module_->updateFunctionCode(
        funcIdx,
        currentFunction_->functionStartOffset,
        module_->getCurrentOffset() - currentFunction_->functionStartOffset,
        currentFunction_->nextRegister
    );

    fixups_.clear();
    currentFunction_ = nullptr;
}

void BytecodeCompiler::compileBasicBlock(ir::BasicBlock* block) {
    // Skip unreachable blocks (blocks with no predecessors)
    // These are dead code and should not be compiled
    if (block->getNumPredecessors() == 0 && block != currentFunction_->irFunction->getEntryBlock()) {
        // Still record offset for the block in case there are forward references
        // but don't emit any code
        currentFunction_->blockOffsets[block] = module_->getCurrentOffset();
        return;
    }

    // 1. Record block offset for branch resolution
    currentFunction_->blockOffsets[block] = module_->getCurrentOffset();

    // 2. Compile non-terminator instructions
    for (auto* instr : block->getInstructions()) {
        if (!instr->isTerminator()) {
            compileInstruction(instr);
        }
    }

    // 3. Get the terminator to find successors
    ir::Instruction* terminator = block->getTerminator();
    if (!terminator) return;  // No terminator (shouldn't happen in valid IR)

    // 4. Emit phi moves for each successor BEFORE the branch
    for (ir::BasicBlock* successor : block->getSuccessors()) {
        emitPhiMoves(block, successor);
    }

    // 5. Compile the terminator (branch/return)
    compileInstruction(terminator);
}

void BytecodeCompiler::compileInstruction(ir::Instruction* instr) {
    switch (instr->getOpcode()) {
        case ir::Instruction::Opcode::Add: {
            compileBinaryOp(instr, &BytecodeCompiler::emitAdd);
            break;
        }
        case ir::Instruction::Opcode::Sub: {
            compileBinaryOp(instr, &BytecodeCompiler::emitSub);
            break;
        }
        case ir::Instruction::Opcode::Mul: {
            compileBinaryOp(instr, &BytecodeCompiler::emitMul);
            break;
        }
        case ir::Instruction::Opcode::Div: {
            compileBinaryOp(instr, &BytecodeCompiler::emitDiv);
            break;
        }
        case ir::Instruction::Opcode::Rem: {
            compileBinaryOp(instr, &BytecodeCompiler::emitRem);
            break;
        }
        case ir::Instruction::Opcode::Pow: {
            compileBinaryOp(instr, &BytecodeCompiler::emitPow);
            break;
        }
        case ir::Instruction::Opcode::Neg: {
            compileUnaryOp(instr, &BytecodeCompiler::emitNeg);
            break;
        }
        case ir::Instruction::Opcode::Eq: {
            compileBinaryOp(instr, &BytecodeCompiler::emitCmpEq);
            break;
        }
        case ir::Instruction::Opcode::Ne: {
            compileBinaryOp(instr, &BytecodeCompiler::emitCmpNe);
            break;
        }
        case ir::Instruction::Opcode::Lt: {
            compileBinaryOp(instr, &BytecodeCompiler::emitCmpLt);
            break;
        }        
        case ir::Instruction::Opcode::Le: {
            compileBinaryOp(instr, &BytecodeCompiler::emitCmpLe);
            break;
        }
        case ir::Instruction::Opcode::Gt: {
            compileBinaryOp(instr, &BytecodeCompiler::emitCmpGt);
            break;
        }
        case ir::Instruction::Opcode::Ge: {
            compileBinaryOp(instr, &BytecodeCompiler::emitCmpGe);
            break;
        }
        case ir::Instruction::Opcode::And: {
            compileBinaryOp(instr, &BytecodeCompiler::emitAnd);
            break;
        }
        case ir::Instruction::Opcode::Or: {
            compileBinaryOp(instr, &BytecodeCompiler::emitOr);
            break;
        }
        case ir::Instruction::Opcode::Not: {
            compileUnaryOp(instr, &BytecodeCompiler::emitNot);
            break;
        }
        case ir::Instruction::Opcode::Ret: {
            auto* retInst = static_cast<ir::ReturnInst*>(instr);

            if (retInst->hasReturnValue()) {
                byte rVal = getOrAllocateRegister(retInst->getReturnValue());
                emitRet(rVal);
            } else{
                emitRetVoid();
            }
            break;
        }
        case ir::Instruction::Opcode::Br: {
            auto* brInst = static_cast<ir::BranchInst*>(instr);
            auto* target = brInst->getDestination();

            emitBr(0);

            recordFixup(target);
            break;
        }
        case ir::Instruction::Opcode::CondBr: {
            auto* condBr = static_cast<ir::CondBranchInst*>(instr);

            byte rCond = getOrAllocateRegister(condBr->getCondition());

            auto* trueBlock = condBr->getTrueDest();
            auto* falseBlock = condBr->getFalseDest();

            emitBrIfTrue(rCond, 0);
            recordFixup(trueBlock);

            emitBr(0);  // Unconditional branch to false block (condition already false if we reach here)
            recordFixup(falseBlock);
            break;
        }
        case ir::Instruction::Opcode::Call: {
            auto* callInst = static_cast<ir::CallInst*>(instr);
            ir::Function* callee = callInst->getCallee();

            index funcIdx = module_->getFunctionIndex(callee->getName());

            setupCallArguments(callInst);

            byte rResult = getOrAllocateRegister(instr);

            emitCall(funcIdx, rResult);
            break;
        }
        case ir::Instruction::Opcode::Cast: {
            (void)static_cast<ir::CastInst*>(instr);  // Unused but keep cast for type safety

            byte rD = getOrAllocateRegister(instr);
            byte rSrc = getOrAllocateRegister(instr->getOperand(0));

            auto srcType = instr->getOperand(0)->getType();
            auto destType = instr->getType();

            if (srcType->kind() == ir::IRType::Kind::I64 &&
                destType->kind() == ir::IRType::Kind::F64) {
                emitCastIntFloat(rD, rSrc);
            } else if (srcType->kind() == ir::IRType::Kind::F64 &&
                destType->kind() == ir::IRType::Kind::I64) {
                emitCastFloatInt(rD, rSrc);
            } else throw std::runtime_error("Unsupported cast type.");
            break;
        }
        case ir::Instruction::Opcode::OptionCheck: {
            compileUnaryOp(instr, &BytecodeCompiler::emitIsSome);
            break;
        }
        case ir::Instruction::Opcode::OptionWrap: {
            byte rD = getOrAllocateRegister(instr);
            byte rVal = getOrAllocateRegister(instr->getOperand(0));
            
            emitOptionWrap(rD, rVal);
            break;
        }
        case ir::Instruction::Opcode::OptionUnwrap: {
            byte rD = getOrAllocateRegister(instr);
            byte rOpt = getOrAllocateRegister(instr->getOperand(0));
            
            emitOptionUnwrap(rD, rOpt);
            break;
        }
        case ir::Instruction::Opcode::ArrayGet: {
            compileBinaryOp(instr, &BytecodeCompiler::emitArrayGet);
            break;
        }
        case ir::Instruction::Opcode::ArraySet: {
            byte rArr = getOrAllocateRegister(instr->getOperand(0));
            byte rIdx = getOrAllocateRegister(instr->getOperand(1));
            byte rVal = getOrAllocateRegister(instr->getOperand(2));

            emitArraySet(rArr, rIdx, rVal);
            break;
        }
        case ir::Instruction::Opcode::ExtractValue: {
            auto* extractVal = dyn_cast<ir::ExtractValueInst>(instr);
            byte rD = getOrAllocateRegister(instr);
            byte rStruct = getOrAllocateRegister(extractVal->getStruct());
            byte fieldIdx = static_cast<byte>(extractVal->getFieldIndex());

            emitStructGet(rD, rStruct, fieldIdx);
            break;
        }
        case ir::Instruction::Opcode::InsertValue: {
            auto* insertVal = dyn_cast<ir::InsertValueInst>(instr);
            byte rD = getOrAllocateRegister(instr);
            byte rStruct = getOrAllocateRegister(insertVal->getStruct());
            byte rValue = getOrAllocateRegister(insertVal->getNewValue());
            byte fieldIdx = static_cast<byte>(insertVal->getFieldIndex());

            emitStructSet(rD, rStruct, rValue, fieldIdx);
            break;
        }
        case ir::Instruction::Opcode::ArrayLen: {
            compileUnaryOp(instr, &BytecodeCompiler::emitArrayLen);
            break;
        }
        case ir::Instruction::Opcode::ArrayNew: {
            byte rD = getOrAllocateRegister(instr);
            byte rSize = getOrAllocateRegister(instr->getOperand(0));
            
            emitArrayNew(rD, rSize);
            break;
        }
        case ir::Instruction::Opcode::ArraySlice: {
            byte rD = getOrAllocateRegister(instr);
            byte rArray = getOrAllocateRegister(instr->getOperand(0));
            byte rStart = getOrAllocateRegister(instr->getOperand(1));
            byte rEnd = getOrAllocateRegister(instr->getOperand(2));
            emitArraySlice(rD, rArray, rStart, rEnd);
            break;
        }
        case ir::Instruction::Opcode::Alloca: {
            getOrAllocateRegister(instr);
            break;
        }
        case ir::Instruction::Opcode::GCAlloc: {
            auto* gcAlloc = dyn_cast<ir::GCAllocInst>(instr);
            auto allocType = gcAlloc->getAllocatedType();

            // Only handle struct types for now
            auto* structType = allocType->asStruct();
            if (!structType) {
                throw std::runtime_error("GCAlloc only supports struct types currently");
            }

            // Analyze struct to create TypeInfo
            Volta::GC::TypeInfo typeInfo;
            typeInfo.kind = Volta::GC::TypeInfo::PRIMITIVE;  // Default
            typeInfo.elementTypeId = 0;

            // Check each field to find pointers
            for (unsigned i = 0; i < structType->getNumFields(); i++) {
                auto fieldType = structType->getFieldTypeAtIdx(i);

                // If field is a pointer/object type, record its offset
                if (fieldType->kind() == ir::IRType::Kind::Pointer ||
                    fieldType->kind() == ir::IRType::Kind::String ||
                    fieldType->kind() == ir::IRType::Kind::Struct) {
                    typeInfo.kind = Volta::GC::TypeInfo::OBJECT;
                    // Field offset = field index * sizeof(Value) = field index * 8
                    typeInfo.pointerFieldOffsets.push_back(i * 8);
                }
            }

            // Register type and get ID
            uint32_t typeId = nextTypeId_++;
            module_->registerType(typeId, typeInfo);

            // Emit GCALLOC instruction
            byte rD = getOrAllocateRegister(instr);
            emitGCAlloc(rD, typeId, structType->getNumFields());
            break;
        }
        case ir::Instruction::Opcode::Load: {
            byte rD = getOrAllocateRegister(instr);
            byte rSrc = getOrAllocateRegister(instr->getOperand(0));
            emitMove(rD, rSrc);
            break;
        }
        case ir::Instruction::Opcode::Store: {
            byte rD = getOrAllocateRegister(instr->getOperand(1));
            byte rSrc = getOrAllocateRegister(instr->getOperand(0));
            emitMove(rD, rSrc);
            break;
        }
        default:
            throw std::runtime_error("Unsupported ir instruction.");
    }
}

index BytecodeCompiler::getOrAddIntConstant(int64_t value) {
    return module_->addIntConstant(value);
}

index BytecodeCompiler::getOrAddFloatConstant(double value) {
    return module_->addFloatConstant(value);
}

index BytecodeCompiler::getOrAddStringConstant(const std::string& value) {
    return module_->addStringConstant(value);
}

byte BytecodeCompiler::getOrAllocateRegister(ir::Value* value) {
    auto map = currentFunction_->valueToRegister;
    auto it = map.find(value);
    if (it != map.end()) return it->second;

    if (isConstant(value)) return emitConstantLoad(value);
    
    // TODO: Add function to add register in functioncompcontext directly
    byte reg = currentFunction_->nextRegister;
    currentFunction_->nextRegister++;
    currentFunction_->valueToRegister[value] = reg;
    
    return reg;
}

byte BytecodeCompiler::getRegister(ir::Value* value) {
    auto map = currentFunction_->valueToRegister;
    auto it = map.find(value);

    assert(it != map.end() && "Tried to access non-existant register");
    return it->second;
}

void BytecodeCompiler::emitByte(byte val) {
    module_->emitByte(val);
}

void BytecodeCompiler::emitU16(uint16_t val) {
    module_->emitU16(val);
}

void BytecodeCompiler::emitI16(int16_t val) {
    module_->emitI16(val);
}



uint32_t BytecodeCompiler::currentOffset() const {
    return module_->getCurrentOffset();
}

void BytecodeCompiler::emitAdd(byte rD, byte rA, byte rB) {
    emitByte(static_cast<byte>(Opcode::ADD));
    emitByte(rD);
    emitByte(rA);
    emitByte(rB);
}

void BytecodeCompiler::emitSub(byte rD, byte rA, byte rB) {
    emitByte(static_cast<byte>(Opcode::SUB));
    emitByte(rD);
    emitByte(rA);
    emitByte(rB);
}

void BytecodeCompiler::emitMul(byte rD, byte rA, byte rB) {
    emitByte(static_cast<byte>(Opcode::MUL));
    emitByte(rD);
    emitByte(rA);
    emitByte(rB);
}

void BytecodeCompiler::emitDiv(byte rD, byte rA, byte rB) {
    emitByte(static_cast<byte>(Opcode::DIV));
    emitByte(rD);
    emitByte(rA);
    emitByte(rB);
}

void BytecodeCompiler::emitRem(byte rD, byte rA, byte rB) {
    emitByte(static_cast<byte>(Opcode::REM));
    emitByte(rD);
    emitByte(rA);
    emitByte(rB);
}

void BytecodeCompiler::emitPow(byte rD, byte rA, byte rB) {
    emitByte(static_cast<byte>(Opcode::POW));
    emitByte(rD);
    emitByte(rA);
    emitByte(rB);
}

void BytecodeCompiler::emitNeg(byte rD, byte rA) {
    emitByte(static_cast<byte>(Opcode::NEG));
    emitByte(rD);
    emitByte(rA);
}


void BytecodeCompiler::emitCmpEq(byte rD, byte rA, byte rB) {
    emitByte(static_cast<byte>(Opcode::CMP_EQ));
    emitByte(rD);
    emitByte(rA);
    emitByte(rB);
}

void BytecodeCompiler::emitCmpNe(byte rD, byte rA, byte rB) {
    emitByte(static_cast<byte>(Opcode::CMP_NE));
    emitByte(rD);
    emitByte(rA);
    emitByte(rB);
}

void BytecodeCompiler::emitCmpLt(byte rD, byte rA, byte rB) {
    emitByte(static_cast<byte>(Opcode::CMP_LT));
    emitByte(rD);
    emitByte(rA);
    emitByte(rB);
}

void BytecodeCompiler::emitCmpLe(byte rD, byte rA, byte rB) {
    emitByte(static_cast<byte>(Opcode::CMP_LE));
    emitByte(rD);
    emitByte(rA);
    emitByte(rB);
}

void BytecodeCompiler::emitCmpGt(byte rD, byte rA, byte rB) {
    emitByte(static_cast<byte>(Opcode::CMP_GT));
    emitByte(rD);
    emitByte(rA);
    emitByte(rB);
}

void BytecodeCompiler::emitCmpGe(byte rD, byte rA, byte rB) {
    emitByte(static_cast<byte>(Opcode::CMP_GE));
    emitByte(rD);
    emitByte(rA);
    emitByte(rB);
}


void BytecodeCompiler::emitAnd(byte rD, byte rA, byte rB) {
    emitByte(static_cast<byte>(Opcode::AND));
    emitByte(rD);
    emitByte(rA);
    emitByte(rB);
}

void BytecodeCompiler::emitOr(byte rD, byte rA, byte rB) {
    emitByte(static_cast<byte>(Opcode::OR));
    emitByte(rD);
    emitByte(rA);
    emitByte(rB);
}

void BytecodeCompiler::emitNot(byte rD, byte rA) {
    emitByte(static_cast<byte>(Opcode::NOT));
    emitByte(rD);
    emitByte(rA);
}

void BytecodeCompiler::emitLoadConstInt(byte rD, index poolIdx) {
    emitByte(static_cast<byte>(Opcode::LOAD_CONST_INT));
    emitByte(rD);
    emitU16(poolIdx);
}

void BytecodeCompiler::emitLoadConstFloat(byte rD, index poolIdx) {
    emitByte(static_cast<byte>(Opcode::LOAD_CONST_FLOAT));
    emitByte(rD);
    emitU16(poolIdx);
}

void BytecodeCompiler::emitLoadConstString(byte rD, index poolIdx) {
    emitByte(static_cast<byte>(Opcode::LOAD_CONST_STRING));
    emitByte(rD);
    emitU16(poolIdx);
}

void BytecodeCompiler::emitLoadTrue(byte rD) {
    emitByte(static_cast<byte>(Opcode::LOAD_TRUE));
    emitByte(rD);
}

void BytecodeCompiler::emitLoadFalse(byte rD) {
    emitByte(static_cast<byte>(Opcode::LOAD_FALSE));
    emitByte(rD);
}

void BytecodeCompiler::emitLoadNone(byte rD) {
    emitByte(static_cast<byte>(Opcode::LOAD_NONE));
    emitByte(rD);
}

void BytecodeCompiler::emitBr(int16_t offset) {
    emitByte(static_cast<byte>(Opcode::BR));
    emitI16(offset);
}

void BytecodeCompiler::emitBrIfTrue(byte rCond, int16_t offset) {
    emitByte(static_cast<byte>(Opcode::BR_IF_TRUE));
    emitByte(rCond);
    emitI16(offset);
}

void BytecodeCompiler::emitBrIfFalse(byte rCond, int16_t offset) {
    emitByte(static_cast<byte>(Opcode::BR_IF_FALSE));
    emitByte(rCond);
    emitI16(offset);
}

void BytecodeCompiler::emitCall(index funcIdx, byte rResult) {
    emitByte(static_cast<byte>(Opcode::CALL));
    emitU16(funcIdx);
    emitByte(rResult);
}

void BytecodeCompiler::emitRet(byte rVal) {
    emitByte(static_cast<byte>(Opcode::RET));
    emitByte(rVal);
}

void BytecodeCompiler::emitRetVoid() {
    emitByte(static_cast<byte>(Opcode::RET_VOID));
}

void BytecodeCompiler::emitHalt() {
    emitByte(static_cast<byte>(Opcode::HALT));
}


void BytecodeCompiler::emitMove(byte rDest, byte rSrc) {
    emitByte(static_cast<byte>(Opcode::MOVE));
    emitByte(rDest);
    emitByte(rSrc);
}

void BytecodeCompiler::emitArrayNew(byte rD, byte rSize) {
    emitByte(static_cast<byte>(Opcode::ARRAY_NEW));
    emitByte(rD);
    emitByte(rSize);
}

void BytecodeCompiler::emitArraySlice(byte rD, byte rArray, byte rStart, byte rEnd) {
    emitByte(static_cast<byte>(Opcode::ARRAY_SLICE));
    emitByte(rD);
    emitByte(rArray);
    emitByte(rStart);
    emitByte(rEnd);
}

void BytecodeCompiler::emitArrayGet(byte rD, byte rArray, byte rIndex) {
    emitByte(static_cast<byte>(Opcode::ARRAY_GET));
    emitByte(rD);
    emitByte(rArray);
    emitByte(rIndex);
}

void BytecodeCompiler::emitArraySet(byte rArray, byte rIndex, byte rValue) {
    emitByte(static_cast<byte>(Opcode::ARRAY_SET));
    emitByte(rArray);
    emitByte(rIndex);
    emitByte(rValue);
}

void BytecodeCompiler::emitStructGet(byte rD, byte rStruct, byte fieldIdx) {
    emitByte(static_cast<byte>(Opcode::STRUCT_GET));
    emitByte(rD);
    emitByte(rStruct);
    emitByte(fieldIdx);
}

void BytecodeCompiler::emitStructSet(byte rD, byte rStruct, byte rValue, byte fieldIdx) {
    emitByte(static_cast<byte>(Opcode::STRUCT_SET));
    emitByte(rD);
    emitByte(rStruct);
    emitByte(rValue);
    emitByte(fieldIdx);
}

void BytecodeCompiler::emitGCAlloc(byte rD, uint32_t typeId, uint32_t fieldCount) {
    emitByte(static_cast<byte>(Opcode::GCALLOC));
    emitByte(rD);
    emitByte(static_cast<byte>(typeId));     // Type ID (for GC tracing)
    emitByte(static_cast<byte>(fieldCount)); // Number of fields
}

void BytecodeCompiler::emitArrayLen(byte rD, byte rArray) {
    emitByte(static_cast<byte>(Opcode::ARRAY_LEN));
    emitByte(rD);
    emitByte(rArray);
}

void BytecodeCompiler::emitStringLen(byte rD, byte rString) {
    emitByte(static_cast<byte>(Opcode::STRING_LEN));
    emitByte(rD);
    emitByte(rString);
}

void BytecodeCompiler::emitCastIntFloat(byte rD, byte rSrc) {
    emitByte(static_cast<byte>(Opcode::CAST_INT_FLOAT));
    emitByte(rD);
    emitByte(rSrc);
}

void BytecodeCompiler::emitCastFloatInt(byte rD, byte rSrc) {
    emitByte(static_cast<byte>(Opcode::CAST_FLOAT_INT));
    emitByte(rD);
    emitByte(rSrc);
}

void BytecodeCompiler::emitIsSome(byte rD, byte rOption) {
    emitByte(static_cast<byte>(Opcode::IS_SOME));
    emitByte(rD);
    emitByte(rOption);
}

void BytecodeCompiler::emitOptionWrap(byte rD, byte rVal) {
    emitByte(static_cast<byte>(Opcode::OPTION_WRAP));
    emitByte(rD);
    emitByte(rVal);
}

void BytecodeCompiler::emitOptionUnwrap(byte rD, byte rOption) {
    emitByte(static_cast<byte>(Opcode::OPTION_UNWRAP));
    emitByte(rD);
    emitByte(rOption);
}

void BytecodeCompiler::recordFixup(ir::BasicBlock* target) {
    fixups_.push_back(LabelFixup {
        .patchOffset = currentOffset() - 2,
        .targetBlock = target,
        .sourceOffset = currentOffset()
    });
}

void BytecodeCompiler::resolveFixups() {
    for (const auto& fixup : fixups_) {
        auto offset = calculateBranchOffset(fixup);
        patchOffset(fixup.patchOffset, offset); 
    }
}

void BytecodeCompiler::emitPhiMoves(ir::BasicBlock* current, ir::BasicBlock* successor) {
    // Find all phi nodes in the successor block
    for (ir::Instruction* inst : successor->getInstructions()) {
        // Only process phi nodes (they're always at the start of a block)
        ir::PhiNode* phi = dynamic_cast<ir::PhiNode*>(inst);
        if (!phi) break;  // No more phi nodes

        // Find the incoming value from 'current' block
        ir::Value* incomingValue = nullptr;
        for (unsigned i = 0; i < phi->getNumIncomingValues(); i++) {
            if (phi->getIncomingBlock(i) == current) {
                incomingValue = phi->getIncomingValue(i);
                break;
            }
        }

        if (!incomingValue) continue;  // No value from this block

        // Get/allocate registers
        byte phiReg = getOrAllocateRegister(phi);
        byte valueReg = getOrAllocateRegister(incomingValue);

        // Emit MOVE: phi_reg = value_reg
        emitMove(phiReg, valueReg);
    }
}

bool BytecodeCompiler::isConstant(ir::Value* value) {
    return value->isConstant();
}

byte BytecodeCompiler::emitConstantLoad(ir::Value* constantValue) {
    switch (constantValue->getKind()) {
        case ir::Constant::ValueKind::ConstantInt: {
            auto* constInt = static_cast<ir::ConstantInt*>(constantValue);
            int64_t value = constInt->getValue();
            index poolIdx = getOrAddIntConstant(value);

            byte reg = currentFunction_->nextRegister++;
            // NOTE: Don't save constants in valueToRegister map
            // Each use of a constant needs to load it fresh
            // (Otherwise constants from one branch aren't available in other branches)
            emitLoadConstInt(reg, poolIdx);
            return reg;
        }
        case ir::Constant::ValueKind::ConstantFloat: {
            auto* constFloat = static_cast<ir::ConstantFloat*>(constantValue);
            double value = constFloat->getValue();
            index poolIdx = getOrAddFloatConstant(value);

            byte reg = currentFunction_->nextRegister++;
            // NOTE: Don't save constants in valueToRegister map
            emitLoadConstFloat(reg, poolIdx);
            return reg;
        }
        case ir::Constant::ValueKind::ConstantBool: {
            auto* constBool = static_cast<ir::ConstantBool*>(constantValue);
            bool value = constBool->getValue();
            byte reg = currentFunction_->nextRegister++;
            // NOTE: Don't save constants in valueToRegister map

            if (value) emitLoadTrue(reg);
            else emitLoadFalse(reg);
            return reg;
        }
        case ir::Constant::ValueKind::ConstantString: {
            auto* constStr = static_cast<ir::ConstantString*>(constantValue);
            std::string value = constStr->getValue();
            index poolIdx = getOrAddStringConstant(value);

            byte reg = currentFunction_->nextRegister++;
            // NOTE: Don't save constants in valueToRegister map
            emitLoadConstString(reg, poolIdx);
            return reg;
        }
        case ir::Constant::ValueKind::ConstantNone: {
            byte reg = currentFunction_->nextRegister++;
            // NOTE: Don't save constants in valueToRegister map
    
            emitLoadNone(reg);
            
            return reg;
        }
        default:
            throw std::runtime_error("Tried to constant load unrecognized constant type.");
    }
}

void BytecodeCompiler::setupCallArguments(ir::Instruction* callInst) {
    /*
        this implementation has a potential bug 
        if you have arguments like (r1, r0), you'll clobber r0 before reading it!
        Different approaches:
            Temporary registers
            Cycle detection
            Smart ordering
    */
    std::vector<byte> argRegs;
    auto* ci = static_cast<ir::CallInst*>(callInst);

    for (unsigned i = 0; i < ci->getNumArguments(); i++) {
        auto* arg = ci->getArgument(i);
        byte argReg = getOrAllocateRegister(arg);
        argRegs.push_back(argReg);
    }

    for (unsigned i = 0; i < argRegs.size(); i++) {
        byte expectedReg = i;
        byte actualReg = argRegs[i];

        if (actualReg != expectedReg) emitMove(expectedReg, actualReg);
    }
}

int16_t BytecodeCompiler::calculateBranchOffset(const LabelFixup& fixup) {
    auto it = currentFunction_->blockOffsets.find(fixup.targetBlock);
    if (it == currentFunction_->blockOffsets.end()) {
        throw std::runtime_error("Branch to undefined block");
    }
    uint32_t targetOffset = it->second;
    int32_t offset = static_cast<int32_t>(targetOffset) - static_cast<int32_t>(fixup.sourceOffset);
    if (offset < INT16_MIN || offset > INT16_MAX)
        throw std::out_of_range("Branch offset out of 16-bit range");
    return static_cast<int16_t>(offset);
}

void BytecodeCompiler::patchOffset(uint32_t offset, int16_t value) {
    module_->patchI16(offset, value);
}

void BytecodeCompiler::compileBinaryOp(ir::Instruction* inst, void (BytecodeCompiler::*emitFunc)(byte, byte, byte)) {
    byte rD = getOrAllocateRegister(inst);
    byte rA = getOrAllocateRegister(inst->getOperand(0));
    byte rB = getOrAllocateRegister(inst->getOperand(1));
    
    (this->*emitFunc)(rD, rA, rB);
}

void BytecodeCompiler::compileUnaryOp(ir::Instruction* inst, void (BytecodeCompiler::*emitFunc)(byte, byte)) {
    byte rD = getOrAllocateRegister(inst);
    byte rA = getOrAllocateRegister(inst->getOperand(0));\
    (this->*emitFunc)(rD, rA);
}

void BytecodeCompiler::registerRuntimeFunctions() {
    // Register all print overloads with mangled names
    module_->addNativeFunction("print_i64", runtime_print, 1);
    module_->addNativeFunction("print_f64", runtime_print, 1);
    module_->addNativeFunction("print_bool", runtime_print, 1);
    module_->addNativeFunction("print_str", runtime_print, 1);
}

} // namespace volta::vm
