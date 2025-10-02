#pragma once

#include "../bytecode/Bytecode.hpp"
#include "../bytecode/BytecodeCompiler.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <string>
#include <functional>

namespace volta::vm {

// ============================================================================
// Value Representation
// ============================================================================

/**
 * Runtime value type tags
 * Every value on the stack carries a type tag for dynamic type checking
 */
enum class ValueType : uint8_t {
    Null,       ///< Void/null value
    Int,        ///< 64-bit signed integer
    Float,      ///< 64-bit floating point
    Bool,       ///< Boolean (true/false)
    String,     ///< String reference (index into string pool)
    Object,     ///< Heap object reference (struct, array, etc.)
};

/**
 * Runtime value - tagged union for stack values
 * Represents a single value on the stack or in a local variable
 *
 * Note: For simplicity, we use a union. In the future, this could be optimized
 * with NaN-boxing or pointer tagging.
 */
struct Value {
    ValueType type;

    union {
        int64_t asInt;
        double asFloat;
        bool asBool;
        uint32_t asStringIndex;    ///< Index into string pool
        void* asObject;            ///< Pointer to heap object
    };

    /// Constructors for convenience
    static Value makeNull() {
        Value v;
        v.type = ValueType::Null;
        v.asInt = 0;
        return v;
    }

    static Value makeInt(int64_t val) {
        Value v;
        v.type = ValueType::Int;
        v.asInt = val;
        return v;
    }

    static Value makeFloat(double val) {
        Value v;
        v.type = ValueType::Float;
        v.asFloat = val;
        return v;
    }

    static Value makeBool(bool val) {
        Value v;
        v.type = ValueType::Bool;
        v.asBool = val;
        return v;
    }

    static Value makeString(uint32_t index) {
        Value v;
        v.type = ValueType::String;
        v.asStringIndex = index;
        return v;
    }

    static Value makeObject(void* obj) {
        Value v;
        v.type = ValueType::Object;
        v.asObject = obj;
        return v;
    }

    /// Helper to check if value is truthy (for conditional branches)
    bool isTruthy() const {
        switch (type) {
            case ValueType::Null: return false;
            case ValueType::Bool: return asBool;
            case ValueType::Int: return asInt != 0;
            case ValueType::Float: return asFloat != 0.0;
            case ValueType::String: return true;  // Non-null string is truthy
            case ValueType::Object: return asObject != nullptr;
        }
        return false;
    }
};

// ============================================================================
// Heap Objects
// ============================================================================

/**
 * Base class for heap-allocated objects
 * Includes header with type information and size
 *
 * Memory layout:
 * [ObjectHeader][payload data...]
 */
struct ObjectHeader {
    enum class ObjectKind : uint8_t {
        Struct,     ///< User-defined struct
        Array,      ///< Dynamic array
        String,     ///< String (future optimization, currently in pool)
    };

    ObjectKind kind;
    uint32_t size;          ///< Size in bytes (including header)
    uint32_t typeId;        ///< Type identifier (for structs)

    // Note: GC metadata (mark bit, generation, etc.) will go here when GC is added
};

/**
 * Struct object on the heap
 * Layout: [ObjectHeader][field0][field1]...[fieldN]
 */
struct StructObject {
    ObjectHeader header;
    Value fields[1];  // Flexible array member (actual size determined at allocation)
};

/**
 * Array object on the heap
 * Layout: [ObjectHeader][length][element0][element1]...[elementN]
 */
struct ArrayObject {
    ObjectHeader header;
    uint32_t length;
    Value elements[1];  // Flexible array member
};

// ============================================================================
// Call Stack Frame
// ============================================================================

/**
 * Call stack frame - represents one function call
 * Contains:
 * - Return address (bytecode offset in caller)
 * - Caller's function index
 * - Local variables (parameters + locals)
 * - Frame pointer to previous frame
 */
struct CallFrame {
    uint32_t functionIndex;     ///< Index of the currently executing function
    size_t returnAddress;       ///< Bytecode offset to return to in caller
    size_t framePointer;        ///< Base pointer for this frame's locals in localStack
    uint32_t localCount;        ///< Number of local variables in this frame
};

// ============================================================================
// Virtual Machine
// ============================================================================

/**
 * Volta Virtual Machine - stack-based bytecode interpreter
 *
 * Architecture:
 * - Evaluation stack: operand stack for expression evaluation
 * - Call stack: function call frames
 * - Local stack: local variables for all active frames
 * - Global array: global variables
 * - Heap: dynamically allocated objects
 *
 * Memory model (simplified, no GC for now):
 * - Objects are allocated via malloc/new
 * - No automatic deallocation (will leak until GC is implemented)
 * - Future: generational garbage collector
 */
class VM {
public:
    /**
     * Create a VM for executing a compiled module
     */
    explicit VM(std::shared_ptr<bytecode::CompiledModule> module);

    ~VM();

    // ========== Execution ==========

    /**
     * Execute the module's entry point (e.g., "main" function)
     * Returns the exit code (0 = success, non-zero = error)
     */
    int execute();

    /**
     * Execute a specific function by name
     * Arguments should be pushed onto the stack before calling
     * Returns the return value (or null for void functions)
     */
    Value executeFunction(const std::string& functionName);

    /**
     * Execute a specific function by index
     * Arguments should be pushed onto the stack before calling
     * Returns the return value (or null for void functions)
     */
    Value executeFunction(uint32_t functionIndex);

    // ========== Stack Manipulation (for FFI/testing) ==========

    /**
     * Push a value onto the evaluation stack
     */
    void push(const Value& value);

    /**
     * Pop a value from the evaluation stack
     */
    Value pop();

    /**
     * Peek at the top of the stack without popping
     */
    const Value& peek(size_t depth = 0) const;

    /**
     * Get current stack size
     */
    size_t stackSize() const { return stackTop_; }

    // ========== Foreign Function Interface ==========

    /**
     * Native function signature for foreign functions
     * Takes VM reference (to access stack), returns number of return values
     */
    using NativeFunction = std::function<int(VM&)>;

    /**
     * Register a native C++ function as a foreign function
     * The function can access arguments via pop() and return values via push()
     */
    void registerNativeFunction(const std::string& name, NativeFunction func);

    /**
     * Call a native function by index
     * Used internally by CallForeign bytecode instruction
     */
    void callNativeFunction(uint32_t foreignIndex, uint32_t argCount);

    // ========== Debugging & Introspection ==========

    /**
     * Enable/disable debug tracing (prints each instruction as it executes)
     */
    void setDebugTrace(bool enabled) { debugTrace_ = enabled; }

    /**
     * Get the current instruction pointer
     */
    size_t getIP() const { return ip_; }

    /**
     * Get the current function being executed
     */
    const bytecode::CompiledFunction* getCurrentFunction() const;

    /**
     * Print the current stack state (for debugging)
     */
    void dumpStack(std::ostream& out) const;

    /**
     * Print the current call stack (for debugging/error reporting)
     */
    void dumpCallStack(std::ostream& out) const;

    // ========== Error Handling ==========

    /**
     * Runtime error - thrown when VM encounters an error
     */
    struct RuntimeError {
        std::string message;
        uint32_t functionIndex;
        size_t instructionOffset;
        uint32_t lineNumber;

        RuntimeError(const std::string& msg, uint32_t funcIdx, size_t offset, uint32_t line)
            : message(msg), functionIndex(funcIdx), instructionOffset(offset), lineNumber(line) {}
    };

    /**
     * Check if the VM has encountered an error
     */
    bool hasError() const { return hasError_; }

    /**
     * Get the last error message
     */
    const std::string& getErrorMessage() const { return errorMessage_; }

private:
    // ========== Core Execution Loop ==========

    /**
     * Main execution loop - fetch, decode, execute
     * Returns when Return/ReturnVoid is encountered or error occurs
     */
    void run();

    /**
     * Execute a single instruction at current IP
     * Returns false if execution should stop (return/error)
     */
    bool executeInstruction();

    // ========== Instruction Implementations ==========

    void execConstInt();
    void execConstFloat();
    void execConstBool();
    void execConstString();
    void execConstNull();

    void execAddInt();
    void execAddFloat();
    void execSubInt();
    void execSubFloat();
    void execMulInt();
    void execMulFloat();
    void execDivInt();
    void execDivFloat();
    void execModInt();

    void execNegInt();
    void execNegFloat();

    void execEqInt();
    void execEqFloat();
    void execEqBool();
    void execNeInt();
    void execNeFloat();
    void execNeBool();
    void execLtInt();
    void execLtFloat();
    void execLeInt();
    void execLeFloat();
    void execGtInt();
    void execGtFloat();
    void execGeInt();
    void execGeFloat();

    void execAnd();
    void execOr();
    void execNot();

    void execLoadLocal();
    void execStoreLocal();
    void execLoadGlobal();
    void execStoreGlobal();

    void execAlloc();
    void execLoad();
    void execStore();

    void execGetField();
    void execSetField();

    void execNewArray();
    void execGetElement();
    void execSetElement();
    void execArrayLength();

    void execJump();
    void execJumpIfTrue();
    void execJumpIfFalse();

    void execCall();
    void execCallForeign();
    void execReturn();
    void execReturnVoid();

    void execIntToFloat();
    void execFloatToInt();
    void execIntToBool();

    void execPrint();
    void execHalt();

    void execPop();
    void execDup();
    void execSwap();

    // ========== Memory Management ==========

    /**
     * Allocate a struct object on the heap
     * Returns pointer to the allocated StructObject
     * Note: Currently no deallocation (GC will handle this in the future)
     */
    StructObject* allocateStruct(uint32_t typeId, uint32_t fieldCount);

    /**
     * Allocate an array object on the heap
     * Returns pointer to the allocated ArrayObject
     */
    ArrayObject* allocateArray(uint32_t length);

    /**
     * Free all heap objects (called in destructor)
     * Temporary until GC is implemented
     */
    void freeHeap();

    // ========== Helper Functions ==========

    /**
     * Read a byte from the current chunk at IP and advance IP
     */
    uint8_t readByte();

    /**
     * Read a 4-byte signed integer operand and advance IP
     */
    int32_t readInt32();

    /**
     * Read an 8-byte signed integer operand and advance IP
     */
    int64_t readInt64();

    /**
     * Read an 8-byte float operand and advance IP
     */
    double readFloat64();

    /**
     * Read a 1-byte boolean operand and advance IP
     */
    bool readBool();

    /**
     * Get the current bytecode chunk being executed
     */
    const bytecode::Chunk& currentChunk() const;

    /**
     * Runtime error handler - sets error state and optionally throws
     */
    void runtimeError(const std::string& message);

    /**
     * Type check helper - throws error if value is not the expected type
     */
    void expectType(const Value& value, ValueType expected, const std::string& operation);

    // ========== State ==========

    /// The compiled module being executed
    std::shared_ptr<bytecode::CompiledModule> module_;

    /// Evaluation stack for operands
    static constexpr size_t MAX_STACK_SIZE = 256 * 1024;  // 256K values
    std::vector<Value> stack_;
    size_t stackTop_ = 0;  ///< Index of next free slot

    /// Call stack for function calls
    static constexpr size_t MAX_CALL_DEPTH = 1024;
    std::vector<CallFrame> callStack_;
    size_t callStackTop_ = 0;

    /// Local variables (all frames share one contiguous array)
    std::vector<Value> localStack_;
    size_t localStackTop_ = 0;

    /// Global variables
    std::vector<Value> globals_;

    /// Heap objects (for cleanup on destruction)
    /// Note: This is temporary until GC is implemented
    std::vector<void*> heapObjects_;

    /// Instruction pointer (offset into current chunk)
    size_t ip_ = 0;

    /// Foreign function registry
    std::unordered_map<std::string, NativeFunction> nativeFunctions_;
    std::vector<NativeFunction> nativeFunctionTable_;  // Indexed access

    /// Error state
    bool hasError_ = false;
    std::string errorMessage_;

    /// Debug tracing
    bool debugTrace_ = false;
};

} // namespace volta::vm
