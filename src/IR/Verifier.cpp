#include "IR/Verifier.hpp"

namespace volta::ir {

Verifier::Verifier() {
    // TODO: Initialize verifier
}

bool Verifier::verify(const Module& module) {
    // TODO: Implement
    // Verify all functions in module
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
    // TODO: Implement
    // 1. Verify phi placement
    // 2. Verify each instruction
    return true;
}

bool Verifier::verifyInstruction(const Instruction* inst) {
    // TODO: Implement
    // 1. Verify instruction types
    // 2. Verify operands
    return true;
}

void Verifier::reportError(const std::string& message) {
    errors_.push_back(message);
}

bool Verifier::verifyCFG(const Function* func) {
    // TODO: Implement
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
    // TODO: Implement
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
    // TODO: Implement
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
