#pragma once

#include "Bytecode.hpp"
#include "BytecodeCompiler.hpp"
#include <string>
#include <sstream>
#include <ostream>

namespace volta::bytecode {

/**
 * Bytecode disassembler - converts bytecode back to human-readable form
 *
 * Output format (similar to LLVM IR or JVM bytecode):
 * ```
 * Function: main (params=0, locals=2)
 *   0000: ConstInt 42
 *   0009: StoreLocal 0
 *   0014: LoadLocal 0
 *   0019: Print
 *   0020: ReturnVoid
 * ```
 */
class Disassembler {
public:
    Disassembler() = default;

    // ========== Module Disassembly ==========

    /**
     * Disassemble an entire compiled module
     * Returns human-readable string representation
     */
    std::string disassembleModule(const CompiledModule& module);

    /**
     * Disassemble an entire compiled module to an output stream
     */
    void disassembleModule(const CompiledModule& module, std::ostream& out);

    // ========== Function Disassembly ==========

    /**
     * Disassemble a single function
     * Returns human-readable string representation
     */
    std::string disassembleFunction(const CompiledFunction& function);

    /**
     * Disassemble a single function to an output stream
     */
    void disassembleFunction(const CompiledFunction& function, std::ostream& out);

    // ========== Chunk Disassembly ==========

    /**
     * Disassemble a bytecode chunk
     * Returns human-readable string representation
     */
    std::string disassembleChunk(const Chunk& chunk);

    /**
     * Disassemble a bytecode chunk to an output stream
     */
    void disassembleChunk(const Chunk& chunk, std::ostream& out);

    // ========== Instruction Disassembly ==========

    /**
     * Disassemble a single instruction at a given offset
     * Returns the number of bytes consumed (opcode + operands)
     * Writes disassembled instruction to output stream
     */
    size_t disassembleInstruction(const Chunk& chunk, size_t offset, std::ostream& out);

    // ========== Configuration ==========

    /**
     * Enable/disable showing bytecode offsets
     * Default: true
     */
    void setShowOffsets(bool show) { showOffsets_ = show; }

    /**
     * Enable/disable showing line numbers (from debug info)
     * Default: false
     */
    void setShowLineNumbers(bool show) { showLineNumbers_ = show; }

    /**
     * Enable/disable showing raw bytes (hex dump)
     * Default: false
     */
    void setShowRawBytes(bool show) { showRawBytes_ = show; }

    /**
     * Set the string pool for resolving ConstString operands
     * Must be called before disassembling if the module uses strings
     */
    void setStringPool(const std::vector<std::string>* stringPool) {
        stringPool_ = stringPool;
    }

private:
    // ========== Operand Decoding Helpers ==========

    /**
     * Read a 4-byte signed integer operand at offset
     */
    int32_t readInt32(const Chunk& chunk, size_t offset) const;

    /**
     * Read an 8-byte signed integer operand at offset
     */
    int64_t readInt64(const Chunk& chunk, size_t offset) const;

    /**
     * Read an 8-byte floating point operand at offset
     */
    double readFloat64(const Chunk& chunk, size_t offset) const;

    /**
     * Read a 1-byte boolean operand at offset
     */
    bool readBool(const Chunk& chunk, size_t offset) const;

    // ========== Formatting Helpers ==========

    /**
     * Format a bytecode offset as a fixed-width string
     * Example: "0042" for offset 42
     */
    std::string formatOffset(size_t offset) const;

    /**
     * Format raw bytes as hex string
     * Example: "05 2A 00 00 00" for 5-byte instruction
     */
    std::string formatRawBytes(const Chunk& chunk, size_t offset, size_t length) const;

    /**
     * Format a string constant reference
     * Example: "@str0 (\"hello world\")" if string pool is available
     * Example: "@str0" if string pool is not available
     */
    std::string formatStringConstant(uint32_t index) const;

    // ========== State ==========

    bool showOffsets_ = true;
    bool showLineNumbers_ = false;
    bool showRawBytes_ = false;

    /// Optional string pool for resolving string constants
    const std::vector<std::string>* stringPool_ = nullptr;
};

/**
 * Utility function: Disassemble and print a module to stdout
 */
void dumpModule(const CompiledModule& module);

/**
 * Utility function: Disassemble and print a function to stdout
 */
void dumpFunction(const CompiledFunction& function);

/**
 * Utility function: Disassemble and print a chunk to stdout
 */
void dumpChunk(const Chunk& chunk);

} // namespace volta::bytecode
