#pragma once

#include "IR.hpp"
#include <memory>
#include <vector>
#include <deque>
#include <unordered_map>
#include <string>

namespace volta::ir {

/**
 * IR Module - container for an entire program's IR
 * Contains:
 * - Functions (user-defined and foreign declarations)
 * - Global variables
 * - String literals (for constant pool)
 */
class IRModule {
public:
    IRModule(const std::string& name) : name_(name) {}

    const std::string& name() const { return name_; }

    // ========== Function Management ==========

    /**
     * Add a function to the module
     * Returns pointer to the added function
     */
    Function* addFunction(std::unique_ptr<Function> function);

    /**
     * Get function by name
     * Returns nullptr if not found
     */
    Function* getFunction(const std::string& name) const;

    /**
     * Get all functions
     */
    const std::vector<std::unique_ptr<Function>>& functions() const { return functions_; }

    // ========== Global Variable Management ==========

    /**
     * Add a global variable
     * Returns pointer to the added global
     */
    GlobalVariable* addGlobal(std::unique_ptr<GlobalVariable> global);

    /**
     * Get global by name
     * Returns nullptr if not found
     */
    GlobalVariable* getGlobal(const std::string& name) const;

    /**
     * Get all globals
     */
    const std::vector<std::unique_ptr<GlobalVariable>>& globals() const { return globals_; }

    // ========== Value Arena Management (Memory Safety) ==========

    /**
     * Create a constant value in the module's arena
     * Constants are deduplicated - same value returns same pointer
     * Memory is owned by the module and lives until module destruction
     */
    Constant* createConstant(std::shared_ptr<semantic::Type> type,
                            const Constant::ConstantValue& value);

    /**
     * Create a parameter in the module's arena
     * Memory is owned by the module
     */
    Parameter* createParameter(std::shared_ptr<semantic::Type> type,
                              const std::string& name,
                              size_t index);

    /**
     * Create an instruction in the module's arena
     * Memory is owned by the module
     * Returns raw pointer - valid until module destruction
     */
    template<typename T, typename... Args>
    T* createInstruction(Args&&... args) {
        auto inst = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = inst.get();
        instructionArena_.push_back(std::move(inst));
        return ptr;
    }

    /**
     * Create a basic block in the module's arena
     * Memory is owned by the module
     * Returns raw pointer - valid until module destruction
     */
    BasicBlock* createBasicBlock(const std::string& name, Function* parent = nullptr);

    // ========== String Literal Management ==========

    /**
     * Add a string literal to the constant pool
     * Returns the index of the string (used for bytecode generation)
     * If string already exists, returns existing index
     */
    size_t addStringLiteral(const std::string& str);

    /**
     * Get string literal by index
     */
    const std::string& getStringLiteral(size_t index) const;

    /**
     * Get all string literals
     */
    const std::vector<std::string>& stringLiterals() const { return stringLiterals_; }

    // ========== Foreign Function Interface ==========

    /**
     * Register a foreign (C) function declaration
     * This creates a Function object marked as foreign
     */
    Function* declareForeignFunction(
        const std::string& name,
        std::shared_ptr<semantic::FunctionType> type);

    // ========== Validation ==========

    /**
     * Verify the module is well-formed
     * Checks:
     * - All basic blocks end with terminators
     * - All branches target valid blocks
     * - All values are defined before use
     * - Type consistency
     * Returns true if valid, false otherwise
     */
    bool verify(std::ostream& errors) const;

private:
    std::string name_;

    // Functions in this module
    std::vector<std::unique_ptr<Function>> functions_;
    std::unordered_map<std::string, Function*> functionMap_;

    // Global variables
    std::vector<std::unique_ptr<GlobalVariable>> globals_;
    std::unordered_map<std::string, GlobalVariable*> globalMap_;

    // String literals (for constant pool)
    std::vector<std::string> stringLiterals_;
    std::unordered_map<std::string, size_t> stringLiteralMap_;

    // ========== Memory Arenas (owns all Values) ==========

    // Arena for constants (with deduplication)
    std::vector<std::unique_ptr<Constant>> constants_;

    // Deduplication maps for constants
    std::unordered_map<int64_t, Constant*> intConstantMap_;
    std::unordered_map<double, Constant*> floatConstantMap_;
    std::unordered_map<bool, Constant*> boolConstantMap_;
    std::unordered_map<std::string, Constant*> stringConstantMap_;
    Constant* noneConstant_ = nullptr;

    // Arena for parameters
    std::vector<std::unique_ptr<Parameter>> parameters_;

    // Arena for all instructions (bulk allocation)
    std::deque<std::unique_ptr<Instruction>> instructionArena_;

    // Arena for all basic blocks
    std::deque<std::unique_ptr<BasicBlock>> basicBlockArena_;
};

} // namespace volta::ir
