#pragma once

#include "Bytecode.hpp"
#include "../IR/IRModule.hpp"
#include "../IR/IR.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

namespace volta::bytecode {

/**
 * Compiled function metadata
 * Contains bytecode chunk and metadata for a single function
 */
struct CompiledFunction {
    std::string name;                     ///< Function name
    uint32_t index;                       ///< Function index in module
    uint32_t parameterCount;              ///< Number of parameters
    uint32_t localCount;                  ///< Number of local variables (params + locals)
    Chunk chunk;                          ///< Bytecode for this function
    bool isForeign;                       ///< Is this a foreign function?
};

/**
 * Compiled module - complete bytecode program
 * Contains all compiled functions, globals, and constant pools
 */
class CompiledModule {
public:
    CompiledModule(const std::string& name) : name_(name) {}

    const std::string& name() const { return name_; }

    /// Get all compiled functions
    const std::vector<CompiledFunction>& functions() const { return functions_; }
    std::vector<CompiledFunction>& functions() { return functions_; }

    /// Get function by index
    const CompiledFunction* getFunction(uint32_t index) const;

    /// Get function by name
    const CompiledFunction* getFunction(const std::string& name) const;

    /// Get entry point function (e.g., "main")
    const CompiledFunction* getEntryPoint() const;

    /// String constant pool
    const std::vector<std::string>& stringPool() const { return stringPool_; }
    std::vector<std::string>& stringPool() { return stringPool_; }

    /// Global variable metadata (count and initialization info)
    uint32_t globalCount() const { return globalCount_; }
    void setGlobalCount(uint32_t count) { globalCount_ = count; }

    /// Foreign function declarations
    const std::vector<std::string>& foreignFunctions() const { return foreignFunctions_; }
    std::vector<std::string>& foreignFunctions() { return foreignFunctions_; }

private:
    std::string name_;
    std::vector<CompiledFunction> functions_;
    std::vector<std::string> stringPool_;
    std::vector<std::string> foreignFunctions_;
    uint32_t globalCount_ = 0;
};

/**
 * Bytecode compiler - translates IR to stack-based bytecode
 *
 * Architecture:
 * - Stack-based evaluation (no registers)
 * - Local variables indexed by number
 * - Global variables indexed by number
 * - Jump offsets are relative to current instruction
 * - Function calls use call stack
 *
 * Compilation strategy:
 * 1. Build function index (name -> index mapping)
 * 2. Compile each function's basic blocks
 * 3. Convert IR instructions to bytecode
 * 4. Resolve labels and patch jumps
 * 5. Build constant pools
 */
class BytecodeCompiler {
public:
    BytecodeCompiler() = default;

    /**
     * Compile an IR module to bytecode
     * Returns compiled module ready for execution
     */
    std::unique_ptr<CompiledModule> compile(const ir::IRModule& module);

private:
    // ========== Function Compilation ==========

    /**
     * Compile a single IR function to bytecode
     * Returns CompiledFunction with bytecode chunk
     */
    CompiledFunction compileFunction(const ir::Function* function);

    /**
     * Compile a single basic block within a function
     * Emits bytecode for all instructions in the block
     */
    void compileBasicBlock(const ir::BasicBlock* block, Chunk& chunk);

    /**
     * Compile a single IR instruction to bytecode
     * Emits one or more bytecode instructions
     */
    void compileInstruction(const ir::Instruction* inst, Chunk& chunk);

    // ========== Instruction Compilation Helpers ==========

    /// Compile binary arithmetic/comparison instruction
    void compileBinaryInst(const ir::BinaryInst* inst, Chunk& chunk);

    /// Compile unary instruction (negation, not)
    void compileUnaryInst(const ir::UnaryInst* inst, Chunk& chunk);

    /// Compile allocation instruction
    void compileAllocInst(const ir::AllocInst* inst, Chunk& chunk);

    /// Compile load instruction
    void compileLoadInst(const ir::LoadInst* inst, Chunk& chunk);

    /// Compile store instruction
    void compileStoreInst(const ir::StoreInst* inst, Chunk& chunk);

    /// Compile get field instruction
    void compileGetFieldInst(const ir::GetFieldInst* inst, Chunk& chunk);

    /// Compile set field instruction
    void compileSetFieldInst(const ir::SetFieldInst* inst, Chunk& chunk);

    /// Compile get element instruction (array indexing)
    void compileGetElementInst(const ir::GetElementInst* inst, Chunk& chunk);

    /// Compile set element instruction (array assignment)
    void compileSetElementInst(const ir::SetElementInst* inst, Chunk& chunk);

    /// Compile unconditional branch
    void compileBranchInst(const ir::BranchInst* inst, Chunk& chunk);

    /// Compile conditional branch
    void compileBranchIfInst(const ir::BranchIfInst* inst, Chunk& chunk);

    /// Compile return instruction
    void compileReturnInst(const ir::ReturnInst* inst, Chunk& chunk);

    /// Compile function call
    void compileCallInst(const ir::CallInst* inst, Chunk& chunk);

    /// Compile foreign function call
    void compileCallForeignInst(const ir::CallForeignInst* inst, Chunk& chunk);

    // ========== Value Emission ==========

    /**
     * Emit bytecode to load a value onto the stack
     * Handles constants, parameters, local variables, globals
     */
    void emitLoadValue(const ir::Value* value, Chunk& chunk);

    /**
     * Emit a constant value
     * Chooses appropriate ConstInt/ConstFloat/ConstBool/ConstString instruction
     */
    void emitConstant(const ir::Constant* constant, Chunk& chunk);

    // ========== Symbol Resolution ==========

    /**
     * Get the bytecode function index for an IR function
     * Used for Call instructions
     */
    uint32_t getFunctionIndex(const ir::Function* function) const;

    /**
     * Get the foreign function index for a foreign name
     * Used for CallForeign instructions
     */
    uint32_t getForeignFunctionIndex(const std::string& name) const;

    /**
     * Get the local variable index for an IR value
     * Maps IR SSA values to stack frame slots
     */
    uint32_t getLocalIndex(const ir::Value* value) const;

    /**
     * Get the global variable index for an IR global
     */
    uint32_t getGlobalIndex(const ir::GlobalVariable* global) const;

    /**
     * Add a string to the string pool, return its index
     * Deduplicates identical strings
     */
    uint32_t addStringConstant(const std::string& str);

    // ========== Label Resolution ==========

    /**
     * Get or create a label placeholder for a basic block
     * Used for forward jumps that need to be patched later
     */
    size_t getBlockLabel(const ir::BasicBlock* block);

    /**
     * Mark the current bytecode offset as the target of a basic block
     * Patches all forward jumps to this block
     */
    void defineBlockLabel(const ir::BasicBlock* block, Chunk& chunk);

    // ========== State ==========

    /// Current module being compiled
    const ir::IRModule* currentModule_ = nullptr;

    /// Current function being compiled
    const ir::Function* currentFunction_ = nullptr;

    /// IR Function -> bytecode function index
    std::unordered_map<const ir::Function*, uint32_t> functionIndexMap_;

    /// Foreign function name -> index
    std::unordered_map<std::string, uint32_t> foreignFunctionMap_;

    /// IR Value -> local variable index (within current function)
    std::unordered_map<const ir::Value*, uint32_t> localIndexMap_;

    /// IR GlobalVariable -> global variable index
    std::unordered_map<const ir::GlobalVariable*, uint32_t> globalIndexMap_;

    /// String constant pool (shared across all functions)
    std::vector<std::string> stringPool_;
    std::unordered_map<std::string, uint32_t> stringPoolMap_;

    /// Basic block -> bytecode offset (for jump resolution)
    std::unordered_map<const ir::BasicBlock*, size_t> blockLabelMap_;

    /// Forward jump patches: <chunk-offset, target-block>
    /// These are jumps to blocks we haven't compiled yet
    std::vector<std::pair<size_t, const ir::BasicBlock*>> forwardJumps_;
};

} // namespace volta::bytecode
