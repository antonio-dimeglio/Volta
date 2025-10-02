#include "../include/vm/VM.hpp"
#include "../include/vm/GC.hpp"
#include <iostream>
#include <cstring>

namespace volta::vm {

// ========== Constructor/Destructor ==========

VM::VM(std::shared_ptr<bytecode::CompiledModule> module)
    : module_(module) {
    stack_.resize(MAX_STACK_SIZE);
    callStack_.resize(MAX_CALL_DEPTH);
    localStack_.resize(MAX_STACK_SIZE);  // Local stack needs space too
    globals_.resize(module->globalCount());

    // Initialize garbage collector
    GCConfig gcConfig;
    gcConfig.youngGenMaxSize = 2 * 1024 * 1024;  // 2MB young gen
    gcConfig.oldGenMaxSize = 16 * 1024 * 1024;   // 16MB old gen
    gcConfig.promotionAge = 3;
    gc_ = std::make_unique<GarbageCollector>(this, gcConfig);

    // Build foreign function and build native function table.
}

VM::~VM() {
    // GC destructor automatically cleans up all objects
}

// ========== Execution ==========

int VM::execute() {
    const auto* entryPoint = module_->getEntryPoint();
    if (!entryPoint) {
        // For a scripting language, it's okay to have no entry point
        // (e.g., if there's only function/struct definitions and no top-level code)
        return 0;
    }
    
    // Create initial call frame for main
    CallFrame frame;
    frame.functionIndex = entryPoint->index;
    frame.returnAddress = 0;
    frame.framePointer = 0;
    frame.localCount = entryPoint->localCount;
    
    callStack_[callStackTop_++] = frame;
    
    // Initialize locals to null
    for (uint32_t i = 0; i < entryPoint->localCount; i++) {
        localStack_[i] = Value::makeNull();
    }
    localStackTop_ = entryPoint->localCount;
    
    ip_ = 0;
    
    run();
    
    return hasError_ ? 1 : 0;
}

Value VM::executeFunction(const std::string& functionName) {
    // Find function by name
    const auto* func = module_->getFunction(functionName);
    if (!func) {
        runtimeError("Function not found: " + functionName);
        return Value::makeNull();
    }
    return executeFunction(func->index);
}

Value VM::executeFunction(uint32_t functionIndex) {
    const auto* function = module_->getFunction(functionIndex);
    if (!function) {
        runtimeError("Invalid function index");
        return Value::makeNull();
    }

    // Create call frame
    CallFrame frame;
    frame.functionIndex = functionIndex;
    frame.returnAddress = ip_;  // Save current IP
    frame.framePointer = localStackTop_;
    frame.localCount = function->localCount;

    callStack_[callStackTop_++] = frame;

    // Arguments are already on the eval stack (caller pushed them)
    // Transfer them to local stack as parameters
    uint32_t paramCount = function->parameterCount;
    for (uint32_t i = 0; i < paramCount; i++) {
        if (stackTop_ == 0) {
            runtimeError("Not enough arguments for function call");
            return Value::makeNull();
        }
    }

    // Check if we have enough space in local stack
    if (localStackTop_ + function->localCount > localStack_.size()) {
        runtimeError("Local stack overflow");
        return Value::makeNull();
    }

    // Pop arguments from eval stack and place in local stack (in reverse order)
    for (int i = paramCount - 1; i >= 0; i--) {
        localStack_[localStackTop_ + i] = pop();
    }

    // Initialize remaining locals to null
    for (uint32_t i = paramCount; i < function->localCount; i++) {
        localStack_[localStackTop_ + i] = Value::makeNull();
    }

    localStackTop_ += function->localCount;

    // Save old IP and set to function start
    size_t oldIP = ip_;
    ip_ = 0;

    // Execute the function
    run();

    // Restore IP
    ip_ = oldIP;

    // Get return value (should be on top of eval stack)
    if (stackTop_ > 0) {
        return pop();
    }

    return Value::makeNull();
}

// ========== Stack Manipulation ==========

void VM::push(const Value& value) {
    if (stackTop_ >= MAX_STACK_SIZE) {
        runtimeError("Stack overflow");
        // runtimeError throws exception, so we never reach here
    }
    stack_[stackTop_++] = value;
}

Value VM::pop() {
    if (stackTop_ == 0) {
        runtimeError("Stack underflow");
    }
    return stack_[--stackTop_];
}

const Value& VM::peek(size_t depth) const {
    if (stackTop_ == 0 || depth >= stackTop_) {
        static Value null = Value::makeNull();
        return null;
    }
    return stack_[stackTop_ - 1 - depth];
}

// ========== Foreign Function Interface ==========

void VM::registerNativeFunction(const std::string& name, NativeFunction func) {
    nativeFunctions_[name] = func;
    nativeFunctionTable_.push_back(func);
}

void VM::callNativeFunction(uint32_t foreignIndex, uint32_t argCount) {
    if (foreignIndex >= nativeFunctionTable_.size()) {
        runtimeError("Foreign function index out of bounds");
        return;
    }

    NativeFunction& nativeFunc = nativeFunctionTable_[foreignIndex];
    int returnCount = nativeFunc(*this);  // Pass VM reference so function can access stack

    // Native function is expected to have popped its arguments and pushed return values
    // returnCount indicates how many return values were pushed
}

// ========== Debugging & Introspection ==========

const bytecode::CompiledFunction* VM::getCurrentFunction() const {
    if (callStackTop_ == 0) {
        return nullptr;
    }
    uint32_t funcIndex = callStack_[callStackTop_ - 1].functionIndex;
    return module_->getFunction(funcIndex);
}

void VM::dumpStack(std::ostream& out) const {
    out << "=== Evaluation Stack ===\n";
    out << "Stack size: " << stackTop_ << "\n";

    if (stackTop_ == 0) {
        out << "(empty)\n";
        return;
    }

    for (size_t i = 0; i < stackTop_; i++) {
        out << "[" << i << "] ";
        const Value& val = stack_[i];

        switch (val.type) {
            case ValueType::Int:
                out << "Int(" << val.asInt << ")";
                break;
            case ValueType::Float:
                out << "Float(" << val.asFloat << ")";
                break;
            case ValueType::Bool:
                out << "Bool(" << (val.asBool ? "true" : "false") << ")";
                break;
            case ValueType::String:
                out << "String(#" << val.asStringIndex << ")";
                break;
            case ValueType::Object:
                out << "Object(" << val.asObject << ")";
                break;
            case ValueType::Null:
                out << "Null";
                break;
        }
        out << "\n";
    }
}

void VM::dumpCallStack(std::ostream& out) const {
    out << "=== Call Stack ===\n";
    out << "Call stack depth: " << callStackTop_ << "\n";

    if (callStackTop_ == 0) {
        out << "(empty)\n";
        return;
    }

    for (size_t i = 0; i < callStackTop_; i++) {
        const CallFrame& frame = callStack_[i];
        const auto* func = module_->getFunction(frame.functionIndex);

        out << "[" << i << "] ";
        if (func) {
            out << func->name << " (index=" << frame.functionIndex << ")";
        } else {
            out << "unknown (index=" << frame.functionIndex << ")";
        }
        out << " returnAddr=" << frame.returnAddress
            << " fp=" << frame.framePointer
            << " locals=" << frame.localCount
            << "\n";
    }
}

// ========== Core Execution Loop ==========

void VM::run() {
    // Todo: finish this.
    while (true) {
        if (debugTrace_) {
            // Print current instruction
        }
        
        if (!executeInstruction()) {
            break;  // Return or error
        }
    }
}

bool VM::executeInstruction() {
    uint8_t opcode = readByte();

    switch (static_cast<bytecode::Opcode>(opcode)) {
        // Constants
        case bytecode::Opcode::ConstInt: execConstInt(); break;
        case bytecode::Opcode::ConstFloat: execConstFloat(); break;
        case bytecode::Opcode::ConstBool: execConstBool(); break;
        case bytecode::Opcode::ConstString: execConstString(); break;
        case bytecode::Opcode::ConstNull: execConstNull(); break;

        // Arithmetic
        case bytecode::Opcode::AddInt: execAddInt(); break;
        case bytecode::Opcode::AddFloat: execAddFloat(); break;
        case bytecode::Opcode::SubInt: execSubInt(); break;
        case bytecode::Opcode::SubFloat: execSubFloat(); break;
        case bytecode::Opcode::MulInt: execMulInt(); break;
        case bytecode::Opcode::MulFloat: execMulFloat(); break;
        case bytecode::Opcode::DivInt: execDivInt(); break;
        case bytecode::Opcode::DivFloat: execDivFloat(); break;
        case bytecode::Opcode::ModInt: execModInt(); break;
        case bytecode::Opcode::NegInt: execNegInt(); break;
        case bytecode::Opcode::NegFloat: execNegFloat(); break;

        // Comparison
        case bytecode::Opcode::EqInt: execEqInt(); break;
        case bytecode::Opcode::EqFloat: execEqFloat(); break;
        case bytecode::Opcode::EqBool: execEqBool(); break;
        case bytecode::Opcode::NeInt: execNeInt(); break;
        case bytecode::Opcode::NeFloat: execNeFloat(); break;
        case bytecode::Opcode::NeBool: execNeBool(); break;
        case bytecode::Opcode::LtInt: execLtInt(); break;
        case bytecode::Opcode::LtFloat: execLtFloat(); break;
        case bytecode::Opcode::LeInt: execLeInt(); break;
        case bytecode::Opcode::LeFloat: execLeFloat(); break;
        case bytecode::Opcode::GtInt: execGtInt(); break;
        case bytecode::Opcode::GtFloat: execGtFloat(); break;
        case bytecode::Opcode::GeInt: execGeInt(); break;
        case bytecode::Opcode::GeFloat: execGeFloat(); break;

        // Logical
        case bytecode::Opcode::And: execAnd(); break;
        case bytecode::Opcode::Or: execOr(); break;
        case bytecode::Opcode::Not: execNot(); break;

        // Variables
        case bytecode::Opcode::LoadLocal: execLoadLocal(); break;
        case bytecode::Opcode::StoreLocal: execStoreLocal(); break;
        case bytecode::Opcode::LoadGlobal: execLoadGlobal(); break;
        case bytecode::Opcode::StoreGlobal: execStoreGlobal(); break;

        // Memory
        case bytecode::Opcode::Alloc: execAlloc(); break;
        case bytecode::Opcode::Load: execLoad(); break;
        case bytecode::Opcode::Store: execStore(); break;

        // Struct/Object
        case bytecode::Opcode::GetField: execGetField(); break;
        case bytecode::Opcode::SetField: execSetField(); break;

        // Array
        case bytecode::Opcode::NewArray: execNewArray(); break;
        case bytecode::Opcode::GetElement: execGetElement(); break;
        case bytecode::Opcode::SetElement: execSetElement(); break;
        case bytecode::Opcode::ArrayLength: execArrayLength(); break;

        // Control flow
        case bytecode::Opcode::Jump: execJump(); break;
        case bytecode::Opcode::JumpIfTrue: execJumpIfTrue(); break;
        case bytecode::Opcode::JumpIfFalse: execJumpIfFalse(); break;

        // Function calls
        case bytecode::Opcode::Call: execCall(); break;
        case bytecode::Opcode::CallForeign: execCallForeign(); break;
        case bytecode::Opcode::Return:
            execReturn();
            return callStackTop_ > 0;  // Continue if there are still frames
        case bytecode::Opcode::ReturnVoid:
            execReturnVoid();
            return callStackTop_ > 0;

        // Type conversions
        case bytecode::Opcode::IntToFloat: execIntToFloat(); break;
        case bytecode::Opcode::FloatToInt: execFloatToInt(); break;
        case bytecode::Opcode::IntToBool: execIntToBool(); break;

        // Utilities
        case bytecode::Opcode::Print: execPrint(); break;
        case bytecode::Opcode::Pop: execPop(); break;
        case bytecode::Opcode::Dup: execDup(); break;
        case bytecode::Opcode::Swap: execSwap(); break;
        case bytecode::Opcode::Halt:
            return false;

        default:
            runtimeError("Unknown opcode");
            return false;
    }

    return true;
}

// ========== Instruction Implementations ==========

void VM::execConstInt() {
    int64_t val = readInt64();
    push(Value::makeInt(val));
}

void VM::execConstFloat() {
    double val = readFloat64();
    push(Value::makeFloat(val));
}

void VM::execConstBool() {
    bool val = readBool();
    push(Value::makeBool(val));
}

void VM::execConstString() {
    uint32_t idx = readInt32();
    push(Value::makeString(idx));
}

void VM::execConstNull() {
    push(Value::makeNull());
}

void VM::execAddInt() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Int, "AddInt left operand");
    expectType(right, ValueType::Int, "AddInt right operand");
    
    int64_t res = left.asInt + right.asInt;
    push(Value::makeInt(res));
}

void VM::execAddFloat() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Float, "AddFloat left operand");
    expectType(right, ValueType::Float, "AddFloat right operand");
    
    double res = left.asFloat + right.asFloat;
    push(Value::makeFloat(res));
}

void VM::execSubInt() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Int, "SubInt left operand");
    expectType(right, ValueType::Int, "SubInt right operand");
    
    int64_t res = left.asInt - right.asInt;
    push(Value::makeInt(res));
}

void VM::execSubFloat() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Float, "SubFloat left operand");
    expectType(right, ValueType::Float, "SubFloat right operand");
    
    double res = left.asFloat - right.asFloat;
    push(Value::makeFloat(res));
}

void VM::execMulInt() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Int, "MulInt left operand");
    expectType(right, ValueType::Int, "MulInt right operand");
    
    int64_t res = left.asInt * right.asInt;
    push(Value::makeInt(res));
}

void VM::execMulFloat() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Float, "MulFloat left operand");
    expectType(right, ValueType::Float, "MulFloat right operand");
    
    double res = left.asFloat * right.asFloat;
    push(Value::makeFloat(res));
}

void VM::execDivInt() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Int, "DivInt left operand");
    expectType(right, ValueType::Int, "DivInt right operand");
    
    if (right.asInt == 0) {
        runtimeError("Division by zero");
        push(Value::makeInt(0));
        return;
    }
    
    int64_t res = left.asInt / right.asInt;
    push(Value::makeInt(res));
}

void VM::execDivFloat() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Float, "DivFloat left operand");
    expectType(right, ValueType::Float, "DivFloat right operand");
    
    double res = left.asFloat / right.asFloat;
    push(Value::makeFloat(res));
}

void VM::execModInt() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Int, "ModInt left operand");
    expectType(right, ValueType::Int, "ModInt right operand");
    
    int64_t res = left.asInt % right.asInt;
    push(Value::makeInt(res));
}

void VM::execNegInt() {
    Value val = pop();
    expectType(val, ValueType::Int, "NegInt left operand");
    push(Value::makeInt(-val.asInt));
}

void VM::execNegFloat() {
    Value val = pop();
    expectType(val, ValueType::Float, "NegFloat operand");
    push(Value::makeFloat(-val.asFloat));  // Bug fix: should be makeFloat, not makeInt
}

void VM::execEqInt() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Int, "EqInt left operand");
    expectType(right, ValueType::Int, "EqInt right operand");
    
    bool res = left.asInt == right.asInt;
    push(Value::makeBool(res));
}

void VM::execEqFloat() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Float, "EqFloat left operand");
    expectType(right, ValueType::Float, "EqFloat right operand");
    
    bool res = left.asFloat == right.asFloat;
    push(Value::makeBool(res));
}

void VM::execEqBool() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Bool, "EqBool left operand");
    expectType(right, ValueType::Bool, "EqBool right operand");
    
    bool res = left.asBool == right.asBool;
    push(Value::makeBool(res));
}

void VM::execNeInt() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Int, "NeInt left operand");
    expectType(right, ValueType::Int, "NeInt right operand");
    
    bool res = left.asInt != right.asInt;
    push(Value::makeBool(res));
}

void VM::execNeFloat() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Float, "NeFloat left operand");
    expectType(right, ValueType::Float, "NeFloat right operand");
    
    bool res = left.asFloat != right.asFloat;
    push(Value::makeBool(res));
}

void VM::execNeBool() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Bool, "NeBool left operand");
    expectType(right, ValueType::Bool, "NeBool right operand");
    
    bool res = left.asBool != right.asBool;
    push(Value::makeBool(res));
}

void VM::execLtInt() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Int, "LtInt left operand");
    expectType(right, ValueType::Int, "LtInt right operand");
    
    bool res = left.asInt < right.asInt;
    push(Value::makeBool(res));
}

void VM::execLtFloat() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Float, "LtFloat left operand");
    expectType(right, ValueType::Float, "LtFloat right operand");
    
    bool res = left.asFloat < right.asFloat;
    push(Value::makeBool(res));
}

void VM::execLeInt() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Int, "LeInt left operand");
    expectType(right, ValueType::Int, "LeInt right operand");
    
    bool res = left.asInt <= right.asInt;
    push(Value::makeBool(res));
}

void VM::execLeFloat() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Float, "LeFloat left operand");
    expectType(right, ValueType::Float, "LeFloat right operand");
    
    bool res = left.asFloat <= right.asFloat;
    push(Value::makeBool(res));
}

void VM::execGtInt() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Int, "GtInt left operand");
    expectType(right, ValueType::Int, "GtInt right operand");
    
    bool res = left.asInt > right.asInt;
    push(Value::makeBool(res));
}

void VM::execGtFloat() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Float, "GtFloat left operand");
    expectType(right, ValueType::Float, "GtFloat right operand");
    
    bool res = left.asFloat > right.asFloat;
    push(Value::makeBool(res));
}

void VM::execGeInt() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Int, "GeInt left operand");
    expectType(right, ValueType::Int, "GeInt right operand");
    
    bool res = left.asInt >= right.asInt;
    push(Value::makeBool(res));
}

void VM::execGeFloat() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Float, "GeFloat left operand");
    expectType(right, ValueType::Float, "GeFloat right operand");
    
    bool res = left.asFloat >= right.asFloat;
    push(Value::makeBool(res));
}

void VM::execAnd() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Bool, "And left operand");
    expectType(right, ValueType::Bool, "And right operand");
    
    bool res = left.asBool && right.asBool;
    push(Value::makeBool(res));
}

void VM::execOr() {
    Value right = pop();
    Value left = pop();
    
    expectType(left, ValueType::Bool, "Or left operand");
    expectType(right, ValueType::Bool, "Or right operand");
    
    bool res = left.asBool || right.asBool;
    push(Value::makeBool(res));
}

void VM::execNot() {
    Value val = pop();
    
    expectType(val, ValueType::Bool, "Not left operand");
    
    bool res = !val.asBool;
    push(Value::makeBool(res));
}

void VM::execLoadLocal() {
    int32_t localIdx = readInt32();

    // Bounds check
    if (callStackTop_ == 0) {
        runtimeError("LoadLocal: no active call frame");
        return;
    }

    const CallFrame& frame = callStack_[callStackTop_ - 1];
    if (localIdx < 0 || static_cast<uint32_t>(localIdx) >= frame.localCount) {
        runtimeError("LoadLocal: local index out of bounds");
        return;
    }

    // Calculate absolute index: framePointer + localIndex
    size_t absIdx = frame.framePointer + localIdx;

    push(localStack_[absIdx]);
}

void VM::execStoreLocal() {
    int32_t localIndex = readInt32();
    Value value = pop();

    // Bounds check
    if (callStackTop_ == 0) {
        runtimeError("StoreLocal: no active call frame");
        return;
    }

    const CallFrame& frame = callStack_[callStackTop_ - 1];
    if (localIndex < 0 || static_cast<uint32_t>(localIndex) >= frame.localCount) {
        runtimeError("StoreLocal: local index out of bounds");
        return;
    }

    size_t absoluteIndex = frame.framePointer + localIndex;

    localStack_[absoluteIndex] = value;
}

void VM::execLoadGlobal() {
    int32_t globalIndex = readInt32();
    push(globals_[globalIndex]);
}

void VM::execStoreGlobal() {
    int32_t globalIndex = readInt32();
    Value value = pop();
    globals_[globalIndex] = value;
}

void VM::execAlloc() {
    int32_t typeId = readInt32();
    int32_t size = readInt32();  // Size in bytes (or field count?)
    
    StructObject* obj = allocateStruct(typeId, size);
    push(Value::makeObject(obj));
}

void VM::execLoad() {
    // Load: dereferences a pointer and loads the value
    // Expects: offset (int32) from instruction, address (object) from stack
    // The offset allows indexing into memory regions
    int32_t offset = readInt32();
    Value address = pop();
    expectType(address, ValueType::Object, "Load");

    if (!address.asObject) {
        runtimeError("Null pointer dereference in Load");
        return;
    }

    // Determine object type and load appropriately
    ObjectHeader* header = static_cast<ObjectHeader*>(address.asObject);

    if (header->kind == ObjectHeader::ObjectKind::Struct) {
        StructObject* obj = static_cast<StructObject*>(address.asObject);
        // offset is the field index
        push(obj->fields[offset]);
    } else if (header->kind == ObjectHeader::ObjectKind::Array) {
        ArrayObject* arr = static_cast<ArrayObject*>(address.asObject);
        // offset is the element index
        if (offset < 0 || static_cast<uint32_t>(offset) >= arr->length) {
            runtimeError("Array index out of bounds in Load");
            return;
        }
        push(arr->elements[offset]);
    } else {
        runtimeError("Load: unsupported object kind");
    }
}

void VM::execStore() {
    // Store: writes a value to a dereferenced pointer
    // Expects: offset (int32) from instruction, value from stack, address (object) from stack
    int32_t offset = readInt32();
    Value value = pop();
    Value address = pop();
    expectType(address, ValueType::Object, "Store");

    if (!address.asObject) {
        runtimeError("Null pointer dereference in Store");
        return;
    }

    // Determine object type and store appropriately
    ObjectHeader* header = static_cast<ObjectHeader*>(address.asObject);

    if (header->kind == ObjectHeader::ObjectKind::Struct) {
        StructObject* obj = static_cast<StructObject*>(address.asObject);
        obj->fields[offset] = value;

        // Write barrier for GC
        if (value.type == ValueType::Object && value.asObject != nullptr) {
            gc_->writeBarrier(address.asObject, value.asObject);
        }
    } else if (header->kind == ObjectHeader::ObjectKind::Array) {
        ArrayObject* arr = static_cast<ArrayObject*>(address.asObject);
        if (offset < 0 || static_cast<uint32_t>(offset) >= arr->length) {
            runtimeError("Array index out of bounds in Store");
            return;
        }
        arr->elements[offset] = value;

        // Write barrier for GC
        if (value.type == ValueType::Object && value.asObject != nullptr) {
            gc_->writeBarrier(address.asObject, value.asObject);
        }
    } else {
        runtimeError("Store: unsupported object kind");
    }
}

void VM::execGetField() {
    int32_t fieldIndex = readInt32();
    Value object = pop();
    
    StructObject* obj = static_cast<StructObject*>(object.asObject);
    push(obj->fields[fieldIndex]);
}

void VM::execSetField() {
    int32_t fieldIndex = readInt32();
    Value value = pop();
    Value object = pop();

    StructObject* obj = static_cast<StructObject*>(object.asObject);
    obj->fields[fieldIndex] = value;

    // Write barrier for GC
    if (value.type == ValueType::Object && value.asObject != nullptr) {
        gc_->writeBarrier(object.asObject, value.asObject);
    }
}

void VM::execNewArray() {
    int32_t length = readInt32();

    if (length < 0) {
        runtimeError("Array length cannot be negative");
        return;
    }

    // Allocate the array
    ArrayObject* arr = allocateArray(static_cast<uint32_t>(length));

    // Pop elements from stack and initialize the array (in reverse order)
    // Elements are pushed in order, so we pop them in reverse
    for (int32_t i = length - 1; i >= 0; i--) {
        Value val = pop();
        arr->elements[i] = val;
        if (debugTrace_) {
            std::cout << "  NewArray[" << i << "] = " << val.asInt << std::endl;
        }
    }

    // Push the array reference onto the stack
    push(Value::makeObject(arr));
}

void VM::execGetElement() {
    Value index = pop();
    Value array = pop();

    expectType(array, ValueType::Object, "GetElement array");
    expectType(index, ValueType::Int, "GetElement index");

    ArrayObject* arr = static_cast<ArrayObject*>(array.asObject);
    if (!arr || arr->header.kind != ObjectHeader::ObjectKind::Array) {
        runtimeError("GetElement: not an array");
        return;
    }

    int64_t idx = index.asInt;
    if (idx < 0 || static_cast<uint32_t>(idx) >= arr->length) {
        runtimeError("Array index out of bounds in GetElement");
        return;
    }

    Value elem = arr->elements[idx];
    if (debugTrace_) {
        std::cout << "  GetElement[" << idx << "] = " << elem.asInt << " (type=" << static_cast<int>(elem.type) << ")" << std::endl;
    }
    push(elem);
}

void VM::execSetElement() {
    Value value = pop();
    Value index = pop();
    Value array = pop();

    expectType(array, ValueType::Object, "SetElement array");
    expectType(index, ValueType::Int, "SetElement index");

    ArrayObject* arr = static_cast<ArrayObject*>(array.asObject);
    if (!arr || arr->header.kind != ObjectHeader::ObjectKind::Array) {
        runtimeError("SetElement: not an array");
        return;
    }

    int64_t idx = index.asInt;
    if (idx < 0 || static_cast<uint32_t>(idx) >= arr->length) {
        runtimeError("Array index out of bounds in SetElement");
        return;
    }

    arr->elements[idx] = value;

    // Write barrier for GC
    if (value.type == ValueType::Object && value.asObject != nullptr) {
        gc_->writeBarrier(array.asObject, value.asObject);
    }
}

void VM::execArrayLength() {
    Value array = pop();
    expectType(array, ValueType::Object, "ArrayLength");

    ArrayObject* arr = static_cast<ArrayObject*>(array.asObject);
    if (!arr || arr->header.kind != ObjectHeader::ObjectKind::Array) {
        runtimeError("ArrayLength: not an array");
        return;
    }

    push(Value::makeInt(static_cast<int64_t>(arr->length)));
}

void VM::execJump() {
    int32_t offset = readInt32();   // Relative offset from current position
    size_t currentPos = ip_;         // Position after reading operand
    ip_ = currentPos + offset;       // Jump relative to current position
}

void VM::execJumpIfTrue() {
    int32_t offset = readInt32();   // Relative offset from current position
    size_t currentPos = ip_;         // Position after reading operand
    Value condition = pop();

    if (condition.isTruthy()) {
        ip_ = currentPos + offset;   // Jump relative to current position
    }
    // Otherwise, fall through (IP already past the offset operand)
}

void VM::execJumpIfFalse() {
    int32_t offset = readInt32();   // Relative offset from current position
    size_t currentPos = ip_;         // Position after reading operand
    Value condition = pop();

    if (!condition.isTruthy()) {
        ip_ = currentPos + offset;   // Jump relative to current position
    }
    // Otherwise, fall through (IP already past the offset operand)
}

void VM::execCall() {
    int32_t functionIndex = readInt32();
    int32_t argCount = readInt32();

    const auto* function = module_->getFunction(functionIndex);

    // Create new call frame
    CallFrame frame;
    frame.functionIndex = functionIndex;
    frame.returnAddress = ip_;  // Where to return to
    frame.framePointer = localStackTop_;  // Start of new frame's locals
    frame.localCount = function->localCount;

    callStack_[callStackTop_++] = frame;

    // Transfer arguments from eval stack to local stack
    // Arguments are the first N locals (parameters)
    for (int i = argCount - 1; i >= 0; i--) {
        localStack_[localStackTop_ + i] = pop();  // Pop in reverse order
    }

    // Initialize remaining locals to null
    for (uint32_t i = argCount; i < function->localCount; i++) {
        localStack_[localStackTop_ + i] = Value::makeNull();
    }

    localStackTop_ += function->localCount;

    // Jump to function start
    ip_ = 0;  // Functions always start at offset 0
}

void VM::execCallForeign() {
    int32_t foreignIndex = readInt32();
    int32_t argCount = readInt32();
    
    // Call the native function (it can access stack via VM&)
    NativeFunction& nativeFunc = nativeFunctionTable_[foreignIndex];
    int returnCount = nativeFunc(*this);  // Pass VM reference
}

void VM::execReturn() {
    Value returnValue = pop();  // Get return value

    if (debugTrace_) {
        std::cout << "  Return value: " << returnValue.asInt << " (type=" << static_cast<int>(returnValue.type) << ")" << std::endl;
    }

    // Get current frame before popping it
    CallFrame& currentFrame = callStack_[callStackTop_ - 1];

    // Restore locals (pop this frame's locals)
    localStackTop_ -= currentFrame.localCount;

    // Restore IP to return address
    ip_ = currentFrame.returnAddress;

    // Pop call frame
    callStackTop_--;

    // Push return value for caller
    push(returnValue);
}

void VM::execReturnVoid() {
    // Same as execReturn but no return value
    if (callStackTop_ > 0) {
        // Get current frame before popping it
        CallFrame& currentFrame = callStack_[callStackTop_ - 1];

        // Restore locals (pop this frame's locals)
        localStackTop_ -= currentFrame.localCount;

        // Restore IP to return address
        ip_ = currentFrame.returnAddress;

        // Pop call frame
        callStackTop_--;
    }
    // If callStackTop_ == 0, we've returned from main - execution ends
}

void VM::execIntToFloat() {
    Value val = pop();
    expectType(val, ValueType::Int, "IntToFloat");
    push(Value::makeFloat(static_cast<double>(val.asInt)));
}

void VM::execFloatToInt() {
    Value val = pop();
    expectType(val, ValueType::Float, "FloatToInt");
    push(Value::makeInt(static_cast<int64_t>(val.asFloat)));
}

void VM::execIntToBool() {
    Value val = pop();
    expectType(val, ValueType::Int, "IntToBool");
    push(Value::makeBool(val.asInt != 0));
}

void VM::execPrint() {
    Value val = pop();

    switch (val.type) {
        case ValueType::Int:
            std::cout << val.asInt;
            break;
        case ValueType::Float:
            std::cout << val.asFloat;
            break;
        case ValueType::Bool:
            std::cout << (val.asBool ? "true" : "false");
            break;
        case ValueType::String:
            if (val.asStringIndex < module_->stringPool().size()) {
                std::cout << module_->stringPool()[val.asStringIndex];
            } else {
                std::cout << "<invalid string index>";
            }
            break;
        case ValueType::Object: {
            ObjectHeader* header = static_cast<ObjectHeader*>(val.asObject);
            if (header->kind == ObjectHeader::ObjectKind::Array) {
                ArrayObject* arr = static_cast<ArrayObject*>(val.asObject);
                std::cout << "[";
                for (uint32_t i = 0; i < arr->length; i++) {
                    if (i > 0) std::cout << ", ";
                    // Recursively print elements (simplified, doesn't handle nested objects)
                    Value elem = arr->elements[i];
                    switch (elem.type) {
                        case ValueType::Int: std::cout << elem.asInt; break;
                        case ValueType::Float: std::cout << elem.asFloat; break;
                        case ValueType::Bool: std::cout << (elem.asBool ? "true" : "false"); break;
                        case ValueType::String:
                            if (elem.asStringIndex < module_->stringPool().size()) {
                                std::cout << module_->stringPool()[elem.asStringIndex];
                            }
                            break;
                        default: std::cout << "<object>"; break;
                    }
                }
                std::cout << "]";
            } else {
                std::cout << "<struct@" << val.asObject << ">";
            }
            break;
        }
        case ValueType::Null:
            std::cout << "null";
            break;
    }
    std::cout << "\n";
}

void VM::execHalt() {
    // Halt is handled in executeInstruction() by returning false
}

void VM::execPop() {
    if (stackTop_ > 0) {
        stackTop_--;  // Just discard the top value
    }
}

void VM::execDup() {
    if (stackTop_ == 0) {
        runtimeError("Stack underflow in Dup");
        return;
    }
    Value val = peek();
    push(val);
}

void VM::execSwap() {
    if (stackTop_ < 2) {
        runtimeError("Not enough values on stack for Swap");
        return;
    }
    Value top = pop();
    Value second = pop();
    push(top);
    push(second);
}

// ========== Memory Management ==========


StructObject* VM::allocateStruct(uint32_t typeId, uint32_t fieldCount) {
    // Delegate to GC (it handles all initialization)
    return gc_->allocateStruct(typeId, fieldCount);
}

ArrayObject* VM::allocateArray(uint32_t length) {
    // Delegate to GC (it handles all initialization)
    return gc_->allocateArray(length);
}

// ========== Helper Functions ==========

uint8_t VM::readByte() {
    return currentChunk().code_[ip_++];
}

int32_t VM::readInt32() {
    int32_t value;
    std::memcpy(&value, &currentChunk().code_[ip_], sizeof(value));
    ip_ += 4;
    return value;
}

int64_t VM::readInt64() {
    int64_t value;
    std::memcpy(&value, &currentChunk().code_[ip_], sizeof(value));
    ip_ += 8;
    return value;
}

double VM::readFloat64() {
    double value;
    std::memcpy(&value, &currentChunk().code_[ip_], sizeof(value));
    ip_ += 8;
    return value;
}

bool VM::readBool() {
    bool value;
    std::memcpy(&value, &currentChunk().code_[ip_], sizeof(value));
    ip_ += 1;
    return value;
}

const bytecode::Chunk& VM::currentChunk() const {
    if (callStackTop_ == 0) {
        static bytecode::Chunk empty;
        return empty;
    }
    uint32_t funcIndex = callStack_[callStackTop_ - 1].functionIndex;
    return module_->getFunction(funcIndex)->chunk;
}

void VM::runtimeError(const std::string& message) {
    hasError_ = true;
    errorMessage_ = message;

    // Print error with call stack trace
    std::cerr << "Runtime Error: " << message << "\n";
    if (callStackTop_ > 0) {
        std::cerr << "  at ";
        const auto* func = getCurrentFunction();
        if (func) {
            std::cerr << func->name << ":" << ip_;
        }
        std::cerr << "\n";
    }

    // Throw RuntimeError exception
    uint32_t funcIndex = callStackTop_ > 0 ? callStack_[callStackTop_ - 1].functionIndex : 0;
    uint32_t lineNumber = 0;  // TODO: Add line number tracking
    throw RuntimeError(message, funcIndex, ip_, lineNumber);
}

void VM::expectType(const Value& value, ValueType expected, const std::string& operation) {
    if (value.type != expected) {
        std::string msg = operation + ": expected ";
        switch (expected) {
            case ValueType::Int: msg += "Int"; break;
            case ValueType::Float: msg += "Float"; break;
            case ValueType::Bool: msg += "Bool"; break;
            case ValueType::String: msg += "String"; break;
            case ValueType::Object: msg += "Object"; break;
            case ValueType::Null: msg += "Null"; break;
        }
        msg += " but got ";
        switch (value.type) {
            case ValueType::Int: msg += "Int"; break;
            case ValueType::Float: msg += "Float"; break;
            case ValueType::Bool: msg += "Bool"; break;
            case ValueType::String: msg += "String"; break;
            case ValueType::Object: msg += "Object"; break;
            case ValueType::Null: msg += "Null"; break;
        }
        runtimeError(msg);
    }
}

} // namespace volta::vm
