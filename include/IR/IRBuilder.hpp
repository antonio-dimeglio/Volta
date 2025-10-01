#pragma once

#include "IR.hpp"
#include "IRModule.hpp"
#include <memory>
#include <string>
#include <stdexcept>

namespace volta::ir {

/**
 * IRBuilder - Helper class for constructing IR
 *
 * This class provides a convenient API for building IR instructions.
 * It automatically:
 * - Generates unique names for temporaries (%0, %1, %2, ...)
 * - Inserts instructions into the current basic block
 * - Manages the current function and block context
 *
 * Example usage:
 *   IRBuilder builder;
 *   Function* func = builder.createFunction("add", ...);
 *   BasicBlock* entry = builder.createBasicBlock("entry", func);
 *   builder.setInsertPoint(entry);
 *
 *   Value* sum = builder.createAdd(param0, param1);
 *   builder.createRet(sum);
 */
class IRBuilder {
public:
    IRBuilder() : currentBlock_(nullptr), nextTempId_(0) {}

    // ========== Context Management ==========

    /**
     * Set the current insertion point
     * New instructions will be added to this block
     */
    void setInsertPoint(BasicBlock* block) { currentBlock_ = block; }

    /**
     * Get current insertion point
     */
    BasicBlock* insertPoint() const { return currentBlock_; }

    /**
     * Clear insertion point
     */
    void clearInsertPoint() { currentBlock_ = nullptr; }

    // ========== Function & Basic Block Creation ==========

    /**
     * Create a new function
     * Does not add to module - caller must add it
     */
    std::unique_ptr<Function> createFunction(
        const std::string& name,
        std::shared_ptr<semantic::FunctionType> type);

    /**
     * Create a new basic block
     * If parent is provided, adds block to parent function
     */
    std::unique_ptr<BasicBlock> createBasicBlock(
        const std::string& name,
        Function* parent = nullptr);

    // ========== Arithmetic Instructions ==========

    Value* createAdd(Value* left, Value* right);
    Value* createSub(Value* left, Value* right);
    Value* createMul(Value* left, Value* right);
    Value* createDiv(Value* left, Value* right);
    Value* createMod(Value* left, Value* right);
    Value* createNeg(Value* operand);

    // ========== Comparison Instructions ==========

    Value* createEq(Value* left, Value* right);
    Value* createNe(Value* left, Value* right);
    Value* createLt(Value* left, Value* right);
    Value* createLe(Value* left, Value* right);
    Value* createGt(Value* left, Value* right);
    Value* createGe(Value* left, Value* right);

    // ========== Logical Instructions ==========

    Value* createAnd(Value* left, Value* right);
    Value* createOr(Value* left, Value* right);
    Value* createNot(Value* operand);

    // ========== Memory Instructions ==========

    /**
     * Allocate memory for a value of given type
     * Returns a pointer-like value
     */
    Value* createAlloc(std::shared_ptr<semantic::Type> type);

    /**
     * Load value from address
     */
    Value* createLoad(std::shared_ptr<semantic::Type> type, Value* address);

    /**
     * Store value to address
     */
    void createStore(Value* value, Value* address);

    /**
     * Get struct field
     */
    Value* createGetField(Value* object, size_t fieldIndex,
                         std::shared_ptr<semantic::Type> fieldType);

    /**
     * Set struct field
     */
    void createSetField(Value* object, size_t fieldIndex, Value* value);

    /**
     * Get array element
     */
    Value* createGetElement(Value* array, Value* index,
                           std::shared_ptr<semantic::Type> elementType);

    /**
     * Set array element
     */
    void createSetElement(Value* array, Value* index, Value* value);

    // ========== Control Flow Instructions ==========

    /**
     * Unconditional branch
     */
    void createBr(BasicBlock* target);

    /**
     * Conditional branch
     */
    void createBrIf(Value* condition, BasicBlock* thenBlock, BasicBlock* elseBlock);

    /**
     * Return void
     */
    void createRetVoid();

    /**
     * Return value
     */
    void createRet(Value* value);

    /**
     * Function call
     */
    Value* createCall(Function* callee, const std::vector<Value*>& arguments);

    /**
     * Foreign function call
     */
    Value* createCallForeign(const std::string& foreignName,
                            std::shared_ptr<semantic::Type> returnType,
                            const std::vector<Value*>& arguments);

    // ========== Constant Creation ==========

    /**
     * Create integer constant
     */
    Constant* getInt(int64_t value);

    /**
     * Create float constant
     */
    Constant* getFloat(double value);

    /**
     * Create boolean constant
     */
    Constant* getBool(bool value);

    /**
     * Create string constant
     */
    Constant* getString(const std::string& value);

    /**
     * Create none constant
     */
    Constant* getNone();

    // ========== Parameter Creation ==========

    /**
     * Create function parameter
     */
    std::unique_ptr<Parameter> createParameter(
        std::shared_ptr<semantic::Type> type,
        const std::string& name,
        size_t index);

    // ========== Helpers ==========

    /**
     * Generate a unique temporary name (%0, %1, %2, ...)
     */
    std::string getUniqueTempName();

    /**
     * Reset temporary counter (for new function)
     */
    void resetTempCounter() { nextTempId_ = 0; }

private:
    BasicBlock* currentBlock_;
    size_t nextTempId_;

    // Cache for constants (avoid duplicates)
    std::unordered_map<int64_t, std::unique_ptr<Constant>> intConstants_;
    std::unordered_map<double, std::unique_ptr<Constant>> floatConstants_;
    std::unordered_map<bool, std::unique_ptr<Constant>> boolConstants_;
    std::unordered_map<std::string, std::unique_ptr<Constant>> stringConstants_;
    std::unique_ptr<Constant> noneConstant_;

    // Helper to insert instruction
    template<typename T>
    T* insert(std::unique_ptr<T> inst) {
        if (!currentBlock_) {
            throw std::runtime_error("No insertion point set");
        }
        T* ptr = inst.get();
        currentBlock_->addInstruction(std::move(inst));
        return ptr;
    }

    // Helper for binary operations
    Value* createBinaryOp(Instruction::Opcode opcode, Value* left, Value* right);

    // Helper for unary operations
    Value* createUnaryOp(Instruction::Opcode opcode, Value* operand);
};

} // namespace volta::ir
