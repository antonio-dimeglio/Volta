#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "IR/Module.hpp"
#include "IR/Function.hpp"
#include "IR/BasicBlock.hpp"
#include "IR/Instruction.hpp"
#include "IR/Value.hpp"
#include "IR/IRType.hpp"

namespace volta::ir {

/**
 * IRBuilder - High-level API for constructing SSA-form IR
 *
 * Design Philosophy:
 * - Builder Pattern: Fluent API for IR construction
 * - Automatic SSA: Manages SSA form automatically (with alloca/load/store initially)
 * - Insertion Point: Tracks where to insert new instructions
 * - Type Safety: Validates types before creating instructions
 * - Error Prevention: Makes it hard to create invalid IR
 *
 * Key Concepts:
 * - Insertion Point: Current location where new instructions are inserted
 * - SSA Variables: Automatic management of SSA values
 * - Block Management: Helpers for creating and switching between blocks
 * - Type Context: Access to module's type system
 *
 * Usage Pattern:
 *   IRBuilder builder(module);
 *   Function* func = builder.createFunction("main", intType, {});
 *   builder.setInsertionPoint(func->getEntryBlock());
 *
 *   // Build IR fluently
 *   Value* x = builder.createAdd(a, b, "sum");
 *   Value* y = builder.createMul(x, c, "product");
 *   builder.createRet(y);
 *
 * Learning Goals:
 * - Understand builder pattern for complex construction
 * - Master SSA form construction techniques
 * - Learn how to design user-friendly APIs
 * - Practice type checking and validation
 * - Appreciate LLVM IRBuilder design
 */
class IRBuilder {
public:
    /**
     * Create a new IR builder
     * @param module The module to build IR in
     */
    explicit IRBuilder(Module& module);

    // ========================================================================
    // Insertion Point Management
    // ========================================================================

    /**
     * Set the insertion point to the end of a basic block
     * All subsequent create* calls will insert instructions here
     */
    void setInsertionPoint(BasicBlock* block);

    /**
     * Set insertion point before a specific instruction
     */
    void setInsertionPointBefore(Instruction* inst);

    /**
     * Set insertion point after a specific instruction
     * LEARNING TIP: Must handle the case where inst is a terminator!
     */
    void setInsertionPointAfter(Instruction* inst);

    /**
     * Get current insertion block (nullptr if not set)
     */
    BasicBlock* getInsertionBlock() const { return insertionBlock_; }

    /**
     * Get current insertion point (nullptr if inserting at end)
     */
    Instruction* getInsertionPoint() const { return insertionPoint_; }

    /**
     * Check if insertion point is set
     */
    bool hasInsertionPoint() const { return insertionBlock_ != nullptr; }

    /**
     * Clear insertion point (no longer inserting anywhere)
     */
    void clearInsertionPoint();

    // ========================================================================
    // Type Helpers (Convenience Access to Module Types)
    // ========================================================================

    std::shared_ptr<IRType> getIntType();
    std::shared_ptr<IRType> getFloatType();
    std::shared_ptr<IRType> getBoolType();
    std::shared_ptr<IRType> getStringType();
    std::shared_ptr<IRType> getVoidType();
    std::shared_ptr<IRType> getPointerType(std::shared_ptr<IRType> pointeeType);
    std::shared_ptr<IRType> getArrayType(std::shared_ptr<IRType> elementType, size_t size);
    std::shared_ptr<IRType> getOptionType(std::shared_ptr<IRType> innerType);

    // ========================================================================
    // Function and Block Creation
    // ========================================================================

    /**
     * Create a new function and optionally set insertion point to its entry block
     * @param name Function name
     * @param returnType Return type
     * @param paramTypes Parameter types
     * @param setAsInsertionPoint If true, set insertion point to entry block
     * @return The created function
     *
     * LEARNING TIP: This should call module.createFunction() and
     * automatically create an entry block for the function
     */
    Function* createFunction(const std::string& name,
                            std::shared_ptr<IRType> returnType,
                            const std::vector<std::shared_ptr<IRType>>& paramTypes = {},
                            bool setAsInsertionPoint = true);

    /**
     * Create a basic block in the current function
     * @param name Block name
     * @param insertIntoFunction If true and there's a current function, add block to it
     * @return The created block
     *
     * LEARNING TIP: How do we know what the "current function" is?
     * Hint: It's the parent of insertionBlock_
     */
    BasicBlock* createBasicBlock(const std::string& name = "",
                                 bool insertIntoFunction = true);

    // ========================================================================
    // Constant Creation
    // ========================================================================

    ConstantInt* getInt(int64_t value);
    ConstantFloat* getFloat(double value);
    ConstantBool* getBool(bool value);
    ConstantBool* getTrue();
    ConstantBool* getFalse();
    ConstantString* getString(const std::string& value);
    ConstantNone* getNone(std::shared_ptr<IRType> optionType);
    UndefValue* getUndef(std::shared_ptr<IRType> type);

    // ========================================================================
    // Arithmetic Instructions
    // ========================================================================

    /**
     * Create addition: result = lhs + rhs
     * LEARNING TIP: Check that lhs and rhs have compatible types!
     */
    Value* createAdd(Value* lhs, Value* rhs, const std::string& name = "");

    /**
     * Create subtraction: result = lhs - rhs
     */
    Value* createSub(Value* lhs, Value* rhs, const std::string& name = "");

    /**
     * Create multiplication: result = lhs * rhs
     */
    Value* createMul(Value* lhs, Value* rhs, const std::string& name = "");

    /**
     * Create division: result = lhs / rhs
     */
    Value* createDiv(Value* lhs, Value* rhs, const std::string& name = "");

    /**
     * Create remainder: result = lhs % rhs
     */
    Value* createRem(Value* lhs, Value* rhs, const std::string& name = "");

    /**
     * Create power: result = lhs ** rhs
     */
    Value* createPow(Value* lhs, Value* rhs, const std::string& name = "");

    /**
     * Create negation: result = -operand
     */
    Value* createNeg(Value* operand, const std::string& name = "");

    // ========================================================================
    // Comparison Instructions
    // ========================================================================

    Value* createEq(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createNe(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createLt(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createLe(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createGt(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createGe(Value* lhs, Value* rhs, const std::string& name = "");

    // ========================================================================
    // Logical Instructions
    // ========================================================================

    Value* createAnd(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createOr(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createNot(Value* operand, const std::string& name = "");

    // ========================================================================
    // Memory Instructions
    // ========================================================================

    /**
     * Create stack allocation: %ptr = alloca type
     * LEARNING TIP: Returns a pointer to the allocated type
     */
    Value* createAlloca(std::shared_ptr<IRType> type, const std::string& name = "");

    /**
     * Create load: %value = load %ptr
     */
    Value* createLoad(Value* ptr, const std::string& name = "");

    /**
     * Create store: store %value, %ptr
     * LEARNING TIP: Store doesn't return a value (it's void)
     */
    void createStore(Value* value, Value* ptr);

    /**
     * Create heap allocation: %ptr = gcalloc type
     */
    Value* createGCAlloc(std::shared_ptr<IRType> type, const std::string& name = "");

    // ========================================================================
    // Array Instructions
    // ========================================================================

    Value* createArrayNew(std::shared_ptr<IRType> elementType, Value* size,
                         const std::string& name = "");
    Value* createArrayGet(Value* array, Value* index, const std::string& name = "");
    void createArraySet(Value* array, Value* index, Value* value);
    Value* createArrayLen(Value* array, const std::string& name = "");
    Value* createArraySlice(Value* array, Value* start, Value* end,
                           const std::string& name = "");

    // ========================================================================
    // Type Conversion Instructions
    // ========================================================================

    Value* createCast(Value* value, std::shared_ptr<IRType> destType,
                     const std::string& name = "");
    Value* createOptionWrap(Value* value, std::shared_ptr<IRType> optionType,
                           const std::string& name = "");
    Value* createOptionUnwrap(Value* option, const std::string& name = "");
    Value* createOptionCheck(Value* option, const std::string& name = "");

    // ========================================================================
    // Control Flow Instructions (Terminators)
    // ========================================================================

    /**
     * Create return instruction: ret value
     * LEARNING TIP: Validate that the return type matches the function's return type!
     */
    void createRet(Value* value = nullptr);

    /**
     * Create unconditional branch: br dest
     * LEARNING TIP: Must update CFG edges (predecessor/successor)!
     */
    void createBr(BasicBlock* dest);

    /**
     * Create conditional branch: br cond, trueBlock, falseBlock
     */
    void createCondBr(Value* condition, BasicBlock* trueBlock, BasicBlock* falseBlock);

    /**
     * Create switch: switch value, defaultDest, [case1: dest1, case2: dest2, ...]
     */
    Value* createSwitch(Value* value, BasicBlock* defaultDest,
                       const std::vector<SwitchInst::CaseEntry>& cases = {});

    // ========================================================================
    // Function Call Instructions
    // ========================================================================

    /**
     * Create direct function call: %result = call @function(arg1, arg2, ...)
     * @param callee The function to call
     * @param args The arguments
     * @param name Optional name for the result
     * @return The call instruction (or nullptr if return type is void)
     *
     * LEARNING TIP: Validate argument count and types match function signature!
     */
    Value* createCall(Function* callee, const std::vector<Value*>& args,
                     const std::string& name = "");

    /**
     * Create indirect function call: %result = call %func_ptr(arg1, arg2, ...)
     */
    Value* createCallIndirect(Value* callee, const std::vector<Value*>& args,
                             const std::string& name = "");

    // ========================================================================
    // SSA Phi Node
    // ========================================================================

    /**
     * Create phi node: %result = phi type [val1, block1], [val2, block2], ...
     * @param type The result type
     * @param incomingValues Incoming value/block pairs
     * @param name Optional name
     * @return The phi node
     *
     * LEARNING TIP: Phi nodes MUST be at the beginning of the block!
     * You can't insert them at the current insertion point if there are
     * already non-phi instructions.
     */
    Value* createPhi(std::shared_ptr<IRType> type,
                    const std::vector<PhiNode::IncomingValue>& incomingValues = {},
                    const std::string& name = "");

    // ========================================================================
    // High-Level Control Flow Helpers
    // ========================================================================

    /**
     * Create an if-then-else structure
     *
     * Example:
     *   auto [thenBlock, elseBlock, mergeBlock] = builder.createIfThenElse(condition);
     *
     *   builder.setInsertionPoint(thenBlock);
     *   // ... build then branch ...
     *
     *   builder.setInsertionPoint(elseBlock);
     *   // ... build else branch ...
     *
     *   builder.setInsertionPoint(mergeBlock);
     *   // ... continue after if-else ...
     *
     * @param condition The condition to test
     * @param thenName Name for then block
     * @param elseName Name for else block
     * @param mergeName Name for merge block
     * @return Tuple of (thenBlock, elseBlock, mergeBlock)
     *
     * LEARNING TIP: This creates the structure but doesn't create the branches
     * from then/else to merge - the caller must do that!
     */
    struct IfThenElseBlocks {
        BasicBlock* thenBlock;
        BasicBlock* elseBlock;
        BasicBlock* mergeBlock;
    };
    IfThenElseBlocks createIfThenElse(Value* condition,
                                      const std::string& thenName = "if.then",
                                      const std::string& elseName = "if.else",
                                      const std::string& mergeName = "if.merge");

    /**
     * Create a loop structure
     *
     * Example:
     *   auto [headerBlock, bodyBlock, exitBlock] = builder.createLoop();
     *
     *   builder.setInsertionPoint(headerBlock);
     *   Value* cond = ...;
     *   builder.createCondBr(cond, bodyBlock, exitBlock);
     *
     *   builder.setInsertionPoint(bodyBlock);
     *   // ... build loop body ...
     *   builder.createBr(headerBlock);  // Loop back
     *
     *   builder.setInsertionPoint(exitBlock);
     *   // ... continue after loop ...
     *
     * @return Tuple of (headerBlock, bodyBlock, exitBlock)
     */
    struct LoopBlocks {
        BasicBlock* headerBlock;
        BasicBlock* bodyBlock;
        BasicBlock* exitBlock;
    };
    LoopBlocks createLoop(const std::string& headerName = "loop.header",
                         const std::string& bodyName = "loop.body",
                         const std::string& exitName = "loop.exit");

    // ========================================================================
    // SSA Variable Management (High-Level)
    // ========================================================================

    /**
     * Create a named variable (using alloca for now)
     * Later, we'll implement mem2reg to convert to SSA
     *
     * @param name Variable name
     * @param type Variable type
     * @param initialValue Optional initial value (creates a store)
     * @return Pointer to the allocated variable
     *
     * LEARNING TIP: This is a convenience wrapper around createAlloca + createStore
     */
    Value* createVariable(const std::string& name, std::shared_ptr<IRType> type,
                         Value* initialValue = nullptr);

    /**
     * Read a variable (creates a load)
     * @param varPtr Pointer returned from createVariable
     * @param name Optional name for the loaded value
     */
    Value* readVariable(Value* varPtr, const std::string& name = "");

    /**
     * Write to a variable (creates a store)
     * @param varPtr Pointer returned from createVariable
     * @param value Value to write
     */
    void writeVariable(Value* varPtr, Value* value);

    // ========================================================================
    // Named Value Tracking (Symbol Table)
    // ========================================================================

    /**
     * Register a named value (e.g., function parameter, variable)
     * Allows lookup by name for convenient IR building
     */
    void setNamedValue(const std::string& name, Value* value);

    /**
     * Get a named value
     * @return The value, or nullptr if not found
     */
    Value* getNamedValue(const std::string& name) const;

    /**
     * Clear all named values (useful when entering new scope)
     */
    void clearNamedValues();

    /**
     * Remove a named value
     */
    void removeNamedValue(const std::string& name);

    // ========================================================================
    // Validation and Debugging
    // ========================================================================

    /**
     * Get the module this builder is building IR for
     */
    Module& getModule() { return module_; }
    const Module& getModule() const { return module_; }

    /**
     * Get the current function (parent of insertion block)
     */
    Function* getCurrentFunction() const;

    /**
     * Validate that builder state is correct
     * @param error Optional output for error message
     * @return true if valid
     */
    bool validate(std::string* error = nullptr) const;

private:
    // ========================================================================
    // Helper Methods
    // ========================================================================

    /**
     * Insert an instruction at the current insertion point
     * LEARNING TIP: This is the core method that all create* methods use!
     */
    void insert(Instruction* inst);

    /**
     * Validate binary operand types
     * LEARNING TIP: Both operands should have the same type
     */
    bool validateBinaryOp(Value* lhs, Value* rhs, const std::string& opName,
                         std::string* error = nullptr) const;

    /**
     * Validate pointer type for load/store
     */
    bool validatePointerOp(Value* ptr, const std::string& opName,
                          std::string* error = nullptr) const;

    // ========================================================================
    // State
    // ========================================================================

    Module& module_;                              // The module we're building IR in
    BasicBlock* insertionBlock_;                  // Current block to insert into
    Instruction* insertionPoint_;                 // Insert before this (nullptr = end)

    // Named value tracking (simple symbol table)
    std::unordered_map<std::string, Value*> namedValues_;
};

} // namespace volta::ir
