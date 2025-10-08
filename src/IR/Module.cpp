#include "IR/Module.hpp"
#include "IR/BasicBlock.hpp"
#include "IR/Instruction.hpp"
#include "IR/OptimizationPass.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_set>

namespace volta::ir {

// ============================================================================
// Constructor and Destructor
// ============================================================================

Module::Module(const std::string& name, size_t chunkSize)
    : name_(name),
      arena_(chunkSize) {
    intType_ = nullptr;
    floatType_ = nullptr;
    boolType_ = nullptr;
    stringType_= nullptr;
    voidType_ = nullptr;

    // Initialize constant pools in constructor body (after arena is ready)
    intPool_ = std::make_unique<std::unordered_map<int64_t, ConstantInt*>>();
    floatPool_ = std::make_unique<std::unordered_map<double, ConstantFloat*>>();
    boolPool_ = std::make_unique<std::unordered_map<bool, ConstantBool*>>();
    stringPool_ = std::make_unique<std::unordered_map<std::string, ConstantString*>>();
}

Module::~Module() {
    // Clean up functions (arena-allocated)
    // The arena will handle cleanup of arena-allocated objects
    // But we need to manually clean up heap-allocated shared_ptrs

    // Note: Arena destructor will handle cleanup of arena-allocated memory
    // The shared_ptr types will be automatically cleaned up
}

Module::Module(Module&& other) noexcept
    : name_(std::move(other.name_)),
      arena_(std::move(other.arena_)),
      functions_(std::move(other.functions_)),
      functionMap_(std::move(other.functionMap_)),
      globals_(std::move(other.globals_)),
      globalMap_(std::move(other.globalMap_)),
      intType_(std::move(other.intType_)),
      floatType_(std::move(other.floatType_)),
      boolType_(std::move(other.boolType_)),
      stringType_(std::move(other.stringType_)),
      voidType_(std::move(other.voidType_)),
      pointerTypes_(std::move(other.pointerTypes_)),
      arrayTypes_(std::move(other.arrayTypes_)),
      optionTypes_(std::move(other.optionTypes_)),
      intPool_(std::move(other.intPool_)),
      floatPool_(std::move(other.floatPool_)),
      boolPool_(std::move(other.boolPool_)),
      stringPool_(std::move(other.stringPool_)) {
}

Module& Module::operator=(Module&& other) noexcept {
    if (this != &other) {
        name_ = std::move(other.name_);
        arena_ = std::move(other.arena_);
        functions_ = std::move(other.functions_);
        functionMap_ = std::move(other.functionMap_);
        globals_ = std::move(other.globals_);
        globalMap_ = std::move(other.globalMap_);
        intType_ = std::move(other.intType_);
        floatType_ = std::move(other.floatType_);
        boolType_ = std::move(other.boolType_);
        stringType_ = std::move(other.stringType_);
        voidType_ = std::move(other.voidType_);
        pointerTypes_ = std::move(other.pointerTypes_);
        arrayTypes_ = std::move(other.arrayTypes_);
        optionTypes_ = std::move(other.optionTypes_);
        intPool_ = std::move(other.intPool_);
        floatPool_ = std::move(other.floatPool_);
        boolPool_ = std::move(other.boolPool_);
        stringPool_ = std::move(other.stringPool_);
    }
    return *this;
}

// ============================================================================
// Function Management
// ============================================================================

Function* Module::createFunction(const std::string& name,
                                 std::shared_ptr<IRType> returnType,
                                 const std::vector<std::shared_ptr<IRType>>& paramTypes) {
    auto* func = arena_.allocate<Function>(name, returnType, paramTypes);
    func->setParent(this);

    // Create arguments using arena allocation
    for (size_t i = 0; i < paramTypes.size(); ++i) {
        auto* arg = createArgument(paramTypes[i], i, "arg" + std::to_string(i));
        arg->setParent(func);
        func->arguments_.push_back(arg);
    }

    functions_.push_back(func);
    functionMap_[name] = func;
    return func;
}

Function* Module::getFunction(const std::string& name) const {
    auto it = functionMap_.find(name);
    return it == functionMap_.end() ? nullptr : it->second;
}

Function* Module::getOrInsertFunction(const std::string& name,
                                      std::shared_ptr<IRType> returnType,
                                      const std::vector<std::shared_ptr<IRType>>& paramTypes) {
    auto* existing = getFunction(name);
    if (existing) return existing;
    return createFunction(name, returnType, paramTypes);
}

void Module::removeFunction(Function* func) {
    auto it = std::find(functions_.begin(), functions_.end(), func);
    if (it != functions_.end()) functions_.erase(it);
    functionMap_.erase(func->getName());
}

// ============================================================================
// Global Variable Management
// ============================================================================

GlobalVariable* Module::createGlobalVariable(const std::string& name,
                                             std::shared_ptr<IRType> type,
                                             Constant* initializer,
                                             bool isConstant) {
    auto* global = arena_.allocate<GlobalVariable>(type, name, initializer, isConstant);
    globals_.push_back(global);
    globalMap_[name] = global;
    return global;
}

GlobalVariable* Module::getGlobalVariable(const std::string& name) const {
    auto it = globalMap_.find(name);
    return it == globalMap_.end() ? nullptr : it->second;
}

void Module::removeGlobal(GlobalVariable* global) {
    auto it = std::find(globals_.begin(), globals_.end(), global);
    if (it != globals_.end()) globals_.erase(it);
    globalMap_.erase(global->getName());
}

// ============================================================================
// Type Caching
// ============================================================================

std::shared_ptr<IRType> Module::getIntType() {
    if (!intType_) {
        intType_ = std::make_shared<IRPrimitiveType>(IRType::Kind::I64);
    }
    return intType_;
}

std::shared_ptr<IRType> Module::getFloatType() {
    if (!floatType_) {
        floatType_ = std::make_shared<IRPrimitiveType>(IRType::Kind::F64);
    }
    return floatType_;
}

std::shared_ptr<IRType> Module::getBoolType() {
    if (!boolType_) {
        boolType_ = std::make_shared<IRPrimitiveType>(IRType::Kind::I1);
    }
    return boolType_;
}

std::shared_ptr<IRType> Module::getStringType() {
    if (!stringType_) {
        stringType_ = std::make_shared<IRPrimitiveType>(IRType::Kind::String);
    }
    return stringType_;
}

std::shared_ptr<IRType> Module::getVoidType() {
    if (!voidType_) {
        voidType_ = std::make_shared<IRPrimitiveType>(IRType::Kind::Void);
    }
    return voidType_;
}

std::shared_ptr<IRType> Module::getPointerType(std::shared_ptr<IRType> pointeeType) {
    auto ptrType = std::make_shared<IRPointerType>(pointeeType);

    auto it = pointerTypes_.find(ptrType);  
    if (it != pointerTypes_.end()) {
        return *it;
    }

    pointerTypes_.insert(ptrType);
    return ptrType;
}

std::shared_ptr<IRType> Module::getArrayType(std::shared_ptr<IRType> elementType, size_t size) {
    auto arrType = std::make_shared<IRArrayType>(elementType, size);

    auto it = arrayTypes_.find(arrType);  
    if (it != arrayTypes_.end()) {
        return *it;
    }

    arrayTypes_.insert(arrType);
    return arrType;
}

std::shared_ptr<IRType> Module::getOptionType(std::shared_ptr<IRType> innerType) {
    auto optType = std::make_shared<IROptionType>(innerType);

    auto it = optionTypes_.find(optType);  
    if (it != optionTypes_.end()) {
        return *it;
    }

    optionTypes_.insert(optType);
    return optType;
}

// ============================================================================
// IR Object Creation
// ============================================================================

BasicBlock* Module::createBasicBlock(const std::string& name, Function* parent) {
    auto* block = arena_.allocate<BasicBlock>(name, parent);
    if (parent) {
        parent->addBasicBlock(block);
    }
    return block;
}

Argument* Module::createArgument(std::shared_ptr<IRType> type, unsigned argNo, const std::string& name) {
    return arena_.allocate<Argument>(type, argNo, name);
}

// ========================================================================
// Instruction Factory Methods (Arena-Allocated)
// ========================================================================

// Binary operations
BinaryOperator* Module::createBinaryOp(Instruction::Opcode op, Value* lhs, Value* rhs, const std::string& name) {
    return arena_.allocate<BinaryOperator>(op, lhs, rhs, name);
}

UnaryOperator* Module::createUnaryOp(Instruction::Opcode op, Value* operand, const std::string& name) {
    return arena_.allocate<UnaryOperator>(op, operand, name);
}

CmpInst* Module::createCmp(Instruction::Opcode op, Value* lhs, Value* rhs, const std::string& name) {
    return arena_.allocate<CmpInst>(op, lhs, rhs, name);
}

// Memory operations
AllocaInst* Module::createAlloca(std::shared_ptr<IRType> type, const std::string& name) {
    return arena_.allocate<AllocaInst>(type, name);
}

LoadInst* Module::createLoad(Value* ptr, const std::string& name) {
    return arena_.allocate<LoadInst>(ptr, name);
}

StoreInst* Module::createStore(Value* value, Value* ptr) {
    return arena_.allocate<StoreInst>(value, ptr);
}

GCAllocInst* Module::createGCAlloc(std::shared_ptr<IRType> type, const std::string& name) {
    return arena_.allocate<GCAllocInst>(type, name);
}

ExtractValueInst* Module::createExtractValue(Value* structVal, unsigned fieldIndex, const std::string& name) {
    auto structType = structVal->getType();

    auto* irStructType = structType->asStruct();

    if (irStructType == nullptr) {
        throw std::runtime_error("ExtractValue: value is not a struct type");
    }

    auto fieldType = irStructType->getFieldTypeAtIdx(fieldIndex);

    return arena_.allocate<ExtractValueInst>(fieldType, fieldIndex, structVal, name);
}

InsertValueInst* Module::createInsertValue(Value* structVal, Value* newValue, unsigned fieldIndex, const std::string& name) {
    auto structType = structVal->getType();

    return arena_.allocate<InsertValueInst>(structType, structVal, newValue, fieldIndex, name);
}

// Array operations
ArrayNewInst* Module::createArrayNew(std::shared_ptr<IRType> elementType, Value* size, const std::string& name) {
    return arena_.allocate<ArrayNewInst>(elementType, size, name);
}

ArrayGetInst* Module::createArrayGet(Value* array, Value* index, const std::string& name) {
    return arena_.allocate<ArrayGetInst>(array, index, name);
}

ArraySetInst* Module::createArraySet(Value* array, Value* index, Value* value) {
    return arena_.allocate<ArraySetInst>(array, index, value);
}

ArrayLenInst* Module::createArrayLen(Value* array, const std::string& name) {
    return arena_.allocate<ArrayLenInst>(array, name);
}

ArraySliceInst* Module::createArraySlice(Value* array, Value* start, Value* end, const std::string& name) {
    return arena_.allocate<ArraySliceInst>(array, start, end, name);
}

// Type operations
CastInst* Module::createCast(Value* value, std::shared_ptr<IRType> targetType, const std::string& name) {
    return arena_.allocate<CastInst>(value, targetType, name);
}

OptionWrapInst* Module::createOptionWrap(Value* value, const std::string& name) {
    // Infer option type from the value's type
    auto optionType = getOptionType(value->getType());
    return arena_.allocate<OptionWrapInst>(value, optionType, name);
}

OptionUnwrapInst* Module::createOptionUnwrap(Value* option, const std::string& name) {
    return arena_.allocate<OptionUnwrapInst>(option, name);
}

OptionCheckInst* Module::createOptionCheck(Value* option, const std::string& name) {
    return arena_.allocate<OptionCheckInst>(option, name);
}

// Control flow (terminators)
ReturnInst* Module::createReturn(Value* value) {
    return arena_.allocate<ReturnInst>(value);
}

BranchInst* Module::createBranch(BasicBlock* target) {
    return arena_.allocate<BranchInst>(target);
}

CondBranchInst* Module::createCondBranch(Value* condition, BasicBlock* trueBlock, BasicBlock* falseBlock) {
    return arena_.allocate<CondBranchInst>(condition, trueBlock, falseBlock);
}

SwitchInst* Module::createSwitch(Value* value, BasicBlock* defaultBlock, const std::vector<SwitchInst::CaseEntry>& cases) {
    return arena_.allocate<SwitchInst>(value, defaultBlock, cases);
}

// Function calls
CallInst* Module::createCall(Function* callee, const std::vector<Value*>& args, const std::string& name) {
    return arena_.allocate<CallInst>(callee, args, name);
}

CallIndirectInst* Module::createCallIndirect(Value* callee, const std::vector<Value*>& args, const std::string& name) {
    return arena_.allocate<CallIndirectInst>(callee, args, name);
}

// SSA
PhiNode* Module::createPhi(std::shared_ptr<IRType> type,
                           const std::vector<PhiNode::IncomingValue>& incomingValues,
                           const std::string& name) {
    return arena_.allocate<PhiNode>(type, incomingValues, name);
}

// ============================================================================
// Constant Pooling
// ============================================================================

ConstantInt* Module::getConstantInt(int64_t value, std::shared_ptr<IRType> type) {
    auto it = intPool_->find(value);
    if (it != intPool_->end()) {
        return it->second;
    }

    auto* constant = arena_.allocate<ConstantInt>(value, type);
    (*intPool_)[value] = constant;
    return constant;
}

ConstantFloat* Module::getConstantFloat(double value, std::shared_ptr<IRType> type) {
    auto it = floatPool_->find(value);
    if (it != floatPool_->end()) {
        return it->second;
    }

    auto* constant = arena_.allocate<ConstantFloat>(value, type);
    (*floatPool_)[value] = constant;
    return constant;
}

ConstantBool* Module::getConstantBool(bool value, std::shared_ptr<IRType> type) {
    auto it = boolPool_->find(value);
    if (it != boolPool_->end()) {
        return it->second;
    }

    auto* constant = arena_.allocate<ConstantBool>(value, type);
    (*boolPool_)[value] = constant;
    return constant;
}

ConstantString* Module::getConstantString(const std::string& value, std::shared_ptr<IRType> type) {
    auto it = stringPool_->find(value);
    if (it != stringPool_->end()) {
        return it->second;
    }

    auto* constant = arena_.allocate<ConstantString>(value, type);
    (*stringPool_)[value] = constant;
    return constant;
}

// ============================================================================
// Verification
// ============================================================================

bool Module::verify(std::string* error) const {

    for (auto* func : functions_) {
        std::string funcError;
        if (!func->verify(&funcError)) {
            if (error) *error = "Function @" + func->getName() + ": " + funcError;
            return false;
        }
    }

    std::unordered_set<std::string> names;
    for (auto* func : functions_) {
        if (names.count(func->getName())) {
            if (error) *error = "Duplicate function name: " + func->getName();
            return false;
        }
        names.insert(func->getName());
    }

    for (auto* global : globals_) {
        if (global->getInitializer()) {
            // Verify initializer type matches variable type
            auto* init = global->getInitializer();
            if (!init->getType()->equals(global->getType().get())) {
                if (error) {
                    *error = "Global variable '" + global->getName() +
                             "' initializer type mismatch: expected " +
                             global->getType()->toString() + ", got " +
                             init->getType()->toString();
                }
                return false;
            }
        }
    }

    return true;
}

// ============================================================================
// Pretty Printing
// ============================================================================

std::string Module::toString() const {
    std::stringstream ss;
    ss << "; Module: " << name_ << "\n\n";
    
    
    if (!globals_.empty()) {
        ss << "; Global variables\n";
        for (auto* global : globals_) {
            ss << global->toString() << "\n";
        }
    }

   
    for (auto* func : functions_) {
        ss << func->toString() << "\n";
    }

    return ss.str();
}

std::string Module::toStringDetailed() const {
    std::stringstream ss;

    ss << "========================================\n";
    ss << "Module: " << name_ << "\n";
    ss << "========================================\n\n";

    ss << "Statistics:\n";
    ss << "  Functions: " << getNumFunctions() << "\n";
    ss << "  Globals: " << getNumGlobals() << "\n";
    ss << "  Basic Blocks: " << getTotalBasicBlocks() << "\n";
    ss << "  Instructions: " << getTotalInstructions() << "\n";
    ss << "  Arena Usage: " << getArenaUsage() << " bytes\n";
    ss << "  Arena Chunks: " << arena_.getNumChunks() << "\n\n";

    if (!globals_.empty()) {
        ss << "Global Variables:\n";
        for (auto* global : globals_) {
            ss << "  " << global->toString() << "\n";
        }
        ss << "\n";
    }

    if (!functions_.empty()) {
        ss << "Functions:\n";
        for (auto* func : functions_) {
            ss << "  @" << func->getName()
               << " (" << func->getNumBlocks() << " blocks, "
               << func->getNumInstructions() << " instructions)\n";
        }
        ss << "\n";
    }

    ss << toString();

    return ss.str();
}

bool Module::dumpToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    file << toString();
    return true;
}

// ============================================================================
// Optimization Support
// ============================================================================

void Module::runPass(Pass* pass) {
    pass->runOnModule(*this);
}

// ============================================================================
// Statistics
// ============================================================================

size_t Module::getTotalInstructions() const {
    size_t count = 0;
    for (auto* func : functions_) {
        count += func->getNumInstructions();
    }
    return count;
}

size_t Module::getTotalBasicBlocks() const {
    size_t count = 0;
    for (auto* func : functions_) {
        count += func->getNumBlocks();
    }
    return count;
}

} // namespace volta::ir
