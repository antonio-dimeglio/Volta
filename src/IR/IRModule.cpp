#include "IR/IRModule.hpp"
#include <iostream>
#include <unordered_set>

namespace volta::ir {

// ============================================================================
// Function Management
// ============================================================================

Function* IRModule::addFunction(std::unique_ptr<Function> function) {
    Function* ptr = function.get();
    functionMap_[function->name()] = ptr; 
    functions_.push_back(std::move(function));
    return ptr;
}

Function* IRModule::getFunction(const std::string& name) const {
    auto it = functionMap_.find(name);

    if (it != functionMap_.end()) return it->second;
    return nullptr; 
}

// ============================================================================
// Global Variable Management
// ============================================================================

GlobalVariable* IRModule::addGlobal(std::unique_ptr<GlobalVariable> global) {
    GlobalVariable* ptr = global.get();
    globalMap_[global.get()->name()] = ptr; 
    globals_.push_back(std::move(global));
    return ptr;
}

GlobalVariable* IRModule::getGlobal(const std::string& name) const {
    auto it = globalMap_.find(name);
    if (it != globalMap_.end()) {
        return it->second;
    }

    return nullptr;
}

// ============================================================================
// Value Arena Management (Memory Safety)
// ============================================================================

Constant* IRModule::createConstant(std::shared_ptr<semantic::Type> type,
                                   const Constant::ConstantValue& value) {
    // Deduplicate constants by value
    if (std::holds_alternative<int64_t>(value)) {
        int64_t intVal = std::get<int64_t>(value);
        auto it = intConstantMap_.find(intVal);
        if (it != intConstantMap_.end()) {
            return it->second;
        }
        auto constant = std::make_unique<Constant>(type, value);
        Constant* ptr = constant.get();
        intConstantMap_[intVal] = ptr;
        constants_.push_back(std::move(constant));
        return ptr;
    }
    else if (std::holds_alternative<double>(value)) {
        double floatVal = std::get<double>(value);
        auto it = floatConstantMap_.find(floatVal);
        if (it != floatConstantMap_.end()) {
            return it->second;
        }
        auto constant = std::make_unique<Constant>(type, value);
        Constant* ptr = constant.get();
        floatConstantMap_[floatVal] = ptr;
        constants_.push_back(std::move(constant));
        return ptr;
    }
    else if (std::holds_alternative<bool>(value)) {
        bool boolVal = std::get<bool>(value);
        auto it = boolConstantMap_.find(boolVal);
        if (it != boolConstantMap_.end()) {
            return it->second;
        }
        auto constant = std::make_unique<Constant>(type, value);
        Constant* ptr = constant.get();
        boolConstantMap_[boolVal] = ptr;
        constants_.push_back(std::move(constant));
        return ptr;
    }
    else if (std::holds_alternative<std::string>(value)) {
        const std::string& strVal = std::get<std::string>(value);
        auto it = stringConstantMap_.find(strVal);
        if (it != stringConstantMap_.end()) {
            return it->second;
        }
        auto constant = std::make_unique<Constant>(type, value);
        Constant* ptr = constant.get();
        stringConstantMap_[strVal] = ptr;
        constants_.push_back(std::move(constant));
        return ptr;
    }
    else if (std::holds_alternative<std::monostate>(value)) {
        // None/void constant - single instance
        if (noneConstant_ != nullptr) {
            return noneConstant_;
        }
        auto constant = std::make_unique<Constant>(type, value);
        noneConstant_ = constant.get();
        constants_.push_back(std::move(constant));
        return noneConstant_;
    }

    // Should never reach here
    return nullptr;
}

Parameter* IRModule::createParameter(std::shared_ptr<semantic::Type> type,
                                    const std::string& name,
                                    size_t index) {
    auto param = std::make_unique<Parameter>(type, name, index);
    Parameter* ptr = param.get();
    parameters_.push_back(std::move(param));
    return ptr;
}

BasicBlock* IRModule::createBasicBlock(const std::string& name, Function* parent) {
    auto block = std::make_unique<BasicBlock>(name, parent);
    BasicBlock* ptr = block.get();
    basicBlockArena_.push_back(std::move(block));
    return ptr;
}

// ============================================================================
// String Literal Management
// ============================================================================

size_t IRModule::addStringLiteral(const std::string& str) {
    auto it = stringLiteralMap_.find(str);
    if (it != stringLiteralMap_.end()) return it->second;

    size_t index = stringLiterals_.size();
    stringLiterals_.push_back(str);
    stringLiteralMap_[str] = index;
    return index;
}

const std::string& IRModule::getStringLiteral(size_t index) const {
    if (index < stringLiterals_.size()) {
        return stringLiterals_[index];
    } else {
        throw std::out_of_range(
            "Tried to access non-existatnt string literal at idx "
            + std::to_string(index) +
            " while string literal array is of length " + std::to_string(stringLiterals_.size()));
    }
}

// ============================================================================
// Foreign Function Interface
// ============================================================================

Function* IRModule::declareForeignFunction(
    const std::string& name,
    std::shared_ptr<semantic::FunctionType> type) {

    // Foreign functions have no parameters in IR (they're managed externally)
    std::vector<Parameter*> params;
    auto func = std::make_unique<Function>(name, type, std::move(params));
    func->setForeign(true);
    return addFunction(std::move(func));
}

// ============================================================================
// Validation
// ============================================================================

bool IRModule::verify(std::ostream& errors) const {
    bool isValid = true;

    for (const auto& func : functions_) {
        // Skip foreign functions - they have no body
        if (func->isForeign()) {
            continue;
        }

        if (!verifyFunction(func.get(), errors)) {
            isValid = false;
        }
    }

    return isValid;
}

// ============================================================================
// Verification Helpers
// ============================================================================

std::unordered_set<const Value*> IRModule::buildDefinedValuesSet(const Function* func) const {
    std::unordered_set<const Value*> definedValues;

    // Parameters are defined at function entry
    for (const auto& param : func->parameters()) {
        definedValues.insert(param);
    }

    // Globals are always defined
    for (const auto& global : globals_) {
        definedValues.insert(global.get());
    }

    // All constants are always defined
    for (const auto& constant : constants_) {
        definedValues.insert(constant.get());
    }

    return definedValues;
}

bool IRModule::verifyFunction(const Function* func, std::ostream& errors) const {
    bool isValid = true;

    // Check function has at least one basic block
    if (func->basicBlocks().empty()) {
        errors << "ERROR: Function '" << func->name() << "' has no basic blocks\n";
        return false;
    }

    // Build set of all valid blocks in this function for branch validation
    std::unordered_set<const BasicBlock*> validBlocks;
    for (const auto& block : func->basicBlocks()) {
        validBlocks.insert(block);
    }

    // Build set of all defined values
    auto definedValues = buildDefinedValuesSet(func);

    // Check each basic block
    for (const auto& block : func->basicBlocks()) {
        // Check block has terminator
        if (!block->hasTerminator()) {
            errors << "ERROR: Block '" << block->name() << "' in function '"
                   << func->name() << "' has no terminator\n";
            isValid = false;
        }

        // Check block has at least one instruction
        if (block->instructions().empty()) {
            errors << "ERROR: Block '" << block->name() << "' in function '"
                   << func->name() << "' has no instructions\n";
            isValid = false;
            continue;
        }

        // Check each instruction
        for (const auto* inst : block->instructions()) {
            // Add this instruction to defined values (for subsequent uses)
            definedValues.insert(inst);

            // Check operands are defined before use
            for (const auto* operand : inst->operands()) {
                if (definedValues.find(operand) == definedValues.end()) {
                    errors << "ERROR: Instruction '" << inst->name()
                           << "' uses undefined value '" << operand->name()
                           << "' in block '" << block->name()
                           << "' of function '" << func->name() << "'\n";
                    isValid = false;
                }
            }

            // Verify specific instruction types
            if (!verifyBranchTargets(inst, block, func, validBlocks, errors)) {
                isValid = false;
            }

            if (auto* callInst = dynamic_cast<const CallInst*>(inst)) {
                if (!verifyCall(callInst, block, func, errors)) {
                    isValid = false;
                }
            }

            if (auto* retInst = dynamic_cast<const ReturnInst*>(inst)) {
                if (!verifyReturn(retInst, block, func, errors)) {
                    isValid = false;
                }
            }

            if (!verifyArrayOperation(inst, block, func, errors)) {
                isValid = false;
            }

            if (!verifyStructOperation(inst, block, func, errors)) {
                isValid = false;
            }
        }

        // Check that terminators are only at the end
        for (size_t i = 0; i < block->instructions().size() - 1; ++i) {
            const auto* inst = block->instructions()[i];
            if (inst->opcode() == Instruction::Opcode::Br ||
                inst->opcode() == Instruction::Opcode::BrIf ||
                inst->opcode() == Instruction::Opcode::Ret) {
                errors << "ERROR: Terminator instruction in middle of block '"
                       << block->name() << "' in function '" << func->name()
                       << "' at position " << i << "\n";
                isValid = false;
            }
        }
    }

    return isValid;
}

bool IRModule::verifyBranchTargets(const Instruction* inst, const BasicBlock* block,
                                   const Function* func,
                                   const std::unordered_set<const BasicBlock*>& validBlocks,
                                   std::ostream& errors) const {
    bool isValid = true;

    if (auto* brInst = dynamic_cast<const BranchInst*>(inst)) {
        if (validBlocks.find(brInst->target()) == validBlocks.end()) {
            errors << "ERROR: Branch instruction in block '" << block->name()
                   << "' of function '" << func->name()
                   << "' targets invalid block\n";
            isValid = false;
        }
    }
    else if (auto* brIfInst = dynamic_cast<const BranchIfInst*>(inst)) {
        if (validBlocks.find(brIfInst->thenBlock()) == validBlocks.end()) {
            errors << "ERROR: Conditional branch in block '" << block->name()
                   << "' of function '" << func->name()
                   << "' has invalid 'then' target\n";
            isValid = false;
        }
        if (validBlocks.find(brIfInst->elseBlock()) == validBlocks.end()) {
            errors << "ERROR: Conditional branch in block '" << block->name()
                   << "' of function '" << func->name()
                   << "' has invalid 'else' target\n";
            isValid = false;
        }

        // Check condition is boolean
        if (brIfInst->condition() && brIfInst->condition()->type()) {
            auto* primType = dynamic_cast<const semantic::PrimitiveType*>(
                brIfInst->condition()->type().get());
            if (!primType || primType->primitiveKind() !=
                semantic::PrimitiveType::PrimitiveKind::Bool) {
                errors << "ERROR: Conditional branch condition in block '"
                       << block->name() << "' of function '" << func->name()
                       << "' must be boolean type\n";
                isValid = false;
            }
        }
    }

    return isValid;
}

bool IRModule::verifyCall(const CallInst* callInst, const BasicBlock* block,
                         const Function* func, std::ostream& errors) const {
    bool isValid = true;

    if (getFunction(callInst->callee()->name()) == nullptr) {
        errors << "ERROR: Call instruction in block '" << block->name()
               << "' of function '" << func->name()
               << "' calls undefined function '"
               << callInst->callee()->name() << "'\n";
        isValid = false;
    }

    // Check argument count matches
    const auto& funcType = callInst->callee()->type();
    if (funcType->paramTypes().size() != callInst->arguments().size()) {
        errors << "ERROR: Call to '" << callInst->callee()->name()
               << "' in block '" << block->name()
               << "' has " << callInst->arguments().size()
               << " arguments but function expects "
               << funcType->paramTypes().size() << "\n";
        isValid = false;
    }

    return isValid;
}

bool IRModule::verifyReturn(const ReturnInst* retInst, const BasicBlock* block,
                           const Function* func, std::ostream& errors) const {
    bool isValid = true;
    const auto& funcRetType = func->type()->returnType();

    if (retInst->hasReturnValue()) {
        // Function must not be void
        auto* voidType = dynamic_cast<const semantic::PrimitiveType*>(funcRetType.get());
        if (voidType && voidType->primitiveKind() ==
            semantic::PrimitiveType::PrimitiveKind::Void) {
            errors << "ERROR: Return with value in void function '"
                   << func->name() << "' in block '" << block->name() << "'\n";
            isValid = false;
        }
    } else {
        // Function must be void
        auto* voidType = dynamic_cast<const semantic::PrimitiveType*>(funcRetType.get());
        if (!voidType || voidType->primitiveKind() !=
            semantic::PrimitiveType::PrimitiveKind::Void) {
            errors << "ERROR: Return without value in non-void function '"
                   << func->name() << "' in block '" << block->name() << "'\n";
            isValid = false;
        }
    }

    return isValid;
}

bool IRModule::verifyArrayOperation(const Instruction* inst, const BasicBlock* block,
                                   const Function* func, std::ostream& errors) const {
    bool isValid = true;

    if (auto* getElemInst = dynamic_cast<const GetElementInst*>(inst)) {
        auto* arrayType = dynamic_cast<const semantic::ArrayType*>(
            getElemInst->array()->type().get());
        if (!arrayType) {
            errors << "ERROR: GetElement in block '" << block->name()
                   << "' of function '" << func->name()
                   << "' operates on non-array type\n";
            isValid = false;
        }
    }
    else if (auto* setElemInst = dynamic_cast<const SetElementInst*>(inst)) {
        auto* arrayType = dynamic_cast<const semantic::ArrayType*>(
            setElemInst->array()->type().get());
        if (!arrayType) {
            errors << "ERROR: SetElement in block '" << block->name()
                   << "' of function '" << func->name()
                   << "' operates on non-array type\n";
            isValid = false;
        }
    }

    return isValid;
}

bool IRModule::verifyStructOperation(const Instruction* inst, const BasicBlock* block,
                                    const Function* func, std::ostream& errors) const {
    bool isValid = true;

    if (auto* getFieldInst = dynamic_cast<const GetFieldInst*>(inst)) {
        auto* structType = dynamic_cast<const semantic::StructType*>(
            getFieldInst->object()->type().get());
        if (!structType) {
            errors << "ERROR: GetField in block '" << block->name()
                   << "' of function '" << func->name()
                   << "' operates on non-struct type\n";
            isValid = false;
        } else if (getFieldInst->fieldIndex() >= structType->fields().size()) {
            errors << "ERROR: GetField in block '" << block->name()
                   << "' of function '" << func->name()
                   << "' accesses out-of-bounds field index "
                   << getFieldInst->fieldIndex() << "\n";
            isValid = false;
        }
    }
    else if (auto* setFieldInst = dynamic_cast<const SetFieldInst*>(inst)) {
        auto* structType = dynamic_cast<const semantic::StructType*>(
            setFieldInst->object()->type().get());
        if (!structType) {
            errors << "ERROR: SetField in block '" << block->name()
                   << "' of function '" << func->name()
                   << "' operates on non-struct type\n";
            isValid = false;
        } else if (setFieldInst->fieldIndex() >= structType->fields().size()) {
            errors << "ERROR: SetField in block '" << block->name()
                   << "' of function '" << func->name()
                   << "' accesses out-of-bounds field index "
                   << setFieldInst->fieldIndex() << "\n";
            isValid = false;
        }
    }

    return isValid;
}

} // namespace volta::ir
