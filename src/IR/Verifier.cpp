#include "IR/Verifier.hpp"

namespace volta::ir {

Verifier::Verifier() {
    // TODO: Initialize verifier
}

bool Verifier::verify(const Module& module) {
    clearErrors();

    // Verify all functions in module
    for (auto* func : module.getFunctions()) {
        if (!verifyFunction(func)) {
            return false;
        }
    }

    return true;
}

bool Verifier::verifyFunction(const Function* func) {
    // 1. Verify CFG
    if (!verifyCFG(func)) {
        return false;
    }

    // 2. Verify terminators
    if (!verifyTerminators(func)) {
        return false;
    }

    // 3. Verify returns
    if (!verifyReturns(func)) {
        return false;
    }

    // 4. Verify each basic block
    for (auto* block : func->getBlocks()) {
        if (!verifyBasicBlock(block)) {
            return false;
        }
    }

    return true;
}

bool Verifier::verifyBasicBlock(const BasicBlock* block) {
    // 1. Verify phi placement
    if (!verifyPhiNodes(block)) {
        return false;
    }

    // 2. Verify each instruction
    for (auto* inst : block->getInstructions()) {
        if (!verifyInstruction(inst)) {
            return false;
        }
    }

    return true;
}

bool Verifier::verifyInstruction(const Instruction* inst) {
    // 1. Verify instruction has a valid type
    if (!inst->getType()) {
        reportError("Instruction '" + inst->getName() + "' has null type");
        return false;
    }

    // 2. Verify instruction types
    if (!verifyInstructionTypes(inst)) {
        return false;
    }

    // 3. Verify all operands are non-null
    for (unsigned i = 0; i < inst->getNumOperands(); i++) {
        if (!inst->getOperand(i)) {
            reportError("Instruction '" + inst->getName() + "' has null operand at index " + std::to_string(i));
            return false;
        }
    }

    return true;
}

void Verifier::reportError(const std::string& message) {
    errors_.push_back(message);
}

bool Verifier::verifyCFG(const Function* func) {
    // Verify CFG consistency: predecessors/successors are bidirectional
    for (auto* block : func->getBlocks()) {
        // Get successors from terminator
        auto successors = block->getSuccessors();

        // For each successor, verify that this block is in its predecessor list
        for (auto* succ : successors) {
            if (!succ->isPredecessor(block)) {
                reportError("CFG inconsistency: Block '" + block->getName() +
                          "' branches to '" + succ->getName() +
                          "' but is not listed as a predecessor");
                return false;
            }
        }

        // For each predecessor, verify that this block is in its successor list
        for (auto* pred : block->getPredecessors()) {
            auto predSuccessors = pred->getSuccessors();
            bool found = false;
            for (auto* predSucc : predSuccessors) {
                if (predSucc == block) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                reportError("CFG inconsistency: Block '" + pred->getName() +
                          "' is listed as predecessor of '" + block->getName() +
                          "' but does not branch to it");
                return false;
            }
        }
    }

    return true;
}

bool Verifier::verifyTerminators(const Function* func) {
    // Check that every basic block has a terminator
    for (auto* block : func->getBlocks()) {
        if (!block->getTerminator()) {
            reportError("Basic block '" + block->getName() + "' has no terminator");
            return false;
        }
    }
    return true;
}

bool Verifier::verifyPhiNodes(const BasicBlock* block) {
    // Phi nodes must be at the beginning of blocks
    bool foundNonPhi = false;

    for (auto* inst : block->getInstructions()) {
        if (inst->getOpcode() == Instruction::Opcode::Phi) {
            if (foundNonPhi) {
                reportError("Phi node in block '" + block->getName() +
                          "' appears after non-phi instruction");
                return false;
            }

            // Verify phi node has correct number of incoming values
            auto* phi = dynamic_cast<const PhiNode*>(inst);
            if (phi->getNumIncomingValues() != block->getNumPredecessors()) {
                reportError("Phi node in block '" + block->getName() +
                          "' has " + std::to_string(phi->getNumIncomingValues()) +
                          " incoming values but block has " +
                          std::to_string(block->getNumPredecessors()) + " predecessors");
                return false;
            }
        } else {
            foundNonPhi = true;
        }
    }

    return true;
}

bool Verifier::verifyReturns(const Function* func) {
    // Check that return types match function signature
    auto funcRetType = func->getReturnType();

    for (auto* block : func->getBlocks()) {
        auto* term = block->getTerminator();
        if (auto* retInst = dynamic_cast<const ReturnInst*>(term)) {
            auto* retVal = retInst->getReturnValue();

            // Check void returns
            if (funcRetType->kind() == IRType::Kind::Void) {
                if (retVal != nullptr) {
                    reportError("Function '" + func->getName() + "' returns void but return instruction has a value");
                    return false;
                }
            } else {
                // Non-void function must return a value
                if (retVal == nullptr) {
                    reportError("Function '" + func->getName() + "' returns non-void but return instruction has no value");
                    return false;
                }

                // Check type match
                if (!retVal->getType()->equals(funcRetType.get())) {
                    reportError("Function '" + func->getName() + "' return type mismatch");
                    return false;
                }
            }
        }
    }

    return true;
}

bool Verifier::verifyInstructionTypes(const Instruction* inst) {
    auto opcode = inst->getOpcode();

    // Verify binary operations have matching operand types
    if (inst->isBinaryOp()) {
        if (inst->getNumOperands() < 2) {
            reportError("Binary operator '" + inst->getName() + "' has fewer than 2 operands");
            return false;
        }

        auto* lhs = inst->getOperand(0);
        auto* rhs = inst->getOperand(1);

        if (!lhs->getType() || !rhs->getType()) {
            reportError("Binary operator '" + inst->getName() + "' has null operand types");
            return false;
        }

        // For comparison ops, result should be i1
        if (inst->isComparison()) {
            if (inst->getType()->kind() != IRType::Kind::I1) {
                reportError("Comparison instruction '" + inst->getName() +
                          "' does not have i1 result type");
                return false;
            }
        }
    }

    // Verify unary operations have one operand
    if (inst->isUnaryOp()) {
        if (inst->getNumOperands() != 1) {
            reportError("Unary operator '" + inst->getName() + "' does not have exactly 1 operand");
            return false;
        }
    }

    // Verify call instructions
    if (opcode == Instruction::Opcode::Call) {
        auto* callInst = dynamic_cast<const CallInst*>(inst);
        auto* callee = callInst->getCallee();

        if (!callee) {
            reportError("Call instruction '" + inst->getName() + "' has null callee");
            return false;
        }

        // Verify number of arguments matches
        if (callInst->getNumArguments() != callee->getNumParams()) {
            reportError("Call instruction '" + inst->getName() + "' has " +
                      std::to_string(callInst->getNumArguments()) + " arguments but callee expects " +
                      std::to_string(callee->getNumParams()));
            return false;
        }

        // Verify argument types match
        for (unsigned i = 0; i < callInst->getNumArguments(); i++) {
            auto* arg = callInst->getArgument(i);
            auto* param = callee->getParam(i);

            if (!arg->getType()->equals(param->getType().get())) {
                reportError("Call instruction '" + inst->getName() + "' argument " +
                          std::to_string(i) + " type mismatch");
                return false;
            }
        }
    }

    return true;
}

bool Verifier::verifyUseDefChains(const Function* func) {
    // TODO: Implement
    return true;
}

bool Verifier::verifyCallSites(const Function* func) {
    // TODO: Implement
    return true;
}

} // namespace volta::ir
