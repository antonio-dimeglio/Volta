#pragma once

#include <vector>
#include <unordered_map>
#include <stdint.h>
#include <string>
#include <memory>
#include "FunctionInfo.hpp"
#include "gc/TypeRegistry.hpp"

namespace volta::vm {    
using byte = uint8_t;
using index = uint16_t; 

struct ModuleStats {
    size_t totalCodeSize;
    size_t functionCount;
    size_t intConstantCount;
    size_t floatConstantCount;
    size_t stringConstantCount;
    size_t totalMemoryUsage;  // Approximate
};

class BytecodeModule {
public:
    BytecodeModule() = default;
    // Constructor that pre-registers runtime functions
    static std::unique_ptr<BytecodeModule> createWithRuntime();
    // Clear all data (reuse module object)
    void clear();

    // === Constant Pools ===
    std::vector<int64_t> intConstants;
    std::unordered_map<int64_t, uint16_t> intMap;
    std::vector<double> floatConstants;
    std::unordered_map<double, uint16_t> floatMap;
    std::vector<std::string> stringConstants;
    std::unordered_map<std::string, uint16_t> stringMap;

    // === Function Table ===
    std::vector<FunctionInfo> functions;
    std::unordered_map<std::string, uint16_t> functionMap;

    // === Code Stream ===
    std::vector<byte> bytecode;

    // === Type medatadata ===
    std::unordered_map<uint32_t, Volta::GC::TypeInfo> typeTable_;

    index addFunction(
        std::string name,
        uint32_t codeOffset, 
        uint32_t codeLength, 
        byte paramLength,
        byte registerCount
    );

    index addNativeFunction(
        std::string name,
        RuntimeFunctionPtr nativeFn,
        byte paramCount
    );

    // Add function declaration (for two-pass compilation)
    // Registers the function name and params, but with placeholder bytecode info
    index addFunctionDeclaration(
        std::string name,
        byte paramCount
    );

    // Update function's bytecode info (after compilation)
    void updateFunctionCode(
        index funcIdx,
        uint32_t codeOffset,
        uint32_t codeLength,
        byte registerCount
    );

    // Function utils
    const FunctionInfo& getFunction(index idx) const;
    FunctionInfo& getFunction(index idx);
    index getFunctionIndex(std::string name) const;
    bool hasFunction(const std::string& name) const;
    size_t getFunctionCount() const;

    // Constant pool utils
    index addIntConstant(int64_t value);
    index addFloatConstant(double value);
    index addStringConstant(std::string value);
    int64_t getIntConstant(index idx) const;
    double getFloatConstant(index idx) const;
    const std::string& getStringConstant(index idx) const;
    size_t getIntConstantCount() const;
    size_t getFloatConstantCount() const;
    size_t getStringConstantCount() const;

    // Code emission utils
    void emitByte(byte val);
    void emitU16(uint16_t val);
    void emitI16(int16_t val);
    void emitU32(uint32_t val);
    uint32_t getCurrentOffset() const; // Where will the next byte written?

    // Fixup patching
    void patchU16(uint32_t offset, uint16_t value);
    void patchI16(uint32_t offset, int16_t value);
    uint8_t readByte(uint32_t offset) const;

    // Reading helpers
    uint8_t fetchByte(uint32_t& ip) const;
    uint16_t fetchU16(uint32_t& ip) const;
    int16_t fetchI16(uint32_t& ip) const;
    uint8_t peekByte(uint32_t ip) const;
    uint16_t peekU16(uint32_t ip) const;
    int16_t peekI16(uint32_t ip) const;


    // Validation function
    bool verify(std::string* error_msg = nullptr) const;
    bool isValidOffset(uint32_t offset) const;
    bool isValidFunctionIndex(uint16_t index) const;


    // Debugging
    // Print module summary (functions, constants, code size)
    void printSummary(std::ostream& out) const;
    ModuleStats getStats() const;
    void printFunctionTable(std::ostream& out) const;
    void printConstantPools(std::ostream& out) const;

    // Iterators
    const std::vector<FunctionInfo>& getFunctions() const { return functions; };
    auto begin() const { return functions.begin(); }
    auto end() const { return functions.end(); }

    // ========== TYPE METADATA (for GC) ==========

    /**
     * Register a type for GC tracing
     */
    void registerType(uint32_t typeId, const Volta::GC::TypeInfo& info);

    /**
     * Get type information for GC
     */
    const Volta::GC::TypeInfo* getTypeInfo(uint32_t typeId) const;

    /**
     * Check if a type is registered
     */
    bool hasType(uint32_t typeId) const;
    /* Future stuff
        void serialize(std::ostream& out) const;
        static std::unique_ptr<BytecodeModule> deserialize(std::istream& in);
        bool saveToFile(const std::string& filename) const;
        static std::unique_ptr<BytecodeModule> loadFromFile(const std::string& filename);
    */  
};

} // namespace volta::vm