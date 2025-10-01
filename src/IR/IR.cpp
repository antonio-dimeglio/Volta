#include "IR/IR.hpp"
#include <sstream>

namespace volta::ir {

// ============================================================================
// Constant Implementation
// ============================================================================

std::string Constant::toString() const {
    return std::visit([](const auto& val) -> std::string {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, int64_t>) {
            return std::to_string(val);
        } else if constexpr (std::is_same_v<T, double>) {
            return std::to_string(val);
        } else if constexpr (std::is_same_v<T, bool>) {
            return val ? "true" : "false";
        } else if constexpr (std::is_same_v<T, std::string>) {
            return "\"" + val + "\"";
        } else {
            return "none";
        }
    }, value_);
}

// ============================================================================
// Instruction Implementation
// ============================================================================

std::string Instruction::opcodeName(Opcode op) {

    switch (op) {
        // Arithmetics
        case Opcode::Add: return "add";
        case Opcode::Sub: return "sub";
        case Opcode::Mul: return "mul";
        case Opcode::Div: return "div";
        case Opcode::Mod: return "mod";
        case Opcode::Neg: return "neg";

        // Comparison
        case Opcode::Eq: return "eq";
        case Opcode::Ne: return "ne";
        case Opcode::Lt: return "lt";
        case Opcode::Le: return "le";
        case Opcode::Gt: return "gt";
        case Opcode::Ge: return "ge";

        // Logical 
        case Opcode::And: return "and";
        case Opcode::Or: return "or";
        case Opcode::Not: return "not";

        // Memory 
        case Opcode::Alloc: return "alloc";
        case Opcode::Load: return "load";
        case Opcode::Store: return "store";
        case Opcode::GetField: return "get_field";
        case Opcode::SetField: return "set_field";
        case Opcode::GetElement: return "get_element";
        case Opcode::SetElement: return "set_element";

        // Control
        case Opcode::Br: return "br";
        case Opcode::BrIf: return "br_if";
        case Opcode::Ret: return "ret";
        case Opcode::Call: return "call";
        case Opcode::CallForeign: return  "call_foreign";
        case Opcode::Cast: return "cast";
        case Opcode::Phi: return "phi";
    }
    return "unknown";
}

// ============================================================================
// BasicBlock Implementation
// ============================================================================

void BasicBlock::addInstruction(std::unique_ptr<Instruction> inst) {
    inst->setParent(this);
    instructions_.push_back(std::move(inst));
}

bool BasicBlock::hasTerminator() const {    
    return !instructions_.empty() && (
        instructions_.back()->opcode() == Instruction::Opcode::Br ||
        instructions_.back()->opcode() == Instruction::Opcode::BrIf ||    
        instructions_.back()->opcode() == Instruction::Opcode::Ret);
}

Instruction* BasicBlock::terminator() const {
    if (!hasTerminator()) {
        return nullptr; 
    } else {
        return instructions_.back().get();
    }
}

// ============================================================================
// Function Implementation
// ============================================================================

BasicBlock* Function::addBasicBlock(std::unique_ptr<BasicBlock> block) {
    block->setParent(this);
    BasicBlock* ptr = block.get();
    basicBlocks_.push_back(std::move(block));
    return ptr;
}

} // namespace volta::ir
