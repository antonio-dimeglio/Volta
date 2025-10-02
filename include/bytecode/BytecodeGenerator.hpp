#pragma once

#include "IR/IRModule.hpp"
#include "bytecode/ConstantPool.hpp"
#include "bytecode/FunctionCompiler.hpp"
#include <vector>
#include <cstdint>
#include <string>

namespace volta::bytecode {

/**
 * @brief Module header for bytecode format
 */
struct ModuleHeader {
    uint32_t magic;           // 0x564F4C54 ("VOLT")
    uint16_t version_major;   // 0
    uint16_t version_minor;   // 1
    uint32_t constant_count;  // Number of constants
    uint32_t function_count;  // Number of functions
    uint32_t global_count;    // Number of globals
    uint32_t entry_point;     // Entry function index
};

// FunctionMetadata is defined in FunctionCompiler.hpp

/**
 * @brief Main bytecode generator
 *
 * Converts IR module to executable bytecode format.
 *
 * Pipeline:
 * 1. Build constant pool from all constants
 * 2. Compile each function to bytecode
 * 3. Serialize module header
 * 4. Write constant pool
 * 5. Write function table
 * 6. Write global variables table
 */
class BytecodeGenerator {
public:
    /**
     * @brief Construct generator for IR module
     * @param module IR module to compile
     */
    explicit BytecodeGenerator(const ir::IRModule& module);

    /**
     * @brief Generate complete bytecode module
     * @return Bytecode as byte array
     *
     * Implementation:
     * 1. Create ConstantPoolBuilder
     * 2. For each function in module:
     *    - Create FunctionCompiler
     *    - Compile to bytecode
     *    - Store metadata
     * 3. Serialize module:
     *    - Write header
     *    - Write constant pool
     *    - Write function metadata
     *    - Write function bytecode
     *    - Write globals
     * 4. Return complete byte array
     */
    std::vector<uint8_t> generate();

    /**
     * @brief Generate and write to file
     * @param filename Output file path
     *
     * Implementation:
     * 1. Generate bytecode with generate()
     * 2. Open file for binary writing
     * 3. Write all bytes
     * 4. Close file
     */
    void generate_to_file(const std::string& filename);

    /**
     * @brief Get constant pool (for inspection)
     */
    const ConstantPoolBuilder& constant_pool() const { return constants_; }

    /**
     * @brief Get function metadata list
     */
    const std::vector<FunctionMetadata>& functions() const { return functions_; }

private:
    /**
     * @brief Compile module header
     *
     * Implementation:
     * 1. Set magic = 0x564F4C54
     * 2. Set version = 0.1
     * 3. Set constant_count from constants_
     * 4. Set function_count from functions_
     * 5. Set global_count from module globals
     * 6. Find entry point (main function)
     */
    ModuleHeader compile_module_header();

    /**
     * @brief Compile all functions
     *
     * Implementation:
     * For each function in module_:
     * 1. Create FunctionCompiler
     * 2. Call compile()
     * 3. Store returned FunctionMetadata
     * 4. Append bytecode to output
     */
    void compile_functions();

    /**
     * @brief Compile global variables table
     *
     * Implementation:
     * For each global in module:
     * 1. Add name to string table
     * 2. Write name offset
     */
    void compile_globals();

    /**
     * @brief Write header to output
     */
    void write_header(const ModuleHeader& header);

    /**
     * @brief Write 32-bit integer (big-endian)
     */
    void write_uint32(uint32_t value);

    /**
     * @brief Write 16-bit integer (big-endian)
     */
    void write_uint16(uint16_t value);

    /**
     * @brief Write 8-bit integer
     */
    void write_uint8(uint8_t value);

    const ir::IRModule& module_;
    ConstantPoolBuilder constants_;
    std::vector<FunctionMetadata> functions_;
    std::vector<uint8_t> output_;
};

/**
 * @brief Disassembler for debugging bytecode
 */
class BytecodeDisassembler {
public:
    /**
     * @brief Disassemble bytecode to human-readable format
     * @param bytecode Bytecode array
     * @param length Bytecode length
     * @return Disassembly string
     */
    static std::string disassemble(const uint8_t* bytecode, size_t length);

    /**
     * @brief Disassemble single instruction
     * @param bytecode Pointer to instruction
     * @param offset Current offset (for display)
     * @return Disassembly string and bytes consumed
     */
    static std::pair<std::string, size_t> disassemble_instruction(
        const uint8_t* bytecode,
        size_t offset
    );
};

} // namespace volta::bytecode
