#include "IR/OptimizationPass.hpp"
#include "IR/BasicBlock.hpp"
#include "IR/Instruction.hpp"
#include "IR/IRType.hpp"
#include <unordered_set>

namespace volta::ir {

// ============================================================================
// DeadCodeEliminationPass
// ============================================================================

bool DeadCodeEliminationPass::runOnModule(Module& module) {
    bool changed = false;

    for (auto* func : module.getFunctions()) {
        if (runOnFunction(func)) {
            changed = true;
        }
    }

    return changed;
}

bool DeadCodeEliminationPass::hasSideEffects(const Instruction* inst) const {
    // Terminators always have side effects
    if (inst->isTerminator()) return true;

    // Store, Call have side effects
    if (isa<StoreInst>(inst) || isa<CallInst>(inst)) return true;

    // Pure computations have no side effects
    return false;
}

bool DeadCodeEliminationPass::runOnFunction(Function* func) {
    liveInstructions_.clear();

    // 2. Mark phase: Find all live instructions
    for (auto* block : func->getBlocks()) {
        for (auto* inst : block->getInstructions()) {
            // Mark instructions with side effects as live
            if (hasSideEffects(inst)) {
                markLive(inst);
            }
        }
    }

    // 3. Sweep phase: Delete dead instructions
    bool changed = false;
    for (auto* block : func->getBlocks()) {
        auto instructions = block->getInstructions();  // Copy!
        for (auto* inst : instructions) {
            if (!isInstructionLive(inst)) {
                block->removeInstruction(inst);
                changed = true;
            }
        }
    }

    return changed;
}

void DeadCodeEliminationPass::markLive(Instruction* inst) {
    if (isInstructionLive(inst)) {
        return;
    }

    // Mark this instruction
    liveInstructions_.insert(inst);

    // Recursively mark all operands
    for (unsigned i = 0; i < inst->getNumOperands(); i++) {
        Value* operand = inst->getOperand(i);

        // If operand is an instruction, mark it live too
        if (auto* operandInst = dynamic_cast<Instruction*>(operand)) {
            markLive(operandInst);
        }
    }
}

bool DeadCodeEliminationPass::isInstructionLive(const Instruction* inst) const {
    return liveInstructions_.find(inst) != liveInstructions_.end();
}

// ============================================================================
// ConstantFoldingPass
// ============================================================================

bool ConstantFoldingPass::runOnModule(Module& module) {
    // TODO CHECK IF CORRECT
    bool changed = false;

    for (auto* func : module.getFunctions()) {
        if (runOnFunction(func)) {
            changed = true;
        }
    }

    return changed;
}

bool ConstantFoldingPass::runOnFunction(Function* func) {
    bool changed = false;

    // Iterate through all instructions
    for (auto* block : func->getBlocks()) {
        auto instructions = block->getInstructions();

        for (auto* inst : instructions) {
            Value* folded = nullptr;

            // Try to fold based on instruction type
            if (auto* binOp = dynamic_cast<BinaryOperator*>(inst)) {
                // Check if it's a comparison operation
                auto opcode = binOp->getOpcode();
                if (opcode == Instruction::Opcode::Eq || opcode == Instruction::Opcode::Ne ||
                    opcode == Instruction::Opcode::Lt || opcode == Instruction::Opcode::Le ||
                    opcode == Instruction::Opcode::Gt || opcode == Instruction::Opcode::Ge) {
                    folded = foldComparison(binOp);
                } else {
                    folded = foldBinaryOp(binOp);
                }
            } else if (auto* unOp = dynamic_cast<UnaryOperator*>(inst)) {
                folded = foldUnaryOp(unOp);
            }

            // If we folded it, replace all uses
            if (folded) {
                inst->replaceAllUsesWith(folded);
                changed = true;
            }
        }
    }

    return changed;
}

Value* ConstantFoldingPass::foldBinaryOp(BinaryOperator* binOp) {
    // Try to fold integers first
    auto* lhsInt = dynamic_cast<ConstantInt*>(binOp->getLHS());
    auto* rhsInt = dynamic_cast<ConstantInt*>(binOp->getRHS());

    if (lhsInt && rhsInt) {
        // Compute result based on opcode
        int64_t result;
        switch (binOp->getOpcode()) {
            case Instruction::Opcode::Add:
                result = lhsInt->getValue() + rhsInt->getValue();
                break;
            case Instruction::Opcode::Sub:
                result = lhsInt->getValue() - rhsInt->getValue();
                break;
            case Instruction::Opcode::Mul:
                result = lhsInt->getValue() * rhsInt->getValue();
                break;
            case Instruction::Opcode::Div:
                if (rhsInt->getValue() == 0) return nullptr;  // Can't fold division by zero
                result = lhsInt->getValue() / rhsInt->getValue();
                break;
            case Instruction::Opcode::Rem:
                if (rhsInt->getValue() == 0) return nullptr;
                result = lhsInt->getValue() % rhsInt->getValue();
                break;
            default:
                return nullptr;  // Unknown opcode
        }
        return ConstantInt::get(result, binOp->getType());
    }

    // Try to fold floats
    auto* lhsFloat = dynamic_cast<ConstantFloat*>(binOp->getLHS());
    auto* rhsFloat = dynamic_cast<ConstantFloat*>(binOp->getRHS());

    if (lhsFloat && rhsFloat) {
        double result;
        switch (binOp->getOpcode()) {
            case Instruction::Opcode::Add:
                result = lhsFloat->getValue() + rhsFloat->getValue();
                break;
            case Instruction::Opcode::Sub:
                result = lhsFloat->getValue() - rhsFloat->getValue();
                break;
            case Instruction::Opcode::Mul:
                result = lhsFloat->getValue() * rhsFloat->getValue();
                break;
            case Instruction::Opcode::Div:
                if (rhsFloat->getValue() == 0.0) return nullptr;
                result = lhsFloat->getValue() / rhsFloat->getValue();
                break;
            default:
                return nullptr;
        }
        return ConstantFloat::get(result, binOp->getType());
    }

    return nullptr;  // Can't fold non-constants
}

Value* ConstantFoldingPass::foldUnaryOp(UnaryOperator* unOp) {
    if (unOp->getOpcode() == Instruction::Opcode::Neg) {
        if (auto* operand = dynamic_cast<ConstantInt*>(unOp->getOperand())) {
            return ConstantInt::get(-operand->getValue(), unOp->getType());
        }
    } else if (unOp->getOpcode() == Instruction::Opcode::Not) {
        if (auto* operand = dynamic_cast<ConstantBool*>(unOp->getOperand())) {
            return ConstantBool::get(!operand->getValue(), unOp->getType());
        }
    }

    return nullptr;
}

Value* ConstantFoldingPass::foldComparison(BinaryOperator* cmp) {
    auto* lhs = dynamic_cast<ConstantInt*>(cmp->getLHS());
    auto* rhs = dynamic_cast<ConstantInt*>(cmp->getRHS());

    if (!lhs || !rhs) return nullptr;

    bool result;
    switch (cmp->getOpcode()) {
        case Instruction::Opcode::Eq:
            result = (lhs->getValue() == rhs->getValue());
            break;
        case Instruction::Opcode::Ne:
            result = (lhs->getValue() != rhs->getValue());
            break;
        case Instruction::Opcode::Lt:
            result = (lhs->getValue() < rhs->getValue());
            break;
        case Instruction::Opcode::Le:
            result = (lhs->getValue() <= rhs->getValue());
            break;
        case Instruction::Opcode::Gt:
            result = (lhs->getValue() > rhs->getValue());
            break;
        case Instruction::Opcode::Ge:
            result = (lhs->getValue() >= rhs->getValue());
            break;
        default:
            return nullptr;
    }

    // Return bool constant
    auto boolType = std::make_shared<IRPrimitiveType>(IRType::Kind::I1);
    return ConstantBool::get(result, boolType);
}

// ============================================================================
// ConstantPropagationPass
// ============================================================================

bool ConstantPropagationPass::runOnModule(Module& module) {
    bool changed = false;

    for (auto* func : module.getFunctions()) {
        if (runOnFunction(func)) {
            changed = true;
        }
    }

    return changed;
}

bool ConstantPropagationPass::runOnFunction(Function* func) {
    bool changed = false;

    // Build constant map
    std::unordered_map<Value*, Constant*> constantMap;

    for (auto* block : func->getBlocks()) {
        for (auto* inst : block->getInstructions()) {
            // If instruction produces a constant, record it
            if (auto* constant = dynamic_cast<Constant*>(inst)) {
                constantMap[inst] = constant;
            }

            // Replace operands with constants if known
            for (unsigned i = 0; i < inst->getNumOperands(); i++) {
                Value* operand = inst->getOperand(i);

                auto it = constantMap.find(operand);
                if (it != constantMap.end()) {
                    inst->setOperand(i, it->second);
                    changed = true;
                }
            }
        }
    }

    return changed;
}

// ============================================================================
// Mem2RegPass
// ============================================================================

bool Mem2RegPass::runOnModule(Module& module) {
    bool changed = false;

    for (auto* func : module.getFunctions()) {
        if (runOnFunction(func)) {
            changed = true;
        }
    }

    return changed;
}

bool Mem2RegPass::runOnFunction(Function* func) {
    // Algorithm:
    // 1. Find all allocas that can be promoted
    // 2. For each promotable alloca:
    //    a. Insert phi nodes at dominance frontiers
    //    b. Replace loads with SSA values
    //    c. Remove stores
    //    d. Remove alloca
    // 3. Return true if any alloca was promoted

    bool changed = false;
    std::vector<AllocaInst*> allocasToPromote;

    // Find all promotable allocas in entry block
    auto* entry = func->getEntryBlock();
    if (!entry) return false;

    for (auto* inst : entry->getInstructions()) {
        if (auto* alloca = dynamic_cast<AllocaInst*>(inst)) {
            if (isPromotable(alloca)) {
                allocasToPromote.push_back(alloca);
            }
        }
    }

    // Promote each alloca
    for (auto* alloca : allocasToPromote) {
        promoteAlloca(alloca);
        changed = true;
    }

    return changed;
}

bool Mem2RegPass::isPromotable(const AllocaInst* alloca) const {
    if (alloca->getParent() != alloca->getParent()->getParent()->getEntryBlock()) {
        return false;
    }

    // Check all uses
    for (auto* use : alloca->getUses()) {
        auto* user = use->getUser();

        // Only loads and stores allowed
        if (!isa<LoadInst>(user) && !isa<StoreInst>(user)) {
            return false;  // Address taken!
        }

        // For stores, must store TO the alloca, not store the alloca itself
        if (auto* store = dynamic_cast<StoreInst*>(user)) {
            if (store->getValue() == alloca) {
                return false;  // Storing the pointer itself
            }
        }
    }

    return true;
}

void Mem2RegPass::promoteAlloca(AllocaInst* alloca) {
    // Find the store and load
    StoreInst* store = nullptr;
    LoadInst* load = nullptr;

    for (auto* use : alloca->getUses()) {
        if (auto* s = dynamic_cast<StoreInst*>(use->getUser())) {
            store = s;
        } else if (auto* l = dynamic_cast<LoadInst*>(use->getUser())) {
            load = l;
        }
    }

    // Replace load with stored value
    if (store && load) {
        load->replaceAllUsesWith(store->getValue());

        // Remove load, store, and alloca
        load->getParent()->removeInstruction(load);
        store->getParent()->removeInstruction(store);
        alloca->getParent()->removeInstruction(alloca);
    }
}

// ============================================================================
// PassManager
// ============================================================================

void PassManager::addPass(std::unique_ptr<Pass> pass) {
    passes_.push_back(std::move(pass));
}

bool PassManager::run(Module& module) {
    bool changed = false;

    // Run each pass in sequence
    for (auto& pass : passes_) {
        if (pass->runOnModule(module)) {
            changed = true;
        }
    }

    return changed;
}

} // namespace volta::ir
