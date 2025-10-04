#pragma once

#include "Value.hpp"
#include <vector>
#include <memory>

namespace volta::ir {

// Forward declarations
class BasicBlock;
class Function;

/**
 * Instruction - Base class for all IR instructions
 *
 * Design Philosophy:
 * - RISC-style: Simple, orthogonal operations
 * - SSA Form: Each instruction produces a single value
 * - Type-aware: Leverage static types for optimization
 * - GC-aware: Track allocations for garbage collector
 * - Scientific computing optimized: First-class array/matrix operations
 *
 * Key Concepts:
 * - Operands: Values that this instruction uses (via Use objects)
 * - Parent Block: The basic block containing this instruction
 * - Terminators: Instructions that end a basic block (br, ret, etc.)
 */
class Instruction : public Value {
public:
    /**
     * Opcode - The operation type for this instruction
     *
     * Categories:
     * - Arithmetic: Add, Sub, Mul, Div, Rem, Neg, Pow
     * - Comparison: Eq, Ne, Lt, Le, Gt, Ge
     * - Logical: And, Or, Not
     * - Memory: Alloca, Load, Store, GCAlloc
     * - Array/Matrix: ArrayNew, ArrayGet, ArraySet, ArrayLen, ArraySlice, etc.
     * - Type Operations: Cast, OptionWrap, OptionUnwrap, OptionCheck
     * - Control Flow: Ret, Br, CondBr, Switch
     * - Function: Call, CallIndirect
     * - Aggregate: ExtractValue, InsertValue
     * - SSA: Phi
     */
    enum class Opcode {
        // Arithmetic operations (binary)
        Add,        // a + b (int or float)
        Sub,        // a - b
        Mul,        // a * b
        Div,        // a / b
        Rem,        // a % b (modulo)
        Pow,        // a ** b (power)

        // Unary arithmetic
        Neg,        // -a

        // Comparison operations (produce bool)
        Eq,         // a == b
        Ne,         // a != b
        Lt,         // a < b
        Le,         // a <= b
        Gt,         // a > b
        Ge,         // a >= b

        // Logical operations (bool)
        And,        // a and b
        Or,         // a or b
        Not,        // not a

        // Memory operations
        Alloca,     // Stack allocation
        Load,       // Load from memory
        Store,      // Store to memory
        GCAlloc,    // Heap allocation (GC-tracked)

        // Array operations
        ArrayNew,   // Create new array
        ArrayGet,   // Get element: arr[i]
        ArraySet,   // Set element: arr[i] = val
        ArrayLen,   // Get array length
        ArraySlice, // Slice array: arr[start:end]

        // Matrix operations
        MatrixNew,  // Create new matrix
        MatrixGet,  // Get element: mat[i, j]
        MatrixSet,  // Set element: mat[i, j] = val
        MatrixMul,  // Matrix multiplication
        MatrixTranspose, // Transpose

        // Broadcasting (for scientific computing)
        Broadcast,  // Broadcast scalar to array/matrix

        // Type operations
        Cast,       // Type conversion (int->float, etc.)
        OptionWrap, // Wrap value in Some(value)
        OptionUnwrap, // Extract value from Option (with check)
        OptionCheck,  // Check if Option is Some

        // Control flow (terminators - must end basic blocks)
        Ret,        // Return from function
        Br,         // Unconditional branch
        CondBr,     // Conditional branch
        Switch,     // Multi-way branch (for pattern matching)

        // Function calls
        Call,       // Direct function call
        CallIndirect, // Indirect call (first-class functions)

        // Aggregate operations (tuples, structs)
        ExtractValue, // Extract element from aggregate
        InsertValue,  // Insert element into aggregate

        // SSA operations
        Phi,        // SSA phi node (merge values from predecessors)
    };

    virtual ~Instruction();

    // Core properties
    Opcode getOpcode() const { return opcode_; }
    BasicBlock* getParent() const { return parent_; }
    void setParent(BasicBlock* parent) { parent_ = parent; }

    // Operand management (values this instruction uses)
    unsigned getNumOperands() const { return operands_.size(); }
    Value* getOperand(unsigned idx) const;
    void setOperand(unsigned idx, Value* value);
    const std::vector<Use>& getOperands() const { return operands_; }

    // Control flow queries
    bool isTerminator() const;
    bool isBinaryOp() const;
    bool isUnaryOp() const;
    bool isComparison() const;
    bool isArithmetic() const;
    bool isMemoryOp() const;
    bool isArrayOp() const;
    bool isMatrixOp() const;
    bool isCallInst() const;

    // Pretty printing
    std::string toString() const override;
    static const char* getOpcodeName(Opcode op);

    static bool classof(const Value* V) {
        return V->getKind() == ValueKind::Instruction;
    }

    // Allow Module to create instructions via arena allocation
    friend class Arena;

protected:
    Instruction(Opcode opcode,
                std::shared_ptr<IRType> type,
                const std::vector<Value*>& operands = {},
                const std::string& name = "");

    // Add an operand (creates a Use edge)
    void addOperand(Value* value);

    // Operand storage (accessible to subclasses for special cases like PhiNode)
    std::vector<Use> operands_;  // Owned Use objects

private:
    Opcode opcode_;
    BasicBlock* parent_;
};

// ============================================================================
// Binary Operations
// ============================================================================

/**
 * BinaryOperator - Base for binary arithmetic/logical operations
 * Examples: Add, Sub, Mul, Div, And, Or
 */
class BinaryOperator : public Instruction {
public:

    Value* getLHS() const { return getOperand(0); }
    Value* getRHS() const { return getOperand(1); }
    void setLHS(Value* lhs) { setOperand(0, lhs); }
    void setRHS(Value* rhs) { setOperand(1, rhs); }

    static bool classof(const Instruction* I);

    friend class Arena;

    friend class Arena;
private:
    BinaryOperator(Opcode op, Value* lhs, Value* rhs, const std::string& name);
};

// ============================================================================
// Unary Operations
// ============================================================================

/**
 * UnaryOperator - Unary operations (Neg, Not)
 */
class UnaryOperator : public Instruction {
public:

    Value* getOperand() const { return Instruction::getOperand(0); }
    void setOperand(Value* val) { Instruction::setOperand(0, val); }

    static bool classof(const Instruction* I);

    friend class Arena;

private:
    UnaryOperator(Opcode op, Value* operand, const std::string& name);
};

// ============================================================================
// Comparison Operations
// ============================================================================

/**
 * CmpInst - Comparison instruction (produces bool)
 */
class CmpInst : public Instruction {
public:

    Value* getLHS() const { return getOperand(0); }
    Value* getRHS() const { return getOperand(1); }

    static bool classof(const Instruction* I);

    friend class Arena;

private:
    CmpInst(Opcode op, Value* lhs, Value* rhs, const std::string& name);
};

// ============================================================================
// Memory Operations
// ============================================================================

/**
 * AllocaInst - Stack allocation
 * Allocates space on the stack for a value
 */
class AllocaInst : public Instruction {
public:

    std::shared_ptr<IRType> getAllocatedType() const {
        return getType()->asPointer()->pointeeType();
    }

    friend class Arena;
    static bool classof(const Instruction* I) {
        return I->getOpcode() == Opcode::Alloca;
    }

private:
    AllocaInst(std::shared_ptr<IRType> allocatedType, const std::string& name);
};

/**
 * LoadInst - Load from memory
 */
class LoadInst : public Instruction {
public:
    Value* getPointer() const { return getOperand(0); }

    friend class Arena;
    static bool classof(const Instruction* I) {
        return I->getOpcode() == Opcode::Load;
    }

private:
    LoadInst(Value* ptr, const std::string& name);
};

/**
 * StoreInst - Store to memory
 */
class StoreInst : public Instruction {
public:
    Value* getValue() const { return getOperand(0); }
    Value* getPointer() const { return getOperand(1); }

    friend class Arena;
    static bool classof(const Instruction* I) {
        return I->getOpcode() == Opcode::Store;
    }

private:
    StoreInst(Value* value, Value* ptr);
};

/**
 * GCAllocInst - Heap allocation with GC tracking
 */
class GCAllocInst : public Instruction {
public:

    std::shared_ptr<IRType> getAllocatedType() const {
        return getType()->asPointer()->pointeeType();
    }

    friend class Arena;
    static bool classof(const Instruction* I) {
        return I->getOpcode() == Opcode::GCAlloc;
    }

private:
    GCAllocInst(std::shared_ptr<IRType> allocatedType, const std::string& name);
};

// ============================================================================
// Array Operations
// ============================================================================

/**
 * ArrayNewInst - Create new array
 */
class ArrayNewInst : public Instruction {
public:

    std::shared_ptr<IRType> getElementType() const {
        return getType()->asPointer()->pointeeType();
    }
    Value* getSize() const { return getOperand(0); }

    friend class Arena;
    static bool classof(const Instruction* I) {
        return I->getOpcode() == Opcode::ArrayNew;
    }

private:
    ArrayNewInst(std::shared_ptr<IRType> elementType,
                 Value* size,
                 const std::string& name);
};

/**
 * ArrayGetInst - Get element from array: arr[index]
 */
class ArrayGetInst : public Instruction {
public:

    Value* getArray() const { return getOperand(0); }
    Value* getIndex() const { return getOperand(1); }

    friend class Arena;
    static bool classof(const Instruction* I) {
        return I->getOpcode() == Opcode::ArrayGet;
    }

private:
    ArrayGetInst(Value* array, Value* index, const std::string& name);
};

/**
 * ArraySetInst - Set element in array: arr[index] = value
 */
class ArraySetInst : public Instruction {
public:
    Value* getArray() const { return getOperand(0); }
    Value* getIndex() const { return getOperand(1); }
    Value* getValue() const { return getOperand(2); }

    friend class Arena;
    static bool classof(const Instruction* I) {
        return I->getOpcode() == Opcode::ArraySet;
    }

private:
    ArraySetInst(Value* array, Value* index, Value* value);
};

/**
 * ArrayLenInst - Get array length
 */
class ArrayLenInst : public Instruction {
public:
    Value* getArray() const { return getOperand(0); }

    friend class Arena;
    static bool classof(const Instruction* I) {
        return I->getOpcode() == Opcode::ArrayLen;
    }

private:
    ArrayLenInst(Value* array, const std::string& name);
};

/**
 * ArraySliceInst - Slice array: arr[start:end]
 */
class ArraySliceInst : public Instruction {
public:

    Value* getArray() const { return getOperand(0); }
    Value* getStart() const { return getOperand(1); }
    Value* getEnd() const { return getOperand(2); }

    friend class Arena;
    static bool classof(const Instruction* I) {
        return I->getOpcode() == Opcode::ArraySlice;
    }

private:
    ArraySliceInst(Value* array, Value* start, Value* end, const std::string& name);
};

// ============================================================================
// Type Operations
// ============================================================================

/**
 * CastInst - Type conversion (int->float, etc.)
 */
class CastInst : public Instruction {
public:

    Value* getValue() const { return getOperand(0); }
    std::shared_ptr<IRType> getDestType() const { return getType(); }
    std::shared_ptr<IRType> getSrcType() const { return getValue()->getType(); }

    friend class Arena;
    static bool classof(const Instruction* I) {
        return I->getOpcode() == Opcode::Cast;
    }

private:
    CastInst(Value* value, std::shared_ptr<IRType> destType, const std::string& name);
};

/**
 * OptionWrapInst - Wrap value in Some(value)
 */
class OptionWrapInst : public Instruction {
public:

    Value* getValue() const { return getOperand(0); }

    friend class Arena;
    static bool classof(const Instruction* I) {
        return I->getOpcode() == Opcode::OptionWrap;
    }

private:
    OptionWrapInst(Value* value, std::shared_ptr<IRType> optionType,
                   const std::string& name);
};

/**
 * OptionUnwrapInst - Extract value from Option (runtime check)
 */
class OptionUnwrapInst : public Instruction {
public:
    Value* getOption() const { return getOperand(0); }

    friend class Arena;
    static bool classof(const Instruction* I) {
        return I->getOpcode() == Opcode::OptionUnwrap;
    }

private:
    OptionUnwrapInst(Value* option, const std::string& name);
};

/**
 * OptionCheckInst - Check if Option is Some (returns bool)
 */
class OptionCheckInst : public Instruction {
public:
    Value* getOption() const { return getOperand(0); }

    friend class Arena;
    static bool classof(const Instruction* I) {
        return I->getOpcode() == Opcode::OptionCheck;
    }

private:
    OptionCheckInst(Value* option, const std::string& name);
};

// ============================================================================
// Control Flow (Terminators)
// ============================================================================

/**
 * ReturnInst - Return from function
 */
class ReturnInst : public Instruction {
public:
    bool hasReturnValue() const { return getNumOperands() > 0; }
    Value* getReturnValue() const {
        return hasReturnValue() ? getOperand(0) : nullptr;
    }

    friend class Arena;
    static bool classof(const Instruction* I) {
        return I->getOpcode() == Opcode::Ret;
    }

private:
    ReturnInst(Value* returnValue);
};

/**
 * BranchInst - Unconditional branch
 */
class BranchInst : public Instruction {
public:
    BasicBlock* getDestination() const { return destination_; }
    void setDestination(BasicBlock* dest) { destination_ = dest; }

    friend class Arena;
    static bool classof(const Instruction* I) {
        return I->getOpcode() == Opcode::Br;
    }

private:
    BranchInst(BasicBlock* dest);
    BasicBlock* destination_;
};

/**
 * CondBranchInst - Conditional branch (if-then-else)
 */
class CondBranchInst : public Instruction {
public:

    Value* getCondition() const { return getOperand(0); }
    BasicBlock* getTrueDest() const { return trueDest_; }
    BasicBlock* getFalseDest() const { return falseDest_; }

    void setTrueDest(BasicBlock* dest) { trueDest_ = dest; }
    void setFalseDest(BasicBlock* dest) { falseDest_ = dest; }

    friend class Arena;
    static bool classof(const Instruction* I) {
        return I->getOpcode() == Opcode::CondBr;
    }

private:
    CondBranchInst(Value* condition, BasicBlock* trueDest, BasicBlock* falseDest);
    BasicBlock* trueDest_;
    BasicBlock* falseDest_;
};

/**
 * SwitchInst - Multi-way branch (for pattern matching)
 */
class SwitchInst : public Instruction {
public:
    struct CaseEntry {
        Constant* value;
        BasicBlock* dest;
    };


    Value* getValue() const { return getOperand(0); }
    BasicBlock* getDefaultDest() const { return defaultDest_; }
    const std::vector<CaseEntry>& getCases() const { return cases_; }

    void addCase(Constant* value, BasicBlock* dest);

    friend class Arena;
    static bool classof(const Instruction* I) {
        return I->getOpcode() == Opcode::Switch;
    }

private:
    SwitchInst(Value* value, BasicBlock* defaultDest,
               const std::vector<CaseEntry>& cases);
    BasicBlock* defaultDest_;
    std::vector<CaseEntry> cases_;
};

// ============================================================================
// Function Calls
// ============================================================================

/**
 * CallInst - Direct function call
 */
class CallInst : public Instruction {
public:

    Function* getCallee() const { return callee_; }
    unsigned getNumArguments() const { return getNumOperands(); }
    Value* getArgument(unsigned idx) const { return getOperand(idx); }

    friend class Arena;
    static bool classof(const Instruction* I) {
        return I->getOpcode() == Opcode::Call;
    }

private:
    CallInst(Function* callee, const std::vector<Value*>& args, const std::string& name);
    Function* callee_;
};

/**
 * CallIndirectInst - Indirect function call (for first-class functions)
 */
class CallIndirectInst : public Instruction {
public:

    Value* getCallee() const { return getOperand(0); }
    unsigned getNumArguments() const { return getNumOperands() - 1; }
    Value* getArgument(unsigned idx) const { return getOperand(idx + 1); }

    friend class Arena;
    static bool classof(const Instruction* I) {
        return I->getOpcode() == Opcode::CallIndirect;
    }

private:
    CallIndirectInst(Value* callee, const std::vector<Value*>& args,
                     const std::string& name);
};

// ============================================================================
// SSA Operations
// ============================================================================

/**
 * PhiNode - SSA phi node for merging values from different predecessors
 *
 * Example:
 *   entry:
 *     br cond, then_block, else_block
 *   then_block:
 *     %x1 = ...
 *     br merge_block
 *   else_block:
 *     %x2 = ...
 *     br merge_block
 *   merge_block:
 *     %x = phi [%x1, then_block], [%x2, else_block]
 */
class PhiNode : public Instruction {
public:
    struct IncomingValue {
        Value* value;
        BasicBlock* block;
    };


    unsigned getNumIncomingValues() const { return incomingValues_.size(); }
    Value* getIncomingValue(unsigned idx) const;
    BasicBlock* getIncomingBlock(unsigned idx) const;

    void addIncoming(Value* value, BasicBlock* block);
    void removeIncomingValue(unsigned idx);

    const std::vector<IncomingValue>& getIncomingValues() const {
        return incomingValues_;
    }

    friend class Arena;
    static bool classof(const Instruction* I) {
        return I->getOpcode() == Opcode::Phi;
    }

private:
    PhiNode(std::shared_ptr<IRType> type,
            const std::vector<IncomingValue>& incomingValues,
            const std::string& name);
    std::vector<IncomingValue> incomingValues_;
};

} // namespace volta::ir
