#ifdef LLVM_AVAILABLE

#include "Codegen/MIRToLLVM.hpp"
#include <vector>

namespace Codegen {

void MIRToLLVM::lower(const MIR::Program& program) {
    // First pass: Declare all functions (including externs) so they can be called
    for (const auto& function : program.functions) {
        // Convert return type
        llvm::Type* retType = getLLVMType(function.returnType);
        if (function.returnType->kind == Type::TypeKind::Array) {
            retType = llvm::PointerType::getUnqual(retType);
        }

        std::vector<llvm::Type*> paramTypes;
        for (const auto& param : function.params) {
            llvm::Type* paramType = getLLVMType(param.type);
            if (param.type->kind == Type::TypeKind::Array) {
                paramType = llvm::PointerType::getUnqual(paramType);
            }
            paramTypes.push_back(paramType);
        }

        llvm::FunctionType* fnType = llvm::FunctionType::get(retType, paramTypes, false);
        llvm::Function::Create(
            fnType,
            llvm::Function::ExternalLinkage,
            function.name,
            module
        );
    }

    // Second pass: Lower function bodies (skip externs which have no blocks)
    for (const auto& function : program.functions) {
        if (!function.blocks.empty()) {
            lowerFunction(function);
        }
    }
}


void MIRToLLVM::lowerFunction(const MIR::Function& function) {
    valueMap.clear();
    blockMap.clear();

    // Get the already-declared function from the module
    currentFunction = module.getFunction(function.name);

    size_t i = 0;
    for (auto& arg : currentFunction->args()) {
        arg.setName(function.params[i].name);
        valueMap["%" + function.params[i].name] = &arg;
        i++;
    }

    for (const auto& block: function.blocks) {
        auto* bb = llvm::BasicBlock::Create(context, block.label, currentFunction);
        blockMap[block.label] = bb;
    }

    for (const auto& block: function.blocks) {
        lowerBasicBlock(block);
    }
}

void MIRToLLVM::lowerBasicBlock(const MIR::BasicBlock& block) {
    auto* llvmBB = blockMap[block.label];
    builder.SetInsertPoint(llvmBB);

    for (const auto& inst : block.instructions) {
        lowerInstruction(inst);
    }

    lowerTerminator(block.terminator);
}
void MIRToLLVM::lowerInstruction(const MIR::Instruction& inst) {
    using MIR::Opcode;

    switch (inst.opcode) {
        // === Arithmetic (Integer) ===
        case Opcode::IAdd: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateAdd(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::ISub: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateSub(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::IMul: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateMul(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::IDiv: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateSDiv(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::IRem: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateSRem(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::UDiv: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateUDiv(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::URem: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateURem(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }

        // === Arithmetic (Float) ===
        case Opcode::FAdd: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateFAdd(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::FSub: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateFSub(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::FMul: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateFMul(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::FDiv: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateFDiv(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }

        // === Comparison (Integer) ===
        case Opcode::ICmpEQ: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateICmpEQ(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::ICmpNE: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateICmpNE(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::ICmpLT: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateICmpSLT(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::ICmpLE: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateICmpSLE(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::ICmpGT: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateICmpSGT(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::ICmpGE: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateICmpSGE(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }

        // === Comparison (Unsigned Integer) ===
        case Opcode::ICmpULT: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateICmpULT(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::ICmpULE: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateICmpULE(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::ICmpUGT: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateICmpUGT(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::ICmpUGE: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateICmpUGE(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }

        // === Comparison (Float) ===
        case Opcode::FCmpEQ: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateFCmpOEQ(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::FCmpNE: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateFCmpONE(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::FCmpLT: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateFCmpOLT(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::FCmpLE: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateFCmpOLE(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::FCmpGT: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateFCmpOGT(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::FCmpGE: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateFCmpOGE(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }

        // === Logical ===
        case Opcode::And: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateAnd(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::Or: {
            auto* lhs = getValue(inst.operands[0]);
            auto* rhs = getValue(inst.operands[1]);
            auto* result = builder.CreateOr(lhs, rhs, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::Not: {
            auto* operand = getValue(inst.operands[0]);
            auto* result = builder.CreateNot(operand, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }

        // === Type Conversions ===
        case Opcode::Trunc: {
            auto* value = getValue(inst.operands[0]);
            auto* targetType = getLLVMType(inst.result.type);
            auto* result = builder.CreateTrunc(value, targetType, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::ZExt: {
            auto* value = getValue(inst.operands[0]);
            auto* targetType = getLLVMType(inst.result.type);
            auto* result = builder.CreateZExt(value, targetType, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::SExt: {
            auto* value = getValue(inst.operands[0]);
            auto* targetType = getLLVMType(inst.result.type);
            auto* result = builder.CreateSExt(value, targetType, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::FPTrunc: {
            auto* value = getValue(inst.operands[0]);
            auto* targetType = getLLVMType(inst.result.type);
            auto* result = builder.CreateFPTrunc(value, targetType, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::FPExt: {
            auto* value = getValue(inst.operands[0]);
            auto* targetType = getLLVMType(inst.result.type);
            auto* result = builder.CreateFPExt(value, targetType, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::UIToFP: {
            auto* value = getValue(inst.operands[0]);
            auto* targetType = getLLVMType(inst.result.type);
            auto* result = builder.CreateUIToFP(value, targetType, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::SIToFP: {
            auto* value = getValue(inst.operands[0]);
            auto* targetType = getLLVMType(inst.result.type);
            auto* result = builder.CreateSIToFP(value, targetType, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::FPToUI: {
            auto* value = getValue(inst.operands[0]);
            auto* targetType = getLLVMType(inst.result.type);
            auto* result = builder.CreateFPToUI(value, targetType, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::FPToSI: {
            auto* value = getValue(inst.operands[0]);
            auto* targetType = getLLVMType(inst.result.type);
            auto* result = builder.CreateFPToSI(value, targetType, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::Bitcast: {
            auto* value = getValue(inst.operands[0]);
            auto* targetType = getLLVMType(inst.result.type);
            auto* result = builder.CreateBitCast(value, targetType, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }

        // === Memory Operations ===
        case Opcode::Alloca: {
            if (inst.result.type == nullptr) {
                llvm::errs() << "ERROR: Alloca has null result type!\n";
                break;
            }

            auto* allocType = getLLVMType(inst.result.type);

            // For alloca, we need to unwrap the pointer type to get the allocated type
            if (inst.result.type->kind == Type::TypeKind::Pointer) {
                const auto* ptrType = dynamic_cast<const Type::PointerType*>(inst.result.type);
                allocType = getLLVMType(ptrType->pointeeType);

                // Debug: check if we got a valid type
                if (allocType == nullptr) {
                    llvm::errs() << "ERROR: Failed to get LLVM type for allocation of: "
                                 << ptrType->pointeeType->toString() << "\n";
                    break;
                }
            }

            auto* result = builder.CreateAlloca(allocType, nullptr, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::Load: {
            auto* ptr = getValue(inst.operands[0]);

            // If the result type is null, try to infer it from the pointer type
            llvm::Type* elemType = nullptr;
            if (inst.result.type != nullptr) {
                elemType = getLLVMType(inst.result.type);
            } else if ((inst.operands[0].type != nullptr) &&
                       inst.operands[0].type->kind == Type::TypeKind::Pointer) {
                // Infer from pointer operand
                const auto* ptrType = dynamic_cast<const Type::PointerType*>(inst.operands[0].type);
                elemType = getLLVMType(ptrType->pointeeType);
            } else {
                // Can't determine type - skip this load (shouldn't happen in valid MIR)
                llvm::errs() << "Warning: Load instruction has no type information\n";
                break;
            }

            auto* result = builder.CreateLoad(elemType, ptr, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }
        case Opcode::Store: {
            auto* value = getValue(inst.operands[0]);
            auto* ptr = getValue(inst.operands[1]);
            builder.CreateStore(value, ptr);
            // Store has no result
            break;
        }

        // === Array Operations ===
        case Opcode::GetElementPtr: {
            auto* basePtr = getValue(inst.operands[0]);
            auto* index = getValue(inst.operands[1]);

            // Get the array type from the pointer
            if ((inst.operands[0].type == nullptr) || inst.operands[0].type->kind != Type::TypeKind::Pointer) {
                llvm::errs() << "ERROR: GEP operand is not a pointer type!\n";
                break;
            }

            const auto* ptrType = dynamic_cast<const Type::PointerType*>(inst.operands[0].type);
            const auto* arrayType = dynamic_cast<const Type::ArrayType*>(ptrType->pointeeType);
            auto* llvmArrayType = getLLVMType(arrayType);

            // GEP indices: first 0 to dereference the pointer, then the actual index
            std::vector<llvm::Value*> indices = {
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
                index
            };

            auto* result = builder.CreateGEP(llvmArrayType, basePtr, indices, inst.result.name);
            valueMap["%" + inst.result.name] = result;
            break;
        }

        // === Struct Field Access ===
        case Opcode::GetFieldPtr: {
            auto* structPtr = getValue(inst.operands[0]);

            // Get the field index from the second operand (constant)
            int fieldIndex = 0;
            if (inst.operands[1].isConstant() && inst.operands[1].constantInt.has_value()) {
                fieldIndex = static_cast<int>(inst.operands[1].constantInt.value());
            }

            // Get the struct type
            if ((inst.operands[0].type == nullptr) || inst.operands[0].type->kind != Type::TypeKind::Pointer) {
                llvm::errs() << "ERROR: GetFieldPtr operand is not a pointer type!\n";
                break;
            }

            const auto* ptrType = dynamic_cast<const Type::PointerType*>(inst.operands[0].type);
            if ((ptrType->pointeeType == nullptr) || ptrType->pointeeType->kind != Type::TypeKind::Struct) {
                llvm::errs() << "ERROR: GetFieldPtr pointer doesn't point to a struct!\n";
                break;
            }

            const auto* structType = dynamic_cast<const Type::StructType*>(ptrType->pointeeType);
            auto* llvmStructType = getLLVMType(structType);

            if (llvmStructType == nullptr) {
                llvm::errs() << "ERROR: Failed to get LLVM type for struct: " << structType->name << "\n";
                break;
            }

            // Use LLVM's CreateStructGEP to get pointer to the field
            auto* result = builder.CreateStructGEP(
                llvmStructType,
                structPtr,
                fieldIndex,
                inst.result.name
            );

            valueMap["%" + inst.result.name] = result;
            break;
        }

        // === Function Calls ===
        case Opcode::Call: {
            std::string targetName = inst.callTarget.value();
            auto* callee = module.getFunction(targetName);

            if (callee == nullptr) {
                llvm::errs() << "ERROR: Function not found: " << targetName << "\n";
                llvm::errs() << "Available functions in module:\n";
                for (auto& fn : module) {
                    llvm::errs() << "  - " << fn.getName().str() << "\n";
                }
                break;
            }

            std::vector<llvm::Value*> args;
            args.reserve(inst.operands.size());
for (const auto& operand : inst.operands) {
                args.push_back(getValue(operand));
            }

            auto* result = builder.CreateCall(callee, args, inst.result.name);
            if (!inst.result.name.empty()) {
                valueMap["%" + inst.result.name] = result;
            }
            break;
        }
        default:
            break;
    }
}

void MIRToLLVM::lowerTerminator(const MIR::Terminator& term) {
    using MIR::TerminatorKind;

    switch (term.kind) {
        case TerminatorKind::Return:
            if (term.operands.empty()) {
                builder.CreateRetVoid();
            } else {
                auto* retVal = getValue(term.operands[0]);

                // Check if we need to convert the return value type to match function return type
                llvm::Type* expectedRetType = currentFunction->getReturnType();
                if (retVal->getType() != expectedRetType) {
                    // Handle float conversions (f64 -> f32 or f32 -> f64)
                    if (retVal->getType()->isFloatingPointTy() && expectedRetType->isFloatingPointTy()) {
                        if (expectedRetType->isFloatTy()) {
                            // f64 -> f32
                            retVal = builder.CreateFPTrunc(retVal, expectedRetType);
                        } else {
                            // f32 -> f64
                            retVal = builder.CreateFPExt(retVal, expectedRetType);
                        }
                    }
                }

                builder.CreateRet(retVal);
            }
            break;

        case TerminatorKind::Branch: {
            auto* targetBB = blockMap[term.targets[0]];
            builder.CreateBr(targetBB);
            break;
        }

        case TerminatorKind::CondBranch: {
            auto* cond = getValue(term.operands[0]);
            auto* trueBB = blockMap[term.targets[0]];
            auto* falseBB = blockMap[term.targets[1]];
            builder.CreateCondBr(cond, trueBB, falseBB);
            break;
        }

        case TerminatorKind::Switch:
            // TODO: Implement switch when needed for pattern matching
            break;

        case TerminatorKind::Unreachable:
            builder.CreateUnreachable();
            break;
    }
}


llvm::Value* MIRToLLVM::getValue(const MIR::Value& value) {
    using MIR::ValueKind;

    switch (value.kind) {
        case ValueKind::Local:
        case ValueKind::Param:
            return valueMap["%" + value.name];

        case ValueKind::Global:
            return module.getFunction(value.name);

        case ValueKind::Constant:
            if (value.constantNull.has_value() && value.constantNull.value()) {
                // Null pointer constant
                auto* type = getLLVMType(value.type);
                return llvm::ConstantPointerNull::get(static_cast<llvm::PointerType*>(type));
            }
            if (value.constantInt.has_value()) {
                auto* type = getLLVMType(value.type);
                return llvm::ConstantInt::get(type, value.constantInt.value());
            }
            if (value.constantBool.has_value()) {
                return llvm::ConstantInt::get(
                    llvm::Type::getInt1Ty(context),
                    value.constantBool.value() ? 1 : 0
                );
            }
            if (value.constantFloat.has_value()) {
                auto* type = getLLVMType(value.type);
                return llvm::ConstantFP::get(type, value.constantFloat.value());
            }
            if (value.constantString.has_value()) {
                // Check if we've already created this string global
                auto it = stringGlobals.find(value.name);
                if (it != stringGlobals.end()) {
                    return it->second;
                }

                // Create a new global string constant
                const std::string& str = value.constantString.value();
                llvm::Constant* strConstant = llvm::ConstantDataArray::getString(
                    context, str, true  // AddNull = true
                );

                auto* global = new llvm::GlobalVariable(
                    module,
                    strConstant->getType(),
                    true,  // isConstant
                    llvm::GlobalValue::PrivateLinkage,
                    strConstant,
                    value.name
                );

                stringGlobals[value.name] = global;

                // Return a constant GEP to get ptr<i8> from [N x i8]*
                llvm::Constant* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0);
                llvm::Constant* indices[] = {zero, zero};
                return llvm::ConstantExpr::getGetElementPtr(
                    strConstant->getType(),
                    global,
                    indices
                );
            }
            break;
    }

    return nullptr;
}

llvm::Type* MIRToLLVM::getLLVMType(const Type::Type* type) {
    if (type == nullptr) {
        // Null type - return void type as fallback
        return llvm::Type::getVoidTy(context);
    }

    switch (type->kind) {
        case Type::TypeKind::Primitive: {
            const auto* primType = dynamic_cast<const Type::PrimitiveType*>(type);
            switch (primType->kind) {
                // Signed integers
                case Type::PrimitiveKind::I8:
                    return llvm::Type::getInt8Ty(context);
                case Type::PrimitiveKind::I16:
                    return llvm::Type::getInt16Ty(context);
                case Type::PrimitiveKind::I32:
                    return llvm::Type::getInt32Ty(context);
                case Type::PrimitiveKind::I64:
                    return llvm::Type::getInt64Ty(context);

                // Unsigned integers (same LLVM type as signed, signedness is in operations)
                case Type::PrimitiveKind::U8:
                    return llvm::Type::getInt8Ty(context);
                case Type::PrimitiveKind::U16:
                    return llvm::Type::getInt16Ty(context);
                case Type::PrimitiveKind::U32:
                    return llvm::Type::getInt32Ty(context);
                case Type::PrimitiveKind::U64:
                    return llvm::Type::getInt64Ty(context);

                // Floats
                case Type::PrimitiveKind::F32:
                    return llvm::Type::getFloatTy(context);
                case Type::PrimitiveKind::F64:
                    return llvm::Type::getDoubleTy(context);

                // Other
                case Type::PrimitiveKind::Bool:
                    return llvm::Type::getInt1Ty(context);
                case Type::PrimitiveKind::Void:
                    return llvm::Type::getVoidTy(context);
                case Type::PrimitiveKind::String:
                    // str type is represented as ptr<i8> (C-style string for now)
                    return llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0);

                default:
                    return nullptr;
            }
        }

        case Type::TypeKind::Array: {
            const auto* arrType = dynamic_cast<const Type::ArrayType*>(type);
            auto* elemType = getLLVMType(arrType->elementType);
            return llvm::ArrayType::get(elemType, arrType->size);
        }

        case Type::TypeKind::Pointer: {
            const auto* ptrType = dynamic_cast<const Type::PointerType*>(type);
            auto* pointeeType = getLLVMType(ptrType->pointeeType);
            return llvm::PointerType::get(pointeeType, 0);
        }

        case Type::TypeKind::Struct: {
            const auto* structType = dynamic_cast<const Type::StructType*>(type);

            // Check if we've already created this struct type
            auto it = structTypeMap.find(structType->name);
            if (it != structTypeMap.end()) {
                return it->second;
            }

            // Create LLVM struct type with field types
            std::vector<llvm::Type*> fieldTypes;
            fieldTypes.reserve(structType->fields.size());
for (const auto& field : structType->fields) {
                fieldTypes.push_back(getLLVMType(field.type));
            }

            llvm::StructType* llvmStructType = llvm::StructType::create(
                context,
                fieldTypes,
                structType->name
            );

            structTypeMap[structType->name] = llvmStructType;
            return llvmStructType;
        }

        case Type::TypeKind::Opaque:
            // Opaque type (void* in C) is represented as i8 in LLVM
            // When used as ptr<opaque>, it becomes i8*
            return llvm::Type::getInt8Ty(context);

        case Type::TypeKind::Unresolved:
            // Unresolved types should have been resolved by semantic analysis
            // If we reach here, it's an error
            return nullptr;

        default:
            return nullptr;
    }
}

} // namespace Codegen

#endif // LLVM_AVAILABLE
