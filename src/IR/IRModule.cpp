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
    std::vector<std::unique_ptr<Parameter>> params;
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
