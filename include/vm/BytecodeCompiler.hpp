#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include "BytecodeModule.hpp"
#include "IR/Module.hpp"
#include "IR/Function.hpp"
#include "IR/BasicBlock.hpp"
#include "IR/Instruction.hpp"
#include "IR/Value.hpp"
#include "OPCode.hpp"

namespace volta::vm {

using byte = uint8_t;

/**
 * LabelFixup - Records locations where branch offsets need to be patched
 *
 * During two-pass compilation:
 * - Pass 1: Emit branches with placeholder offsets, record fixups
 * - Pass 2: Calculate actual offsets and patch bytecode
 */
struct LabelFixup {
    uint32_t patchOffset;          // Where in bytecode to write the offset
    ir::BasicBlock* targetBlock;   // Block we're branching to
    uint32_t sourceOffset;         // Where the branch instruction starts
};

/**
 * FunctionCompilationContext - Per-function compilation state
 *
 * Created fresh for each function being compiled.
 * Tracks register allocation and block offsets within the function.
 */
struct FunctionCompilationContext {
    ir::Function* irFunction;      // The IR function being compiled

    // SSA value → virtual register mapping
    std::unordered_map<ir::Value*, byte> valueToRegister;

    // Basic block → bytecode offset mapping
    std::unordered_map<ir::BasicBlock*, uint32_t> blockOffsets;

    byte nextRegister;             // Next available register
    uint32_t functionStartOffset;  // Function's starting offset in bytecode

    /**
     * Constructor - Pre-allocates registers for function parameters
     *
     * Parameters arrive in r0, r1, r2... per calling convention.
     * We allocate NEW registers for them so r0-rN can be reused for calls.
     *
     * Reserve r0-r2 for call arguments and return values.
     */
    FunctionCompilationContext(ir::Function* func, uint32_t startOffset):
        irFunction(func),
        nextRegister(std::max<byte>(3, func->getNumParams())),  // Reserve r0-r2, or skip param regs if more
        functionStartOffset(startOffset) {
            // Map each parameter to a fresh register AFTER the param registers
            for (size_t i = 0; i < func->getNumParams(); i++) {
                auto* param = func->getParam(i);
                valueToRegister[param] = nextRegister++;
            }
        };
};

/**
 * BytecodeCompiler - Translates IR to bytecode
 *
 * Two-pass compilation strategy:
 * - Pass 1: Emit bytecode with placeholder branch offsets
 * - Pass 2: Resolve labels and patch branch offsets
 *
 * Register allocation:
 * - Phase 1: Sequential allocation (SSA value N → register N)
 * - Future: Liveness-based allocation for register reuse
 */
class BytecodeCompiler {
public:
    BytecodeCompiler() : currentFunction_(nullptr) {};
    ~BytecodeCompiler() = default;

    /**
     * Main entry point - Compiles an IR module to bytecode
     *
     * @param module The IR module to compile
     * @return Compiled bytecode module, or nullptr on error
     */

    // If the IR has top-level statements (not in any function), 
    // the IR frontend should wrap them in a special function (like __main or __init)
    // Then you iterate through ALL functions in the IR module and compile each one
    // For each function, you create a new FunctionCompilationContext
    std::unique_ptr<BytecodeModule> compile(std::unique_ptr<ir::Module> module);

private:
    // === Per-Function Compilation ===

    /**
     * Compiles a single IR function
     *
     * Two-pass algorithm:
     * 1. Emit bytecode for all blocks
     * 2. Resolve branch fixups
     * 3. Add FunctionInfo to module
     */
    void compileFunction(ir::Function* func);

    /**
     * Compiles a single basic block
     *
     * 1. Record block offset
     * 2. Compile non-terminator instructions
     * 3. Emit phi moves for successors
     * 4. Compile terminator
     */
    void compileBasicBlock(ir::BasicBlock* block);

    /**
     * Compiles a single IR instruction
     *
     * Big switch on IR opcode to emit corresponding bytecode.
     */
    void compileInstruction(ir::Instruction* inst);

    // === Constant Pool Management ===

    /**
     * Get or add integer constant to pool
     * Uses interning - duplicate values reuse same pool index
     */
    index getOrAddIntConstant(int64_t value);

    /**
     * Get or add float constant to pool
     * Uses interning - duplicate values reuse same pool index
     */
    index getOrAddFloatConstant(double value);

    /**
     * Get or add string constant to pool
     * Uses interning - duplicate values reuse same pool index
     */
    index getOrAddStringConstant(const std::string& value);

    // === Register Allocation ===

    /**
     * Get or allocate register for an IR value
     *
     * If value already has a register, return it.
     * Otherwise, allocate next available register.
     *
     * Special handling for constants: allocate register and emit LOAD_CONST.
     */
    byte getOrAllocateRegister(ir::Value* value);

    /**
     * Get the register for an IR value
     *
     * Value must already have a register allocated.
     * Throws if value not found.
     */
    byte getRegister(ir::Value* value);

    // === Low-Level Code Emission ===

    /**
     * Emit a single byte to bytecode stream
     */
    void emitByte(byte val);

    /**
     * Emit 16-bit unsigned integer (little-endian)
     */
    void emitU16(uint16_t val);

    /**
     * Emit 16-bit signed integer (little-endian)
     */
    void emitI16(int16_t val);

    /**
     * Get current bytecode offset
     */
    uint32_t currentOffset() const;

    // === High-Level Instruction Emission ===
    // One helper per opcode for clean code

    // Arithmetic
    void emitAdd(byte rD, byte rA, byte rB);
    void emitSub(byte rD, byte rA, byte rB);
    void emitMul(byte rD, byte rA, byte rB);
    void emitDiv(byte rD, byte rA, byte rB);
    void emitRem(byte rD, byte rA, byte rB);
    void emitPow(byte rD, byte rA, byte rB);
    void emitNeg(byte rD, byte rA);

    // Comparison
    void emitCmpEq(byte rD, byte rA, byte rB);
    void emitCmpNe(byte rD, byte rA, byte rB);
    void emitCmpLt(byte rD, byte rA, byte rB);
    void emitCmpLe(byte rD, byte rA, byte rB);
    void emitCmpGt(byte rD, byte rA, byte rB);
    void emitCmpGe(byte rD, byte rA, byte rB);

    // Logical
    void emitAnd(byte rD, byte rA, byte rB);
    void emitOr(byte rD, byte rA, byte rB);
    void emitNot(byte rD, byte rA);

    // Constants
    void emitLoadConstInt(byte rD, index poolIdx);
    void emitLoadConstFloat(byte rD, index poolIdx);
    void emitLoadConstString(byte rD, index poolIdx);
    void emitLoadTrue(byte rD);
    void emitLoadFalse(byte rD);
    void emitLoadNone(byte rD);

    // Control flow
    void emitBr(int16_t offset);
    void emitBrIfTrue(byte rCond, int16_t offset);
    void emitBrIfFalse(byte rCond, int16_t offset);
    void emitCall(index funcIdx, byte rResult);
    void emitRet(byte rVal);
    void emitRetVoid();
    void emitHalt();

    // Register operations
    void emitMove(byte rDest, byte rSrc);

    // Array operations (Phase 2+)
    void emitArrayNew(byte rD, byte rSize);
    void emitArrayGet(byte rD, byte rArray, byte rIndex);
    void emitArraySet(byte rArray, byte rIndex, byte rValue);
    void emitArrayLen(byte rD, byte rArray);
    void emitArraySlice(byte rD, byte rArray, byte rStart, byte rEnd);

    void emitStructGet(byte rD, byte rStruct, byte fieldIdx);
    void emitStructSet(byte rD, byte rStruct, byte rValue, byte fieldIdx);

    // String operations (Phase 2+)
    void emitStringLen(byte rD, byte rString);

    // Type operations (Phase 2+)
    void emitCastIntFloat(byte rD, byte rSrc);
    void emitCastFloatInt(byte rD, byte rSrc);
    void emitIsSome(byte rD, byte rOption);
    void emitOptionWrap(byte rD, byte rVal);
    void emitOptionUnwrap(byte rD, byte rOption);

    // === Label Fixup (Two-Pass) ===

    /**
     * Record a branch that needs fixup
     *
     * Called during Pass 1 when emitting branches to unknown offsets.
     */
    void recordFixup(ir::BasicBlock* target);

    /**
     * Resolve all recorded fixups
     *
     * Called during Pass 2 after all blocks are emitted.
     * Calculates actual offsets and patches bytecode.
     */
    void resolveFixups();

    // === Phi Node Handling ===

    /**
     * Emit MOVE instructions for phi nodes
     *
     * Before branching from 'current' to 'successor':
     * - Find all phi nodes in successor
     * - Emit MOVE to set phi result from incoming value
     */
    void emitPhiMoves(ir::BasicBlock* current, ir::BasicBlock* successor);

    // === Utility Helpers ===
    // These break down complex tasks into simpler, understandable steps

    /**
     * Check if an IR value is a compile-time constant
     *
     * Returns true for ConstantInt, ConstantFloat, ConstantBool, etc.
     */
    bool isConstant(ir::Value* value);

    /**
     * Handle constant value emission
     *
     * For a constant IR value:
     * 1. Add to appropriate constant pool (with interning)
     * 2. Allocate a register
     * 3. Emit LOAD_CONST instruction
     * 4. Return the register
     *
     * Called by getOrAllocateRegister when it detects a constant.
     */
    byte emitConstantLoad(ir::Value* constantValue);

    /**
     * Setup function call arguments
     *
     * Emit MOVE instructions to shuffle arguments into calling convention:
     * - arg0 → r0
     * - arg1 → r1
     * - arg2 → r2
     * - etc.
     *
     * Used by compileInstruction when handling Call instructions.
     */
    void setupCallArguments(ir::Instruction* callInst);

    /**
     * Calculate branch offset for label fixup
     *
     * Given a fixup record:
     * 1. Look up target block's offset
     * 2. Calculate relative offset from branch end
     * 3. Return signed 16-bit offset
     *
     * Used by resolveFixups during Pass 2.
     */
    int16_t calculateBranchOffset(const LabelFixup& fixup);

    /**
     * Patch bytecode at offset with 16-bit value
     *
     * Writes a 16-bit value at given offset (little-endian).
     * Used by resolveFixups to fix branch offsets.
     */
    void patchOffset(uint32_t offset, int16_t value);

    /**
     * Compile a binary arithmetic operation
     *
     * Helper for Add, Sub, Mul, Div, Rem, Pow:
     * 1. Get/allocate destination register
     * 2. Get source registers for operands
     * 3. Emit appropriate instruction
     *
     * Reduces duplication in compileInstruction switch.
     */
    void compileBinaryOp(ir::Instruction* inst, void (BytecodeCompiler::*emitFunc)(byte, byte, byte));

    /**
     * Compile a comparison operation
     *
     * Helper for CmpEq, CmpNe, CmpLt, etc:
     * 1. Get/allocate destination register (holds bool result)
     * 2. Get source registers for operands
     * 3. Emit appropriate CMP instruction
     */
    void compileCompareOp(ir::Instruction* inst, void (BytecodeCompiler::*emitFunc)(byte, byte, byte));

    /**
     * Compile a unary operation
     *
     * Helper for Neg, Not:
     * 1. Get/allocate destination register
     * 2. Get source register
     * 3. Emit appropriate instruction
     */
    void compileUnaryOp(ir::Instruction* inst, void (BytecodeCompiler::*emitFunc)(byte, byte));

    /**
     * Get instruction length in bytes
     *
     * Returns the total byte length of an instruction given its opcode.
     * Used for offset calculations in branch fixups.
     *
     * Example: BR is 1 (opcode) + 2 (offset) = 3 bytes
     */
    uint32_t getInstructionLength(Opcode opcode) const;

    /**
     * Register runtime functions in function table
     *
     * Called at start of compile() to populate indices 0-N with
     * runtime functions like __alloc_array, __print_int, etc.
     *
     * Makes the compile() method cleaner by extracting this setup.
     */
    void registerRuntimeFunctions();

    /**
     * Validate function doesn't exceed limits
     *
     * Checks:
     * - Register count < 256
     * - Code size reasonable
     * - No unresolved references
     *
     * Called at end of compileFunction before adding to module.
     */
    bool validateFunction(ir::Function* func);

    // === State ===

    std::unique_ptr<ir::Module> irModule_;
    std::unique_ptr<BytecodeModule> module_;

    // Constant pool interning (deduplication)
    std::unordered_map<int64_t, index> intPoolMap_;
    std::unordered_map<double, index> floatPoolMap_;
    std::unordered_map<std::string, index> stringPoolMap_;

    // Current function compilation context
    std::unique_ptr<FunctionCompilationContext> currentFunction_;

    // Label fixups for current function
    std::vector<LabelFixup> fixups_;
};

} // namespace volta::vm