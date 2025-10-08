#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include "IR/Arena.hpp"
#include "IR/Function.hpp"
#include "IR/Value.hpp"
#include "IR/IRType.hpp"

namespace volta::ir {

// Forward declarations
class BasicBlock;
class Instruction;
class Pass;

/**
 * Module - Top-level IR container
 *
 * Design Philosophy:
 * - Owns all IR objects via arena allocation
 * - Provides unified interface for IR construction
 * - Caches common types for efficiency
 * - Manages function and global symbol tables
 *
 * Key Concepts:
 * - Arena Allocation: All IR objects allocated from arena
 * - Ownership: Module owns everything, simplifies lifetime management
 * - Type Caching: Reuse common type instances
 * - Symbol Resolution: Maps names to functions/globals
 *
 * Benefits:
 * - 10-100x faster allocation than new/delete
 * - No use-after-free bugs
 * - Simple ownership model
 * - Easy serialization/deserialization
 *
 * Usage:
 *   Module module("my_program");
 *   auto* func = module.createFunction("main", module.getIntType(), {});
 *   // Build IR...
 *   module.verify();
 *   std::cout << module.toString();
 *
 * Learning Goals:
 * - Understand module-level IR organization
 * - Master arena-based memory management
 * - Learn type interning techniques
 * - Appreciate LLVM's design
 */
class Module {
public:
    /**
     * Create a new module
     * @param name Module name (e.g., "my_program.volta")
     * @param chunkSize Arena chunk size (default 1MB)
     */
    explicit Module(const std::string& name, size_t chunkSize = 1024 * 1024);
    ~Module();

    // Disable copy (module owns unique resources)
    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;

    // Allow move
    Module(Module&&) noexcept;
    Module& operator=(Module&&) noexcept;

    // ========================================================================
    // Basic Properties
    // ========================================================================

    /**
     * Get module name
     */
    const std::string& getName() const { return name_; }

    /**
     * Set module name
     */
    void setName(const std::string& name) { name_ = name; }

    /**
     * Get the arena allocator
     * LEARNING TIP: Advanced users can allocate directly from arena
     */
    Arena& getArena() { return arena_; }

    // ========================================================================
    // Function Management
    // ========================================================================

    /**
     * Create a new function and add it to the module
     * @param name Function name (e.g., "main", "fibonacci")
     * @param returnType Return type
     * @param paramTypes Parameter types
     * @return Newly created function (allocated in arena)
     *
     * LEARNING TIP: Use arena_.allocate<Function>(...) instead of new
     */
    Function* createFunction(const std::string& name,
                            std::shared_ptr<IRType> returnType,
                            const std::vector<std::shared_ptr<IRType>>& paramTypes = {});

    /**
     * Get function by name
     * @return Function pointer or nullptr if not found
     */
    Function* getFunction(const std::string& name) const;

    /**
     * Get or create function (for external declarations)
     * If function exists, return it. Otherwise, create declaration.
     */
    Function* getOrInsertFunction(const std::string& name,
                                 std::shared_ptr<IRType> returnType,
                                 const std::vector<std::shared_ptr<IRType>>& paramTypes = {});

    /**
     * Get all functions in the module
     */
    const std::vector<Function*>& getFunctions() const { return functions_; }

    /**
     * Get number of functions
     */
    size_t getNumFunctions() const { return functions_.size(); }

    /**
     * Remove function from module
     * IMPORTANT: Does NOT delete! Arena owns the memory.
     */
    void removeFunction(Function* func);

    // ========================================================================
    // Global Variable Management
    // ========================================================================

    /**
     * Create a global variable
     * @param name Variable name (e.g., "counter", "PI")
     * @param type Variable type
     * @param initializer Initial value (can be nullptr)
     * @param isConstant Whether this is a constant
     */
    GlobalVariable* createGlobalVariable(const std::string& name,
                                        std::shared_ptr<IRType> type,
                                        Constant* initializer = nullptr,
                                        bool isConstant = false);

    /**
     * Get global variable by name
     */
    GlobalVariable* getGlobalVariable(const std::string& name) const;

    /**
     * Get all globals
     */
    const std::vector<GlobalVariable*>& getGlobals() const { return globals_; }

    /**
     * Get number of globals
     */
    size_t getNumGlobals() const { return globals_.size(); }

    /**
     * Remove global variable
     */
    void removeGlobal(GlobalVariable* global);

    // ========================================================================
    // Type Caching (Type Interning)
    // ========================================================================

    /**
     * Get cached primitive types
     * These are singleton instances shared across the module
     */
    std::shared_ptr<IRType> getIntType();
    std::shared_ptr<IRType> getFloatType();
    std::shared_ptr<IRType> getBoolType();
    std::shared_ptr<IRType> getStringType();
    std::shared_ptr<IRType> getVoidType();

    /**
     * Get or create pointer type
     * Uses cache to avoid duplicate pointer types
     */
    std::shared_ptr<IRType> getPointerType(std::shared_ptr<IRType> pointeeType);

    /**
     * Get or create array type
     */
    std::shared_ptr<IRType> getArrayType(std::shared_ptr<IRType> elementType, size_t size);

    CreateEnumInst* createEnum(std::shared_ptr<IRType> enumType, unsigned variantTag,
                           std::vector<Value*> fieldValues, const std::string& name = "");
    GetEnumTagInst* createGetEnumTag(Value* enumValue, const std::string& name = "");
    ExtractEnumDataInst* createExtractEnumData(std::shared_ptr<IRType> resultType, Value* enumValue,
                                            unsigned fieldIndex, const std::string& name = "");

    // ========================================================================
    // IR Object Creation Helpers
    // ========================================================================

    /**
     * Create a basic block (allocated in arena)
     * @param name Block name
     * @param parent Optional parent function
     */
    BasicBlock* createBasicBlock(const std::string& name = "", Function* parent = nullptr);

    /**
     * Create an instruction of type T
     */
    template<typename T, typename... Args>
    T* createInstruction(Args&&... args) {
        return arena_.allocate<T>(std::forward<Args>(args)...);
    }

    /**
     * Create a constant value
     */
    template<typename T, typename... Args>
    T* createConstant(Args&&... args) {
        return arena_.allocate<T>(std::forward<Args>(args)...);
    }

    /**
     * Create an argument
     */
    Argument* createArgument(std::shared_ptr<IRType> type, unsigned argNo, const std::string& name = "");

    // ========================================================================
    // Instruction Factory Methods (Arena-Allocated)
    // ========================================================================

    // Binary operations
    BinaryOperator* createBinaryOp(Instruction::Opcode op, Value* lhs, Value* rhs, const std::string& name = "");
    UnaryOperator* createUnaryOp(Instruction::Opcode op, Value* operand, const std::string& name = "");
    CmpInst* createCmp(Instruction::Opcode op, Value* lhs, Value* rhs, const std::string& name = "");

    // Memory operations
    AllocaInst* createAlloca(std::shared_ptr<IRType> type, const std::string& name = "");
    LoadInst* createLoad(Value* ptr, const std::string& name = "");
    StoreInst* createStore(Value* value, Value* ptr);
    GCAllocInst* createGCAlloc(std::shared_ptr<IRType> type, const std::string& name = "");
    ExtractValueInst* createExtractValue(Value* structVal, unsigned fieldIndex, const std::string& name = "");
    InsertValueInst* createInsertValue(Value* structVal, Value* newValue, unsigned fieldIndex, const std::string& name = "");

    // Array operations
    ArrayNewInst* createArrayNew(std::shared_ptr<IRType> elementType, Value* size, const std::string& name = "");
    ArrayGetInst* createArrayGet(Value* array, Value* index, const std::string& name = "");
    ArraySetInst* createArraySet(Value* array, Value* index, Value* value);
    ArrayLenInst* createArrayLen(Value* array, const std::string& name = "");
    ArraySliceInst* createArraySlice(Value* array, Value* start, Value* end, const std::string& name = "");

    // Type operations
    CastInst* createCast(Value* value, std::shared_ptr<IRType> targetType, const std::string& name = "");

    // Control flow (terminators)
    ReturnInst* createReturn(Value* value = nullptr);
    BranchInst* createBranch(BasicBlock* target);
    CondBranchInst* createCondBranch(Value* condition, BasicBlock* trueBlock, BasicBlock* falseBlock);
    SwitchInst* createSwitch(Value* value, BasicBlock* defaultBlock,
                             const std::vector<SwitchInst::CaseEntry>& cases = {});

    // Function calls
    CallInst* createCall(Function* callee, const std::vector<Value*>& args, const std::string& name = "");
    CallIndirectInst* createCallIndirect(Value* callee, const std::vector<Value*>& args, const std::string& name = "");

    // SSA
    PhiNode* createPhi(std::shared_ptr<IRType> type,
                       const std::vector<PhiNode::IncomingValue>& incomingValues = {},
                       const std::string& name = "");

    // ========================================================================
    // Constant Pooling
    // ========================================================================

    /**
     * Get or create integer constant (pooled for deduplication)
     */
    ConstantInt* getConstantInt(int64_t value, std::shared_ptr<IRType> type);

    /**
     * Get or create float constant (pooled)
     */
    ConstantFloat* getConstantFloat(double value, std::shared_ptr<IRType> type);

    /**
     * Get or create bool constant (pooled)
     */
    ConstantBool* getConstantBool(bool value, std::shared_ptr<IRType> type);

    /**
     * Get or create string constant (pooled)
     */
    ConstantString* getConstantString(const std::string& value, std::shared_ptr<IRType> type);

    // ========================================================================
    // Verification
    // ========================================================================

    /**
     * Verify module correctness
     * @param error Optional output for error message
     * @return true if valid, false otherwise
     *
     * Checks:
     * 1. All functions are valid
     * 2. No duplicate function names
     * 3. Globals are properly initialized
     * 4. No dangling references
     */
    bool verify(std::string* error = nullptr) const;

    // ========================================================================
    // Pretty Printing
    // ========================================================================

    /**
     * Convert to string representation
     * Format:
     *   ; Module: name
     *
     *   @global1: type = value
     *
     *   function @func1(...) -> type {
     *     ...
     *   }
     */
    std::string toString() const;

    /**
     * Print with statistics
     */
    std::string toStringDetailed() const;

    /**
     * Dump module to file
     * @param filename Output file path
     * @return true on success
     */
    bool dumpToFile(const std::string& filename) const;

    // ========================================================================
    // Optimization Support
    // ========================================================================

    /**
     * Run an optimization pass on this module
     * LEARNING TIP: This will be used later for DCE, inlining, etc.
     */
    void runPass(Pass* pass);

    // ========================================================================
    // Statistics
    // ========================================================================

    /**
     * Get arena memory usage
     */
    size_t getArenaUsage() const { return arena_.getBytesAllocated(); }

    /**
     * Get total number of instructions across all functions
     */
    size_t getTotalInstructions() const;

    /**
     * Get total number of basic blocks across all functions
     */
    size_t getTotalBasicBlocks() const;

private:
    std::string name_;
    Arena arena_;

    // Functions
    std::vector<Function*> functions_;
    std::unordered_map<std::string, Function*> functionMap_;

    // Global variables
    std::vector<GlobalVariable*> globals_;
    std::unordered_map<std::string, GlobalVariable*> globalMap_;

    // Type cache (for type interning)
    std::shared_ptr<IRType> intType_;
    std::shared_ptr<IRType> floatType_;
    std::shared_ptr<IRType> boolType_;
    std::shared_ptr<IRType> stringType_;
    std::shared_ptr<IRType> voidType_;

    std::unordered_set<std::shared_ptr<IRType>, IRTypeHash, IRTypeEqual> pointerTypes_;
    std::unordered_set<std::shared_ptr<IRType>, IRTypeHash, IRTypeEqual> arrayTypes_;

    // Constant pooling (lazy initialized to avoid construction order issues)
    std::unique_ptr<std::unordered_map<int64_t, ConstantInt*>> intPool_;
    std::unique_ptr<std::unordered_map<double, ConstantFloat*>> floatPool_;
    std::unique_ptr<std::unordered_map<bool, ConstantBool*>> boolPool_;
    std::unique_ptr<std::unordered_map<std::string, ConstantString*>> stringPool_;
};

} // namespace volta::ir
