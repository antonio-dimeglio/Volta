#include "vm/BytecodeModule.hpp"
#include <stdexcept>

namespace volta::vm {

std::unique_ptr<BytecodeModule> BytecodeModule::createWithRuntime() {
    auto module = std::make_unique<BytecodeModule>();

    // Register runtime funcs

    return module;
}

void BytecodeModule::clear() {
    intConstants.clear();
    floatConstants.clear();
    stringConstants.clear();

    intMap.clear();
    floatMap.clear();
    stringMap.clear();

    functions.clear();
    functionMap.clear();

    bytecode.clear();
}

index BytecodeModule::addFunction(
    std::string name,
    uint32_t codeOffset, 
    uint32_t codeLength, 
    byte paramLength,
    byte registerCount
) {
    auto fn = FunctionInfo(name, codeOffset, codeLength, paramLength, registerCount);
    size_t idx = functions.size();
    functions.push_back(fn);
    functionMap[name] = idx;
    return idx;
}
index BytecodeModule::addNativeFunction(
    std::string name,
    RuntimeFunctionPtr nativeFn,
    byte paramCount
) {
    auto fn = FunctionInfo(name, nativeFn, paramCount);
    size_t idx = functions.size();
    functions.push_back(fn);
    functionMap[name] = idx;
    return idx;
}

index BytecodeModule::addFunctionDeclaration(
    std::string name,
    byte paramCount
) {
    // Add function with placeholder bytecode info (offset=0, length=0, regCount=0)
    auto fn = FunctionInfo(name, 0, 0, paramCount, 0);
    size_t idx = functions.size();
    functions.push_back(fn);
    functionMap[name] = idx;
    return idx;
}

void BytecodeModule::updateFunctionCode(
    index funcIdx,
    uint32_t codeOffset,
    uint32_t codeLength,
    byte registerCount
) {
    if (funcIdx >= functions.size()) {
        throw std::runtime_error("Invalid function index for update");
    }
    functions[funcIdx].setCodeInfo(codeOffset, codeLength, registerCount);
}

const FunctionInfo& BytecodeModule::getFunction(index idx) const {
    if (idx >= functions.size()) throw std::runtime_error("Tried to access illegal function idx.");
    return functions[idx];
}

FunctionInfo& BytecodeModule::getFunction(index idx) {
    if (idx >= functions.size()) throw std::runtime_error("Tried to access illegal function idx.");
    return functions[idx];
}

index BytecodeModule::getFunctionIndex(std::string name) const {
    auto it = functionMap.find(name);
    if (it == functionMap.end()) throw std::runtime_error("Tried to access undefined function name.");

    return it->second;
}

bool BytecodeModule::hasFunction(const std::string& name) const {
    return functionMap.count(name) > 0;
}

size_t BytecodeModule::getFunctionCount() const {
    return functions.size();
}

index BytecodeModule::addIntConstant(int64_t value) {
    auto it = intMap.find(value);
    if (it != intMap.end()) return it->second;
    size_t idx = intConstants.size();
    intConstants.push_back(value);
    intMap[value] = idx;
    return idx;
}

index BytecodeModule::addFloatConstant(double value) {
    auto it = floatMap.find(value);
    if (it != floatMap.end()) return it->second;
    size_t idx = floatConstants.size();
    floatConstants.push_back(value);
    floatMap[value] = idx;
    return idx;
}

index BytecodeModule::addStringConstant(std::string value) {
    auto it = stringMap.find(value);
    if (it != stringMap.end()) return it->second;
    size_t idx = stringConstants.size();
    stringConstants.push_back(value);
    stringMap[value] = idx;
    return idx;
}

int64_t BytecodeModule::getIntConstant(index idx) const {
    if (idx >= intConstants.size()) throw std::runtime_error("Tried to access illegal intConstants idx.");
    return intConstants[idx];
}

double BytecodeModule::getFloatConstant(index idx) const {
    if (idx >= floatConstants.size()) throw std::runtime_error("Tried to access illegal floatConstants idx.");
    return floatConstants[idx];
}

const std::string& BytecodeModule::getStringConstant(index idx) const {
    if (idx >= stringConstants.size()) throw std::runtime_error("Tried to access illegal stringConstants idx.");
    return stringConstants[idx];
}

size_t BytecodeModule::getIntConstantCount() const {
    return intConstants.size();
}

size_t BytecodeModule::getFloatConstantCount() const {
    return floatConstants.size();
}

size_t BytecodeModule::getStringConstantCount() const {
    return stringConstants.size();
}

void BytecodeModule::emitByte(byte val) {
    bytecode.push_back(val);
}

void BytecodeModule::emitU16(uint16_t val) {
    bytecode.push_back(val & 0xFF);
    bytecode.push_back((val >> 8) & 0xFF);
}

void BytecodeModule::emitI16(int16_t val) {
    bytecode.push_back(val & 0xFF);
    bytecode.push_back((val >> 8) & 0xFF);
}

void BytecodeModule::emitU32(uint32_t val) {
    bytecode.push_back(val & 0xFF);
    bytecode.push_back((val >> 8) & 0xFF);
    bytecode.push_back((val >> 16) & 0xFF);
    bytecode.push_back((val >> 24) & 0xFF);
}

uint32_t BytecodeModule::getCurrentOffset() const {
    return bytecode.size();
}

void BytecodeModule::patchU16(uint32_t offset, uint16_t value) {
    bytecode[offset] = value & 0xFF;           // Low byte
    bytecode[offset + 1] = (value >> 8) & 0xFF; // High byte
}

void BytecodeModule::patchI16(uint32_t offset, int16_t value) {
    patchU16(offset, static_cast<uint16_t>(value));
}

uint8_t BytecodeModule::readByte(uint32_t offset) const {
    return bytecode[offset];
}

uint8_t BytecodeModule::fetchByte(uint32_t& ip) const {
    return bytecode[ip++];
}

uint16_t BytecodeModule::fetchU16(uint32_t& ip) const {
    uint8_t low = bytecode[ip++];
    uint8_t high = bytecode[ip++];
    return (uint16_t) low | ((uint16_t)high << 8);
}

int16_t BytecodeModule::fetchI16(uint32_t& ip) const {
    uint16_t unsigned_val = fetchU16(ip);
    return static_cast<int16_t>(unsigned_val);
}

uint8_t BytecodeModule::peekByte(uint32_t ip) const {
    return bytecode[ip];
}

uint16_t BytecodeModule::peekU16(uint32_t ip) const {
    uint16_t b1 = bytecode[ip];
    uint16_t b2 = bytecode[ip + 1];
    return static_cast<uint16_t>(b1 | (b2 << 8));
}

int16_t BytecodeModule::peekI16(uint32_t ip) const {
    uint16_t b1 = bytecode[ip];
    uint16_t b2 = bytecode[ip + 1];
    return static_cast<int16_t>(b1 | (b2 << 8));
}

bool BytecodeModule::verify(std::string* error_msg) const {
    // Helper macro for setting error message
    auto set_error = [error_msg](const std::string& msg) {
        if (error_msg) *error_msg = msg;
    };

    // 1. Check each bytecode function
    for (size_t i = 0; i < functions.size(); i++) {
        const FunctionInfo& func = functions[i];
        
        // Skip native functions (they don't have code offsets)
        if (func.isNative()) {
            continue;
        }

        // Check code offset is valid
        if (!isValidOffset(func.getCodeOffset())) {
            set_error("Function '" + func.getName() + 
                     "' has invalid code offset: " + 
                     std::to_string(func.getCodeOffset()));
            return false;
        }

        // Check code doesn't exceed buffer
        uint32_t end_offset = func.getCodeOffset() + func.getCodeLength();
        if (end_offset > bytecode.size()) {
            set_error("Function '" + func.getName() + 
                     "' code exceeds buffer bounds");
            return false;
        }
    }

    // 2. Check function_map consistency
    for (const auto& [name, index] : functionMap) {
        // Check index is valid
        if (!isValidFunctionIndex(index)) {
            set_error("Function map has invalid index for '" + name + "'");
            return false;
        }

        // Check name matches
        if (functions[index].getName() != name) {
            set_error("Function map name mismatch for '" + name + "'");
            return false;
        }
    }

    // 3. Check function_map size matches functions size
    if (functionMap.size() != functions.size()) {
        set_error("Function map size doesn't match function count");
        return false;
    }

    return true;
}

bool BytecodeModule::isValidOffset(uint32_t offset) const {
    return offset < bytecode.size();
}

bool BytecodeModule::isValidFunctionIndex(uint16_t index) const {
    return index < functions.size();
}

void BytecodeModule::printSummary(std::ostream& out) const {
    out << "╔════════════════════════════════════════════╗\n";
    out << "║     Bytecode Module Summary                ║\n";
    out << "╚════════════════════════════════════════════╝\n";
    out << "  Functions:        " << functions.size() << "\n";
    out << "  Code size:        " << bytecode.size() << " bytes\n";
    out << "  Int constants:    " << intConstants.size() << "\n";
    out << "  Float constants:  " << floatConstants.size() << "\n";
    out << "  String constants: " << stringConstants.size() << "\n";
    out << "════════════════════════════════════════════\n";
}

ModuleStats BytecodeModule::getStats() const {
    ModuleStats stats;
    stats.totalCodeSize = bytecode.size();
    stats.functionCount = functions.size();
    stats.intConstantCount = intConstants.size();
    stats.floatConstantCount = floatConstants.size();
    stats.stringConstantCount = stringConstants.size();

    // Approximate memory usage calculation
    stats.totalMemoryUsage =
        bytecode.size() +                                           // Code bytes
        intConstants.size() * sizeof(int64_t) +                    // Int pool
        floatConstants.size() * sizeof(double) +                   // Float pool
        functions.size() * sizeof(FunctionInfo);                   // Function table

    // Add string pool size (each string's actual bytes)
    for (const auto& str : stringConstants) {
        stats.totalMemoryUsage += str.size();
    }

    return stats;
}

void BytecodeModule::printFunctionTable(std::ostream& out) const {
    out << "╔════════════════════════════════════════════════════════════════════╗\n";
    out << "║                         Function Table                             ║\n";
    out << "╠════╦══════════════════════╦══════════╦══════════╦════════╦═════════╣\n";
    out << "║ ID ║ Name                 ║ Type     ║ Offset   ║ Length ║ Params  ║\n";
    out << "╠════╬══════════════════════╬══════════╬══════════╬════════╬═════════╣\n";

    for (size_t i = 0; i < functions.size(); i++) {
        const FunctionInfo& func = functions[i];

        char buffer[256];
        if (func.isNative()) {
            snprintf(buffer, sizeof(buffer),
                     "║ %-2zu ║ %-20s ║ Native   ║ -------- ║ ------ ║ %-7d ║",
                     i,
                     func.getName().c_str(),
                     (int)func.getParamCount());
        } else {
            snprintf(buffer, sizeof(buffer),
                     "║ %-2zu ║ %-20s ║ Bytecode ║ %08x ║ %6u ║ %-7d ║",
                     i,
                     func.getName().c_str(),
                     func.getCodeOffset(),
                     func.getCodeLength(),
                     (int)func.getParamCount());
        }

        out << buffer << "\n";
    }

    out << "╚════╩══════════════════════╩══════════╩══════════╩════════╩═════════╝\n";
}

void BytecodeModule::printConstantPools(std::ostream& out) const {
    out << "╔════════════════════════════════════════════╗\n";
    out << "║           Constant Pools                   ║\n";
    out << "╚════════════════════════════════════════════╝\n";

    // Int constants
    out << "\n── Int Constants (" << intConstants.size() << ") ──\n";
    for (size_t i = 0; i < intConstants.size(); i++) {
        out << "  [" << i << "] = " << intConstants[i] << "\n";
    }

    // Float constants
    out << "\n── Float Constants (" << floatConstants.size() << ") ──\n";
    for (size_t i = 0; i < floatConstants.size(); i++) {
        out << "  [" << i << "] = " << floatConstants[i] << "\n";
    }

    // String constants
    out << "\n── String Constants (" << stringConstants.size() << ") ──\n";
    for (size_t i = 0; i < stringConstants.size(); i++) {
        out << "  [" << i << "] = \"" << stringConstants[i] << "\"\n";
    }

    out << "════════════════════════════════════════════\n";
}

// ========== TYPE METADATA ==========

void BytecodeModule::registerType(uint32_t typeId, const Volta::GC::TypeInfo& info) {
    typeTable_[typeId] = info;
}

const Volta::GC::TypeInfo* BytecodeModule::getTypeInfo(uint32_t typeId) const {
    auto it = typeTable_.find(typeId);
    if (it == typeTable_.end()) {
        return nullptr;
    }
    return &it->second;
}

bool BytecodeModule::hasType(uint32_t typeId) const {
    return typeTable_.find(typeId) != typeTable_.end();
}


} // namespace volta::vm
