#include "MIR/MIRVerifier.hpp"

namespace MIR {

bool MIRVerifier::verify(const Program& program) {
    hasErrors = false;

    for (const auto& fn : program.functions) {
        if (!verifyFunction(fn)) {
            hasErrors = true;
}
    }

    return !hasErrors;  // Return true if NO errors (valid program)
}

bool MIRVerifier::verifyFunction(const Function& function) {
    definedValues.clear();
    blockLabels.clear();

    for (const auto& param : function.params) {
        definedValues.insert("%" + param.name);
    }

    for (const auto& block : function.blocks) {
        blockLabels.insert(block.label);
    }

    if (function.blocks.empty()) {
        // Empty blocks = extern function declaration, which is valid
        return true;
    }

    if (function.blocks[0].label != "entry") {
        warning("First block should be labeled 'entry'");
    }

    bool functionValid = true;

    for (const auto& block : function.blocks) {
        if (!verifyBasicBlock(block)) {
            functionValid = false;
        }
    }

    return functionValid;
}

bool MIRVerifier::verifyBasicBlock(const BasicBlock& block) {
    bool blockValid = true;

    for (const auto& inst : block.instructions) {
        if (!verifyInstruction(inst)) {
            blockValid = false;
        }
        if (!inst.result.name.empty()) {
            definedValues.insert("%" + inst.result.name);
        }
    }

    // Check that block has a terminator
    if (!block.hasTerminator()) {
        error("Basic block '" + block.label + "' does not have a terminator");
        return false;
    }

    // Verify the terminator
    if (!verifyTerminator(block.terminator)) {
        blockValid = false;
    }

    return blockValid;
}

bool MIRVerifier::verifyInstruction(const Instruction& inst) {
    bool instValid = true;

    // Verify all operands are defined
    for (const auto& operand : inst.operands) {
        if (operand.kind == ValueKind::Local || operand.kind == ValueKind::Param) {
            std::string valueName = "%" + operand.name;
            if (definedValues.find(valueName) == definedValues.end()) {
                error("Use of undefined value: " + valueName);
                instValid = false;
            }
        }
    }

    // Result value should not already be defined (SSA property)
    if (!inst.result.name.empty() && inst.result.kind == ValueKind::Local) {
        std::string resultName = "%" + inst.result.name;
        if (definedValues.find(resultName) != definedValues.end()) {
            error("SSA violation: value " + resultName + " defined multiple times");
            instValid = false;
        }
    }

    return instValid;
}

bool MIRVerifier::verifyTerminator(const Terminator& term) {
    bool termValid = true;

    // Verify all operands (like return values, conditions) are defined
    for (const auto& operand : term.operands) {
        if (operand.kind == ValueKind::Local || operand.kind == ValueKind::Param) {
            std::string valueName = "%" + operand.name;
            if (definedValues.find(valueName) == definedValues.end()) {
                error("Use of undefined value in terminator: " + valueName);
                termValid = false;
            }
        }
    }

    // Verify all branch targets exist as basic block labels
    for (const auto& target : term.targets) {
        if (blockLabels.find(target) == blockLabels.end()) {
            error("Branch to undefined block label: " + target);
            termValid = false;
        }
    }

    // Verify terminator kind-specific requirements
    switch (term.kind) {
        case TerminatorKind::Return:
            // Return can have 0 (void) or 1 (value) operands
            if (term.operands.size() > 1) {
                error("Return terminator should have at most 1 operand");
                termValid = false;
            }
            if (!term.targets.empty()) {
                error("Return terminator should not have branch targets");
                termValid = false;
            }
            break;

        case TerminatorKind::Branch:
            // Unconditional branch has no operands, 1 target
            if (!term.operands.empty()) {
                error("Unconditional branch should have no operands");
                termValid = false;
            }
            if (term.targets.size() != 1) {
                error("Unconditional branch should have exactly 1 target");
                termValid = false;
            }
            break;

        case TerminatorKind::CondBranch:
            // Conditional branch has 1 operand (condition), 2 targets
            if (term.operands.size() != 1) {
                error("Conditional branch should have exactly 1 operand (condition)");
                termValid = false;
            }
            if (term.targets.size() != 2) {
                error("Conditional branch should have exactly 2 targets (true/false)");
                termValid = false;
            }
            break;

        case TerminatorKind::Switch:
            // Switch has 1 operand (value) and multiple targets
            if (term.operands.size() != 1) {
                error("Switch should have exactly 1 operand (switch value)");
                termValid = false;
            }
            if (term.targets.empty()) {
                error("Switch should have at least 1 target (default case)");
                termValid = false;
            }
            break;

        case TerminatorKind::Unreachable:
            // Unreachable has no operands or targets
            if (!term.operands.empty() || !term.targets.empty()) {
                error("Unreachable terminator should have no operands or targets");
                termValid = false;
            }
            break;
    }

    return termValid;
}


void MIRVerifier::error(const std::string& message) {
    diag.error(message, 0, 0);
}

void MIRVerifier::warning(const std::string& message) {
    diag.warning(message, 0, 0);
}

} // namespace MIR
