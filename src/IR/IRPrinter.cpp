#include "IR/IRPrinter.hpp"

namespace volta::ir {

// ============================================================================
// Constructor
// ============================================================================

IRPrinter::IRPrinter()
    : nextValueNumber_(0),
      showTypes_(true),
      showCFG_(true),
      showUses_(false) {
}

// ========================================================================
// Top-Level Printing
// ========================================================================

std::string IRPrinter::printModule(const Module& module) {
    std::stringstream ss;

    // 1. Module header
    ss << "; Module: " << module.getName() << "\n\n";

    // 2. Global variables
    for (auto* global : module.getGlobals()) {
        ss << printGlobal(global) << "\n";
    }

    if (!module.getGlobals().empty()) {
        ss << "\n";  // Blank line after globals
    }

    // 3. Functions
    for (auto* func : module.getFunctions()) {
        ss << printFunction(func) << "\n";
    }

    return ss.str();
}

std::string IRPrinter::printFunction(const Function* func) {
    if (!func) return "<null function>";

    // Reset numbering for each function
    reset();

    std::stringstream ss;

    // 1. Function signature
    ss << printFunctionSignature(func) << " {\n";

    // 2. Basic blocks
    for (auto* block : func->getBlocks()) {
        ss << printBasicBlock(block);
    }

    // 3. Closing brace
    ss << "}\n";

    return ss.str();
}

std::string IRPrinter::printBasicBlock(const BasicBlock* block) {
    if (!block) return "";

    std::stringstream ss;

    // 1. Print block label
    ss << block->getName() << ":";

    // 2. Print predecessors (if enabled)
    if (showCFG_) {
        ss << "                              ; preds = ";

        auto preds = block->getPredecessors();
        if (preds.empty()) {
            // Entry block has no predecessors
        } else {
            for (size_t i = 0; i < preds.size(); i++) {
                if (i > 0) ss << ", ";
                ss << "%" << preds[i]->getName();
            }
        }
    }

    ss << "\n";

    // 3. Print instructions with indentation
    for (auto* inst : block->getInstructions()) {
        ss << indent(1);
        ss << printInstruction(inst);

        if (showUses_) {
            ss << "  ; uses=" << inst->getNumUses();
        }

        ss << "\n";
    }

    return ss.str();
}

std::string IRPrinter::printInstruction(const Instruction* inst) {
    if (!inst) return "<null instruction>";

    // Special handling for different instruction types

    // Phi nodes
    if (auto* phi = dynamic_cast<const PhiNode*>(inst)) {
        return printPhi(phi);
    }

    // Call instructions
    if (auto* call = dynamic_cast<const CallInst*>(inst)) {
        return printCall(call);
    }

    // Return instruction
    if (auto* ret = dynamic_cast<const ReturnInst*>(inst)) {
        return printReturn(ret);
    }

    // Branch instructions
    if (inst->getOpcode() == Instruction::Opcode::Br ||
        inst->getOpcode() == Instruction::Opcode::CondBr ||
        inst->getOpcode() == Instruction::Opcode::Switch) {
        return printBranch(inst);
    }

    std::stringstream ss;

    // Store instruction (no result value)
    if (auto* store = dynamic_cast<const StoreInst*>(inst)) {
        ss << "store ";
        if (showTypes_) {
            ss << printType(store->getValue()->getType()) << " ";
        }
        ss << printValue(store->getValue());
        ss << ", " << printValue(store->getPointer());
        return ss.str();
    }

    // For most instructions: %result = opcode type operands

    // Print result name (unless it's void)
    if (inst->getType()->kind() != IRType::Kind::Void) {
        ss << getValueName(inst) << " = ";
    }

    // Print opcode name
    ss << Instruction::getOpcodeName(inst->getOpcode());

    // Binary operations
    if (auto* binOp = dynamic_cast<const BinaryOperator*>(inst)) {
        if (showTypes_) {
            ss << " " << printType(binOp->getType());
        }
        ss << " " << printValue(binOp->getLHS());
        ss << ", " << printValue(binOp->getRHS());
        return ss.str();
    }

    // Unary operations
    if (auto* unOp = dynamic_cast<const UnaryOperator*>(inst)) {
        if (showTypes_) {
            ss << " " << printType(unOp->getType());
        }
        ss << " " << printValue(unOp->getOperand());
        return ss.str();
    }

    // Comparison operations
    if (auto* cmp = dynamic_cast<const CmpInst*>(inst)) {
        if (showTypes_) {
            // Show operand type, not result type (result is always bool)
            ss << " " << printType(cmp->getLHS()->getType());
        }
        ss << " " << printValue(cmp->getLHS());
        ss << ", " << printValue(cmp->getRHS());
        return ss.str();
    }

    // Alloca
    if (auto* alloca = dynamic_cast<const AllocaInst*>(inst)) {
        ss << " " << printType(alloca->getAllocatedType());
        return ss.str();
    }

    // Load
    if (auto* load = dynamic_cast<const LoadInst*>(inst)) {
        if (showTypes_) {
            ss << " " << printType(load->getType()) << ", ";
        }
        ss << printValue(load->getPointer());
        return ss.str();
    }

    // GCAlloc
    if (auto* gcAlloc = dynamic_cast<const GCAllocInst*>(inst)) {
        ss << " " << printType(gcAlloc->getAllocatedType());
        return ss.str();
    }

    // Array operations
    if (auto* arrayNew = dynamic_cast<const ArrayNewInst*>(inst)) {
        ss << " " << printType(arrayNew->getElementType());
        ss << ", " << printValue(arrayNew->getSize());
        return ss.str();
    }

    if (auto* arrayGet = dynamic_cast<const ArrayGetInst*>(inst)) {
        if (showTypes_) {
            ss << " " << printType(arrayGet->getType()) << ", ";
        }
        ss << printValue(arrayGet->getArray());
        ss << "[" << printValue(arrayGet->getIndex()) << "]";
        return ss.str();
    }

    if (auto* arraySet = dynamic_cast<const ArraySetInst*>(inst)) {
        if (showTypes_) {
            ss << " " << printType(arraySet->getValue()->getType()) << " ";
        }
        ss << printValue(arraySet->getValue());
        ss << ", " << printValue(arraySet->getArray());
        ss << "[" << printValue(arraySet->getIndex()) << "]";
        return ss.str();
    }

    if (auto* arrayLen = dynamic_cast<const ArrayLenInst*>(inst)) {
        ss << " " << printValue(arrayLen->getArray());
        return ss.str();
    }

    if (auto* arraySlice = dynamic_cast<const ArraySliceInst*>(inst)) {
        ss << " " << printValue(arraySlice->getArray());
        ss << "[" << printValue(arraySlice->getStart());
        ss << ":" << printValue(arraySlice->getEnd()) << "]";
        return ss.str();
    }

    // Cast
    if (auto* cast = dynamic_cast<const CastInst*>(inst)) {
        ss << " " << printValue(cast->getValue());
        ss << " to " << printType(cast->getDestType());
        return ss.str();
    }

    // Option operations
    if (auto* optWrap = dynamic_cast<const OptionWrapInst*>(inst)) {
        ss << " " << printValue(optWrap->getValue());
        return ss.str();
    }

    if (auto* optUnwrap = dynamic_cast<const OptionUnwrapInst*>(inst)) {
        ss << " " << printValue(optUnwrap->getOption());
        return ss.str();
    }

    if (auto* optCheck = dynamic_cast<const OptionCheckInst*>(inst)) {
        ss << " " << printValue(optCheck->getOption());
        return ss.str();
    }

    // Fallback for unknown instructions
    ss << " <unknown instruction type>";
    return ss.str();
}

std::string IRPrinter::printValue(const Value* value) {
    if (!value) return "<null>";

    // Constants print their literal values
    if (auto* ci = dynamic_cast<const ConstantInt*>(value)) {
        return std::to_string(ci->getValue());
    }
    if (auto* cf = dynamic_cast<const ConstantFloat*>(value)) {
        return std::to_string(cf->getValue());
    }
    if (auto* cb = dynamic_cast<const ConstantBool*>(value)) {
        return cb->getValue() ? "true" : "false";
    }
    if (auto* cs = dynamic_cast<const ConstantString*>(value)) {
        return "\"" + cs->getValue() + "\"";
    }
    if (dynamic_cast<const ConstantNone*>(value)) {
        return "none";
    }
    if (dynamic_cast<const UndefValue*>(value)) {
        return "undef";
    }

    // Globals start with @
    if (auto* global = dynamic_cast<const GlobalVariable*>(value)) {
        return "@" + global->getName();
    }

    // Everything else (instructions, arguments) gets SSA name
    return getValueName(value);
}

std::string IRPrinter::printType(const std::shared_ptr<IRType>& type) {
    if (!type) return "<?>";
    return type->toString();
}

// ========================================================================
// Control
// ========================================================================

void IRPrinter::reset() {
    valueNumbers_.clear();
    nextValueNumber_ = 0;
}

// ========================================================================
// Helper Methods
// ========================================================================

std::string IRPrinter::getValueName(const Value* value) {
    if (!value) return "<null>";

    // 1. If value has a name, use it
    if (!value->getName().empty()) {
        return "%" + value->getName();
    }

    // 2. Check if we already assigned a number
    auto it = valueNumbers_.find(value);
    if (it != valueNumbers_.end()) {
        return "%" + std::to_string(it->second);
    }

    // 3. Assign new number
    int num = nextValueNumber_++;
    valueNumbers_[value] = num;
    return "%" + std::to_string(num);
}

std::string IRPrinter::printGlobal(const GlobalVariable* global) {
    if (!global) return "";

    std::stringstream ss;

    ss << "@" << global->getName() << ": ";
    ss << printType(global->getType());

    if (global->getInitializer()) {
        ss << " = " << printValue(global->getInitializer());
    }

    if (global->isConstant()) {
        ss << " const";
    }

    return ss.str();
}

std::string IRPrinter::printFunctionSignature(const Function* func) {
    if (!func) return "";

    std::stringstream ss;

    ss << "function @" << func->getName() << "(";

    // Parameters
    for (unsigned i = 0; i < func->getNumParams(); i++) {
        if (i > 0) ss << ", ";

        auto* param = func->getParam(i);
        ss << param->getName() << ": " << printType(param->getType());
    }

    ss << ") -> " << printType(func->getReturnType());

    return ss.str();
}

std::string IRPrinter::printOperands(const Instruction* inst) {
    if (!inst) return "";

    std::stringstream ss;

    for (unsigned i = 0; i < inst->getNumOperands(); i++) {
        if (i > 0) ss << ", ";

        Value* operand = inst->getOperand(i);
        if (showTypes_) {
            ss << printType(operand->getType()) << " ";
        }
        ss << printValue(operand);
    }

    return ss.str();
}

std::string IRPrinter::printPhi(const PhiNode* phi) {
    if (!phi) return "";

    std::stringstream ss;

    ss << getValueName(phi) << " = phi ";

    if (showTypes_) {
        ss << printType(phi->getType()) << " ";
    }

    // Print incoming values: [value, block], [value, block], ...
    for (unsigned i = 0; i < phi->getNumIncomingValues(); i++) {
        if (i > 0) ss << ", ";

        ss << "[" << printValue(phi->getIncomingValue(i));
        ss << ", %" << phi->getIncomingBlock(i)->getName() << "]";
    }

    return ss.str();
}

std::string IRPrinter::printCall(const CallInst* call) {
    if (!call) return "";

    std::stringstream ss;

    // Only print result if function returns non-void
    if (call->getType()->kind() != IRType::Kind::Void) {
        ss << getValueName(call) << " = ";
    }

    ss << "call @" << call->getCallee()->getName() << "(";

    // Arguments
    for (unsigned i = 0; i < call->getNumArguments(); i++) {
        if (i > 0) ss << ", ";

        Value* arg = call->getArgument(i);
        if (showTypes_) {
            ss << printType(arg->getType()) << " ";
        }
        ss << printValue(arg);
    }

    ss << ")";
    return ss.str();
}

std::string IRPrinter::printBranch(const Instruction* inst) {
    if (!inst) return "";

    std::stringstream ss;

    // Unconditional branch
    if (auto* br = dynamic_cast<const BranchInst*>(inst)) {
        ss << "br label %" << br->getDestination()->getName();
        return ss.str();
    }

    // Conditional branch
    if (auto* condBr = dynamic_cast<const CondBranchInst*>(inst)) {
        ss << "br ";
        if (showTypes_) {
            ss << "bool ";
        }
        ss << printValue(condBr->getCondition());
        ss << ", label %" << condBr->getTrueDest()->getName();
        ss << ", label %" << condBr->getFalseDest()->getName();
        return ss.str();
    }

    // Switch
    if (auto* sw = dynamic_cast<const SwitchInst*>(inst)) {
        ss << "switch ";
        if (showTypes_) {
            ss << printType(sw->getValue()->getType()) << " ";
        }
        ss << printValue(sw->getValue());
        ss << ", label %" << sw->getDefaultDest()->getName();
        ss << " [";

        auto& cases = sw->getCases();
        for (size_t i = 0; i < cases.size(); i++) {
            if (i > 0) ss << ", ";
            ss << printValue(cases[i].value);
            ss << ": %" << cases[i].dest->getName();
        }

        ss << "]";
        return ss.str();
    }

    return "<unknown branch>";
}

std::string IRPrinter::printReturn(const ReturnInst* ret) {
    if (!ret) return "";

    std::stringstream ss;
    ss << "ret";

    if (ret->hasReturnValue()) {
        Value* retVal = ret->getReturnValue();
        if (showTypes_) {
            ss << " " << printType(retVal->getType());
        }
        ss << " " << printValue(retVal);
    } else {
        ss << " void";
    }

    return ss.str();
}

std::string IRPrinter::indent(int level) const {
    return std::string(level * 2, ' ');  // 2 spaces per level
}

} // namespace volta::ir
