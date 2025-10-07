#include "IR/OptimizationPass.hpp"
#include "IR/BasicBlock.hpp"
#include "IR/Instruction.hpp"
#include "IR/IRType.hpp"
#include <unordered_set>
#include <iostream>

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

    // Store, Call, ArraySet have side effects
    if (isa<StoreInst>(inst) || isa<CallInst>(inst) || isa<ArraySetInst>(inst)) return true;

    // Pure computations have no side effects
    return false;
}

bool DeadCodeEliminationPass::runOnFunction(Function* func) {
    liveInstructions_.clear();

    // 2. Mark phase: Find all live instructions
    for (auto* block : func->getBlocks()) {
        for (auto* inst : block->getInstructions()) {
            if (hasSideEffects(inst)) {
                markLive(inst);
            }
        }
    }

    // 3. Sweep phase: Delete dead instructions
    bool changed = false;
    for (auto* block : func->getBlocks()) {
        auto instructions = block->getInstructions(); 
        for (auto* inst : instructions) {
            if (!isInstructionLive(inst)) {
                block->eraseInstruction(inst);
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
        if (auto* operandInst = dyn_cast<Instruction>(operand)) {
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
    // Store module pointer for constant creation
    module_ = &module;
    bool changed = false;

    for (auto* func : module.getFunctions()) {
        if (runOnFunction(func)) {
            changed = true;
        }
    }

    module_ = nullptr;  // Clear after use
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
            if (auto* binOp = dyn_cast<BinaryOperator>(inst)) {
                folded = foldBinaryOp(binOp);
            } else if (auto* cmpInst = dyn_cast<CmpInst>(inst)) {
                folded = foldComparison(cmpInst);
            } else if (auto* unOp = dyn_cast<UnaryOperator>(inst)) {
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
    auto* lhsInt = dyn_cast<ConstantInt>(binOp->getLHS());
    auto* rhsInt = dyn_cast<ConstantInt>(binOp->getRHS());

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
        return module_->getConstantInt(result, binOp->getType());
    }

    // Try to fold floats
    auto* lhsFloat = dyn_cast<ConstantFloat>(binOp->getLHS());
    auto* rhsFloat = dyn_cast<ConstantFloat>(binOp->getRHS());

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
        return module_->getConstantFloat(result, binOp->getType());
    }

    return nullptr;  // Can't fold non-constants
}

Value* ConstantFoldingPass::foldUnaryOp(UnaryOperator* unOp) {
    if (unOp->getOpcode() == Instruction::Opcode::Neg) {
        if (auto* operand = dyn_cast<ConstantInt>(unOp->getOperand())) {
            return module_->getConstantInt(-operand->getValue(), unOp->getType());
        }
    } else if (unOp->getOpcode() == Instruction::Opcode::Not) {
        if (auto* operand = dyn_cast<ConstantBool>(unOp->getOperand())) {
            return module_->getConstantBool(!operand->getValue(), unOp->getType());
        }
    }

    return nullptr;
}

Value* ConstantFoldingPass::foldComparison(Instruction* cmp) {
    auto* lhs = dyn_cast<ConstantInt>(cmp->getOperand(0));
    auto* rhs = dyn_cast<ConstantInt>(cmp->getOperand(1));

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
    return module_->getConstantBool(result, boolType);
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
    bool changed = false;
    std::vector<AllocaInst*> allocasToPromote;

    // Find all promotable allocas in entry block
    auto* entry = func->getEntryBlock();
    if (!entry) return false;

    for (auto* inst : entry->getInstructions()) {
        if (auto* alloca = dyn_cast<AllocaInst>(inst)) {
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
        if (auto* store = dyn_cast<StoreInst>(user)) {
            if (store->getValue() == alloca) {
                return false;  // Storing the pointer itself
            }
        }
    }

    return true;
}

void Mem2RegPass::promoteAlloca(AllocaInst* alloca) {
    // Find ALL stores and loads
    StoreInst* store = nullptr;
    std::vector<LoadInst*> loads;

    for (auto* use : alloca->getUses()) {
        if (auto* s = dyn_cast<StoreInst>(use->getUser())) {
            store = s;  // Assuming single store for now (simple case)
        } else if (auto* l = dyn_cast<LoadInst>(use->getUser())) {
            loads.push_back(l);
        }
    }

    // Replace ALL loads with stored value
    if (store) {
        Value* storedValue = store->getValue();
        for (auto* load : loads) {
            load->replaceAllUsesWith(storedValue);
            load->getParent()->removeInstruction(load);
        }
        store->getParent()->removeInstruction(store);
        alloca->getParent()->removeInstruction(alloca);
    } else if (loads.empty()) {
        // No stores or loads - just remove unused alloca
        alloca->getParent()->removeInstruction(alloca);
    }
}


// ============================================================================
// SimplifyCFG
// ============================================================================

bool SimplifyCFG::runOnModule(Module& module) {
    bool modified = false;

    for (auto* fn : module.getFunctions()) {
        if (runOnFunction(fn)) modified = true;
        if (mergeBlocks(fn)) modified = true;
    }

    return modified;
}

bool SimplifyCFG::runOnFunction(Function* func) {
    bool changed = false;

    if (func->removeUnreachableBlocks() > 0){
        changed = true;
    }
    return changed;
}

bool SimplifyCFG::mergeBlocks(Function* func) {
    bool modified = false;

    for (auto* block : func->getBlocks()) {
        auto* term = block->getTerminator();
        if(!term) continue;

        auto* br = dyn_cast<BranchInst>(term);
        if (!br) continue;

        auto* dest = br->getDestination();

        if (dest->getNumPredecessors() != 1) continue;

        if (dest->hasPhiNodes()) continue;

        block->removeInstruction(br);

        std::vector<Instruction*> destInsts = dest->getInstructions();

        for (auto* inst : destInsts) {
            dest->removeInstruction(inst);
            block->addInstruction(inst);
        }

        func->eraseBasicBlock(dest);
        modified = true;
    }

    return modified;
}

// ============================================================================
// Algebraic Simplification (Instruction Simplification)
// ============================================================================

bool InstructionSimplifyPass::runOnModule(Module& mod) {
    module_ = &mod;
    bool changed = false;

    for (auto* fn : module_->getFunctions()) {
        if (runOnFunction(fn)) changed = true;        
    }

    return changed;
}

bool InstructionSimplifyPass::runOnFunction(Function* func) {
    bool changed = false;

    for (auto* instr : func->getAllInstructions()) {
        if (auto* op = dyn_cast<BinaryOperator>(instr)) {
            auto simplified = simplifyBinaryOp(op);

            if (simplified) {
                op->replaceAllUsesWith(simplified);
                changed = true;
            }
        }
    }

    return changed;
}


Value* InstructionSimplifyPass::simplifyBinaryOp(BinaryOperator* binOp) {
    switch (binOp->getOpcode()) {
        case Instruction::Opcode::Add: return simplifyAdd(binOp);
        case Instruction::Opcode::Mul: return simplifyMul(binOp);
        case Instruction::Opcode::Sub: return simplifySub(binOp);
        case Instruction::Opcode::Div: return simplifyDiv(binOp);
        case Instruction::Opcode::Pow: return simplifyPow(binOp);
        case Instruction::Opcode::And: return simplifyAnd(binOp);
        case Instruction::Opcode::Or:  return simplifyOr(binOp);
        case Instruction::Opcode::Not: return simplifyNot(binOp);
        case Instruction::Opcode::Gt:  return simplifyGt(binOp);
        case Instruction::Opcode::Ge:  return simplifyGe(binOp);
        case Instruction::Opcode::Lt:  return simplifyLt(binOp);
        case Instruction::Opcode::Le:  return simplifyLe(binOp);
        case Instruction::Opcode::Eq:  return simplifyEq(binOp);
        case Instruction::Opcode::Ne:  return simplifyNe(binOp);
        default: return nullptr;
    }
}
Value* InstructionSimplifyPass::simplifyAdd(BinaryOperator* binOp) {
    Value* lhs = binOp->getLHS();
    Value* rhs = binOp->getRHS();

    if (auto* rhsConst = dyn_cast<ConstantInt>(rhs))
        if (rhsConst->getValue() == 0) return lhs;

    if (auto* lhsConst = dyn_cast<ConstantInt>(lhs))
        if (lhsConst->getValue() == 0) return rhs;

    return nullptr;
}

Value* InstructionSimplifyPass::simplifySub(BinaryOperator* binOp) {
    Value* lhs = binOp->getLHS();
    Value* rhs = binOp->getRHS();

    if (auto* rhsConst = dyn_cast<ConstantInt>(rhs))
        if (rhsConst->getValue() == 0) return lhs;

    if (lhs == rhs) return module_->getConstantInt(0, binOp->getType());
    return nullptr;
}

Value* InstructionSimplifyPass::simplifyMul(BinaryOperator* binOp) {
    Value* lhs = binOp->getLHS();
    Value* rhs = binOp->getRHS();

    if (auto* rhsConst = dyn_cast<ConstantInt>(rhs)) {
        if (rhsConst->getValue() == 0) return module_->getConstantInt(0, binOp->getType());

        if (rhsConst->getValue() == 1) return lhs;
    }

    if (auto* lhsConst = dyn_cast<ConstantInt>(lhs)) {
        if (lhsConst->getValue() == 0) return module_->getConstantInt(0, binOp->getType());

        if (lhsConst->getValue() == 1) return rhs;
    }

    return nullptr;
}

Value* InstructionSimplifyPass::simplifyDiv(BinaryOperator* binOp) {
    Value* lhs = binOp->getLHS();
    Value* rhs = binOp->getRHS();

    if (auto* rhsConst = dyn_cast<ConstantInt>(rhs)) {
        if (rhsConst->getValue() == 1) return lhs;
    }

    if (lhs == rhs) return module_->getConstantInt(1, binOp->getType());

    return nullptr;
}

Value* InstructionSimplifyPass::simplifyMod(BinaryOperator* binOp) {
    Value* lhs = binOp->getLHS();
    Value* rhs = binOp->getRHS();

    if (auto* rhsConst = dyn_cast<ConstantInt>(rhs))
        if (rhsConst->getValue() == 1) return module_->getConstantInt(0, binOp->getType());

    if (lhs == rhs) return module_->getConstantInt(0, binOp->getType());
    return nullptr;
}

Value* InstructionSimplifyPass::simplifyPow(BinaryOperator* binOp) {
    Value* lhs = binOp->getLHS();
    Value* rhs = binOp->getRHS();

    if (auto* rhsConst = dyn_cast<ConstantInt>(rhs)) {
        if (rhsConst->getValue() == 0) return module_->getConstantInt(1, binOp->getType());
        if (rhsConst->getValue() == 1) return lhs;
    }

    if (auto* lhsConst = dyn_cast<ConstantInt>(lhs))
        if (lhsConst->getValue() == 1) return module_->getConstantInt(1, binOp->getType());

    return nullptr;
}

Value* InstructionSimplifyPass::simplifyAnd(BinaryOperator* binOp) {
    Value* lhs = binOp->getLHS();
    Value* rhs = binOp->getRHS();

    if (auto* rhsConst = dyn_cast<ConstantInt>(rhs)) {
        if (rhsConst->getValue() == 0) return module_->getConstantInt(0, binOp->getType());
        if (rhsConst->getValue() == 1) return lhs;
    }

    if (auto* lhsConst = dyn_cast<ConstantInt>(lhs)) {
        if (lhsConst->getValue() == 0) return module_->getConstantInt(0, binOp->getType());
        if (lhsConst->getValue() == 1) return rhs;
    }

    return nullptr;
}

Value* InstructionSimplifyPass::simplifyOr(BinaryOperator* binOp) {
    Value* lhs = binOp->getLHS();
    Value* rhs = binOp->getRHS();

    if (auto* rhsConst = dyn_cast<ConstantInt>(rhs)) {
        if (rhsConst->getValue() == 0) return lhs;
        if (rhsConst->getValue() == 1) return module_->getConstantInt(1, binOp->getType());
    }

    if (auto* lhsConst = dyn_cast<ConstantInt>(lhs)) {
        if (lhsConst->getValue() == 0) return rhs;
        if (lhsConst->getValue() == 1) return module_->getConstantInt(1, binOp->getType());
    }

    return nullptr;
}

Value* InstructionSimplifyPass::simplifyNot(BinaryOperator* binOp) {
    Value* lhs = binOp->getLHS();

    // not(not(x)) → x (double negation)
    if (auto* lhsInst = dyn_cast<Instruction>(lhs)) {
        if (auto* innerNot = dyn_cast<UnaryOperator>(lhsInst)) {
            if (innerNot->getOpcode() == Instruction::Opcode::Not) {
                return innerNot->getOperand();
            }
        }
    }

    return nullptr;
}

Value* InstructionSimplifyPass::simplifyGt(BinaryOperator* binOp) {
    Value* lhs = binOp->getLHS();
    Value* rhs = binOp->getRHS();

    // x > x → false
    if (lhs == rhs) {
        return module_->getConstantInt(0, binOp->getType());
    }

    return nullptr;
}

Value* InstructionSimplifyPass::simplifyGe(BinaryOperator* binOp) {
    Value* lhs = binOp->getLHS();
    Value* rhs = binOp->getRHS();

    // x >= x → true
    if (lhs == rhs) {
        return module_->getConstantInt(1, binOp->getType());
    }

    return nullptr;
}

Value* InstructionSimplifyPass::simplifyLt(BinaryOperator* binOp) {
    Value* lhs = binOp->getLHS();
    Value* rhs = binOp->getRHS();

    // x < x → false
    if (lhs == rhs) {
        return module_->getConstantInt(0, binOp->getType());
    }

    return nullptr;
}

Value* InstructionSimplifyPass::simplifyLe(BinaryOperator* binOp) {
    Value* lhs = binOp->getLHS();
    Value* rhs = binOp->getRHS();

    // x <= x → true
    if (lhs == rhs) {
        return module_->getConstantInt(1, binOp->getType());
    }

    return nullptr;
}

Value* InstructionSimplifyPass::simplifyEq(BinaryOperator* binOp) {
    Value* lhs = binOp->getLHS();
    Value* rhs = binOp->getRHS();

    // x == x → true
    if (lhs == rhs) {
        return module_->getConstantInt(1, binOp->getType());
    }

    return nullptr;
}

Value* InstructionSimplifyPass::simplifyNe(BinaryOperator* binOp) {
    Value* lhs = binOp->getLHS();
    Value* rhs = binOp->getRHS();

    // x != x → false
    if (lhs == rhs) {
        return module_->getConstantInt(0, binOp->getType());
    }

    return nullptr;
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
