#include "MIR/MIRVerifier.hpp"

namespace MIR {

bool MIRVerifier::verify(const Program& program) {
    hasErrors = false;

    for (auto& fn : program.functions) {
        if (!verifyFunction(fn)) 
            hasErrors = true;
    }

    return hasErrors;
}

bool MIRVerifier::verifyFunction(const Function& function) {
    definedValues.clear();
    blockLabels.clear();

    for (auto& param : function.params) {
        definedValues.insert("%" + param.name);
    }

    for (auto& block : function.blocks) {
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

    for (auto& block : function.blocks) {
        if (!verifyBasicBlock(block)) {
            functionValid = false;
        }
    }

    return functionValid;
}

bool MIRVerifier::verifyBasicBlock(const BasicBlock& block) {
    bool blockValid = true;

    for (auto& inst : block.instructions) {
        if (!verifyInstruction(inst)) {
            blockValid = false;
        }
        if (inst.result.name.size() != 0) {
            definedValues.insert("%" + inst.result.name);
        }
    }

    // Check for unreachable code (instructions after terminator shouldn't exist,
    // but this is already enforced by the BasicBlock structure)

    if (!verifyTerminator(block.terminator)) {
        blockValid = false;
    }

    return blockValid;
}

bool MIRVerifier::verifyInstruction(const Instruction& inst) {
    bool instValid = true;
    // TODO: Implement
    return true;
}

bool MIRVerifier::verifyTerminator(const Terminator& term) {
    // TODO: Implement
    return true;
}


void MIRVerifier::error(const std::string& message) {
    diag.error(message, 0, 0);
}

void MIRVerifier::warning(const std::string& message) {
    diag.warning(message, 0, 0);
}

} // namespace MIR
