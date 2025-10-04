#pragma once

#include <vector>
#include <string>
#include <memory>
#include "IR/Value.hpp"
#include "IR/BasicBlock.hpp"
#include "IR/IRType.hpp"

namespace volta::ir {

// Forward declarations
class Module;
class Instruction;

/**
 * Function - Represents a function in the IR
 *
 * Design Philosophy:
 * - CFG Container: Functions own and manage their basic blocks
 * - SSA Form: Parameters are represented as Argument values
 * - Single Entry: Every function has one entry block
 * - Multiple Exits: Functions can have multiple return statements
 * - Verification: Functions can self-verify their correctness
 *
 * Key Concepts:
 * - Basic Blocks: The CFG nodes containing instructions
 * - Arguments: Formal parameters (incoming values)
 * - Entry Block: The first block executed (dominates all others)
 * - Exit Blocks: Blocks that end with return instructions
 * - Symbol Name: The function's global identifier
 *
 * Function Structure:
 *   function @name(param1: type1, param2: type2) -> returnType {
 *   entry:
 *     ... instructions ...
 *   block2:
 *     ... instructions ...
 *   }
 *
 * Learning Goals:
 * - Understand CFG management and traversal
 * - Learn function calling conventions in IR
 * - Practice verification of complex data structures
 * - Master memory ownership in compiler data structures
 */
class Function : public Value {
    friend class Arena;
    friend class Module;
public:
    /**
     * Create a new function
     * @param name Function name (e.g., "main", "fibonacci")
     * @param returnType The type this function returns
     * @param paramTypes Types of the function parameters
     * @return Newly created function
    
     */
    // ========================================================================
    // Basic Properties
    // ========================================================================

    /**
     * Get the function's return type
     */
    std::shared_ptr<IRType> getReturnType() const { return returnType_; }

    /**
     * Get parent module (if any)
     */
    Module* getParent() const { return parent_; }
    void setParent(Module* parent) { parent_ = parent; }

    /**
     * Check if this is just a declaration (no body)
     * LEARNING TIP: Declarations have no basic blocks
     */
    bool isDeclaration() const;

    /**
     * Check if this is a definition (has body)
     */
    bool isDefinition() const;

    /**
     * Check if this function has variable arguments (like printf)
     * LEARNING TIP: Volta doesn't support varargs initially
     */
    bool hasVarArgs() const { return hasVarArgs_; }

    // ========================================================================
    // Parameter/Argument Management
    // ========================================================================

    /**
     * Get all function arguments (parameters)
     */
    const std::vector<Argument*>& getArguments() const { return arguments_; }

    /**
     * Get number of parameters
     */
    size_t getNumParams() const;

    /**
     * Get a specific parameter by index
     * @param idx Parameter index (0-based)
     * LEARNING TIP: Bounds check before accessing!
     */
    Argument* getParam(size_t idx) const;

    /**
     * Check if function has any parameters
     */
    bool hasParams() const { return !arguments_.empty(); }

    // ========================================================================
    // Basic Block Management
    // ========================================================================

    /**
     * Get all basic blocks in this function
     */
    const std::vector<BasicBlock*>& getBlocks() const { return blocks_; }

    /**
     * Get number of basic blocks
     */
    size_t getNumBlocks() const;

    /**
     * Check if function has any blocks
     */
    bool hasBlocks() const;

    /**
     * Get the entry block (first block executed)
     * @return Entry block, or nullptr if function is a declaration
     *
     * LEARNING TIP: Entry block is special - it dominates all other blocks
     */
    BasicBlock* getEntryBlock() const { return entryBlock_; }

    /**
     * Set the entry block
     * @param block Must be in this function's block list
     *
     * LEARNING TIP: Validate that block is actually in blocks_!
     */
    void setEntryBlock(BasicBlock* block);

    /**
     * Add a basic block to this function
     * @param block The block to add
     *
     * IMPORTANT: Sets block's parent to this function.
     * If this is the first block, it becomes the entry block.
     */
    void addBasicBlock(BasicBlock* block);

    /**
     * Remove a basic block from this function
     * @param block The block to remove
     *
     * IMPORTANT: Does NOT delete the block. Caller owns the pointer.
     * Updates entry block if necessary.
     */
    void removeBasicBlock(BasicBlock* block);

    /**
     * Erase a basic block (remove and delete)
     * @param block The block to erase
     */
    void eraseBasicBlock(BasicBlock* block);

    /**
     * Find a block by name
     * @param name Block name to search for
     * @return Found block, or nullptr if not found
     */
    BasicBlock* findBlock(const std::string& name) const;

    /**
     * Check if function has only a single basic block
     * LEARNING TIP: Single-block functions are easier to optimize
     */
    bool hasSingleBlock() const;

    // ========================================================================
    // Instruction Access
    // ========================================================================

    /**
     * Get all instructions in this function (from all blocks)
     * @return Vector of all instructions in block order
     *
     * LEARNING TIP: Useful for optimization passes that need to visit every instruction
     */
    std::vector<Instruction*> getAllInstructions() const;

    /**
     * Get total number of instructions across all blocks
     */
    size_t getNumInstructions() const;

    // ========================================================================
    // CFG Analysis
    // ========================================================================

    /**
     * Get all blocks that end with a return instruction
     * @return Vector of exit blocks
     *
     * LEARNING TIP: Functions can have multiple return points
     */
    std::vector<BasicBlock*> getExitBlocks() const;

    /**
     * Get all unreachable blocks (not reachable from entry)
     * @return Vector of unreachable blocks
     *
     * LEARNING TIP: Unreachable blocks are dead code
     */
    std::vector<BasicBlock*> getUnreachableBlocks() const;

    /**
     * Remove all unreachable blocks from the function
     * @return Number of blocks removed
     *
     * LEARNING TIP: This is a simple dead code elimination pass
     */
    size_t removeUnreachableBlocks();

    /**
     * Check if this function can return normally
     * @return false if all paths end with unreachable/noreturn
     *
     * LEARNING TIP: Functions like `exit()` or `abort()` don't return
     */
    bool canReturn() const;

    /**
     * Check if this function never returns (noreturn)
     * LEARNING TIP: Opposite of canReturn()
     */
    bool doesNotReturn() const;

    /**
     * Check if return type is non-void (function returns a value)
     */
    bool hasReturnValue() const;

    // ========================================================================
    // Verification
    // ========================================================================

    /**
     * Verify that this function is well-formed
     * @param error Optional output parameter for error message
     * @return true if valid, false otherwise
     *
     * Checks:
     * 1. Entry block exists and is in block list
     * 2. All blocks have terminators
     * 3. CFG edges are consistent (successors have matching predecessors)
     * 4. Arguments are set up correctly
     * 5. Return statements match return type
     * 6. All blocks belong to this function
     *
     * LEARNING TIP: Always verify IR before running optimization passes!
     */
    bool verify(std::string* error = nullptr) const;

    // ========================================================================
    // Pretty Printing
    // ========================================================================

    /**
     * Convert to string representation
     * Format:
     *   function @name(arg0: type0, arg1: type1) -> returnType {
     *   entry:
     *     %1 = add i64 %arg0, %arg1
     *     ret i64 %1
     *   }
     */
    std::string toString() const override;

    /**
     * Print function with detailed analysis information
     * Shows: number of blocks, number of instructions, entry/exit blocks
     */
    std::string toStringDetailed() const;

    /**
     * Generate Graphviz DOT format for CFG visualization
     * @return DOT graph representation
     *
     * LEARNING TIP: Use `dot -Tpng output.dot -o cfg.png` to visualize
     */
    std::string printCFG() const;

    // ========================================================================
    // Advanced (Optional - Implement Later)
    // ========================================================================

    /**
     * Create a deep copy of this function
     * LEARNING TIP: Very complex! Clone all blocks and update all references
     */
    Function* clone(const std::string& newName = "") const;

    /**
     * Inline this function at a call site
     * LEARNING TIP: Used by inlining optimization pass
     */
    void inlineInto(CallInst* callSite);

    // ========================================================================
    // LLVM-style RTTI
    // ========================================================================

    static bool classof(const Value* V) {
        return V->getKind() == ValueKind::Function;
    }

private:
    Function(const std::string& name,
             std::shared_ptr<IRType> returnType,
             const std::vector<std::shared_ptr<IRType>>& paramTypes);

    // Function properties
    std::shared_ptr<IRType> returnType_;
    bool hasVarArgs_;
    Module* parent_;

    // Parameters
    std::vector<Argument*> arguments_;  // Owned pointers

    // Basic blocks (CFG)
    std::vector<BasicBlock*> blocks_;   // Owned pointers
    BasicBlock* entryBlock_;            // First block (not owned, points into blocks_)

    // LEARNING NOTE: We store blocks in a vector to preserve order,
    // but entry block is tracked separately for quick access.
    // The entry block MUST also be in the blocks_ vector!
};

} // namespace volta::ir
