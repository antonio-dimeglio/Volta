#include "llvm/RuntimeFunctions.hpp"
#include <llvm/IR/Type.h>

using namespace llvm;

namespace volta::llvm_codegen {

RuntimeFunctions::RuntimeFunctions(Module* module, LLVMContext* context)
    : module_(module),
      context_(context),
      gcAlloc_(nullptr),
      gcCollect_(nullptr),
      printInt_(nullptr),
      printUInt_(nullptr),
      printFloat_(nullptr),
      printBool_(nullptr),
      printString_(nullptr),
      stringNew_(nullptr),
      stringLength_(nullptr),
      stringEq_(nullptr),
      stringCmp_(nullptr),
      stringConcat_(nullptr),
      arrayNew_(nullptr),
      arrayLength_(nullptr),
      arrayPush_(nullptr),
      arrayGet_(nullptr),
      arraySet_(nullptr) {
    // Declare all runtime functions
    declareAll();
}

void RuntimeFunctions::declareAll() {
    // Common types
    auto* voidTy = Type::getVoidTy(*context_);
    auto* i1Ty = Type::getInt1Ty(*context_);
    auto* i8Ty = Type::getInt8Ty(*context_);
    auto* i32Ty = Type::getInt32Ty(*context_);
    auto* i64Ty = Type::getInt64Ty(*context_);
    auto* u64Ty = Type::getInt64Ty(*context_);  // Same as i64 in LLVM
    auto* doubleTy = Type::getDoubleTy(*context_);
    auto* ptrTy = PointerType::get(*context_, 0);  // Opaque pointer
    auto* sizeTy = Type::getInt64Ty(*context_);    // size_t = i64 on most platforms

    // ========================================================================
    // Memory Management
    // ========================================================================

    gcAlloc_ = declareFunction("volta_gc_alloc", ptrTy, {sizeTy});
    gcCollect_ = declareFunction("volta_gc_collect", voidTy, {});

    // ========================================================================
    // Print Functions
    // ========================================================================

    printInt_ = declareFunction("volta_print_int", voidTy, {i64Ty});
    printUInt_ = declareFunction("volta_print_uint", voidTy, {u64Ty});
    printFloat_ = declareFunction("volta_print_float", voidTy, {doubleTy});
    printBool_ = declareFunction("volta_print_bool", voidTy, {i8Ty});
    printString_ = declareFunction("volta_print_string", voidTy, {ptrTy});

    // ========================================================================
    // String Functions
    // ========================================================================

    stringNew_ = declareFunction("volta_string_new", ptrTy, {ptrTy, sizeTy});
    stringLength_ = declareFunction("volta_string_length", i64Ty, {ptrTy});
    stringEq_ = declareFunction("volta_string_eq", i1Ty, {ptrTy, ptrTy});
    stringCmp_ = declareFunction("volta_string_cmp", i32Ty, {ptrTy, ptrTy});
    stringConcat_ = declareFunction("volta_string_concat", ptrTy, {ptrTy, ptrTy});

    // ========================================================================
    // Array Functions
    // ========================================================================

    arrayNew_ = declareFunction("volta_array_new", ptrTy, {sizeTy});
    arrayLength_ = declareFunction("volta_array_length", i64Ty, {ptrTy});
    arrayPush_ = declareFunction("volta_array_push", voidTy, {ptrTy, ptrTy});
    arrayGet_ = declareFunction("volta_array_get", ptrTy, {ptrTy, i64Ty});
    arraySet_ = declareFunction("volta_array_set", voidTy, {ptrTy, i64Ty, ptrTy});
}

Function* RuntimeFunctions::declareFunction(
    const char* name,
    Type* returnType,
    std::vector<Type*> paramTypes) {

    // Create function type
    FunctionType* funcType = FunctionType::get(returnType, paramTypes, false);

    // Create function declaration with external linkage
    Function* func = Function::Create(
        funcType,
        Function::ExternalLinkage,
        name,
        module_
    );

    return func;
}

} // namespace volta::llvm_codegen
