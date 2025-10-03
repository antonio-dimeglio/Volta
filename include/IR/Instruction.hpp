#pragma once

#include "Value.hpp"
#include <memory>
#include <vector>
#include <string>

namespace volta::ir {

// Forward declarations
class BasicBlock;

// ============================================================================
// Instruction - Base class for all IR instructions
// ============================================================================

/**
 * Represents a single IR instruction in SSA form.
 *
 * Instructions:
 * - Produce a value (inherit from Value)
 * - Consume operands (other Values)
 * - Belong to a BasicBlock
 * - Have an opcode (Add, Mul, Load, Store, etc.)
 *
 * Example:
 *   %2 = add i64 %0, %1
 *        ^       ^   ^
 *        |       operands
 *        result (this Instruction is also a Value)
 */
class Instruction : public Value {
public:
    enum class Opcode {
        // Arithmetic
        Add, Sub, Mul, Div, Mod,
        FAdd, FSub, FMul, FDiv,

        // Comparison
        ICmpEQ, ICmpNE, ICmpLT, ICmpLE, ICmpGT, ICmpGE,  // Integer compare
        FCmpEQ, FCmpNE, FCmpLT, FCmpLE, FCmpGT, FCmpGE,  // Float compare

        // Logical
        And, Or, Not,

        // Memory (LLVM style)
        Alloca,    // Allocate stack space: %ptr = alloca i64
        Load,      // Load from memory: %val = load i64, ptr %ptr
        Store,     // Store to memory: store i64 %val, ptr %ptr

        // Control flow
        Br,        // Unconditional branch: br label %block
        CondBr,    // Conditional branch: br i1 %cond, label %then, label %else
        Ret,       // Return: ret i64 %val

        // Function calls
        Call,      // Call function: %ret = call @func(i64 %arg1, i64 %arg2)

        // PHI nodes (SSA merge)
        Phi,       // PHI: %val = phi i64 [%val1, %block1], [%val2, %block2]

        // Type conversions
        Trunc, ZExt, SExt,      // Integer size changes
        FPToSI, FPToUI,         // Float to int
        SIToFP, UIToFP,         // Int to float
        BitCast,                // Type reinterpretation

        // Aggregate operations (for arrays/structs)
        GetElementPtr,  // GEP: compute pointer offset
        ExtractValue,   // Extract element from aggregate
        InsertValue,    // Insert element into aggregate
    };

    Instruction(
        Opcode opcode,
        std::shared_ptr<semantic::Type> type,
        const std::string& name,
        BasicBlock* parent = nullptr
    );

    virtual ~Instruction() = default;

    // Getters
    Opcode getOpcode() const { return opcode_; }
    BasicBlock* getParent() const { return parent_; }
    void setParent(BasicBlock* parent) { parent_ = parent; }

    // Operand management
    virtual size_t getNumOperands() const = 0;
    virtual Value* getOperand(size_t index) const = 0;
    virtual void setOperand(size_t index, Value* value) = 0;

    // Type queries
    bool isTerminator() const;  // Is this a Br/CondBr/Ret?
    bool isBinaryOp() const;    // Is this Add/Sub/Mul/etc?
    bool isCompare() const;     // Is this ICmp/FCmp?
    bool isMemoryOp() const;    // Is this Load/Store/Alloca?

    // For printing
    virtual std::string toString() const = 0;

    static const char* getOpcodeName(Opcode opcode);

protected:
    Opcode opcode_;
    BasicBlock* parent_;
};

// ============================================================================
// Binary Instructions - Operations with 2 operands
// ============================================================================

/**
 * Binary operations: %result = op %lhs, %rhs
 *
 * Examples:
 *   %2 = add i64 %0, %1
 *   %3 = fmul float %x, 2.0
 */
class BinaryInstruction : public Instruction {
public:
    BinaryInstruction(
        Opcode opcode,
        Value* lhs,
        Value* rhs,
        const std::string& name = ""
    );

    // Operand access
    size_t getNumOperands() const override { return 2; }
    Value* getOperand(size_t index) const override;
    void setOperand(size_t index, Value* value) override;

    Value* getLHS() const { return lhs_; }
    Value* getRHS() const { return rhs_; }

    std::string toString() const override;

private:
    Value* lhs_;
    Value* rhs_;
};

// ============================================================================
// Comparison Instructions
// ============================================================================

/**
 * Comparison: %result = icmp pred %lhs, %rhs
 *
 * Examples:
 *   %0 = icmp eq i64 %x, 42
 *   %1 = fcmp lt float %y, 3.14
 */
class CompareInstruction : public Instruction {
public:
    CompareInstruction(
        Opcode opcode,
        Value* lhs,
        Value* rhs,
        const std::string& name = ""
    );

    size_t getNumOperands() const override { return 2; }
    Value* getOperand(size_t index) const override;
    void setOperand(size_t index, Value* value) override;

    Value* getLHS() const { return lhs_; }
    Value* getRHS() const { return rhs_; }

    std::string toString() const override;

private:
    Value* lhs_;
    Value* rhs_;
};

// ============================================================================
// Memory Instructions (LLVM style)
// ============================================================================

/**
 * Alloca: Allocate stack memory
 *
 * Example:
 *   %ptr = alloca i64
 *
 * Used for mutable variables in LLVM style.
 */
class AllocaInstruction : public Instruction {
public:
    explicit AllocaInstruction(
        std::shared_ptr<semantic::Type> allocatedType,
        const std::string& name = ""
    );

    size_t getNumOperands() const override { return 0; }
    Value* getOperand(size_t index) const override { return nullptr; }
    void setOperand(size_t index, Value* value) override {}

    std::shared_ptr<semantic::Type> getAllocatedType() const { return allocatedType_; }

    std::string toString() const override;

private:
    std::shared_ptr<semantic::Type> allocatedType_;
};

/**
 * Load: Read value from memory
 *
 * Example:
 *   %val = load i64, ptr %ptr
 */
class LoadInstruction : public Instruction {
public:
    LoadInstruction(
        Value* pointer,
        const std::string& name = ""
    );

    size_t getNumOperands() const override { return 1; }
    Value* getOperand(size_t index) const override;
    void setOperand(size_t index, Value* value) override;

    Value* getPointer() const { return pointer_; }

    std::string toString() const override;

private:
    Value* pointer_;
};

/**
 * Store: Write value to memory
 *
 * Example:
 *   store i64 %val, ptr %ptr
 *
 * Note: Store doesn't produce a value (returns void).
 */
class StoreInstruction : public Instruction {
public:
    StoreInstruction(Value* value, Value* pointer);

    size_t getNumOperands() const override { return 2; }
    Value* getOperand(size_t index) const override;
    void setOperand(size_t index, Value* value) override;

    Value* getValue() const { return value_; }
    Value* getPointer() const { return pointer_; }

    std::string toString() const override;

private:
    Value* value_;
    Value* pointer_;
};

// ============================================================================
// Control Flow Instructions
// ============================================================================

/**
 * Branch: Unconditional jump
 *
 * Example:
 *   br label %target
 */
class BranchInstruction : public Instruction {
public:
    explicit BranchInstruction(BasicBlock* target);

    size_t getNumOperands() const override { return 0; }
    Value* getOperand(size_t index) const override { return nullptr; }
    void setOperand(size_t index, Value* value) override {}

    BasicBlock* getTarget() const { return target_; }
    void setTarget(BasicBlock* target) { target_ = target; }

    std::string toString() const override;

private:
    BasicBlock* target_;
};

/**
 * Conditional Branch: Jump based on condition
 *
 * Example:
 *   br i1 %cond, label %then, label %else
 */
class CondBranchInstruction : public Instruction {
public:
    CondBranchInstruction(
        Value* condition,
        BasicBlock* thenBlock,
        BasicBlock* elseBlock
    );

    size_t getNumOperands() const override { return 1; }
    Value* getOperand(size_t index) const override;
    void setOperand(size_t index, Value* value) override;

    Value* getCondition() const { return condition_; }
    BasicBlock* getThenBlock() const { return thenBlock_; }
    BasicBlock* getElseBlock() const { return elseBlock_; }

    void setThenBlock(BasicBlock* block) { thenBlock_ = block; }
    void setElseBlock(BasicBlock* block) { elseBlock_ = block; }

    std::string toString() const override;

private:
    Value* condition_;
    BasicBlock* thenBlock_;
    BasicBlock* elseBlock_;
};

/**
 * Return: Return from function
 *
 * Examples:
 *   ret i64 %val
 *   ret void
 */
class ReturnInstruction : public Instruction {
public:
    explicit ReturnInstruction(Value* returnValue = nullptr);

    size_t getNumOperands() const override { return returnValue_ ? 1 : 0; }
    Value* getOperand(size_t index) const override;
    void setOperand(size_t index, Value* value) override;

    Value* getReturnValue() const { return returnValue_; }

    std::string toString() const override;

private:
    Value* returnValue_;
};

// ============================================================================
// Call Instruction
// ============================================================================

/**
 * Call: Invoke a function
 *
 * Example:
 *   %ret = call i64 @add(i64 %a, i64 %b)
 */
class CallInstruction : public Instruction {
public:
    CallInstruction(
        Value* callee,
        std::vector<Value*> arguments,
        const std::string& name = ""
    );

    size_t getNumOperands() const override;
    Value* getOperand(size_t index) const override;
    void setOperand(size_t index, Value* value) override;

    Value* getCallee() const { return callee_; }
    const std::vector<Value*>& getArguments() const { return arguments_; }

    std::string toString() const override;

private:
    Value* callee_;
    std::vector<Value*> arguments_;
};

// ============================================================================
// PHI Instruction - SSA merge points
// ============================================================================

/**
 * PHI: Merge values from different control flow paths
 *
 * Example:
 *   %result = phi i64 [ %val1, %block1 ], [ %val2, %block2 ]
 *
 * This is the HEART of SSA form! When control flow merges (like after an if-else),
 * we need a phi node to represent "which value does this variable have?"
 *
 * In Volta:
 *   x: mut int = 10
 *   if condition {
 *       x = 20
 *   } else {
 *       x = 30
 *   }
 *   print(x)  <- What is x here? PHI decides!
 *
 * In IR:
 *   %x.0 = alloca i64
 *   store i64 10, ptr %x.0
 *   br i1 %cond, label %then, label %else
 * then:
 *   store i64 20, ptr %x.0
 *   br label %merge
 * else:
 *   store i64 30, ptr %x.0
 *   br label %merge
 * merge:
 *   %x.merged = phi i64 [ 20, %then ], [ 30, %else ]
 *   call @print(%x.merged)
 */
class PhiInstruction : public Instruction {
public:
    struct IncomingValue {
        Value* value;
        BasicBlock* block;
    };

    explicit PhiInstruction(
        std::shared_ptr<semantic::Type> type,
        const std::string& name = ""
    );

    size_t getNumOperands() const override { return incomingValues_.size(); }
    Value* getOperand(size_t index) const override;
    void setOperand(size_t index, Value* value) override;

    // PHI-specific operations
    void addIncoming(Value* value, BasicBlock* block);
    void removeIncoming(BasicBlock* block);
    const std::vector<IncomingValue>& getIncomingValues() const { return incomingValues_; }

    Value* getIncomingValueForBlock(BasicBlock* block) const;

    std::string toString() const override;

private:
    std::vector<IncomingValue> incomingValues_;
};

} // namespace volta::ir
