#pragma once

#include "vir/VIRModule.hpp"
#include "semantic/SemanticAnalyzer.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <unordered_map>

namespace volta::vir {

/**
 * Runtime function declarations manager
 *
 * Declares all Volta runtime functions (volta_gc_alloc, volta_array_new, etc.)
 * in the LLVM module.
 */
class RuntimeFunctions {
public:
    RuntimeFunctions(llvm::Module* module, llvm::LLVMContext* context);

    // Memory management
    llvm::Function* getGCAlloc() const { return gcAlloc_; }
    llvm::Function* getGCCollect() const { return gcCollect_; }

    // Printing
    llvm::Function* getPrintInt() const { return printInt_; }
    llvm::Function* getPrintFloat() const { return printFloat_; }
    llvm::Function* getPrintBool() const { return printBool_; }
    llvm::Function* getPrintString() const { return printString_; }

    // Arrays (dynamic arrays)
    llvm::Function* getArrayNew() const { return arrayNew_; }
    llvm::Function* getArrayPush() const { return arrayPush_; }
    llvm::Function* getArrayPop() const { return arrayPop_; }
    llvm::Function* getArrayGet() const { return arrayGet_; }
    llvm::Function* getArraySet() const { return arraySet_; }
    llvm::Function* getArrayLength() const { return arrayLength_; }
    llvm::Function* getArrayMap() const { return arrayMap_; }
    llvm::Function* getArrayFilter() const { return arrayFilter_; }

    // Fixed arrays (Phase 7)
    // Allocate heap-allocated fixed array with runtime-determined size
    // Signature: void* volta_alloc_fixed_array(int64_t element_size, int64_t count)
    llvm::Function* getAllocFixedArray() const { return allocFixedArray_; }

    // Strings
    llvm::Function* getStringNew() const { return stringNew_; }
    llvm::Function* getStringConcat() const { return stringConcat_; }
    llvm::Function* getStringLength() const { return stringLength_; }

    // Panic
    llvm::Function* getPanic() const { return panic_; }

private:
    void declareRuntimeFunctions();

    llvm::Module* module_;
    llvm::LLVMContext* context_;

    // Runtime function declarations
    llvm::Function* gcAlloc_;
    llvm::Function* gcCollect_;
    llvm::Function* printInt_;
    llvm::Function* printFloat_;
    llvm::Function* printBool_;
    llvm::Function* printString_;
    llvm::Function* arrayNew_;
    llvm::Function* arrayPush_;
    llvm::Function* arrayPop_;
    llvm::Function* arrayGet_;
    llvm::Function* arraySet_;
    llvm::Function* arrayLength_;
    llvm::Function* arrayMap_;
    llvm::Function* arrayFilter_;
    llvm::Function* allocFixedArray_;  // Phase 7: Fixed array allocation
    llvm::Function* stringNew_;
    llvm::Function* stringConcat_;
    llvm::Function* stringLength_;
    llvm::Function* panic_;
};

/**
 * VIR Codegen: VIR → LLVM IR
 *
 * Mechanically translates VIR nodes to LLVM IR instructions.
 */
class VIRCodegen {
public:
    VIRCodegen(llvm::LLVMContext* context,
               llvm::Module* module,
               const volta::semantic::SemanticAnalyzer* analyzer);

    ~VIRCodegen();

    /**
     * Generate LLVM IR from VIR module
     */
    bool generate(const VIRModule* virModule);

private:
    llvm::LLVMContext* context_;
    llvm::Module* module_;
    std::unique_ptr<llvm::IRBuilder<>> builder_;
    std::unique_ptr<llvm::IRBuilder<>> allocaBuilder_;  // For entry block allocas
    std::unique_ptr<RuntimeFunctions> runtime_;
    const volta::semantic::SemanticAnalyzer* analyzer_;

    // Codegen state
    llvm::Function* currentFunction_;

    // Scope management: stack of scopes, each scope maps variable names to allocas
    std::vector<std::unordered_map<std::string, llvm::AllocaInst*>> scopeStack_;

    // Loop state for break/continue
    struct LoopState {
        llvm::BasicBlock* conditionBlock;
        llvm::BasicBlock* exitBlock;
        LoopState(llvm::BasicBlock* cond, llvm::BasicBlock* exit)
            : conditionBlock(cond), exitBlock(exit) {}
    };
    std::vector<LoopState> loopStack_;

    // Struct type registry
    std::unordered_map<std::string, llvm::StructType*> structTypes_;

    // Error tracking
    bool hasErrors_;
    std::vector<std::string> errors_;

    // ========================================================================
    // Top-Level Generation
    // ========================================================================

    /**
     * Phase 1: Declare all structs (create LLVM struct types)
     */
    void declareStructs(const VIRModule* module);
    void declareStruct(const VIRStructDecl* structDecl);

    /**
     * Phase 2: Declare all function prototypes
     */
    void declareFunctions(const VIRModule* module);
    void declareFunction(const VIRFunction* func);

    /**
     * Phase 3: Generate function bodies
     */
    void generateFunctions(const VIRModule* module);
    void generateFunction(const VIRFunction* func);

    // ========================================================================
    // Statement Codegen
    // ========================================================================

    void codegen(const VIRStmt* stmt);
    void codegen(const VIRBlock* block);
    void codegen(const VIRVarDecl* varDecl);
    void codegen(const VIRReturn* ret);
    void codegen(const VIRIfStmt* ifStmt);
    void codegen(const VIRWhile* whileLoop);
    void codegen(const VIRForRange* forRangeLoop);
    void codegen(const VIRFor* forLoop);
    void codegen(const VIRBreak* breakStmt);
    void codegen(const VIRContinue* continueStmt);
    void codegen(const VIRExprStmt* exprStmt);

    // ========================================================================
    // Expression Codegen
    // ========================================================================

    llvm::Value* codegen(const VIRExpr* expr);

    // Literals
    llvm::Value* codegen(const VIRIntLiteral* lit);
    llvm::Value* codegen(const VIRFloatLiteral* lit);
    llvm::Value* codegen(const VIRBoolLiteral* lit);
    llvm::Value* codegen(const VIRStringLiteral* lit);

    // References
    llvm::Value* codegen(const VIRLocalRef* ref);
    llvm::Value* codegen(const VIRParamRef* ref);
    llvm::Value* codegen(const VIRGlobalRef* ref);

    // Operations
    llvm::Value* codegen(const VIRBinaryOp* op);
    llvm::Value* codegen(const VIRUnaryOp* op);

    // Type operations
    llvm::Value* codegen(const VIRBox* box);
    llvm::Value* codegen(const VIRUnbox* unbox);
    llvm::Value* codegen(const VIRCast* cast);
    llvm::Value* codegen(const VIRImplicitCast* cast);

    // Calls
    llvm::Value* codegen(const VIRCall* call);
    llvm::Value* codegen(const VIRCallRuntime* call);
    llvm::Value* codegen(const VIRCallIndirect* call);

    // Wrapper generation (THE KEY INNOVATION!)
    llvm::Function* codegen(const VIRWrapFunction* wrap);

    // Memory operations
    llvm::Value* codegen(const VIRAlloca* alloca);
    llvm::Value* codegen(const VIRLoad* load);
    llvm::Value* codegen(const VIRStore* store);

    // Control flow
    llvm::Value* codegen(const VIRIfExpr* ifExpr);

    // Struct operations
    llvm::Value* codegen(const VIRStructNew* structNew);
    llvm::Value* codegen(const VIRStructGet* structGet);
    llvm::Value* codegen(const VIRStructSet* structSet);

    // Array operations (dynamic arrays - runtime calls)
    llvm::Value* codegen(const VIRArrayNew* arrayNew);
    llvm::Value* codegen(const VIRArrayGet* arrayGet);
    llvm::Value* codegen(const VIRArraySet* arraySet);

    // ========================================================================
    // Fixed Array Operations (Phase 7)
    // ========================================================================

    /**
     * Generate code for fixed array creation
     * Handles both stack and heap allocation based on isStackAllocated flag
     *
     * Stack path: alloca llvm::ArrayType, initialize elements
     * Heap path: volta_gc_alloc, cast, initialize elements
     */
    llvm::Value* codegen(const VIRFixedArrayNew* arrayNew);

    /**
     * Generate code for fixed array element access
     * Uses GEP (GetElementPtr) to compute element address, then load
     *
     * Index is already bounds-checked by VIRBoundsCheck
     */
    llvm::Value* codegen(const VIRFixedArrayGet* arrayGet);

    /**
     * Generate code for fixed array element update
     * Uses GEP to compute element address, then store
     *
     * Index is already bounds-checked by VIRBoundsCheck
     */
    llvm::Value* codegen(const VIRFixedArraySet* arraySet);

    // Safety operations
    llvm::Value* codegen(const VIRBoundsCheck* boundsCheck);

    // ========================================================================
    // Scope Managment
    // ========================================================================

    /*
     * Add a scope to the top of the scope stack
     */
    void pushScope();

    /*
     * Pops a scope from the top of the scope stack
     */
    void popScope();

    /*
     * Declares a variable in the current scope
     */
    void declareVariable(std::string name, llvm::AllocaInst* ptr);

    /*
     * Looks up a variable, starting from the inner most scope up to the global scope
     */
    llvm::AllocaInst* lookupVariable(std::string name);

    // ========================================================================
    // Type Lowering
    // ========================================================================

    /**
     * Convert Volta semantic type to LLVM type
     */
    llvm::Type* lowerType(std::shared_ptr<volta::semantic::Type> type);

    /**
     * Get LLVM function type from Volta function type
     */
    llvm::FunctionType* lowerFunctionType(const VIRFunction* func);

    // ========================================================================
    // Helper Functions
    // ========================================================================

    /**
     * Create alloca in entry block of function (for proper mem2reg optimization)
     */
    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* fn,
                                              const std::string& varName,
                                              llvm::Type* type);

    /**
     * Box a value into void* for runtime
     */
    llvm::Value* createBox(llvm::Value* value, llvm::Type* valueType);

    /**
     * Unbox void* into typed value
     */
    llvm::Value* createUnbox(llvm::Value* boxed, llvm::Type* targetType);

    /**
     * Get struct type by name
     */
    llvm::StructType* getStructType(const std::string& name);

    /**
     * Report codegen error
     */
    void reportError(const std::string& message);
};

} // namespace volta::vir
