#pragma once

#include "Function.hpp"
#include "Value.hpp"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

namespace volta::ir {

// ============================================================================
// IRModule - Top-level container for IR
// ============================================================================

/**
 * IRModule represents a complete compilation unit.
 *
 * Contains:
 * - Global variables
 * - Function declarations and definitions
 * - Type information
 * - Module-level metadata
 *
 * Example Volta program:
 *   counter: mut int = 0
 *
 *   fn increment() -> void {
 *       counter = counter + 1
 *   }
 *
 *   fn main() -> int {
 *       increment()
 *       return counter
 *   }
 *
 * In IR (IRModule contains):
 *   @counter = global i64 0
 *
 *   define void @increment() { ... }
 *   define i64 @main() { ... }
 *
 * This is the root of the IR hierarchy:
 *   IRModule
 *     ├─ GlobalVariable (multiple)
 *     └─ Function (multiple)
 *          └─ BasicBlock (multiple)
 *               └─ Instruction (multiple)
 */
class IRModule {
public:
    explicit IRModule(const std::string& name);
    ~IRModule();

    const std::string& getName() const { return name_; }

    // Global variables
    void addGlobalVariable(std::unique_ptr<GlobalVariable> global);
    const std::vector<std::unique_ptr<GlobalVariable>>& getGlobalVariables() const {
        return globals_;
    }
    GlobalVariable* getGlobalVariable(const std::string& name) const;

    // Functions
    Function* createFunction(
        const std::string& name,
        std::vector<std::unique_ptr<Parameter>> parameters,
        std::shared_ptr<semantic::Type> returnType
    );

    void addFunction(std::unique_ptr<Function> function);
    void removeFunction(Function* function);

    const std::vector<std::unique_ptr<Function>>& getFunctions() const {
        return functions_;
    }

    Function* getFunction(const std::string& name) const;

    // Find main function (entry point)
    Function* getMainFunction() const;

    // Module verification (check IR is well-formed)
    bool verify() const;

    // For printing/debugging
    std::string toString() const;

private:
    std::string name_;

    // Global variables
    std::vector<std::unique_ptr<GlobalVariable>> globals_;
    std::unordered_map<std::string, GlobalVariable*> globalMap_;

    // Functions
    std::vector<std::unique_ptr<Function>> functions_;
    std::unordered_map<std::string, Function*> functionMap_;
};

} // namespace volta::ir
