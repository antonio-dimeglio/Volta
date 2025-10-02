#include "IR/IRPrinter.hpp"
#include <iomanip>

namespace volta::ir {

// ============================================================================
// Module Printing
// ============================================================================

void IRPrinter::print(const IRModule& module) {
    out_ << "; Module: " << module.name() << "\n\n";
    for (const auto& global : module.globals()) {
        printGlobal(*global);
    }
    for (const auto& func : module.functions()) {
        printFunction(*func);
        out_ << "\n";  
    }
}

// ============================================================================
// Function Printing
// ============================================================================

void IRPrinter::printFunction(const Function& function) {
    out_ << "function @" << function.name() << "(";
    
    for (size_t i = 0; i < function.parameters().size(); i++) {
        const auto& param = function.parameters()[i];
        if (i > 0) out_ << ", ";
        out_ << param->name() << ": " << typeToString(param->type().get());
    }
    
    out_ << ") -> " << typeToString(function.type()->returnType().get()) << ":\n";

    for (const auto& bb : function.basicBlocks()) {
        printBasicBlock(*bb);
    }

}

// ============================================================================
// Basic Block Printing
// ============================================================================

void IRPrinter::printBasicBlock(const BasicBlock& block) {
    out_ << "  " << block.name() << ":\n";
    indentLevel_++;

    for(auto& instr : block.instructions()) {
        printInstruction(*instr);
    }

    indentLevel_--;
}

// ============================================================================
// Instruction Printing
// ============================================================================

void IRPrinter::printInstruction(const Instruction& inst) {
    indent();

    if (auto* binInst = dynamic_cast<const BinaryInst*>(&inst)) {
        out_ << inst.name() << " = " << Instruction::opcodeName(inst.opcode())
             << " " << valueToString(binInst->left())
             << ", " << valueToString(binInst->right()) << "\n";
    }
    else if (auto* unaryInst = dynamic_cast<const UnaryInst*>(&inst)) {
        out_ << inst.name() << " = " << Instruction::opcodeName(inst.opcode())
             << " " << valueToString(unaryInst->operand()) << "\n";
    }
    else if (auto* allocInst = dynamic_cast<const AllocInst*>(&inst)) {
        out_ << inst.name() << " = alloc " << typeToString(allocInst->allocatedType().get()) << "\n";
    }
    else if (auto* loadInst = dynamic_cast<const LoadInst*>(&inst)) {
        out_ << inst.name() << " = load [" << valueToString(loadInst->address()) << "]\n";
    }
    else if (auto* storeInst = dynamic_cast<const StoreInst*>(&inst)) {
        out_ << "store " << valueToString(storeInst->value())
             << ", [" << valueToString(storeInst->address()) << "]\n";
    }
    else if (auto* getFieldInst = dynamic_cast<const GetFieldInst*>(&inst)) {
        out_ << inst.name() << " = get_field " << valueToString(getFieldInst->object())
             << ", " << getFieldInst->fieldIndex() << "\n";
    }
    else if (auto* setFieldInst = dynamic_cast<const SetFieldInst*>(&inst)) {
        out_ << "set_field " << valueToString(setFieldInst->object())
             << ", " << setFieldInst->fieldIndex()
             << ", " << valueToString(setFieldInst->value()) << "\n";
    }
    else if (auto* getElemInst = dynamic_cast<const GetElementInst*>(&inst)) {
        out_ << inst.name() << " = get_element " << valueToString(getElemInst->array())
             << ", " << valueToString(getElemInst->index()) << "\n";
    }
    else if (auto* setElemInst = dynamic_cast<const SetElementInst*>(&inst)) {
        out_ << "set_element " << valueToString(setElemInst->array())
             << ", " << valueToString(setElemInst->index())
             << ", " << valueToString(setElemInst->value()) << "\n";
    }
    else if (auto* newArrayInst = dynamic_cast<const NewArrayInst*>(&inst)) {
        out_ << inst.name() << " = new_array [";
        const auto& elems = newArrayInst->elements();
        for (size_t i = 0; i < elems.size(); i++) {
            if (i > 0) out_ << ", ";
            out_ << valueToString(elems[i]);
        }
        out_ << "]\n";
    }
    else if (auto* brInst = dynamic_cast<const BranchInst*>(&inst)) {
        out_ << "br " << brInst->target()->name() << "\n";
    }
    else if (auto* brIfInst = dynamic_cast<const BranchIfInst*>(&inst)) {
        out_ << "br_if " << valueToString(brIfInst->condition())
             << ", " << brIfInst->thenBlock()->name()
             << ", " << brIfInst->elseBlock()->name() << "\n";
    }
    else if (auto* retInst = dynamic_cast<const ReturnInst*>(&inst)) {
        if (retInst->hasReturnValue()) {
            out_ << "ret " << valueToString(retInst->returnValue()) << "\n";
        } else {
            out_ << "ret\n";
        }
    }
    else if (auto* callInst = dynamic_cast<const CallInst*>(&inst)) {
        if (!inst.name().empty()) {
            out_ << inst.name() << " = ";
        }
        out_ << "call @" << callInst->callee()->name();
        const auto& args = callInst->arguments();
        if (!args.empty()) {
            out_ << ", ";
            for (size_t i = 0; i < args.size(); i++) {
                if (i > 0) out_ << ", ";
                out_ << valueToString(args[i]);
            }
        }
        out_ << "\n";
    }
    else if (auto* callForeignInst = dynamic_cast<const CallForeignInst*>(&inst)) {
        if (!inst.name().empty()) {
            out_ << inst.name() << " = ";
        }
        out_ << "call_foreign @" << callForeignInst->foreignName();
        const auto& args = callForeignInst->arguments();
        if (!args.empty()) {
            out_ << ", ";
            for (size_t i = 0; i < args.size(); i++) {
                if (i > 0) out_ << ", ";
                out_ << valueToString(args[i]);
            }
        }
        out_ << "\n";
    }
    else if (auto* phiInst = dynamic_cast<const PhiInst*>(&inst)) {
        out_ << inst.name() << " = phi ";
        const auto& incoming = phiInst->incomingValues();
        for (size_t i = 0; i < incoming.size(); i++) {
            if (i > 0) out_ << ", ";
            out_ << "[" << valueToString(incoming[i].first)
                 << ", " << incoming[i].second->name() << "]";
        }
        out_ << "\n";
    }
    else {
        out_ << "<unknown instruction>\n";
    }
}

// ============================================================================
// Global Variable Printing
// ============================================================================

void IRPrinter::printGlobal(const GlobalVariable& global) {
    out_ << global.name() << ": " << typeToString(global.type().get());
    if (global.initializer()) {
        out_ << " = " << global.initializer()->toString();
    }
    out_ << "\n";
}

// ============================================================================
// Helper Methods
// ============================================================================

std::string IRPrinter::valueToString(const Value* value) {
    if (value == nullptr) {
        return "null";
    }

    if(value->kind() == ir::Value::Kind::Constant) {
        return value->toString();
    }

    return value->name();
}

std::string IRPrinter::typeToString(const semantic::Type* type) {
    if (type == nullptr) {
        return "null";
    }
    return type->toString();
}

void IRPrinter::indent() {
    for (int i = 0; i < indentLevel_; i++) {
        out_ << "  ";
    }
}

} // namespace volta::ir
