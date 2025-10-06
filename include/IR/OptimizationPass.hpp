#pragma once

#include <string>
#include <memory>
#include <unordered_set>
#include "IR/Module.hpp"
#include "IR/Function.hpp"

namespace volta::ir {

/**
 * Pass - Base class for optimization passes
 *
 * Design Philosophy:
 * - Single Responsibility: Each pass does one thing well
 * - Composable: Passes can be chained together
 * - Measurable: Report what changed
 * - Idempotent: Running twice = running once (for most passes)
 *
 * Pass Types:
 * 1. **Module Pass**: Operates on entire module
 * 2. **Function Pass**: Operates on one function at a time
 * 3. **BasicBlock Pass**: Operates on one block at a time
 *
 * Common Passes:
 * - Dead Code Elimination (DCE)
 * - Constant Folding
 * - Constant Propagation
 * - Copy Propagation
 * - Common Subexpression Elimination (CSE)
 * - Inlining
 * - Mem2Reg (alloca → SSA)
 */
class Pass {
public:
    virtual ~Pass() = default;

    /**
     * Get pass name for debugging
     */
    virtual std::string getName() const = 0;

    /**
     * Run pass on module
     * @return true if IR was modified
     */
    virtual bool runOnModule(Module& module) = 0;
};

// ============================================================================
// Dead Code Elimination
// ============================================================================

/**
 * DeadCodeEliminationPass - Remove unused instructions
 *
 * Algorithm:
 * 1. Mark all instructions as dead
 * 2. Mark instructions with side effects as live
 * 3. Recursively mark operands of live instructions
 * 4. Delete unmarked (dead) instructions
 *
 * Example:
 *   %1 = add i64 5, 3      ; dead (result unused)
 *   %2 = mul i64 2, 4      ; live (used below)
 *   %3 = sub i64 %2, 1     ; live (used in return)
 *   ret i64 %3
 *
 * After DCE:
 *   %2 = mul i64 2, 4
 *   %3 = sub i64 %2, 1
 *   ret i64 %3
 */
class DeadCodeEliminationPass : public Pass {
public:
    std::string getName() const override { return "DCE"; }
    bool runOnModule(Module& module) override;

private:
    bool runOnFunction(Function* func);
    void markLive(Instruction* inst);
    
    bool hasSideEffects(const Instruction* inst) const;
    bool isInstructionLive(const Instruction* inst) const;

    std::unordered_set<const Instruction*> liveInstructions_;
};

// ============================================================================
// Constant Folding
// ============================================================================

/**
 * ConstantFoldingPass - Evaluate constant expressions at compile time
 *
 * Example:
 *   %1 = add i64 5, 3      →  %1 = 8
 *   %2 = mul i64 2, 4      →  %2 = 8
 *   %3 = eq i64 %1, %2     →  %3 = true
 */
class ConstantFoldingPass : public Pass {
public:
    std::string getName() const override { return "ConstFold"; }
    bool runOnModule(Module& module) override;

private:
    bool runOnFunction(Function* func);
    Value* foldBinaryOp(BinaryOperator* binOp);
    Value* foldUnaryOp(UnaryOperator* unOp);
    Value* foldComparison(Instruction* cmp);

    Module* module_ = nullptr;  // Module pointer for constant creation
};

// ============================================================================
// Constant Propagation
// ============================================================================

/**
 * ConstantPropagationPass - Replace variable uses with known constants
 *
 * Example:
 *   %x = 5
 *   %y = add i64 %x, 3     →  %y = add i64 5, 3
 *   %z = mul i64 %y, 2     →  %z = mul i64 8, 2  (after folding)
 */
class ConstantPropagationPass : public Pass {
public:
    std::string getName() const override { return "ConstProp"; }
    bool runOnModule(Module& module) override;

private:
    bool runOnFunction(Function* func);
};

// ============================================================================
// Mem2Reg (Promote Memory to Registers)
// ============================================================================

/**
 * Mem2RegPass - Convert alloca/load/store to pure SSA
 *
 * This is one of the most important passes!
 *
 * Before:
 *   %ptr = alloca i64
 *   store i64 5, %ptr
 *   %val = load %ptr
 *   ret i64 %val
 *
 * After:
 *   ret i64 5
 *
 * Algorithm (simplified):
 * 1. Find all allocas that can be promoted
 * 2. Insert phi nodes at merge points
 * 3. Replace loads with SSA values
 * 4. Remove stores and allocas
 */
class Mem2RegPass : public Pass {
public:
    std::string getName() const override { return "Mem2Reg"; }
    bool runOnModule(Module& module) override;

private:
    bool runOnFunction(Function* func);
    bool isPromotable(const AllocaInst* alloca) const;
    void promoteAlloca(AllocaInst* alloca);
};


// ============================================================================
// SimplifyCFG
// ============================================================================

class SimplifyCFG : public Pass {
public:
    std::string getName() const override { return "SimplifyCFG"; }
    bool runOnModule(Module& module) override;

private:
    bool runOnFunction(Function* func);
    bool mergeBlocks(Function* func);
};

// ============================================================================
// Pass Manager
// ============================================================================

/**
 * PassManager - Runs multiple passes in sequence
 *
 * Usage:
 *   PassManager pm;
 *   pm.addPass(std::make_unique<ConstantFoldingPass>());
 *   pm.addPass(std::make_unique<DeadCodeEliminationPass>());
 *   pm.run(module);
 */
class PassManager {
public:
    /**
     * Add a pass to the pipeline
     */
    void addPass(std::unique_ptr<Pass> pass);

    /**
     * Run all passes on module
     * @param module The module to optimize
     * @return true if any pass modified the IR
     */
    bool run(Module& module);

    /**
     * Get number of passes in pipeline
     */
    size_t getNumPasses() const { return passes_.size(); }

private:
    std::vector<std::unique_ptr<Pass>> passes_;
};

} // namespace volta::ir
