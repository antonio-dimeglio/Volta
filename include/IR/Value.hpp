#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include "IR/IRType.hpp"

namespace volta::ir {

// Forward declarations
class Instruction;
class BasicBlock;
class Function;
class Use;
class Module;
class Arena;

/**
 * Value - The base class for all values in the IR
 *
 * Design Philosophy:
 * - SSA Form: Each Value represents a single definition
 * - Use-Def Chains: Track all uses of this value
 * - Type Safety: Every value has a type
 * - Naming: Values can have optional names for debugging
 *
 * Value Hierarchy:
 *   Value (abstract)
 *   ├── Constant (compile-time constants)
 *   │   ├── ConstantInt
 *   │   ├── ConstantFloat
 *   │   ├── ConstantBool
 *   │   ├── ConstantString
 *   │   ├── ConstantNone
 *   │   └── UndefValue (undefined/uninitialized)
 *   ├── Instruction (runtime computed values)
 *   ├── Argument (function parameters)
 *   └── GlobalValue (global variables, functions)
 */
class Value {
public:
    enum class ValueKind {
        // Constants
        ConstantInt,
        ConstantFloat,
        ConstantBool,
        ConstantString,
        UndefValue,

        // Runtime values
        Argument,
        Instruction,
        BasicBlock,

        // Global entities
        GlobalVariable,
        Function
    };

    // Core properties
    ValueKind getKind() const { return kind_; }
    std::shared_ptr<IRType> getType() const { return type_; }

    // Naming (for debugging and readability)
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }
    bool hasName() const { return !name_.empty(); }

    // Use-def chain management
    const std::vector<Use*>& getUses() const { return uses_; }
    bool hasUses() const { return !uses_.empty(); }
    size_t getNumUses() const { return uses_.size(); }

    // Replace all uses of this value with another value
    void replaceAllUsesWith(Value* newValue);

    // Add/remove a use
    void addUse(Use* use);
    void removeUse(Use* use);

    // Type checking helpers
    bool isConstant() const;
    bool isInstruction() const;
    bool isArgument() const;
    bool isGlobalValue() const;

    // Pretty printing for debugging
    virtual std::string toString() const = 0;

    // Get a unique identifier for this value (for SSA naming)
    uint64_t getID() const { return id_; }

protected:
    Value(ValueKind kind, std::shared_ptr<IRType> type, const std::string& name = "");

private:
    ValueKind kind_;
    std::shared_ptr<IRType> type_;
    std::string name_;
    std::vector<Use*> uses_;  // All uses of this value
    uint64_t id_;             // Unique ID for SSA naming

    static uint64_t nextID_;
};

/**
 * Use - Represents a use of a Value
 *
 * Design: Each use tracks both the value being used and the user (instruction)
 * This enables efficient def-use and use-def chain traversal
 */
class Use {
public:
    Use(Value* value, Instruction* user);

    // Destructor - clean up use-def chain
    ~Use();

    // Disable copy to prevent use-def chain corruption
    Use(const Use&) = delete;
    Use& operator=(const Use&) = delete;

    // Allow move for vector reallocation (but implement carefully)
    Use(Use&& other) noexcept;
    Use& operator=(Use&& other) noexcept;

    Value* getValue() const { return value_; }
    void setValue(Value* newValue);

    Instruction* getUser() const { return user_; }

private:
    Value* value_;           // The value being used
    Instruction* user_;      // The instruction using this value
};

// ============================================================================
// Constants
// ============================================================================

/**
 * Constant - Base class for compile-time constant values
 * Constants are immutable and unique (interned)
 */
class Constant : public Value {
public:

    static bool classof(const Value* V) {
        return V->getKind() >= ValueKind::ConstantInt &&
               V->getKind() <= ValueKind::UndefValue;
    }

protected:
    Constant(ValueKind kind, std::shared_ptr<IRType> type, const std::string& name = "");
};

/**
 * ConstantInt - Integer constant (64-bit signed)
 */
class ConstantInt : public Constant {
public:
    // REMOVED: get() - Use Module::getConstantInt() instead

    int64_t getValue() const { return value_; }

    std::string toString() const override;

    static bool classof(const Value* V) {
        return V->getKind() == ValueKind::ConstantInt;
    }

    friend class Module;
    friend class Arena;

private:
    ConstantInt(int64_t value, std::shared_ptr<IRType> type);
    int64_t value_;
};

/**
 * ConstantFloat - Floating-point constant (64-bit double)
 */
class ConstantFloat : public Constant {
public:
    // REMOVED: get() - Use Module::getConstantFloat() instead

    double getValue() const { return value_; }

    std::string toString() const override;

    static bool classof(const Value* V) {
        return V->getKind() == ValueKind::ConstantFloat;
    }

    friend class Module;
    friend class Arena;

private:
    ConstantFloat(double value, std::shared_ptr<IRType> type);
    double value_;
};

/**
 * ConstantBool - Boolean constant
 */
class ConstantBool : public Constant {
public:
    // REMOVED: get(), getTrue(), getFalse() - Use Module::getConstantBool() instead

    bool getValue() const { return value_; }

    std::string toString() const override;

    static bool classof(const Value* V) {
        return V->getKind() == ValueKind::ConstantBool;
    }

    friend class Module;
    friend class Arena;

private:
    ConstantBool(bool value, std::shared_ptr<IRType> type);
    bool value_;
};

/**
 * ConstantString - String constant
 * Note: String constants are interned for memory efficiency
 */
class ConstantString : public Constant {
public:
    // REMOVED: get() - Use Module::getConstantString() instead

    const std::string& getValue() const { return value_; }

    std::string toString() const override;

    static bool classof(const Value* V) {
        return V->getKind() == ValueKind::ConstantString;
    }

    friend class Module;
    friend class Arena;

private:
    ConstantString(const std::string& value, std::shared_ptr<IRType> type);
    std::string value_;
};


/**
 * UndefValue - Represents an undefined/uninitialized value
 * Used for uninitialized variables before first assignment
 */
class UndefValue : public Constant {
public:
    // REMOVED: get() - Allocate through Module/Arena instead

    std::string toString() const override;

    static bool classof(const Value* V) {
        return V->getKind() == ValueKind::UndefValue;
    }

    friend class Arena;

private:
    UndefValue(std::shared_ptr<IRType> type);
};

// ============================================================================
// Argument
// ============================================================================

/**
 * Argument - Represents a function parameter
 * Arguments are special values that represent incoming function parameters
 */
class Argument : public Value {
public:
    Argument(std::shared_ptr<IRType> type,
             unsigned argNo,
             const std::string& name = "");

    unsigned getArgNo() const { return argNo_; }
    Function* getParent() const { return parent_; }
    void setParent(Function* parent) { parent_ = parent; }

    std::string toString() const override;

    static bool classof(const Value* V) {
        return V->getKind() == ValueKind::Argument;
    }

private:
    unsigned argNo_;      // Argument index (0-based)
    Function* parent_;    // Parent function
};

// ============================================================================
// Global Value
// ============================================================================

/**
 * GlobalVariable - Represents a global variable
 * Global variables live for the entire program execution
 */
class GlobalVariable : public Value {
public:
    GlobalVariable(std::shared_ptr<IRType> type,
                   const std::string& name,
                   Constant* initializer = nullptr,
                   bool isConstant = false);

    bool isConstant() const { return isConstant_; }

    Constant* getInitializer() const { return initializer_; }
    void setInitializer(Constant* init) { initializer_ = init; }

    std::string toString() const override;

    static bool classof(const Value* V) {
        return V->getKind() == ValueKind::GlobalVariable;
    }

private:
    bool isConstant_;
    Constant* initializer_;
};

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Typed value casting with LLVM-style RTTI
 * Usage: if (auto* ci = dyn_cast<ConstantInt>(value)) { ... }
 */
template<typename To, typename From>
inline To* dyn_cast(From* value) {
    if (value && To::classof(value)) {
        return static_cast<To*>(value);
    }
    return nullptr;
}

template<typename To, typename From>
inline const To* dyn_cast(const From* value) {
    if (value && To::classof(value)) {
        return static_cast<const To*>(value);
    }
    return nullptr;
}

template<typename To, typename From>
inline bool isa(const From* value) {
    return value && To::classof(value);
}

} // namespace volta::ir
