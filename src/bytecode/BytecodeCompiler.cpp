#include "../include/bytecode/BytecodeCompiler.hpp"
#include <stdexcept>

namespace volta::bytecode {

// Utils
inline bool isIntType(const std::shared_ptr<semantic::Type>& type) {
    return type->kind() == semantic::Type::Kind::Int;
}

inline bool isFloatType(const std::shared_ptr<semantic::Type>& type) {
    return type->kind() == semantic::Type::Kind::Float;
}

inline bool isBoolType(const std::shared_ptr<semantic::Type>& type) {
    return type->kind() == semantic::Type::Kind::Bool;
}

// ========== CompiledModule Implementation ==========

const CompiledFunction* CompiledModule::getFunction(uint32_t index) const {
    if (index < functions_.size()) {
        return &functions_[index];
    } else {
        return nullptr;
    }
    
}

const CompiledFunction* CompiledModule::getFunction(const std::string& name) const {
    for (auto& fn : functions_) {
        if (fn.name == name) {
            return &fn;
        }
    }

    return nullptr; 
}

const CompiledFunction* CompiledModule::getEntryPoint() const {
    // For a scripting language, we look for __init__ first (top-level code)
    // If that doesn't exist, we fall back to main (for compatibility)
    const auto* initFunc = getFunction("__init__");
    if (initFunc) {
        return initFunc;
    }
    return getFunction("main");
}

// ========== BytecodeCompiler Implementation ==========

std::unique_ptr<CompiledModule> BytecodeCompiler::compile(const ir::IRModule& module) {
    // Set current module context
    currentModule_ = &module;

    // Clear global state
    functionIndexMap_.clear();
    foreignFunctionMap_.clear();
    globalIndexMap_.clear();
    stringPool_.clear();
    stringPoolMap_.clear();

    // Create the compiled module
    auto compiledModule = std::make_unique<CompiledModule>(module.name());

    // Step 1: Build function index map
    uint32_t functionIndex = 0;
    for (const auto& function : module.functions()) {
        functionIndexMap_[function.get()] = functionIndex++;
    }

    // Step 2: Build foreign function index map
    uint32_t foreignIndex = 0;
    for (const auto& function : module.functions()) {
        if (function->isForeign()) {
            foreignFunctionMap_[function->name()] = foreignIndex++;
            compiledModule->foreignFunctions().push_back(function->name());
        }
    }

    // Step 3: Build global variable index map
    uint32_t globalIndex = 0;
    for (const auto& global : module.globals()) {
        globalIndexMap_[global.get()] = globalIndex++;
    }
    compiledModule->setGlobalCount(globalIndex);

    // Step 4: Compile each function
    for (const auto& function : module.functions()) {
        CompiledFunction compiled = compileFunction(function.get());
        compiled.index = functionIndexMap_[function.get()];
        compiledModule->functions().push_back(std::move(compiled));
    }

    // Step 5: Copy string pool to module
    compiledModule->stringPool() = stringPool_;

    return compiledModule;
}

CompiledFunction BytecodeCompiler::compileFunction(const ir::Function* function) {
    // Set current function context
    currentFunction_ = function;

    // Clear per-function state
    localIndexMap_.clear();
    blockLabelMap_.clear();
    forwardJumps_.clear();

    // Build local variable index map
    // Parameters occupy the first N locals
    uint32_t localIndex = function->parameters().size();

    // Assign indices to all SSA values (instructions that produce values)
    for (const auto* block : function->basicBlocks()) {
        for (const auto* inst : block->instructions()) {
            // Only instructions that produce values need local slots
            // (not Store, SetField, SetElement, Branch, Return, etc.)
            if (inst->type()->kind() != semantic::Type::Kind::Void) {
                localIndexMap_[inst] = localIndex++;
            }
        }
    }

    // Create the bytecode chunk
    Chunk chunk;

    // Compile all basic blocks
    for (const auto* block : function->basicBlocks()) {
        compileBasicBlock(block, chunk);
    }

    // Check for unpatched forward jumps (shouldn't happen in valid IR)
    if (!forwardJumps_.empty()) {
        throw std::runtime_error("Unresolved forward jumps in function " + function->name());
    }

    // Build CompiledFunction
    CompiledFunction compiled;
    compiled.name = function->name();
    compiled.index = 0; // Will be set by compile()
    compiled.parameterCount = function->parameters().size();
    compiled.localCount = localIndex; // Total locals (params + SSA values)
    compiled.chunk = std::move(chunk);
    compiled.isForeign = function->isForeign();

    return compiled;
}

void BytecodeCompiler::compileBasicBlock(const ir::BasicBlock* block, Chunk& chunk) {
    // Mark where this block starts (for jump targets)
    defineBlockLabel(block, chunk);

    // Compile each instruction in the block
    for (const auto* inst : block->instructions()) {
        compileInstruction(inst, chunk);
    }
}

void BytecodeCompiler::compileInstruction(const ir::Instruction* inst, Chunk& chunk) {
    switch (inst->opcode()) {
        // Binary operations
        case ir::Instruction::Opcode::Add:
        case ir::Instruction::Opcode::Sub:
        case ir::Instruction::Opcode::Mul:
        case ir::Instruction::Opcode::Div:
        case ir::Instruction::Opcode::Mod:
        case ir::Instruction::Opcode::Eq:
        case ir::Instruction::Opcode::Ne:
        case ir::Instruction::Opcode::Lt:
        case ir::Instruction::Opcode::Le:
        case ir::Instruction::Opcode::Gt:
        case ir::Instruction::Opcode::Ge:
        case ir::Instruction::Opcode::And:
        case ir::Instruction::Opcode::Or:
            compileBinaryInst(static_cast<const ir::BinaryInst*>(inst), chunk);
            break;

        // Unary operations
        case ir::Instruction::Opcode::Neg:
        case ir::Instruction::Opcode::Not:
            compileUnaryInst(static_cast<const ir::UnaryInst*>(inst), chunk);
            break;

        // Memory operations
        case ir::Instruction::Opcode::Alloc:
            compileAllocInst(static_cast<const ir::AllocInst*>(inst), chunk);
            break;
        case ir::Instruction::Opcode::Load:
            compileLoadInst(static_cast<const ir::LoadInst*>(inst), chunk);
            break;
        case ir::Instruction::Opcode::Store:
            compileStoreInst(static_cast<const ir::StoreInst*>(inst), chunk);
            break;

        // Struct/Object operations
        case ir::Instruction::Opcode::GetField:
            compileGetFieldInst(static_cast<const ir::GetFieldInst*>(inst), chunk);
            break;
        case ir::Instruction::Opcode::SetField:
            compileSetFieldInst(static_cast<const ir::SetFieldInst*>(inst), chunk);
            break;

        // Array operations
        case ir::Instruction::Opcode::NewArray:
            compileNewArrayInst(static_cast<const ir::NewArrayInst*>(inst), chunk);
            break;
        case ir::Instruction::Opcode::GetElement:
            compileGetElementInst(static_cast<const ir::GetElementInst*>(inst), chunk);
            break;
        case ir::Instruction::Opcode::SetElement:
            compileSetElementInst(static_cast<const ir::SetElementInst*>(inst), chunk);
            break;

        // Control flow
        case ir::Instruction::Opcode::Br:
            compileBranchInst(static_cast<const ir::BranchInst*>(inst), chunk);
            break;
        case ir::Instruction::Opcode::BrIf:
            compileBranchIfInst(static_cast<const ir::BranchIfInst*>(inst), chunk);
            break;
        case ir::Instruction::Opcode::Ret:
            compileReturnInst(static_cast<const ir::ReturnInst*>(inst), chunk);
            break;

        // Function calls
        case ir::Instruction::Opcode::Call:
            compileCallInst(static_cast<const ir::CallInst*>(inst), chunk);
            break;
        case ir::Instruction::Opcode::CallForeign:
            compileCallForeignInst(static_cast<const ir::CallForeignInst*>(inst), chunk);
            break;

        default:
            throw std::runtime_error("Unsupported instruction opcode");
    }
}

void BytecodeCompiler::compileBinaryInst(const ir::BinaryInst* inst, Chunk& chunk) {
    // Step 1: Load both operands onto the stack
    emitLoadValue(inst->left(), chunk);
    emitLoadValue(inst->right(), chunk);

    // Step 2: Emit the operation
    switch (inst->opcode()) {
        // Arithmetic operations
        case ir::Instruction::Opcode::Add:
            chunk.emitOpcode(isIntType(inst->type()) ? Opcode::AddInt : Opcode::AddFloat);
            break;
        case ir::Instruction::Opcode::Sub:
            chunk.emitOpcode(isIntType(inst->type()) ? Opcode::SubInt : Opcode::SubFloat);
            break;
        case ir::Instruction::Opcode::Mul:
            chunk.emitOpcode(isIntType(inst->type()) ? Opcode::MulInt : Opcode::MulFloat);
            break;
        case ir::Instruction::Opcode::Div:
            chunk.emitOpcode(isIntType(inst->type()) ? Opcode::DivInt : Opcode::DivFloat);
            break;
        case ir::Instruction::Opcode::Mod:
            chunk.emitOpcode(Opcode::ModInt); // Only int modulo
            break;

        // Comparison operations (check left operand type for comparison ops)
        case ir::Instruction::Opcode::Eq:
            if (isIntType(inst->left()->type())) {
                chunk.emitOpcode(Opcode::EqInt);
            } else if (isFloatType(inst->left()->type())) {
                chunk.emitOpcode(Opcode::EqFloat);
            } else if (isBoolType(inst->left()->type())) {
                chunk.emitOpcode(Opcode::EqBool);
            }
            break;
        case ir::Instruction::Opcode::Ne:
            if (isIntType(inst->left()->type())) {
                chunk.emitOpcode(Opcode::NeInt);
            } else if (isFloatType(inst->left()->type())) {
                chunk.emitOpcode(Opcode::NeFloat);
            } else if (isBoolType(inst->left()->type())) {
                chunk.emitOpcode(Opcode::NeBool);
            }
            break;
        case ir::Instruction::Opcode::Lt:
            chunk.emitOpcode(isIntType(inst->left()->type()) ? Opcode::LtInt : Opcode::LtFloat);
            break;
        case ir::Instruction::Opcode::Le:
            chunk.emitOpcode(isIntType(inst->left()->type()) ? Opcode::LeInt : Opcode::LeFloat);
            break;
        case ir::Instruction::Opcode::Gt:
            chunk.emitOpcode(isIntType(inst->left()->type()) ? Opcode::GtInt : Opcode::GtFloat);
            break;
        case ir::Instruction::Opcode::Ge:
            chunk.emitOpcode(isIntType(inst->left()->type()) ? Opcode::GeInt : Opcode::GeFloat);
            break;

        // Logical operations
        case ir::Instruction::Opcode::And:
            chunk.emitOpcode(Opcode::And);
            break;
        case ir::Instruction::Opcode::Or:
            chunk.emitOpcode(Opcode::Or);
            break;

        default:
            break;
    }

    // Step 3: Store the result to a local variable for this instruction
    chunk.emitOpcode(Opcode::StoreLocal);
    chunk.emitInt32(getLocalIndex(inst));
}

void BytecodeCompiler::compileUnaryInst(const ir::UnaryInst* inst, Chunk& chunk) {
    emitLoadValue(inst->operand(), chunk);
    switch (inst->opcode()) {
        case ir::Instruction::Opcode::Neg: 
            chunk.emitOpcode(isIntType(inst->operand()->type()) ? Opcode::NegInt : Opcode::NegFloat);
            break;
        case ir::Instruction::Opcode::Not: 
            chunk.emitOpcode(Opcode::Not);
            break;
        default:
            throw std::runtime_error("Undefined unary.");
    }

    chunk.emitOpcode(Opcode::StoreLocal);
    chunk.emitInt32(getLocalIndex(inst));
}

void BytecodeCompiler::compileAllocInst(const ir::AllocInst* inst, Chunk& chunk) {
    // For primitive types (int, float, bool, string), we don't need heap allocation
    // The local slot itself can hold the value directly
    auto allocType = inst->allocatedType();
    auto kind = allocType->kind();

    bool isPrimitive = (kind == semantic::Type::Kind::Int ||
                       kind == semantic::Type::Kind::Float ||
                       kind == semantic::Type::Kind::Bool ||
                       kind == semantic::Type::Kind::String ||
                       kind == semantic::Type::Kind::Void);

    if (isPrimitive) {
        // Primitive types don't need heap allocation
        // Just initialize the local slot to null/zero
        chunk.emitOpcode(Opcode::ConstNull);
        chunk.emitOpcode(Opcode::StoreLocal);
        chunk.emitInt32(getLocalIndex(inst));
    } else {
        // For structs/arrays, allocate on the heap
        chunk.emitOpcode(Opcode::Alloc);
        chunk.emitInt32(0); // type-id (TODO: implement proper type indexing)
        chunk.emitInt32(8); // size in bytes (TODO: calculate from type)

        // Store the allocated address to a local
        chunk.emitOpcode(Opcode::StoreLocal);
        chunk.emitInt32(getLocalIndex(inst));
    }
}

void BytecodeCompiler::compileLoadInst(const ir::LoadInst* inst, Chunk& chunk) {
    // Check if we're loading from a primitive alloca
    // If so, just load directly from the local slot
    if (auto* allocInst = dynamic_cast<const ir::AllocInst*>(inst->address())) {
        auto kind = allocInst->allocatedType()->kind();
        bool isPrimitive = (kind == semantic::Type::Kind::Int ||
                           kind == semantic::Type::Kind::Float ||
                           kind == semantic::Type::Kind::Bool ||
                           kind == semantic::Type::Kind::String ||
                           kind == semantic::Type::Kind::Void);
        if (isPrimitive) {
            // Loading from a primitive local - just copy the local
            chunk.emitOpcode(Opcode::LoadLocal);
            chunk.emitInt32(getLocalIndex(allocInst));
            chunk.emitOpcode(Opcode::StoreLocal);
            chunk.emitInt32(getLocalIndex(inst));
            return;
        }
    }

    // Otherwise, load from heap memory
    emitLoadValue(inst->address(), chunk);  // Load address onto stack
    chunk.emitOpcode(Opcode::Load);         // Load from that address
    chunk.emitOpcode(Opcode::StoreLocal);
    chunk.emitInt32(getLocalIndex(inst));
}

void BytecodeCompiler::compileStoreInst(const ir::StoreInst* inst, Chunk& chunk) {
    // Check if we're storing to a primitive alloca
    // If so, just store directly to the local slot
    if (auto* allocInst = dynamic_cast<const ir::AllocInst*>(inst->address())) {
        auto kind = allocInst->allocatedType()->kind();
        bool isPrimitive = (kind == semantic::Type::Kind::Int ||
                           kind == semantic::Type::Kind::Float ||
                           kind == semantic::Type::Kind::Bool ||
                           kind == semantic::Type::Kind::String ||
                           kind == semantic::Type::Kind::Void);
        if (isPrimitive) {
            // Storing to a primitive local - just store directly
            emitLoadValue(inst->value(), chunk);    // Load value onto stack
            chunk.emitOpcode(Opcode::StoreLocal);
            chunk.emitInt32(getLocalIndex(allocInst));
            return;
        }
    }

    // Otherwise, store to heap memory
    emitLoadValue(inst->value(), chunk);    // Load value onto stack
    emitLoadValue(inst->address(), chunk);  // Load address onto stack
    chunk.emitOpcode(Opcode::Store);        // Store value to address
}

void BytecodeCompiler::compileGetFieldInst(const ir::GetFieldInst* inst, Chunk& chunk) {
    // Get field from struct: %result = get_field %object, field_index
    emitLoadValue(inst->object(), chunk);       // Load object onto stack
    chunk.emitOpcode(Opcode::GetField);         // Get field
    chunk.emitInt32(inst->fieldIndex());        // Field index

    // Store result
    chunk.emitOpcode(Opcode::StoreLocal);
    chunk.emitInt32(getLocalIndex(inst));
}

void BytecodeCompiler::compileSetFieldInst(const ir::SetFieldInst* inst, Chunk& chunk) {
    // Set field in struct: set_field %object, field_index, %value
    // Stack layout for VM: [... object value] -> SetField pops value then object
    emitLoadValue(inst->object(), chunk);       // Load object onto stack
    emitLoadValue(inst->value(), chunk);        // Load value onto stack
    chunk.emitOpcode(Opcode::SetField);         // Set field
    chunk.emitInt32(inst->fieldIndex());        // Field index

    // SetField doesn't produce a result
}

void BytecodeCompiler::compileNewArrayInst(const ir::NewArrayInst* inst, Chunk& chunk) {
    // Create new array: %result = new_array [%elem0, %elem1, ...]
    // Strategy:
    // 1. Push each element value onto the stack first
    // 2. Emit NewArray instruction with element count
    // 3. VM will pop elements and initialize the array
    // 4. Store the resulting array reference

    // Push each element onto the stack (VM will pop them in reverse order)
    for (const auto* elem : inst->elements()) {
        emitLoadValue(elem, chunk);
    }

    // Emit NewArray instruction with element count
    chunk.emitOpcode(Opcode::NewArray);
    chunk.emitInt32(static_cast<int32_t>(inst->elementCount()));

    // Store the result (array reference) in a local
    chunk.emitOpcode(Opcode::StoreLocal);
    chunk.emitInt32(getLocalIndex(inst));
}

void BytecodeCompiler::compileGetElementInst(const ir::GetElementInst* inst, Chunk& chunk) {
    // Get element from array: %result = get_element %array, %index
    emitLoadValue(inst->array(), chunk);        // Load array onto stack
    emitLoadValue(inst->index(), chunk);        // Load index onto stack
    chunk.emitOpcode(Opcode::GetElement);       // Get element

    // Store result
    chunk.emitOpcode(Opcode::StoreLocal);
    chunk.emitInt32(getLocalIndex(inst));
}

void BytecodeCompiler::compileSetElementInst(const ir::SetElementInst* inst, Chunk& chunk) {
    // Set element in array: set_element %array, %index, %value
    emitLoadValue(inst->array(), chunk);        // Load array onto stack
    emitLoadValue(inst->index(), chunk);        // Load index onto stack
    emitLoadValue(inst->value(), chunk);        // Load value onto stack
    chunk.emitOpcode(Opcode::SetElement);       // Set element

    // SetElement doesn't produce a result
}

void BytecodeCompiler::compileBranchInst(const ir::BranchInst* inst, Chunk& chunk) {
    // Unconditional branch: br label
    chunk.emitOpcode(Opcode::Jump);

    // Check if target block already has a known offset
    auto it = blockLabelMap_.find(inst->target());
    if (it != blockLabelMap_.end()) {
        // Backward jump - we know the offset
        int32_t targetOffset = it->second;
        int32_t currentOffset = chunk.currentOffset() + 4; // +4 for the operand we're about to emit
        int32_t relativeOffset = targetOffset - currentOffset;
        chunk.emitInt32(relativeOffset);
    } else {
        // Forward jump - need to patch later
        size_t patchOffset = chunk.currentOffset();
        chunk.emitInt32(0xFFFFFFFF); // Placeholder
        forwardJumps_.push_back({patchOffset, inst->target()});
    }
}

void BytecodeCompiler::compileBranchIfInst(const ir::BranchIfInst* inst, Chunk& chunk) {
    // Conditional branch: br_if %condition, then_label, else_label
    emitLoadValue(inst->condition(), chunk);
    chunk.emitOpcode(Opcode::JumpIfFalse);

    // Emit jump to else block (if condition is false)
    auto elseIt = blockLabelMap_.find(inst->elseBlock());
    if (elseIt != blockLabelMap_.end()) {
        // Backward jump
        int32_t targetOffset = elseIt->second;
        int32_t currentOffset = chunk.currentOffset() + 4;
        int32_t relativeOffset = targetOffset - currentOffset;
        chunk.emitInt32(relativeOffset);
    } else {
        // Forward jump - need to patch later
        size_t patchOffset = chunk.currentOffset();
        chunk.emitInt32(0xFFFFFFFF);
        forwardJumps_.push_back({patchOffset, inst->elseBlock()});
    }

    // Fall through to then block (if condition is true)
    // Emit unconditional jump to then block
    chunk.emitOpcode(Opcode::Jump);
    auto thenIt = blockLabelMap_.find(inst->thenBlock());
    if (thenIt != blockLabelMap_.end()) {
        // Backward jump
        int32_t targetOffset = thenIt->second;
        int32_t currentOffset = chunk.currentOffset() + 4;
        int32_t relativeOffset = targetOffset - currentOffset;
        chunk.emitInt32(relativeOffset);
    } else {
        // Forward jump - need to patch later
        size_t patchOffset = chunk.currentOffset();
        chunk.emitInt32(0xFFFFFFFF);
        forwardJumps_.push_back({patchOffset, inst->thenBlock()});
    }
}

void BytecodeCompiler::compileReturnInst(const ir::ReturnInst* inst, Chunk& chunk) {
    if (inst->hasReturnValue()) {
        emitLoadValue(inst->returnValue(), chunk);
        chunk.emitOpcode(Opcode::Return);
    } else {
        chunk.emitOpcode(Opcode::ReturnVoid);
    }
}

void BytecodeCompiler::compileCallInst(const ir::CallInst* inst, Chunk& chunk) {
    for (auto& arg: inst->arguments()) {
        emitLoadValue(arg, chunk);
    }

    chunk.emitOpcode(Opcode::Call);
    chunk.emitInt32(getFunctionIndex(inst->callee()));
    chunk.emitInt32(inst->arguments().size());

    if (inst->type()->kind() != semantic::Type::Kind::Void) {
        chunk.emitOpcode(Opcode::StoreLocal);
        chunk.emitInt32(getLocalIndex(inst));
    }
}

void BytecodeCompiler::compileCallForeignInst(const ir::CallForeignInst* inst, Chunk& chunk) {
    // TODO: Check if correct.
    for (auto& arg: inst->arguments()) {
        emitLoadValue(arg, chunk);
    }

    chunk.emitOpcode(Opcode::CallForeign);
    chunk.emitInt32(getForeignFunctionIndex(inst->foreignName()));
    chunk.emitInt32(inst->arguments().size());

    if (inst->type()->kind() != semantic::Type::Kind::Void) {
        chunk.emitOpcode(Opcode::StoreLocal);
        chunk.emitInt32(getLocalIndex(inst));
    }
}

void BytecodeCompiler::emitLoadValue(const ir::Value* value, Chunk& chunk) {
    if (!value) {
        // Null value - this might happen with unimplemented features
        chunk.emitOpcode(Opcode::ConstNull);
        return;
    }
    if (value->kind() == ir::Value::Kind::Constant) {
        emitConstant(static_cast<const ir::Constant*>(value), chunk);
    } else if (value->kind() == ir::Value::Kind::Parameter) {
        // Parameters are just the first N local variables!
        const ir::Parameter* param = static_cast<const ir::Parameter*>(value);
        chunk.emitOpcode(Opcode::LoadLocal);
        chunk.emitInt32(param->index());
    } else if (value->kind() == ir::Value::Kind::GlobalVariable) {
        const ir::GlobalVariable* global = static_cast<const ir::GlobalVariable*>(value);
        chunk.emitOpcode(Opcode::LoadGlobal);
        chunk.emitInt32(getGlobalIndex(global));
    } else if (value->kind() == ir::Value::Kind::Instruction) {
        // Instructions produce values that are stored in local variables
        // We need to look up which local slot this SSA value is in
        chunk.emitOpcode(Opcode::LoadLocal);
        chunk.emitInt32(getLocalIndex(value));
    }
}

void BytecodeCompiler::emitConstant(const ir::Constant* constant, Chunk& chunk) {
    if (std::holds_alternative<int64_t>(constant->value())) {
        int64_t val = std::get<int64_t>(constant->value());
        chunk.emitOpcode(Opcode::ConstInt);
        chunk.emitInt64(val);
    } else if (std::holds_alternative<double>(constant->value())) {
        double val = std::get<double>(constant->value());
        chunk.emitOpcode(Opcode::ConstFloat);
        chunk.emitFloat64(val);
    } else if (std::holds_alternative<std::string>(constant->value())) {
        std::string strVal = std::get<std::string>(constant->value());
        uint32_t val = addStringConstant(strVal);
        chunk.emitOpcode(Opcode::ConstString);
        chunk.emitInt32(val); // Emit pool idx
    } else if (std::holds_alternative<bool>(constant->value())) {
        bool val = std::get<bool>(constant->value());
        chunk.emitOpcode(Opcode::ConstBool);
        chunk.emitBool(val);
    } else if (std::holds_alternative<std::monostate>(constant->value())) {
        chunk.emitOpcode(Opcode::ConstNull);
    } else {
        throw std::runtime_error("New constant type unsupported.");
    }
}

uint32_t BytecodeCompiler::getFunctionIndex(const ir::Function* function) const {
    auto it = functionIndexMap_.find(function);

    if (it != functionIndexMap_.end()) {
        return it->second;
    }

    throw std::runtime_error("Tried to access non-existant function index.");
}

uint32_t BytecodeCompiler::getForeignFunctionIndex(const std::string& name) const {
    auto it = foreignFunctionMap_.find(name);

    if (it != foreignFunctionMap_.end()) {
        return it->second;
    }

    throw std::runtime_error("Tried to access non-existant foreign function index.");
}

uint32_t BytecodeCompiler::getLocalIndex(const ir::Value* value) const {
    auto it = localIndexMap_.find(value);

    if (it != localIndexMap_.end()) {
        return it->second;
    }

    throw std::runtime_error("Tried to access non-existant local index.");
}

uint32_t BytecodeCompiler::getGlobalIndex(const ir::GlobalVariable* global) const {
    auto it = globalIndexMap_.find(global);

    if (it != globalIndexMap_.end()) {
        return it->second;
    }

    throw std::runtime_error("Tried to access non-existant local index.");
}

uint32_t BytecodeCompiler::addStringConstant(const std::string& str) {
    auto it = stringPoolMap_.find(str);
    if (it != stringPoolMap_.end()) {
        return it->second;
    }
    
    size_t idx = stringPool_.size();
    stringPoolMap_[str] = idx;
    stringPool_.push_back(str);
    
    return idx;
}

size_t BytecodeCompiler::getBlockLabel(const ir::BasicBlock* block) {
    auto it = blockLabelMap_.find(block);

    if (it != blockLabelMap_.end()) {
        return it->second;
    }

    throw std::runtime_error("Tried to access non-existant block.");
}

void BytecodeCompiler::defineBlockLabel(const ir::BasicBlock* block, Chunk& chunk) {
    // Record where this block starts in the bytecode
    size_t blockOffset = chunk.currentOffset();
    blockLabelMap_[block] = blockOffset;

    // Patch all forward jumps to this block
    for (auto it = forwardJumps_.begin(); it != forwardJumps_.end(); ) {
        if (it->second == block) {
            // Calculate relative offset from jump instruction
            size_t jumpOffset = it->first;
            int32_t relativeOffset = blockOffset - (jumpOffset + 4); // +4 because operand is 4 bytes
            chunk.patchInt32(jumpOffset, relativeOffset);
            it = forwardJumps_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace volta::bytecode
