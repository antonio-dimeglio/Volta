#pragma once

#ifdef LLVM_AVAILABLE

#include "MIR/MIR.hpp"
#include "Type/TypeRegistry.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Value.h>
#include <unordered_map>
#include <string>

namespace Codegen {

class MIRToLLVM {
private:
    llvm::LLVMContext& context;
    llvm::Module& module;
    llvm::IRBuilder<> builder;
    Type::TypeRegistry& typeRegistry;

    std::unordered_map<std::string, llvm::Value*> valueMap;
    std::unordered_map<std::string, llvm::BasicBlock*> blockMap;
    std::unordered_map<std::string, llvm::GlobalVariable*> stringGlobals;
    std::unordered_map<std::string, llvm::StructType*> structTypeMap;

    llvm::Function* currentFunction = nullptr;

public:
    MIRToLLVM(llvm::LLVMContext& ctx, llvm::Module& mod, Type::TypeRegistry& tr)
        : context(ctx), module(mod), builder(ctx), typeRegistry(tr) {}

    void lower(const MIR::Program& program);

private:
    void lowerFunction(const MIR::Function& function);
    void lowerBasicBlock(const MIR::BasicBlock& block);
    void lowerInstruction(const MIR::Instruction& inst);
    void lowerTerminator(const MIR::Terminator& term);
    llvm::Value* getValue(const MIR::Value& value);
    llvm::Type* getLLVMType(const Type::Type* type);

};

} // namespace Codegen

#endif // LLVM_AVAILABLE
