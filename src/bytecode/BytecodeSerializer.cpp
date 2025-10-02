#include "../include/bytecode/BytecodeSerializer.hpp"
#include <fstream>

namespace volta::bytecode {

// ========== Serialization (Save) ==========

bool BytecodeSerializer::serialize(const CompiledModule& module, std::ostream& out) {
    hasError_ = false;
    errorMessage_.clear();

    writeHeader(module, out);
    if (hasError_) return false;

    writeStringPool(module, out);
    if (hasError_) return false;

    writeForeignFunctions(module, out);
    if (hasError_) return false;

    writeGlobals(module, out);
    if (hasError_) return false;

    writeFunctions(module, out);
    if (hasError_) return false;

    return !hasError_;
}

bool BytecodeSerializer::serializeToFile(const CompiledModule& module, const std::string& filepath) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file) {
        setError("Failed to open file for writing: " + filepath);
        return false;
    }

    bool result = serialize(module, file);
    file.close();
    return result;
}

// ========== Deserialization (Load) ==========

std::unique_ptr<CompiledModule> BytecodeSerializer::deserialize(std::istream& in) {
    hasError_ = false;
    errorMessage_.clear();

    // Read header and get module name
    std::string moduleName;
    if (!readHeader(in, moduleName)) {
        return nullptr;
    }

    // Create module
    auto module = std::make_unique<CompiledModule>(moduleName);

    // Read sections
    if (!readStringPool(in, *module)) return nullptr;
    if (!readForeignFunctions(in, *module)) return nullptr;
    if (!readGlobals(in, *module)) return nullptr;
    if (!readFunctions(in, *module)) return nullptr;

    // Verify module integrity
    if (!verifyModule(*module)) {
        setError("Module verification failed");
        return nullptr;
    }

    return module;
}

std::unique_ptr<CompiledModule> BytecodeSerializer::deserializeFromFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        setError("Failed to open file for reading: " + filepath);
        return nullptr;
    }

    auto module = deserialize(file);
    file.close();
    return module;
}

// ========== Serialization Helpers ==========

void BytecodeSerializer::writeHeader(const CompiledModule& module, std::ostream& out) {
    // Magic number
    writeUInt32(out, MAGIC_NUMBER);

    // Version
    writeUInt32(out, (VERSION_MAJOR << 16) | VERSION_MINOR);

    // Module name
    writeString(out, module.name());
}

void BytecodeSerializer::writeStringPool(const CompiledModule& module, std::ostream& out) {
    const auto& pool = module.stringPool();
    writeUInt32(out, pool.size());

    for (const auto& str : pool) {
        writeString(out, str);
    }
}

void BytecodeSerializer::writeForeignFunctions(const CompiledModule& module, std::ostream& out) {
    const auto& foreignFunctions = module.foreignFunctions();
    writeUInt32(out, foreignFunctions.size());

    for (const auto& name : foreignFunctions) {
        writeString(out, name);
    }
}

void BytecodeSerializer::writeGlobals(const CompiledModule& module, std::ostream& out) {
    writeUInt32(out, module.globalCount());
}

void BytecodeSerializer::writeFunctions(const CompiledModule& module, std::ostream& out) {
    const auto& functions = module.functions();
    writeUInt32(out, functions.size());

    for (const auto& function : functions) {
        writeFunction(function, out);
    }
}

void BytecodeSerializer::writeFunction(const CompiledFunction& function, std::ostream& out) {
    writeString(out, function.name);
    writeUInt32(out, function.index);
    writeUInt32(out, function.parameterCount);
    writeUInt32(out, function.localCount);
    writeUInt8(out, function.isForeign ? 1 : 0);
    writeChunk(function.chunk, out);
}

void BytecodeSerializer::writeChunk(const Chunk& chunk, std::ostream& out) {
    const auto& code = chunk.code();
    writeUInt32(out, code.size());
    writeBytes(out, code.data(), code.size());
}

// ========== Deserialization Helpers ==========

bool BytecodeSerializer::readHeader(std::istream& in, std::string& moduleName) {
    // Read magic number
    uint32_t magic;
    if (!readUInt32(in, magic)) {
        setError("Failed to read magic number");
        return false;
    }

    if (magic != MAGIC_NUMBER) {
        setError("Invalid magic number");
        return false;
    }

    // Read version
    uint32_t version;
    if (!readUInt32(in, version)) {
        setError("Failed to read version");
        return false;
    }

    uint16_t major = version >> 16;
    uint16_t minor = version & 0xFFFF;

    if (major != VERSION_MAJOR) {
        setError("Incompatible version");
        return false;
    }

    // Read module name
    if (!readString(in, moduleName)) {
        setError("Failed to read module name");
        return false;
    }

    return true;
}

bool BytecodeSerializer::readStringPool(std::istream& in, CompiledModule& module) {
    uint32_t count;
    if (!readUInt32(in, count)) {
        setError("Failed to read string pool count");
        return false;
    }

    auto& pool = module.stringPool();
    pool.reserve(count);

    for (uint32_t i = 0; i < count; ++i) {
        std::string str;
        if (!readString(in, str)) {
            setError("Failed to read string from pool");
            return false;
        }
        pool.push_back(std::move(str));
    }

    return true;
}

bool BytecodeSerializer::readForeignFunctions(std::istream& in, CompiledModule& module) {
    uint32_t count;
    if (!readUInt32(in, count)) {
        setError("Failed to read foreign function count");
        return false;
    }

    auto& foreignFunctions = module.foreignFunctions();
    foreignFunctions.reserve(count);

    for (uint32_t i = 0; i < count; ++i) {
        std::string name;
        if (!readString(in, name)) {
            setError("Failed to read foreign function name");
            return false;
        }
        foreignFunctions.push_back(std::move(name));
    }

    return true;
}

bool BytecodeSerializer::readGlobals(std::istream& in, CompiledModule& module) {
    uint32_t count;
    if (!readUInt32(in, count)) {
        setError("Failed to read global count");
        return false;
    }

    module.setGlobalCount(count);
    return true;
}

bool BytecodeSerializer::readFunctions(std::istream& in, CompiledModule& module) {
    uint32_t count;
    if (!readUInt32(in, count)) {
        setError("Failed to read function count");
        return false;
    }

    auto& functions = module.functions();
    functions.reserve(count);

    for (uint32_t i = 0; i < count; ++i) {
        CompiledFunction function;
        if (!readFunction(in, function)) {
            return false;
        }
        functions.push_back(std::move(function));
    }

    return true;
}

bool BytecodeSerializer::readFunction(std::istream& in, CompiledFunction& function) {
    if (!readString(in, function.name)) {
        setError("Failed to read function name");
        return false;
    }

    if (!readUInt32(in, function.index)) {
        setError("Failed to read function index");
        return false;
    }

    if (!readUInt32(in, function.parameterCount)) {
        setError("Failed to read parameter count");
        return false;
    }

    if (!readUInt32(in, function.localCount)) {
        setError("Failed to read local count");
        return false;
    }

    uint8_t isForeign;
    if (!readUInt8(in, isForeign)) {
        setError("Failed to read isForeign flag");
        return false;
    }
    function.isForeign = (isForeign != 0);

    if (!readChunk(in, function.chunk)) {
        return false;
    }

    return true;
}

bool BytecodeSerializer::readChunk(std::istream& in, Chunk& chunk) {
    uint32_t size;
    if (!readUInt32(in, size)) {
        setError("Failed to read chunk size");
        return false;
    }

    chunk.code_.resize(size);
    if (!readBytes(in, chunk.code_.data(), size)) {
        setError("Failed to read chunk data");
        return false;
    }

    return true;
}

// ========== I/O Primitives ==========

void BytecodeSerializer::writeUInt8(std::ostream& out, uint8_t value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void BytecodeSerializer::writeUInt32(std::ostream& out, uint32_t value) {
    // Write in little-endian format
    uint8_t bytes[4];
    bytes[0] = value & 0xFF;
    bytes[1] = (value >> 8) & 0xFF;
    bytes[2] = (value >> 16) & 0xFF;
    bytes[3] = (value >> 24) & 0xFF;
    out.write(reinterpret_cast<const char*>(bytes), 4);
}

void BytecodeSerializer::writeString(std::ostream& out, const std::string& str) {
    writeUInt32(out, str.size());
    out.write(str.data(), str.size());
}

void BytecodeSerializer::writeBytes(std::ostream& out, const void* data, size_t size) {
    out.write(reinterpret_cast<const char*>(data), size);
}

bool BytecodeSerializer::readUInt8(std::istream& in, uint8_t& value) {
    in.read(reinterpret_cast<char*>(&value), sizeof(value));
    return in.good();
}

bool BytecodeSerializer::readUInt32(std::istream& in, uint32_t& value) {
    // Read in little-endian format
    uint8_t bytes[4];
    in.read(reinterpret_cast<char*>(bytes), 4);
    if (!in.good()) return false;

    value = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
    return true;
}

bool BytecodeSerializer::readString(std::istream& in, std::string& str) {
    uint32_t length;
    if (!readUInt32(in, length)) return false;

    str.resize(length);
    in.read(&str[0], length);
    return in.good();
}

bool BytecodeSerializer::readBytes(std::istream& in, void* data, size_t size) {
    in.read(reinterpret_cast<char*>(data), size);
    return in.good();
}

// ========== Validation ==========

bool BytecodeSerializer::verifyModule(const CompiledModule& module) {
    // Basic validation checks

    // Check that all functions have valid indices
    for (size_t i = 0; i < module.functions().size(); ++i) {
        const auto& func = module.functions()[i];
        if (func.index != i) {
            return false;
        }
    }

    // All checks passed
    return true;
}

// ========== Error Handling ==========

void BytecodeSerializer::setError(const std::string& message) {
    hasError_ = true;
    errorMessage_ = message;
}

// ========== Utility Functions ==========

bool saveCompiledModule(const CompiledModule& module, const std::string& filepath) {
    BytecodeSerializer serializer;
    return serializer.serializeToFile(module, filepath);
}

std::unique_ptr<CompiledModule> loadCompiledModule(const std::string& filepath) {
    BytecodeSerializer serializer;
    return serializer.deserializeFromFile(filepath);
}

} // namespace volta::bytecode
