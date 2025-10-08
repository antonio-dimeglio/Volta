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
    if (auto* phi = dyn_cast<PhiNode>(inst)) {
        return printPhi(phi);
    }

    // Call instructions
    if (auto* call = dyn_cast<CallInst>(inst)) {
        return printCall(call);
    }

    // Return instruction
    if (auto* ret = dyn_cast<ReturnInst>(inst)) {
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
    if (auto* store = dyn_cast<StoreInst>(inst)) {
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
    if (auto* binOp = dyn_cast<BinaryOperator>(inst)) {
        if (showTypes_) {
            ss << " " << printType(binOp->getType());
        }
        ss << " " << printValue(binOp->getLHS());
        ss << ", " << printValue(binOp->getRHS());
        return ss.str();
    }

    // Unary operations
    if (auto* unOp = dyn_cast<UnaryOperator>(inst)) {
        if (showTypes_) {
            ss << " " << printType(unOp->getType());
        }
        ss << " " << printValue(unOp->getOperand());
        return ss.str();
    }

    // Comparison operations
    if (auto* cmp = dyn_cast<CmpInst>(inst)) {
        if (showTypes_) {
            // Show operand type, not result type (result is always bool)
            ss << " " << printType(cmp->getLHS()->getType());
        }
        ss << " " << printValue(cmp->getLHS());
        ss << ", " << printValue(cmp->getRHS());
        return ss.str();
    }

    // Alloca
    if (auto* alloca = dyn_cast<AllocaInst>(inst)) {
        ss << " " << printType(alloca->getAllocatedType());
        return ss.str();
    }

    // Load
    if (auto* load = dyn_cast<LoadInst>(inst)) {
        if (showTypes_) {
            ss << " " << printType(load->getType()) << ", ";
        }
        ss << printValue(load->getPointer());
        return ss.str();
    }

    // GCAlloc
    if (auto* gcAlloc = dyn_cast<GCAllocInst>(inst)) {
        ss << " " << printType(gcAlloc->getAllocatedType());
        return ss.str();
    }

    // Array operations
    if (auto* arrayNew = dyn_cast<ArrayNewInst>(inst)) {
        ss << " " << printType(arrayNew->getElementType());
        ss << ", " << printValue(arrayNew->getSize());
        return ss.str();
    }

    if (auto* arrayGet = dyn_cast<ArrayGetInst>(inst)) {
        if (showTypes_) {
            ss << " " << printType(arrayGet->getType()) << ", ";
        }
        ss << printValue(arrayGet->getArray());
        ss << "[" << printValue(arrayGet->getIndex()) << "]";
        return ss.str();
    }

    if (auto* arraySet = dyn_cast<ArraySetInst>(inst)) {
        if (showTypes_) {
            ss << " " << printType(arraySet->getValue()->getType()) << " ";
        }
        ss << printValue(arraySet->getValue());
        ss << ", " << printValue(arraySet->getArray());
        ss << "[" << printValue(arraySet->getIndex()) << "]";
        return ss.str();
    }

    if (auto* arrayLen = dyn_cast<ArrayLenInst>(inst)) {
        ss << " " << printValue(arrayLen->getArray());
        return ss.str();
    }

    if (auto* arraySlice = dyn_cast<ArraySliceInst>(inst)) {
        ss << " " << printValue(arraySlice->getArray());
        ss << "[" << printValue(arraySlice->getStart());
        ss << ":" << printValue(arraySlice->getEnd()) << "]";
        return ss.str();
    }

    // Struct operations
    if (auto* extractVal = dyn_cast<ExtractValueInst>(inst)) {
        if (showTypes_) {
            ss << " " << printType(extractVal->getType()) << " ";
        }
        ss << printValue(extractVal->getStruct());
        ss << ", " << extractVal->getFieldIndex();
        return ss.str();
    }

    if (auto* insertVal = dyn_cast<InsertValueInst>(inst)) {
        if (showTypes_) {
            ss << " " << printType(insertVal->getType()) << " ";
        }
        ss << printValue(insertVal->getStruct());
        ss << ", " << printValue(insertVal->getNewValue());
        ss << ", " << insertVal->getFieldIndex();
        return ss.str();
    }

    if (auto* createEnum = dyn_cast<CreateEnumInst>(inst)) {
        if (showTypes_) {
            ss << " " << printType(createEnum->getType()) << " ";
        }
        ss << "variant=" << createEnum->getVariantTag() << ", [";
        for (unsigned i = 0; i < createEnum->getNumFields(); i++) {
            if (i > 0) ss << ", ";
            ss << printValue(createEnum->getFieldValue(i));
        }
        ss << "]";
        return ss.str();
    }

    if (auto* getTag = dyn_cast<GetEnumTagInst>(inst)) {
        ss << " " << printValue(getTag->getEnum());
        return ss.str();
    }

    if (auto* extractData = dyn_cast<ExtractEnumDataInst>(inst)) {
        if (showTypes_) {
            ss << " " << printType(extractData->getType()) << " ";
        }
        ss << printValue(extractData->getEnum());
        ss << ", field=" << extractData->getFieldIndex();
        return ss.str();
    }

    // Cast
    if (auto* cast = dyn_cast<CastInst>(inst)) {
        ss << " " << printValue(cast->getValue());
        ss << " to " << printType(cast->getDestType());
        return ss.str();
    }

    // Fallback for unknown instructions
    ss << " <unknown instruction type>";
    return ss.str();
}

std::string IRPrinter::printValue(const Value* value) {
    if (!value) return "<null>";

    // Constants print their literal values
    if (auto* ci = dyn_cast<ConstantInt>(value)) {
        return std::to_string(ci->getValue());
    }
    if (auto* cf = dyn_cast<ConstantFloat>(value)) {
        return std::to_string(cf->getValue());
    }
    if (auto* cb = dyn_cast<ConstantBool>(value)) {
        return cb->getValue() ? "true" : "false";
    }
    if (auto* cs = dyn_cast<ConstantString>(value)) {
        return "\"" + cs->getValue() + "\"";
    }
    if (dyn_cast<UndefValue>(value)) {
        return "undef";
    }

    // Globals start with @
    if (auto* global = dyn_cast<GlobalVariable>(value)) {
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

    ss << "define @" << func->getName() << "(";

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
    if (auto* br = dyn_cast<BranchInst>(inst)) {
        ss << "br label %" << br->getDestination()->getName();
        return ss.str();
    }

    // Conditional branch
    if (auto* condBr = dyn_cast<CondBranchInst>(inst)) {
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
    if (auto* sw = dyn_cast<SwitchInst>(inst)) {
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
