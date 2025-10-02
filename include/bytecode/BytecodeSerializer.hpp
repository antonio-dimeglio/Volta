#pragma once

#include "BytecodeCompiler.hpp"
#include <ostream>
#include <istream>
#include <string>
#include <memory>

namespace volta::bytecode {

/**
 * Bytecode serializer - save/load compiled modules to/from binary format
 *
 * Binary format (simplified version, similar to Java .class or Python .pyc):
 *
 * Header:
 *   - Magic number (4 bytes): "VLTB" (Volta Bytecode)
 *   - Version (4 bytes): major.minor
 *   - Module name length (4 bytes) + name (UTF-8)
 *
 * String pool:
 *   - Count (4 bytes)
 *   - For each string: length (4 bytes) + data (UTF-8)
 *
 * Foreign functions:
 *   - Count (4 bytes)
 *   - For each: length (4 bytes) + name (UTF-8)
 *
 * Globals:
 *   - Count (4 bytes)
 *
 * Functions:
 *   - Count (4 bytes)
 *   - For each function:
 *     - Name length (4 bytes) + name (UTF-8)
 *     - Index (4 bytes)
 *     - Parameter count (4 bytes)
 *     - Local count (4 bytes)
 *     - Is foreign (1 byte)
 *     - Bytecode length (4 bytes) + bytecode data
 *     - Line number table length (4 bytes) + entries
 *
 * This format allows fast loading without parsing or compilation.
 */
class BytecodeSerializer {
public:
    BytecodeSerializer() = default;

    // ========== Serialization (Save) ==========

    /**
     * Serialize a compiled module to a binary stream
     * Returns true on success, false on error
     */
    bool serialize(const CompiledModule& module, std::ostream& out);

    /**
     * Serialize a compiled module to a file
     * Returns true on success, false on error
     */
    bool serializeToFile(const CompiledModule& module, const std::string& filepath);

    // ========== Deserialization (Load) ==========

    /**
     * Deserialize a compiled module from a binary stream
     * Returns nullptr on error
     */
    std::unique_ptr<CompiledModule> deserialize(std::istream& in);

    /**
     * Deserialize a compiled module from a file
     * Returns nullptr on error
     */
    std::unique_ptr<CompiledModule> deserializeFromFile(const std::string& filepath);

    // ========== Error Handling ==========

    /**
     * Check if the last operation encountered an error
     */
    bool hasError() const { return hasError_; }

    /**
     * Get the last error message
     */
    const std::string& getErrorMessage() const { return errorMessage_; }

private:
    // ========== Serialization Helpers ==========

    /**
     * Write the file header (magic, version, module name)
     */
    void writeHeader(const CompiledModule& module, std::ostream& out);

    /**
     * Write the string pool section
     */
    void writeStringPool(const CompiledModule& module, std::ostream& out);

    /**
     * Write the foreign functions section
     */
    void writeForeignFunctions(const CompiledModule& module, std::ostream& out);

    /**
     * Write the globals section
     */
    void writeGlobals(const CompiledModule& module, std::ostream& out);

    /**
     * Write the functions section
     */
    void writeFunctions(const CompiledModule& module, std::ostream& out);

    /**
     * Write a single function
     */
    void writeFunction(const CompiledFunction& function, std::ostream& out);

    /**
     * Write a bytecode chunk
     */
    void writeChunk(const Chunk& chunk, std::ostream& out);

    // ========== Deserialization Helpers ==========

    /**
     * Read and verify the file header
     * Returns false if header is invalid
     */
    bool readHeader(std::istream& in, std::string& moduleName);

    /**
     * Read the string pool section
     */
    bool readStringPool(std::istream& in, CompiledModule& module);

    /**
     * Read the foreign functions section
     */
    bool readForeignFunctions(std::istream& in, CompiledModule& module);

    /**
     * Read the globals section
     */
    bool readGlobals(std::istream& in, CompiledModule& module);

    /**
     * Read the functions section
     */
    bool readFunctions(std::istream& in, CompiledModule& module);

    /**
     * Read a single function
     */
    bool readFunction(std::istream& in, CompiledFunction& function);

    /**
     * Read a bytecode chunk
     */
    bool readChunk(std::istream& in, Chunk& chunk);

    // ========== I/O Primitives ==========

    /**
     * Write a 1-byte unsigned integer
     */
    void writeUInt8(std::ostream& out, uint8_t value);

    /**
     * Write a 4-byte unsigned integer (little-endian)
     */
    void writeUInt32(std::ostream& out, uint32_t value);

    /**
     * Write a string (length-prefixed, UTF-8)
     */
    void writeString(std::ostream& out, const std::string& str);

    /**
     * Write raw bytes
     */
    void writeBytes(std::ostream& out, const void* data, size_t size);

    /**
     * Read a 1-byte unsigned integer
     */
    bool readUInt8(std::istream& in, uint8_t& value);

    /**
     * Read a 4-byte unsigned integer (little-endian)
     */
    bool readUInt32(std::istream& in, uint32_t& value);

    /**
     * Read a string (length-prefixed, UTF-8)
     */
    bool readString(std::istream& in, std::string& str);

    /**
     * Read raw bytes into a buffer
     */
    bool readBytes(std::istream& in, void* data, size_t size);

    // ========== Validation ==========

    /**
     * Verify that a deserialized module is valid
     * Checks for consistency and completeness
     */
    bool verifyModule(const CompiledModule& module);

    // ========== Error Handling ==========

    /**
     * Set error state with a message
     */
    void setError(const std::string& message);

    // ========== Constants ==========

    static constexpr uint32_t MAGIC_NUMBER = 0x424C5456;  // "VLTB" in little-endian
    static constexpr uint16_t VERSION_MAJOR = 0;
    static constexpr uint16_t VERSION_MINOR = 1;

    // ========== State ==========

    bool hasError_ = false;
    std::string errorMessage_;
};

/**
 * Utility function: Save a compiled module to a .vltb file
 * Returns true on success
 */
bool saveCompiledModule(const CompiledModule& module, const std::string& filepath);

/**
 * Utility function: Load a compiled module from a .vltb file
 * Returns nullptr on error
 */
std::unique_ptr<CompiledModule> loadCompiledModule(const std::string& filepath);

} // namespace volta::bytecode
