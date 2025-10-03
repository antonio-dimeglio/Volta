#pragma once

#include <memory>
#include <string>
#include <vector>
#include <variant>
#include "../semantic/Type.hpp"
#include "../error/ErrorTypes.hpp"

namespace volta::ir {

// Forward declarations
class Value;
class Instruction;
class BasicBlock;
class Function;

// ============================================================================
// IR Value System
// ============================================================================

/**
 * Represents a value in IR - can be an instruction result, parameter, constant, etc.
 * Every value has a type and a name (for SSA form: %0, %1, etc.)
 */
class Value {
public:
    enum class Kind {
        Instruction,    // Result of an instruction
        Parameter,      // Function parameter
        Constant,       // Literal constant
        GlobalVariable, // Global variable
        BasicBlock      // Basic block (used as branch target)
    };

    Value(Kind kind, std::shared_ptr<semantic::Type> type, const std::string& name)
        : kind_(kind), type_(std::move(type)), name_(name) {}

    virtual ~Value() = default;

    Kind kind() const { return kind_; }
    const std::shared_ptr<semantic::Type>& type() const { return type_; }
    const std::string& name() const { return name_; }

    // For printing: %0, %foo, @global, etc.
    virtual std::string toString() const { return name_; }

private:
    Kind kind_;
    std::shared_ptr<semantic::Type> type_;
    std::string name_;
};

/**
 * Constant values (literals)
 */
class Constant : public Value {
public:
    using ConstantValue = std::variant<int64_t, double, bool, std::string, std::monostate>;

    Constant(std::shared_ptr<semantic::Type> type, ConstantValue value, const std::string& name = "")
        : Value(Kind::Constant, std::move(type), name), value_(std::move(value)) {}

    const ConstantValue& value() const { return value_; }

    std::string toString() const override;

private:
    ConstantValue value_;
};

/**
 * Function parameter
 */
class Parameter : public Value {
public:
    Parameter(std::shared_ptr<semantic::Type> type, const std::string& name, size_t index)
        : Value(Kind::Parameter, std::move(type), name), index_(index) {}

    size_t index() const { return index_; }

private:
    size_t index_;
};

/**
 * Global variable
 */
class GlobalVariable : public Value {
public:
    GlobalVariable(std::shared_ptr<semantic::Type> type, const std::string& name,
                   std::shared_ptr<Constant> initializer = nullptr)
        : Value(Kind::GlobalVariable, std::move(type), "@" + name),
          initializer_(std::move(initializer)) {}

    const std::shared_ptr<Constant>& initializer() const { return initializer_; }

private:
    std::shared_ptr<Constant> initializer_;
};

// ============================================================================
// IR Instructions
// ============================================================================

/**
 * Base class for all IR instructions
 * Instructions are the only values that can be in basic blocks
 */
class Instruction : public Value {
public:
    enum class Opcode {
        // Arithmetic
        Add, Sub, Mul, Div, Mod, Neg,

        // Comparison
        Eq, Ne, Lt, Le, Gt, Ge,

        // Logical
        And, Or, Not,

        // Memory
        Alloc, Load, Store, GetField, SetField, GetElement, SetElement,

        // Array operations
        NewArray,

        // Control flow
        Br, BrIf, Ret, Call, CallForeign,

        // Type operations
        Cast,

        // Special
        Phi  // For future SSA with proper phi nodes
    };

    Instruction(Opcode opcode, std::shared_ptr<semantic::Type> type,
                const std::string& name, BasicBlock* parent = nullptr)
        : Value(Kind::Instruction, std::move(type), name),
          opcode_(opcode), parent_(parent) {}

    Opcode opcode() const { return opcode_; }
    BasicBlock* parent() const { return parent_; }
    void setParent(BasicBlock* parent) { parent_ = parent; }

    // Get operands (values this instruction uses)
    virtual std::vector<Value*> operands() const = 0;

    // Get name of opcode for printing
    static std::string opcodeName(Opcode op);

private:
    Opcode opcode_;
    BasicBlock* parent_;
};

/**
 * Binary operation: %result = op %left, %right
 */
class BinaryInst : public Instruction {
public:
    BinaryInst(Opcode opcode, std::shared_ptr<semantic::Type> type,
               const std::string& name, Value* left, Value* right)
        : Instruction(opcode, std::move(type), name), left_(left), right_(right) {}

    Value* left() const { return left_; }
    Value* right() const { return right_; }

    std::vector<Value*> operands() const override { return {left_, right_}; }

private:
    Value* left_;
    Value* right_;
};

/**
 * Unary operation: %result = op %operand
 */
class UnaryInst : public Instruction {
public:
    UnaryInst(Opcode opcode, std::shared_ptr<semantic::Type> type,
              const std::string& name, Value* operand)
        : Instruction(opcode, std::move(type), name), operand_(operand) {}

    Value* operand() const { return operand_; }

    std::vector<Value*> operands() const override { return {operand_}; }

private:
    Value* operand_;
};

/**
 * Memory allocation: %result = alloc Type
 */
class AllocInst : public Instruction {
public:
    AllocInst(std::shared_ptr<semantic::Type> allocType, const std::string& name)
        : Instruction(Opcode::Alloc, allocType, name), allocatedType_(allocType) {}

    const std::shared_ptr<semantic::Type>& allocatedType() const { return allocatedType_; }

    std::vector<Value*> operands() const override { return {}; }

private:
    std::shared_ptr<semantic::Type> allocatedType_;
};

/**
 * Memory load: %result = load [%address]
 */
class LoadInst : public Instruction {
public:
    LoadInst(std::shared_ptr<semantic::Type> type, const std::string& name, Value* address)
        : Instruction(Opcode::Load, std::move(type), name), address_(address) {}

    Value* address() const { return address_; }

    std::vector<Value*> operands() const override { return {address_}; }

private:
    Value* address_;
};

/**
 * Memory store: store %value, [%address]
 */
class StoreInst : public Instruction {
public:
    StoreInst(Value* value, Value* address)
        : Instruction(Opcode::Store,
                     std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::Void),
                     ""),
          value_(value), address_(address) {}

    Value* value() const { return value_; }
    Value* address() const { return address_; }

    std::vector<Value*> operands() const override { return {value_, address_}; }

private:
    Value* value_;
    Value* address_;
};

/**
 * Get field: %result = get_field %object, field_index
 */
class GetFieldInst : public Instruction {
public:
    GetFieldInst(std::shared_ptr<semantic::Type> type, const std::string& name,
                 Value* object, size_t fieldIndex)
        : Instruction(Opcode::GetField, std::move(type), name),
          object_(object), fieldIndex_(fieldIndex) {}

    Value* object() const { return object_; }
    size_t fieldIndex() const { return fieldIndex_; }

    std::vector<Value*> operands() const override { return {object_}; }

private:
    Value* object_;
    size_t fieldIndex_;
};

/**
 * Set field: set_field %object, field_index, %value
 */
class SetFieldInst : public Instruction {
public:
    SetFieldInst(Value* object, size_t fieldIndex, Value* value)
        : Instruction(Opcode::SetField,
                     std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::Void),
                     ""),
          object_(object), fieldIndex_(fieldIndex), value_(value) {}

    Value* object() const { return object_; }
    size_t fieldIndex() const { return fieldIndex_; }
    Value* value() const { return value_; }

    std::vector<Value*> operands() const override { return {object_, value_}; }

private:
    Value* object_;
    size_t fieldIndex_;
    Value* value_;
};

/**
 * Get element: %result = get_element %array, %index
 */
class GetElementInst : public Instruction {
public:
    GetElementInst(std::shared_ptr<semantic::Type> type, const std::string& name,
                   Value* array, Value* index)
        : Instruction(Opcode::GetElement, std::move(type), name),
          array_(array), index_(index) {}

    Value* array() const { return array_; }
    Value* index() const { return index_; }

    std::vector<Value*> operands() const override { return {array_, index_}; }

private:
    Value* array_;
    Value* index_;
};

/**
 * Set element: set_element %array, %index, %value
 */
class SetElementInst : public Instruction {
public:
    SetElementInst(Value* array, Value* index, Value* value)
        : Instruction(Opcode::SetElement,
                     std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::Void),
                     ""),
          array_(array), index_(index), value_(value) {}

    Value* array() const { return array_; }
    Value* index() const { return index_; }
    Value* value() const { return value_; }

    std::vector<Value*> operands() const override { return {array_, index_, value_}; }

private:
    Value* array_;
    Value* index_;
    Value* value_;
};

/**
 * Create new array: %result = new_array %length, [%elem0, %elem1, ...]
 * Creates a new array with the specified elements
 */
class NewArrayInst : public Instruction {
public:
    NewArrayInst(std::shared_ptr<semantic::Type> arrayType, const std::string& name,
                 std::vector<Value*> elements)
        : Instruction(Opcode::NewArray, std::move(arrayType), name),
          elements_(std::move(elements)) {}

    const std::vector<Value*>& elements() const { return elements_; }
    size_t elementCount() const { return elements_.size(); }

    std::vector<Value*> operands() const override { return elements_; }

private:
    std::vector<Value*> elements_;
};

/**
 * Unconditional branch: br label
 */
class BranchInst : public Instruction {
public:
    BranchInst(BasicBlock* target)
        : Instruction(Opcode::Br,
                     std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::Void),
                     ""),
          target_(target) {}

    BasicBlock* target() const { return target_; }

    std::vector<Value*> operands() const override { return {}; }

private:
    BasicBlock* target_;
};

/**
 * Conditional branch: br_if %condition, then_label, else_label
 */
class BranchIfInst : public Instruction {
public:
    BranchIfInst(Value* condition, BasicBlock* thenBlock, BasicBlock* elseBlock)
        : Instruction(Opcode::BrIf,
                     std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::Void),
                     ""),
          condition_(condition), thenBlock_(thenBlock), elseBlock_(elseBlock) {}

    Value* condition() const { return condition_; }
    BasicBlock* thenBlock() const { return thenBlock_; }
    BasicBlock* elseBlock() const { return elseBlock_; }

    std::vector<Value*> operands() const override { return {condition_}; }

private:
    Value* condition_;
    BasicBlock* thenBlock_;
    BasicBlock* elseBlock_;
};

/**
 * Return: ret / ret %value
 */
class ReturnInst : public Instruction {
public:
    explicit ReturnInst(Value* returnValue = nullptr)
        : Instruction(Opcode::Ret,
                     std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::Void),
                     ""),
          returnValue_(returnValue) {}

    Value* returnValue() const { return returnValue_; }
    bool hasReturnValue() const { return returnValue_ != nullptr; }

    std::vector<Value*> operands() const override {
        if (returnValue_) return {returnValue_};
        return {};
    }

private:
    Value* returnValue_;
};

/**
 * Function call: %result = call @function, %arg1, %arg2, ...
 */
class CallInst : public Instruction {
public:
    CallInst(std::shared_ptr<semantic::Type> returnType, const std::string& name,
             Function* callee, std::vector<Value*> arguments)
        : Instruction(Opcode::Call, std::move(returnType), name),
          callee_(callee), arguments_(std::move(arguments)) {}

    Function* callee() const { return callee_; }
    const std::vector<Value*>& arguments() const { return arguments_; }

    std::vector<Value*> operands() const override { return arguments_; }

private:
    Function* callee_;
    std::vector<Value*> arguments_;
};

/**
 * Foreign function call: %result = call_foreign @c_function, %arg1, %arg2, ...
 */
class CallForeignInst : public Instruction {
public:
    CallForeignInst(std::shared_ptr<semantic::Type> returnType, const std::string& name,
                    const std::string& foreignName, std::vector<Value*> arguments)
        : Instruction(Opcode::CallForeign, std::move(returnType), name),
          foreignName_(foreignName), arguments_(std::move(arguments)) {}

    const std::string& foreignName() const { return foreignName_; }
    const std::vector<Value*>& arguments() const { return arguments_; }

    std::vector<Value*> operands() const override { return arguments_; }

private:
    std::string foreignName_;
    std::vector<Value*> arguments_;
};

/**
 * Phi node (for future proper SSA): %result = phi [%val1, bb1], [%val2, bb2], ...
 * Not used initially, but here for completeness
 */
class PhiInst : public Instruction {
public:
    using IncomingValue = std::pair<Value*, BasicBlock*>;

    PhiInst(std::shared_ptr<semantic::Type> type, const std::string& name,
            std::vector<IncomingValue> incomingValues)
        : Instruction(Opcode::Phi, std::move(type), name),
          incomingValues_(std::move(incomingValues)) {}

    const std::vector<IncomingValue>& incomingValues() const { return incomingValues_; }

    std::vector<Value*> operands() const override {
        std::vector<Value*> ops;
        for (const auto& [val, _] : incomingValues_) {
            ops.push_back(val);
        }
        return ops;
    }

private:
    std::vector<IncomingValue> incomingValues_;
};

// ============================================================================
// Basic Block
// ============================================================================

/**
 * Basic block - sequence of instructions with single entry/exit
 * Instructions execute sequentially, last instruction must be terminator (branch/return)
 */
class BasicBlock : public Value {
public:
    BasicBlock(const std::string& name, Function* parent = nullptr)
        : Value(Kind::BasicBlock,
               std::make_shared<semantic::PrimitiveType>(semantic::PrimitiveType::PrimitiveKind::Void),
               name),
          parent_(parent) {}

    // Add instruction to end of block (arena-allocated, passed as raw pointer)
    void addInstruction(Instruction* inst);

    // Get instructions
    const std::vector<Instruction*>& instructions() const { return instructions_; }

    // Check if block has terminator (branch/return)
    bool hasTerminator() const;

    // Get terminator instruction (last instruction, must be branch/return)
    Instruction* terminator() const;

    Function* parent() const { return parent_; }
    void setParent(Function* parent) { parent_ = parent; }

    std::string toString() const override { return name(); }

private:
    Function* parent_;
    std::vector<Instruction*> instructions_;  // Raw pointers - IRModule owns via arena
};

// ============================================================================
// Function
// ============================================================================

/**
 * IR function - contains basic blocks
 * Parameters are owned by IRModule, stored here as raw pointers
 */
class Function {
public:
    Function(const std::string& name,
             std::shared_ptr<semantic::FunctionType> type,
             std::vector<Parameter*> parameters)
        : name_(name), type_(std::move(type)), parameters_(std::move(parameters)) {}

    const std::string& name() const { return name_; }
    const std::shared_ptr<semantic::FunctionType>& type() const { return type_; }
    const std::vector<Parameter*>& parameters() const { return parameters_; }
    const std::vector<BasicBlock*>& basicBlocks() const { return basicBlocks_; }

    // Add basic block (arena-allocated, passed as raw pointer)
    BasicBlock* addBasicBlock(BasicBlock* block);

    // Get entry block (first block)
    BasicBlock* entryBlock() const { return basicBlocks_.empty() ? nullptr : basicBlocks_[0]; }

    // Check if this is a foreign function declaration
    bool isForeign() const { return isForeign_; }
    void setForeign(bool foreign) { isForeign_ = foreign; }

private:
    std::string name_;
    std::shared_ptr<semantic::FunctionType> type_;
    std::vector<Parameter*> parameters_;
    std::vector<BasicBlock*> basicBlocks_;  // Raw pointers - IRModule owns via arena
    bool isForeign_ = false;
};

} // namespace volta::ir
