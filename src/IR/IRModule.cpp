#include "IR/IRModule.hpp"
#include <iostream>

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
    //TODO: Implement more advanced checks (type consistency, value defined before use).

    bool isValid = true;

    for (const auto& func : functions_) {
        // Skip foreign functions - they have no body
        if (func->isForeign()) {
            continue;
        }

        if (func->basicBlocks().empty()) {
            errors << "Function " << func->name() << " has no basic blocks\n";
            isValid = false;
        }

        for (const auto& block : func->basicBlocks()) {
            if (!block->hasTerminator()) {
                errors << "Block " << block->name() << " in function " << func->name()
                       << " has no terminator\n";
                isValid = false;
            }
        }
    }
    return isValid;
}

} // namespace volta::ir
