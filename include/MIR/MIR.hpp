#pragma once

#include "Type/Type.hpp"
#include <string>
#include <utility>
#include <utility>
#include <vector>
#include <memory>
#include <optional>

namespace MIR {

// Values represent the results of computations in SSA-like form.
// Each value is immutable and has a type. Values can be:
// - Local: SSA temporary (e.g., %1, %2, %result)
// - Param: Function parameter
// - Global: Global variable
// - Constant: Compile-time constant (integers, booleans, etc.)

enum class ValueKind {
    Local,      // SSA temporary: %result, %temp1, etc.
    Param,      // Function parameter: %arg0, %matrix, etc.
    Global,     // Global variable: @main, @some_global
    Constant    // Immediate constant: 42, true, etc.
};

struct Value {
    ValueKind kind;
    std::string name;           // e.g., "result", "temp1", "0" for constants
    const Type::Type* type{};     // Type from TypeRegistry

    // For constants: the actual value
    std::optional<int64_t> constantInt;
    std::optional<bool> constantBool;
    std::optional<double> constantFloat;
    std::optional<std::string> constantString;
    std::optional<bool> constantNull;  // true if this is a null pointer constant

    Value() = default;
    Value(ValueKind k, std::string n, const Type::Type* t)
        : kind(k), name(std::move(std::move(n))), type(t) {}

    // Factory methods for creating different value types
    static Value makeLocal(const std::string& name, const Type::Type* type);
    static Value makeParam(const std::string& name, const Type::Type* type);
    static Value makeGlobal(const std::string& name, const Type::Type* type);
    static Value makeConstantInt(int64_t value, const Type::Type* type);
    static Value makeConstantBool(bool value, const Type::Type* type);
    static Value makeConstantFloat(double value, const Type::Type* type);
    static Value makeConstantString(const std::string& value, const Type::Type* type);
    static Value makeConstantNull(const Type::Type* ptrType);

    // Get string representation: "%result", "@main", "42", etc.
    [[nodiscard]] std::string toString() const;

    [[nodiscard]] bool isConstant() const { return kind == ValueKind::Constant; }
    [[nodiscard]] bool isLocal() const { return kind == ValueKind::Local; }
};

// Instructions are operations that produce values.
// Each instruction has an opcode, operands, and (usually) a result value.

enum class Opcode {
    // === Arithmetic (Integer) ===
    IAdd,       // Integer addition: %result = iadd %a, %b
    ISub,       // Integer subtraction
    IMul,       // Integer multiplication
    IDiv,       // Integer division (signed)
    IRem,       // Integer remainder (signed)
    UDiv,       // Unsigned integer division
    URem,       // Unsigned integer remainder

    // === Arithmetic (Float) ===
    FAdd,       // Float addition: %result = fadd %a, %b
    FSub,       // Float subtraction
    FMul,       // Float multiplication
    FDiv,       // Float division

    // === Comparison (Integer) ===
    ICmpEQ,     // Integer equality: %result = icmp_eq %a, %b
    ICmpNE,     // Integer not equal
    ICmpLT,     // Integer less than (signed)
    ICmpLE,     // Integer less or equal (signed)
    ICmpGT,     // Integer greater than (signed)
    ICmpGE,     // Integer greater or equal (signed)
    ICmpULT,    // Unsigned less than
    ICmpULE,    // Unsigned less or equal
    ICmpUGT,    // Unsigned greater than
    ICmpUGE,    // Unsigned greater or equal

    // === Comparison (Float) ===
    FCmpEQ,     // Float equality (ordered)
    FCmpNE,     // Float not equal (ordered)
    FCmpLT,     // Float less than (ordered)
    FCmpLE,     // Float less or equal (ordered)
    FCmpGT,     // Float greater than (ordered)
    FCmpGE,     // Float greater or equal (ordered)

    // === Logical (Boolean) ===
    And,        // Logical AND: %result = and %a, %b
    Or,         // Logical OR
    Not,        // Logical NOT: %result = not %a

    // === Memory Operations ===
    Alloca,     // Stack allocation: %ptr = alloca [i32; 4]
    Load,       // Load from memory: %value = load %ptr
    Store,      // Store to memory: store %value, %ptr

    // === Array Operations ===
    GetElementPtr,  // Calculate element address: %ptr = getelementptr %array_ptr, %index
                    // This is like LLVM's GEP - calculates address without loading
    // === Struct Operations === 
    GetFieldPtr, // Fetch address of field for a struct: %ptr = getfieldptr %struct_ptr, %field_index

    // === Function Calls ===
    Call,       // Function call: %result = call @function(%arg1, %arg2)

    // === Type Conversions (Integer <-> Float) ===
    SIToFP,     // Signed int to float: %f = sitofp %i
    UIToFP,     // Unsigned int to float: %f = uitofp %i
    FPToSI,     // Float to signed int (truncate): %i = fptosi %f
    FPToUI,     // Float to unsigned int (truncate): %i = fptoui %f

    // === Type Conversions (Integer size changes) ===
    SExt,       // Sign extend integer: %i64 = sext %i32 (i32 -> i64)
    ZExt,       // Zero extend integer: %i64 = zext %i32 (unsigned extension)
    Trunc,      // Truncate integer: %i32 = trunc %i64 (i64 -> i32)

    // === Type Conversions (Float size changes) ===
    FPExt,      // Extend float: %f64 = fpext %f32 (f32 -> f64)
    FPTrunc,    // Truncate float: %f32 = fptrunc %f64 (f64 -> f32)

    // === Bitwise/Other ===
    Bitcast,    // Reinterpret bits (for pointer casts, etc.)

    // === Future Extensions (placeholders) ===
    ExtractValue,   // For structs: %field = extractvalue %struct, field_index
    InsertValue,    // For structs: %new_struct = insertvalue %struct, %value, field_index
    GetDiscriminant,// For ADTs: %tag = get_discriminant %enum
    ExtractVariant, // For ADTs: %payload = extract_variant %enum, variant_index
};


struct Instruction {
    Opcode opcode;
    Value result;                   // Where the result is stored (if any)
    std::vector<Value> operands;    // Input operands

    // Optional: For calls, the function name/value
    std::optional<std::string> callTarget;

    Instruction() = default;
    Instruction(Opcode op, Value res, std::vector<Value> ops)
        : opcode(op), result(std::move(std::move(res))), operands(std::move(ops)) {}

    // Get string representation: "%result = iadd %a, %b"
    [[nodiscard]] std::string toString() const;
};

// Terminators end basic blocks and transfer control.
// Every basic block must have exactly ONE terminator at the end.
enum class TerminatorKind {
    Return,         // Return from function: ret %value  OR  ret void
    Branch,         // Unconditional jump: br label %target
    CondBranch,     // Conditional jump: condbr %cond, label %true, label %false
    Switch,         // Multi-way branch (for pattern matching): switch %value, default %def, [case1 %lab1, ...]
    Unreachable     // Marks unreachable code (e.g., after infinite loop)
};

struct Terminator {
    TerminatorKind kind;
    std::vector<Value> operands;        // Values used (e.g., return value, condition)
    std::vector<std::string> targets;   // Target block labels

    Terminator() : kind(TerminatorKind::Unreachable) {}
    Terminator(TerminatorKind k) : kind(k) {}

    // Factory methods for common terminators
    static Terminator makeReturn(std::optional<Value> value);
    static Terminator makeReturnVoid();  // Helper for void returns
    static Terminator makeBranch(const std::string& target);
    static Terminator makeCondBranch(const Value& condition,
                                      const std::string& trueTarget,
                                      const std::string& falseTarget);

    // Get string representation: "ret %value", "br label %block1", etc.
    [[nodiscard]] std::string toString() const;
};

// A basic block is a sequence of instructions with a single entry point (start)
// and a single exit point (terminator). Control flow only changes at terminators.

struct BasicBlock {
    std::string label;                  // Block label: "entry", "loop_body", "exit"
    std::vector<Instruction> instructions;  // Linear sequence of instructions
    Terminator terminator;              // How this block exits

    BasicBlock() = default;
    explicit BasicBlock(std::string lbl) : label(std::move(lbl)) {}

    // Add an instruction to this block
    void addInstruction(const Instruction& inst);

    // Set the terminator (should only be called once per block)
    void setTerminator(Terminator term);

    // Check if this block has a terminator
    [[nodiscard]] bool hasTerminator() const { return termSet; }

    // Get string representation of the entire block
    [[nodiscard]] std::string toString() const;
private:
    bool termSet = false;
};

// A function is a collection of basic blocks forming a control flow graph.
// The first block is the entry point.

struct Function {
    std::string name;                   // Function name: "main", "add", "determinant_2x2"
    std::vector<Value> params;          // Parameters (all have ValueKind::Param)
    const Type::Type* returnType;       // Return type (or void)
    std::vector<BasicBlock> blocks;     // Basic blocks (first is entry)

    Function() : returnType(nullptr) {}
    Function(std::string n, std::vector<Value> p, const Type::Type* ret)
        : name(std::move(n)), params(std::move(p)), returnType(ret) {}

    // Add a basic block to this function
    void addBlock(const BasicBlock& block);

    // Get a block by label (returns nullptr if not found)
    BasicBlock* getBlock(const std::string& label);

    // Get string representation of the entire function
    [[nodiscard]] std::string toString() const;
};

// The top-level MIR structure containing all functions.

struct Program {
    std::vector<Function> functions;    // All functions in the program

    // Add a function to the program
    void addFunction(const Function& func);

    // Get a function by name (returns nullptr if not found)
    Function* getFunction(const std::string& name);

    // Get string representation of the entire program
    [[nodiscard]] std::string toString() const;
};

} // namespace MIR
