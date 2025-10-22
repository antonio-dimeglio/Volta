#pragma once

#include "MIR/MIR.hpp"
#include "Type/Type.hpp"
#include "Type/TypeRegistry.hpp"
#include <string>
#include <unordered_map>

namespace MIR {

class MIRBuilder {
private:
    Type::TypeRegistry& typeRegistry;
    Program program;

    // Current function being built
    Function* currentFunction = nullptr;

    // Current basic block being built (insertion point)
    BasicBlock* currentBlock = nullptr;

    // Counter for generating unique SSA names: %0, %1, %2, etc.
    int nextValueId = 0;

    // Map from HIR variable names to MIR values
    // Example: "x" -> Value representing %x
    std::unordered_map<std::string, Value> symbolTable;

public:
    explicit MIRBuilder(Type::TypeRegistry& tr) : typeRegistry(tr) {}

    // Create a new function and make it the current function
    // All subsequent blocks/instructions will be added to this function
    void createFunction(const std::string& name,
                        const std::vector<Value>& params,
                        const Type::Type* returnType);

    // Finish building the current function and return to program level
    void finishFunction();

    // Create a new basic block in the current function and set it as insertion point
    // All subsequent instructions will be added to this block
    void createBlock(const std::string& label);

    // Set an existing block as the insertion point
    // Useful for branching back to previous blocks
    void setInsertionPoint(const std::string& blockLabel);

    // Get the current block label (useful for creating branch targets)
    std::string getCurrentBlockLabel() const;

    // Check if the current block has a terminator
    bool currentBlockTerminated() const;

    // Generate a fresh SSA value name: %0, %1, %2, etc.
    Value createTemporary(const Type::Type* type);

    // Create a named value (for parameters, variables, etc.)
    Value createNamedValue(const std::string& name, const Type::Type* type);

    // Create a constant
    Value createConstantInt(int64_t value, const Type::Type* type);
    Value createConstantBool(bool value, const Type::Type* type);
    Value createConstantFloat(double value, const Type::Type* type);
    Value createConstantString(const std::string& value, const Type::Type* type);
    Value createConstantNull(const Type::Type* ptrType);

    // Integer arithmetic
    Value createIAdd(const Value& lhs, const Value& rhs);
    Value createISub(const Value& lhs, const Value& rhs);
    Value createIMul(const Value& lhs, const Value& rhs);
    Value createIDiv(const Value& lhs, const Value& rhs);
    Value createIRem(const Value& lhs, const Value& rhs);
    Value createUDiv(const Value& lhs, const Value& rhs);
    Value createURem(const Value& lhs, const Value& rhs);

    // Float arithmetic
    Value createFAdd(const Value& lhs, const Value& rhs);
    Value createFSub(const Value& lhs, const Value& rhs);
    Value createFMul(const Value& lhs, const Value& rhs);
    Value createFDiv(const Value& lhs, const Value& rhs);

    // Integer comparisons
    Value createICmpEQ(const Value& lhs, const Value& rhs);
    Value createICmpNE(const Value& lhs, const Value& rhs);
    Value createICmpLT(const Value& lhs, const Value& rhs);
    Value createICmpLE(const Value& lhs, const Value& rhs);
    Value createICmpGT(const Value& lhs, const Value& rhs);
    Value createICmpGE(const Value& lhs, const Value& rhs);
    Value createICmpULT(const Value& lhs, const Value& rhs);
    Value createICmpULE(const Value& lhs, const Value& rhs);
    Value createICmpUGT(const Value& lhs, const Value& rhs);
    Value createICmpUGE(const Value& lhs, const Value& rhs);

    // Float comparisons
    Value createFCmpEQ(const Value& lhs, const Value& rhs);
    Value createFCmpNE(const Value& lhs, const Value& rhs);
    Value createFCmpLT(const Value& lhs, const Value& rhs);
    Value createFCmpLE(const Value& lhs, const Value& rhs);
    Value createFCmpGT(const Value& lhs, const Value& rhs);
    Value createFCmpGE(const Value& lhs, const Value& rhs);

    Value createAnd(const Value& lhs, const Value& rhs);
    Value createOr(const Value& lhs, const Value& rhs);
    Value createNot(const Value& operand);

    // Create a type conversion instruction
    // op: conversion opcode (Trunc, SExt, ZExt, FPExt, FPTrunc, SIToFP, UIToFP, FPToSI, FPToUI, Bitcast)
    // value: source value to convert
    // targetType: destination type
    Value createConversion(Opcode op, const Value& value, const Type::Type* targetType);

    // Allocate space on the stack for a value of the given type
    // Returns a pointer to the allocated space
    Value createAlloca(const Type::Type* type);

    // Load a value from a pointer
    Value createLoad(const Value& pointer);

    // Store a value to a pointer (note: store doesn't return a value)
    void createStore(const Value& value, const Value& pointer);

    // Calculate the address of an array element
    // arrayPtr: pointer to array
    // index: element index
    // Returns: pointer to the element
    Value createGetElementPtr(const Value& arrayPtr, const Value& index);

    // Calculate the addres of a field ptr
    // structPtr: pointer to struct
    // index: index of field
    // Returns: pointer to the element
    Value createGetFieldPtr(const Value& structPtr, int fieldIndex);

    // Call a function with the given arguments
    // Returns the result value (or a void value if the function returns void)
    Value createCall(const std::string& functionName,
                     const std::vector<Value>& args,
                     const Type::Type* returnType);

    // Return from the current function with a value
    void createReturn(const Value& value);

    // Return from the current function with no value (void)
    void createReturnVoid();

    // Unconditional branch to a target block
    void createBranch(const std::string& targetLabel);

    // Conditional branch based on a boolean condition
    void createCondBranch(const Value& condition,
                          const std::string& trueLabel,
                          const std::string& falseLabel);

    // Register a variable name -> value mapping
    // Used when lowering HIR variables to MIR values
    void setVariable(const std::string& name, const Value& value);

    // Look up a variable by name
    // Returns the associated MIR value
    Value getVariable(const std::string& name) const;

    // Check if a variable exists
    bool hasVariable(const std::string& name) const;

    // Clear the symbol table (useful between functions)
    void clearSymbolTable();

    // Get the constructed MIR program
    Program& getProgram() { return program; }

    // Get the current function being built
    Function* getCurrentFunction() const { return currentFunction; }
};

} // namespace MIR
