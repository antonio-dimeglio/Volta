#include "MIR/MIRPrinter.hpp"

namespace MIR {

void MIRPrinter::indent() {
    for (int i = 0; i < indentLevel; ++i)
        out << "  ";
}

void MIRPrinter::increaseIndent() {
    indentLevel++;
}

void MIRPrinter::decreaseIndent() {
    indentLevel--;
}

void MIRPrinter::print(const Program& program) {
    for (auto& func : program.functions) {
        printFunction(func);
        out << "\n";
    }
}

void MIRPrinter::printFunction(const Function& function) {
    out << "fn " << function.name << "(";
    size_t i = 0;
    for (auto& param : function.params) {
        printType(param.type);
        out << " ";
        printValue(param);
        if (i < function.params.size() - 1){
            out << ", ";
        }
    }
    out << ") -> ";
    printType(function.returnType);

    out << " {\n";
    for (auto& block : function.blocks) {
        printBasicBlock(block);
    }

    out << "}\n";
}

void MIRPrinter::printBasicBlock(const BasicBlock& block) {
    indent();
    out << block.label << ":\n";
    increaseIndent();
    for (auto& instr : block.instructions) {
        printInstruction(instr);
    }
    printTerminator(block.terminator);
    decreaseIndent();

    out << "\n";
}

void MIRPrinter::printInstruction(const Instruction& inst) {
    indent();

    if (inst.opcode == Opcode::Store) {
        out << "store ";
        printValue(inst.operands[0]);
        out << ", ";
        printValue(inst.operands[1]);
        out << "\n";
        return;
    }

    printValue(inst.result);
    out << " = ";
    out << opcodeToString(inst.opcode);
    out << " ";

    // For Call instructions, print the function name
    if (inst.opcode == Opcode::Call && inst.callTarget.has_value()) {
        out << inst.callTarget.value();
        if (!inst.operands.empty()) {
            out << "(";
        }
    }

    size_t i = 0;

    for (auto& opr : inst.operands) {
        printValue(opr);
        if (i < inst.operands.size() - 1) {
            out << ", ";
        }
        i++;
    }

    // For Call instructions, close the parentheses
    if (inst.opcode == Opcode::Call && inst.callTarget.has_value() && !inst.operands.empty()) {
        out << ")";
    }

    out << "\n";
}

void MIRPrinter::printTerminator(const Terminator& term) {
    out << term.toString();
}

void MIRPrinter::printValue(const Value& value) {
    out << value.toString();
}

void MIRPrinter::printType(const Type::Type* type) {
    out << type->toString();
}

std::string MIRPrinter::opcodeToString(Opcode opcode) const {
    switch (opcode) {
        case Opcode::IAdd: return "iadd";
        case Opcode::ISub: return "isub";
        case Opcode::IMul: return "imul";
        case Opcode::IDiv: return "idiv";
        case Opcode::IRem: return "irem";
        case Opcode::UDiv: return "udiv";
        case Opcode::URem: return "urem";
        case Opcode::FAdd: return "fadd";
        case Opcode::FSub: return "fsub";
        case Opcode::FMul: return "fmul";
        case Opcode::FDiv: return "fdiv";
        case Opcode::ICmpEQ: return "icmp_eq";
        case Opcode::ICmpNE: return "icmp_ne";
        case Opcode::ICmpLT: return "icmp_lt";
        case Opcode::ICmpLE: return "icmp_le";
        case Opcode::ICmpGT: return "icmp_gt";
        case Opcode::ICmpGE: return "icmp_ge";
        case Opcode::ICmpULT: return "icmp_ult";
        case Opcode::ICmpULE: return "icmp_ule";
        case Opcode::ICmpUGT: return "icmp_ugt";
        case Opcode::ICmpUGE: return "icmp_uge";
        case Opcode::FCmpEQ: return "fcmp_eq";
        case Opcode::FCmpNE: return "fcmp_ne";
        case Opcode::FCmpLT: return "fcmp_lt";
        case Opcode::FCmpLE: return "fcmp_le";
        case Opcode::FCmpGT: return "fcmp_gt";
        case Opcode::FCmpGE: return "fcmp_ge";
        case Opcode::And: return "and";
        case Opcode::Or: return "or";
        case Opcode::Not: return "not";
        case Opcode::Alloca: return "alloca";
        case Opcode::Load: return "load";
        case Opcode::Store: return "store";
        case Opcode::GetElementPtr: return "getelementptr";
        case Opcode::GetFieldPtr: return "getfieldptr";
        case Opcode::Call: return "call";
        case Opcode::SIToFP: return "sitofp";
        case Opcode::UIToFP: return "uitofp";
        case Opcode::FPToSI: return "fptosi";
        case Opcode::FPToUI: return "fptoui";
        case Opcode::SExt: return "sext";
        case Opcode::ZExt: return "zext";
        case Opcode::Trunc: return "trunc";
        case Opcode::FPExt: return "fpext";
        case Opcode::FPTrunc: return "fptrunc";
        case Opcode::Bitcast: return "bitcast";
        case Opcode::ExtractValue: return "extractvalue";
        case Opcode::InsertValue: return "insertvalue";
        case Opcode::GetDiscriminant: return "get_discriminant";
        case Opcode::ExtractVariant: return "extract_variant";
    }
    return "unknown";
}

std::string MIRPrinter::terminatorKindToString(TerminatorKind kind) const {

    switch (kind) {
        case TerminatorKind::Branch: return "br";
        case TerminatorKind::CondBranch: return "cb";
        case TerminatorKind::Return: return "ret";
        case TerminatorKind::Switch: return "switch";
        case TerminatorKind::Unreachable: return "unreachable";
    }
    
}

} // namespace MIR
