#pragma once

#include "BasicBlock.hpp"
#include "Value.hpp"
#include "../semantic/Type.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace volta::ir {

// Forward declarations
class IRModule;

// ============================================================================
// Function - Represents a complete function in IR
// ============================================================================

/**
 * Function represents a callable unit of code with:
 * - A name (e.g., @add, @main)
 * - Parameters (SSA values available in entry block)
 * - Return type
 * - CFG of BasicBlocks
 * - Local symbol table
 *
 * Example Volta code:
 *   fn add(a: int, b: int) -> int {
 *       return a + b
 *   }
 *
 * In IR:
 *   define i64 @add(i64 %a, i64 %b) {
 *   entry:
 *       %0 = add i64 %a, %b
 *       ret i64 %0
 *   }
 *
 * Structure:
 * - Parameters are available as SSA values in the entry block
 * - BasicBlocks form a CFG (control flow graph)
 * - Entry block is the first block, always executed first
 */
class Function {
public:
    Function(
        const std::string& name,
        std::vector<std::unique_ptr<Parameter>> parameters,
        std::shared_ptr<semantic::Type> returnType
    );

    ~Function();

    // Identity
    const std::string& getName() const { return name_; }
    std::shared_ptr<semantic::Type> getReturnType() const { return returnType_; }

    IRModule* getParent() const { return parent_; }
    void setParent(IRModule* parent) { parent_ = parent; }

    // Parameters
    const std::vector<std::unique_ptr<Parameter>>& getParameters() const { return parameters_; }
    size_t getNumParameters() const { return parameters_.size(); }
    Parameter* getParameter(size_t index) const;

    // Basic Block management
    BasicBlock* createBasicBlock(const std::string& name);
    void addBasicBlock(std::unique_ptr<BasicBlock> block);
    void removeBasicBlock(BasicBlock* block);

    const std::vector<std::unique_ptr<BasicBlock>>& getBasicBlocks() const { return basicBlocks_; }
    size_t size() const { return basicBlocks_.size(); }
    bool empty() const { return basicBlocks_.empty(); }

    // Entry block (first block, where execution starts)
    BasicBlock* getEntryBlock() const;
    void setEntryBlock(BasicBlock* entry);

    // CFG queries
    bool isEntryBlock(const BasicBlock* block) const;

    // SSA variable tracking (for SSABuilder)
    // Maps variable name -> current SSA value in this scope
    // This is used during IR generation to track variable versions
    Value* getCurrentValue(const std::string& varName) const;
    void setCurrentValue(const std::string& varName, Value* value);

    // For printing/debugging
    std::string toString() const;

private:
    std::string name_;
    std::vector<std::unique_ptr<Parameter>> parameters_;
    std::shared_ptr<semantic::Type> returnType_;
    IRModule* parent_;

    // CFG - ordered list of basic blocks
    std::vector<std::unique_ptr<BasicBlock>> basicBlocks_;

    // Variable tracking for SSA construction
    // Maps variable name -> current SSA value
    std::unordered_map<std::string, Value*> currentValues_;
};

} // namespace volta::ir
