#pragma once

#include "IRModule.hpp"
#include "Function.hpp"
#include "BasicBlock.hpp"
#include "Instruction.hpp"
#include "Value.hpp"
#include "../semantic/Type.hpp"
#include <memory>
#include <string>

namespace volta::ir {

// ============================================================================
// IRBuilder - High-level API for constructing IR
// ============================================================================

/**
 * IRBuilder provides a convenient API for building IR.
 *
 * This is YOUR main tool for Phase 2!
 *
 * Instead of manually creating Instructions and managing operands,
 * you'll use IRBuilder methods like:
 *   builder.createAdd(lhs, rhs)
 *   builder.createLoad(ptr)
 *   builder.createCondBr(cond, thenBlock, elseBlock)
 *
 * IRBuilder handles:
 * - SSA value numbering (%0, %1, %2, ...)
 * - Instruction creation and insertion into current block
 * - Type checking and inference
 * - Use-def chain management
 *
 * Example usage:
 *   IRBuilder builder;
 *   Function* fn = module->createFunction("add", ...);
 *   BasicBlock* entry = fn->createBasicBlock("entry");
 *   builder.setInsertPoint(entry);
 *
 *   Value* a = fn->getParameter(0);
 *   Value* b = fn->getParameter(1);
 *   Value* sum = builder.createAdd(a, b);
 *   builder.createRet(sum);
 */
class IRBuilder {
public:
    IRBuilder();

    // ========== Insert Point Management ==========

    /**
     * Set where new instructions will be inserted.
     * All createXXX methods will add instructions to this block.
     */
    void setInsertPoint(BasicBlock* block);
    BasicBlock* getInsertBlock() const { return insertBlock_; }

    // ========== Arithmetic Operations ==========

    Value* createAdd(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createSub(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createMul(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createDiv(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createMod(Value* lhs, Value* rhs, const std::string& name = "");

    Value* createFAdd(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createFSub(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createFMul(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createFDiv(Value* lhs, Value* rhs, const std::string& name = "");

    // ========== Comparison Operations ==========

    Value* createICmpEQ(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createICmpNE(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createICmpLT(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createICmpLE(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createICmpGT(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createICmpGE(Value* lhs, Value* rhs, const std::string& name = "");

    Value* createFCmpEQ(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createFCmpNE(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createFCmpLT(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createFCmpLE(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createFCmpGT(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createFCmpGE(Value* lhs, Value* rhs, const std::string& name = "");

    // ========== Logical Operations ==========

    Value* createAnd(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createOr(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createNot(Value* operand, const std::string& name = "");

    // ========== Memory Operations (LLVM style) ==========

    /**
     * Allocate stack memory for a local variable.
     * Returns a pointer to the allocated space.
     *
     * Example:
     *   Value* ptr = builder.createAlloca(Type::getInt(), "x");
     *   // %x = alloca i64
     */
    Value* createAlloca(std::shared_ptr<semantic::Type> type, const std::string& name = "");

    /**
     * Load a value from memory.
     *
     * Example:
     *   Value* val = builder.createLoad(ptr);
     *   // %0 = load i64, ptr %x
     */
    Value* createLoad(Value* pointer, const std::string& name = "");

    /**
     * Store a value to memory.
     * Note: Store returns void (no SSA value).
     *
     * Example:
     *   builder.createStore(value, ptr);
     *   // store i64 42, ptr %x
     */
    void createStore(Value* value, Value* pointer);

    // ========== Control Flow ==========

    /**
     * Unconditional branch.
     *
     * Example:
     *   builder.createBr(targetBlock);
     *   // br label %target
     */
    void createBr(BasicBlock* target);

    /**
     * Conditional branch.
     *
     * Example:
     *   builder.createCondBr(cond, thenBlock, elseBlock);
     *   // br i1 %cond, label %then, label %else
     */
    void createCondBr(Value* condition, BasicBlock* thenBlock, BasicBlock* elseBlock);

    /**
     * Return from function.
     *
     * Examples:
     *   builder.createRet(value);  // ret i64 %value
     *   builder.createRetVoid();   // ret void
     */
    void createRet(Value* value);
    void createRetVoid();

    // ========== Function Calls ==========

    /**
     * Call a function.
     *
     * Example:
     *   Value* result = builder.createCall(func, {arg1, arg2});
     *   // %0 = call i64 @add(i64 %arg1, i64 %arg2)
     */
    Value* createCall(Function* callee, const std::vector<Value*>& arguments, const std::string& name = "");

    // ========== PHI Nodes (SSA) ==========

    /**
     * Create a PHI node for merging values from different control flow paths.
     *
     * Example:
     *   PhiInstruction* phi = builder.createPhi(Type::getInt(), "merged");
     *   phi->addIncoming(val1, block1);
     *   phi->addIncoming(val2, block2);
     *   // %merged = phi i64 [ %val1, %block1 ], [ %val2, %block2 ]
     */
    PhiInstruction* createPhi(std::shared_ptr<semantic::Type> type, const std::string& name = "");

    // ========== SSA Value Naming ==========

    /**
     * Generate a unique SSA name (%0, %1, %2, ...).
     * Called automatically by createXXX methods when name is empty.
     */
    std::string getUniqueName();

private:
    // Current insertion point
    BasicBlock* insertBlock_;

    // SSA value counter (for generating %0, %1, %2, ...)
    size_t valueCounter_;

    // Helper: Insert instruction into current block
    void insertInstruction(std::unique_ptr<Instruction> inst);
};

} // namespace volta::ir
