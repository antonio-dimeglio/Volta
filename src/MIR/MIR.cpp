#include "MIR/MIR.hpp"
#include <sstream>

namespace MIR {

// Helper function to convert opcode to string
static std::string opcodeToString(Opcode opcode) {
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


Value Value::makeLocal(const std::string& name, const Type::Type* type) {
    return Value(ValueKind::Local, name, type);
}

Value Value::makeParam(const std::string& name, const Type::Type* type) {
    return Value(ValueKind::Param, name, type);
}

Value Value::makeGlobal(const std::string& name, const Type::Type* type) {
    return Value(ValueKind::Global, name, type);
}

Value Value::makeConstantInt(int64_t value, const Type::Type* type) {
    Value v(ValueKind::Constant, std::to_string(value), type);
    v.constantInt = value;
    return v;
}

Value Value::makeConstantBool(bool value, const Type::Type* type) {
    Value v(ValueKind::Constant, value ? "true" : "false", type);
    v.constantBool = value;
    return v;
}

Value Value::makeConstantFloat(double value, const Type::Type* type) {
    Value v(ValueKind::Constant, std::to_string(value), type);
    v.constantFloat = value;
    return v;
}

Value Value::makeConstantString(const std::string& value, const Type::Type* type) {
    // String constants have a unique name but are Constant kind
    static int stringCounter = 0;
    std::string globalName = ".str." + std::to_string(stringCounter++);
    Value v(ValueKind::Constant, globalName, type);
    v.constantString = value;
    return v;
}

std::string Value::toString() const {
    switch (kind) {
        case ValueKind::Local:
        case ValueKind::Param:
            return "%" + name;
        case ValueKind::Global:
            return "@" + name;
        case ValueKind::Constant:
            if (constantInt.has_value())
                return std::to_string(constantInt.value());
            if (constantBool.has_value())
                return constantBool.value() ? "true" : "false";
            if (constantFloat.has_value())
                return std::to_string(constantFloat.value());
            if (constantString.has_value())
                return "\"" + constantString.value() + "\"";
    }
    return "";
}


std::string Instruction::toString() const {
    std::stringstream ss;

    if (opcode == Opcode::Store) {
        ss << "store ";
        if (operands.size() >= 2) {
            ss << operands[0].toString() << ", " << operands[1].toString();
        }
        return ss.str();
    }

    if (!result.name.empty()) {
        ss << result.toString() << " = ";
    }

    ss << opcodeToString(opcode);

    for (size_t i = 0; i < operands.size(); ++i) {
        ss << " " << operands[i].toString();
        if (i + 1 < operands.size()) {
            ss << ",";
        }
    }

    return ss.str();
}

Terminator Terminator::makeReturn(std::optional<Value> value) {
    Terminator term(TerminatorKind::Return);
    if (value.has_value()) {
        term.operands.push_back(value.value());
    }
    return term;
}

Terminator Terminator::makeReturnVoid() {
    return makeReturn(std::nullopt);
}

Terminator Terminator::makeBranch(const std::string& target) {
    Terminator term(TerminatorKind::Branch);
    term.targets.push_back(target);
    return term;
}

Terminator Terminator::makeCondBranch(const Value& condition,
                                       const std::string& trueTarget,
                                       const std::string& falseTarget) {
    Terminator term(TerminatorKind::CondBranch);  
    term.operands.push_back(condition);
    term.targets.push_back(trueTarget);
    term.targets.push_back(falseTarget);
    return term;
}

std::string Terminator::toString() const {
    switch (kind) {
        case TerminatorKind::Return:
            if (operands.empty())
                return "ret void";
            else
                return "ret " + operands[0].toString();

        case TerminatorKind::Branch:
            if (!targets.empty())
                return "br label %" + targets[0];
            return "br <invalid>";

        case TerminatorKind::CondBranch:
            if (operands.size() == 1 && targets.size() == 2)
                return "condbr " + operands[0].toString() +
                       ", label %" + targets[0] +
                       ", label %" + targets[1];
            return "condbr <invalid>";

        case TerminatorKind::Switch: {
            std::string s = "switch ";
            if (operands.empty())
                return "switch <invalid>";
            s += operands[0].toString() + ", ";
            if (targets.empty())
                return s + "<invalid>";
            s += "label %" + targets[0];
            if (targets.size() > 1) {
                s += ", [";
                for (size_t i = 1; i + 1 < targets.size(); i += 2) {
                    s += operands[i / 2 + 1].toString() + " %" + targets[i];
                    if (i + 2 < targets.size())
                        s += ", ";
                }
                s += "]";
            }
            return s;
        }

        case TerminatorKind::Unreachable:
            return "unreachable";
    }
    return "<unknown terminator>";
}


void BasicBlock::addInstruction(Instruction inst) {
    instructions.push_back(inst);
}

void BasicBlock::setTerminator(Terminator term) {
    if (termSet) {  // Fix: was !termSet, should be termSet
        throw std::runtime_error("Tried to set terminator twice for basic block " + label);
    }
    terminator = term;
    termSet = true;
}

std::string BasicBlock::toString() const {
    std::stringstream oss;

    oss << label;
    for (auto instr : instructions) {
        oss << instr.toString();
    }
    oss << terminator.toString();

    return oss.str();
}


void Function::addBlock(BasicBlock block) {
    blocks.push_back(block);
}

BasicBlock* Function::getBlock(const std::string& label) {
    for (auto& block : blocks) {
        if (block.label == label) {
            return &block;
        }
    }
    return nullptr;
}

std::string Function::toString() const {
    std::stringstream ss;

    ss << "fn " << name << "(";

    // Parameters
    for (size_t i = 0; i < params.size(); ++i) {
        ss << params[i].type->toString() << " " << params[i].toString();
        if (i + 1 < params.size()) {
            ss << ", ";
        }
    }

    ss << ") -> " << returnType->toString() << " {\n";

    // Basic blocks
    for (const auto& block : blocks) {
        ss << "  " << block.label << ":\n";
        for (const auto& inst : block.instructions) {
            ss << "    " << inst.toString() << "\n";
        }
        ss << "    " << block.terminator.toString() << "\n";
    }

    ss << "}\n";

    return ss.str();
}

void Program::addFunction(Function func) {
    functions.push_back(func);
}

Function* Program::getFunction(const std::string& name) {
    for (auto& func : functions) { 
        if (func.name == name) {
            return &func;
        }
    }
    return nullptr;
}

std::string Program::toString() const {
    std::stringstream oss;

    for (auto func : functions) {
        oss << func.toString();
        oss << "\n";
    }
    return oss.str();
}

} // namespace MIR
