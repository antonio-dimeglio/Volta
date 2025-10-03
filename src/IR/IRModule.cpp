#include "IR/IRModule.hpp"
#include <algorithm>

namespace volta::ir {

IRModule::IRModule(const std::string& name) : name_(name) {
}

IRModule::~IRModule() = default;

// ============================================================================
// Global Variables
// ============================================================================

void IRModule::addGlobalVariable(std::unique_ptr<GlobalVariable> global) {
    // TODO: Add the global variable to globals_ vector.
    // Also add it to globalMap_ for fast lookup by name.
    // Format: globalMap_[global->getName()] = global.get()
}

GlobalVariable* IRModule::getGlobalVariable(const std::string& name) const {
    // TODO: Look up and return the global variable by name.
    // Return nullptr if not found.
    // Hint: Use globalMap_
    return nullptr;
}

// ============================================================================
// Functions
// ============================================================================

Function* IRModule::createFunction(
    const std::string& name,
    std::vector<std::unique_ptr<Parameter>> parameters,
    std::shared_ptr<semantic::Type> returnType
) {
    // TODO: Create a new Function with the given parameters.
    // Add it to the module and return a pointer to it.
    // Set the function's parent to this module.
    return nullptr;
}

void IRModule::addFunction(std::unique_ptr<Function> function) {
    // TODO: Add the function to functions_ vector.
    // Also add it to functionMap_ for fast lookup by name.
    // Set the function's parent to this module.
}

void IRModule::removeFunction(Function* function) {
    // TODO: Remove the function from functions_ and functionMap_.
    // Clear the function's parent.
}

Function* IRModule::getFunction(const std::string& name) const {
    // TODO: Look up and return the function by name.
    // Return nullptr if not found.
    return nullptr;
}

Function* IRModule::getMainFunction() const {
    // TODO: Find and return the function named "main" or "@main".
    // This is the entry point of the program.
    return nullptr;
}

// ============================================================================
// Verification
// ============================================================================

bool IRModule::verify() const {
    // TODO: Verify that the IR is well-formed.
    // Check:
    // 1. All functions have valid CFGs (entry block, terminators, etc.)
    // 2. All instructions have valid operands
    // 3. Type consistency
    // 4. SSA properties (each value defined exactly once, used after definition)
    //
    // For now, return true (we'll implement verification in Phase 5)
    return true;
}

// ============================================================================
// Printing/Debugging
// ============================================================================

std::string IRModule::toString() const {
    // TODO: Return a string representation of the entire module.
    // Format:
    //   ; ModuleID = 'module_name'
    //
    //   @global1 = global type initializer
    //   @global2 = global type initializer
    //   ...
    //
    //   define returnType @function1(...) { ... }
    //   define returnType @function2(...) { ... }
    //   ...
    //
    // Example:
    //   ; ModuleID = 'test.volta'
    //
    //   @counter = global i64 0
    //
    //   define void @increment() {
    //   entry:
    //       %0 = load i64, ptr @counter
    //       %1 = add i64 %0, 1
    //       store i64 %1, ptr @counter
    //       ret void
    //   }
    //
    // Hint: Loop through globals_ and functions_, calling toString() on each
    return "";
}

} // namespace volta::ir
