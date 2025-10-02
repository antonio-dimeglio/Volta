#include "../include/bytecode/BytecodeCompiler.hpp"

namespace volta::bytecode {

// ========== CompiledModule Implementation ==========

const CompiledFunction* CompiledModule::getFunction(uint32_t index) const {
    return nullptr;
}

const CompiledFunction* CompiledModule::getFunction(const std::string& name) const {
    return nullptr;
}

const CompiledFunction* CompiledModule::getEntryPoint() const {
    return nullptr;
}

// ========== BytecodeCompiler Implementation ==========

std::unique_ptr<CompiledModule> BytecodeCompiler::compile(const ir::IRModule& module) {
    return nullptr;
}

CompiledFunction BytecodeCompiler::compileFunction(const ir::Function* function) {
    return CompiledFunction{};
}

void BytecodeCompiler::compileBasicBlock(const ir::BasicBlock* block, Chunk& chunk) {
}

void BytecodeCompiler::compileInstruction(const ir::Instruction* inst, Chunk& chunk) {
}

void BytecodeCompiler::compileBinaryInst(const ir::BinaryInst* inst, Chunk& chunk) {
}

void BytecodeCompiler::compileUnaryInst(const ir::UnaryInst* inst, Chunk& chunk) {
}

void BytecodeCompiler::compileAllocInst(const ir::AllocInst* inst, Chunk& chunk) {
}

void BytecodeCompiler::compileLoadInst(const ir::LoadInst* inst, Chunk& chunk) {
}

void BytecodeCompiler::compileStoreInst(const ir::StoreInst* inst, Chunk& chunk) {
}

void BytecodeCompiler::compileGetFieldInst(const ir::GetFieldInst* inst, Chunk& chunk) {
}

void BytecodeCompiler::compileSetFieldInst(const ir::SetFieldInst* inst, Chunk& chunk) {
}

void BytecodeCompiler::compileGetElementInst(const ir::GetElementInst* inst, Chunk& chunk) {
}

void BytecodeCompiler::compileSetElementInst(const ir::SetElementInst* inst, Chunk& chunk) {
}

void BytecodeCompiler::compileBranchInst(const ir::BranchInst* inst, Chunk& chunk) {
}

void BytecodeCompiler::compileBranchIfInst(const ir::BranchIfInst* inst, Chunk& chunk) {
}

void BytecodeCompiler::compileReturnInst(const ir::ReturnInst* inst, Chunk& chunk) {
}

void BytecodeCompiler::compileCallInst(const ir::CallInst* inst, Chunk& chunk) {
}

void BytecodeCompiler::compileCallForeignInst(const ir::CallForeignInst* inst, Chunk& chunk) {
}

void BytecodeCompiler::emitLoadValue(const ir::Value* value, Chunk& chunk) {
}

void BytecodeCompiler::emitConstant(const ir::Constant* constant, Chunk& chunk) {
}

uint32_t BytecodeCompiler::getFunctionIndex(const ir::Function* function) const {
    return 0;
}

uint32_t BytecodeCompiler::getForeignFunctionIndex(const std::string& name) const {
    return 0;
}

uint32_t BytecodeCompiler::getLocalIndex(const ir::Value* value) const {
    return 0;
}

uint32_t BytecodeCompiler::getGlobalIndex(const ir::GlobalVariable* global) const {
    return 0;
}

uint32_t BytecodeCompiler::addStringConstant(const std::string& str) {
    return 0;
}

size_t BytecodeCompiler::getBlockLabel(const ir::BasicBlock* block) {
    return 0;
}

void BytecodeCompiler::defineBlockLabel(const ir::BasicBlock* block, Chunk& chunk) {
}

} // namespace volta::bytecode
