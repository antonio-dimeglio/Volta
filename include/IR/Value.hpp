#pragma once

#include <memory>
#include <string>
#include <vector>
#include <variant>
#include "../semantic/Type.hpp"

namespace volta::ir {

// Forward declarations
class Instruction;
class BasicBlock;
class Function;

// ============================================================================
// IR Value System - SSA Values
// ============================================================================

/**
 * Base class for all values in IR (SSA form).
 *
 * In SSA, every "value" is:
 * - Produced by exactly one instruction (or is a parameter/constant)
 * - Can be used by multiple instructions
 *
 * Examples:
 *   %0 = add i64 %a, %b    <- %0 is a Value (result of add)
 *   %1 = mul i64 %0, 2     <- %0 is used here
 *
 * Values have:
 * - A unique name (for SSA: %0, %1, %result, etc.)
 * - A type (int, float, bool, etc.)
 * - A kind (what produced this value)
 */
class Value {
public:
    enum class Kind {
        Instruction,    // Result of an instruction (%0, %1, etc.)
        Parameter,      // Function parameter (%arg0, %arg1, etc.)
        Constant,       // Literal constant (42, 3.14, true, etc.)
        GlobalVariable, // Global variable (@global_name)
        Argument,       // Actual argument value passed to instruction
    };

    Value(Kind kind, std::shared_ptr<semantic::Type> type, const std::string& name);
    virtual ~Value() = default;

    // Getters
    Kind getKind() const { return kind_; }
    const std::shared_ptr<semantic::Type>& getType() const { return type_; }
    const std::string& getName() const { return name_; }

    // Use tracking (for SSA def-use chains)
    void addUse(Instruction* user);
    void removeUse(Instruction* user);
    const std::vector<Instruction*>& getUses() const { return uses_; }
    bool hasUses() const { return !uses_.empty(); }

    // For debugging/printing
    virtual std::string toString() const;

protected:
    Kind kind_;
    std::shared_ptr<semantic::Type> type_;
    std::string name_;

    // Def-use chain: all instructions that use this value
    std::vector<Instruction*> uses_;
};

// ============================================================================
// Constants - Compile-time known values
// ============================================================================

/**
 * Represents a compile-time constant value.
 *
 * Examples in IR:
 *   %0 = add i64 42, %x        <- 42 is a Constant
 *   %1 = fadd float 3.14, %y   <- 3.14 is a Constant
 *   %2 = and bool true, %z     <- true is a Constant
 */
class Constant : public Value {
public:
    // Variant holds the actual constant value
    using ConstantValue = std::variant<
        int64_t,           // Integer constants
        double,            // Float constants
        bool,              // Boolean constants
        std::string,       // String constants
        std::monostate     // Null/None
    >;

    Constant(std::shared_ptr<semantic::Type> type, ConstantValue value, const std::string& name = "");

    const ConstantValue& getValue() const { return value_; }

    // Factory methods - you'll implement these to create constants
    static std::shared_ptr<Constant> getInt(int64_t value);
    static std::shared_ptr<Constant> getFloat(double value);
    static std::shared_ptr<Constant> getBool(bool value);
    static std::shared_ptr<Constant> getString(const std::string& value);
    static std::shared_ptr<Constant> getNone();

    std::string toString() const override;

private:
    ConstantValue value_;
};

// ============================================================================
// Parameters - Function parameters in SSA form
// ============================================================================

/**
 * Represents a function parameter.
 *
 * Example:
 *   fn add(a: int, b: int) -> int {
 *       return a + b
 *   }
 *
 * In IR:
 *   define i64 @add(i64 %a, i64 %b) {
 *     entry:
 *       %0 = add i64 %a, %b    <- %a and %b are Parameters
 *       ret i64 %0
 *   }
 */
class Parameter : public Value {
public:
    Parameter(std::shared_ptr<semantic::Type> type, const std::string& name, size_t index);

    size_t getIndex() const { return index_; }

    std::string toString() const override;

private:
    size_t index_;  // Position in function signature (0, 1, 2, ...)
};

// ============================================================================
// Global Variables - Module-level storage
// ============================================================================

/**
 * Represents a global variable (mutable or immutable).
 *
 * Example:
 *   counter: mut int = 0
 *
 * In IR:
 *   @counter = global i64 0
 *
 * Used with alloca/load/store in LLVM style.
 */
class GlobalVariable : public Value {
public:
    GlobalVariable(
        std::shared_ptr<semantic::Type> type,
        const std::string& name,
        std::shared_ptr<Constant> initializer = nullptr,
        bool isMutable = false
    );

    const std::shared_ptr<Constant>& getInitializer() const { return initializer_; }
    bool isMutable() const { return isMutable_; }

    std::string toString() const override;

private:
    std::shared_ptr<Constant> initializer_;
    bool isMutable_;
};

} // namespace volta::ir
