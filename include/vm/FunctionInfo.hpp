#pragma once

#include <string>
#include <stdint.h>
#include <ostream>
#include "RuntimeFunction.hpp"

namespace volta::vm {

/**
 * FunctionInfo - Metadata for a function in the function table
 *
 * Represents either:
 *   1. Bytecode function (user-defined Volta code)
 *   2. Native function (C++ runtime function)
 *
 * Bytecode functions have code_offset/length and register allocation.
 * Native functions have a C++ function pointer.
 */
class FunctionInfo {
private:
    std::string name;
    uint32_t codeOffset;      // Byte offset in code stream (bytecode only)
    uint32_t codeLength;      // Function size in bytes (bytecode only)
    uint8_t paramCount;       // Number of parameters
    uint8_t registerCount;    // Registers used (bytecode only)
    bool native;            // Runtime (C++) vs bytecode
    RuntimeFunctionPtr nativePtr;  // C++ function pointer (native only)

public:
    // === Constructors ===

    /**
     * Constructor for bytecode functions
     */
    FunctionInfo(
        const std::string& name,
        uint32_t code_offset,
        uint32_t code_length,
        uint8_t param_count,
        uint8_t register_count
    ) : name(name),
        codeOffset(code_offset),
        codeLength(code_length),
        paramCount(param_count),
        registerCount(register_count),
        native(false),
        nativePtr(nullptr)
    {}

    /**
     * Constructor for native (runtime) functions
     */
    FunctionInfo(
        const std::string& name,
        RuntimeFunctionPtr native_fn,
        uint8_t param_count
    ) : name(name),
        codeOffset(0),
        codeLength(0),
        paramCount(param_count),
        registerCount(0),
        native(true),
        nativePtr(native_fn)
    {}

    // === Accessors ===

    const std::string& getName() const { return name; }
    uint32_t getCodeOffset() const { return codeOffset; }
    uint32_t getCodeLength() const { return codeLength; }
    uint8_t getParamCount() const { return paramCount; }
    uint8_t getRegisterCount() const { return registerCount; }
    bool isNative() const { return native; }
    RuntimeFunctionPtr getNativePtr() const { return nativePtr; }

    // === Setters for two-pass compilation ===
    void setCodeInfo(uint32_t offset, uint32_t length, uint8_t regCount) {
        codeOffset = offset;
        codeLength = length;
        registerCount = regCount;
    }

    // === Debug ===

    /**
     * Print function info in human-readable format
     */
    void print(std::ostream& out) const {
        (void)out;  // TODO: Implement
    }
};

} // namespace volta::vm
