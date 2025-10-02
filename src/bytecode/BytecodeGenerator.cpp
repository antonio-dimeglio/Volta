#include "bytecode/BytecodeGenerator.hpp"
#include "vm/Opcode.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>

namespace volta::bytecode {

BytecodeGenerator::BytecodeGenerator(const ir::IRModule& module)
    : module_(module) {
}

std::vector<uint8_t> BytecodeGenerator::generate() {
    // Clear output
    output_.clear();
    functions_.clear();

    // Temporary storage for function bytecode
    std::vector<std::vector<uint8_t>> function_bytecodes;

    // Compile all functions first
    for (const auto& func : module_.functions()) {
        // Skip foreign functions
        if (func->isForeign()) {
            continue;
        }

        // Compile the function
        FunctionCompiler compiler(*func, constants_);
        FunctionMetadata meta = compiler.compile();

        // Store metadata and bytecode
        functions_.push_back(meta);
        function_bytecodes.push_back(compiler.bytecode());
    }

    // Write module header
    ModuleHeader header = compile_module_header();
    write_header(header);

    // Write constant pool
    std::vector<uint8_t> pool_data = constants_.serialize();
    output_.insert(output_.end(), pool_data.begin(), pool_data.end());

    // Write function metadata
    uint32_t bytecode_offset = 0;
    for (auto& meta : functions_) {
        meta.bytecode_offset = bytecode_offset;
        write_uint32(meta.name_offset);
        write_uint32(meta.bytecode_offset);
        write_uint32(meta.bytecode_length);
        write_uint8(meta.arity);
        write_uint8(meta.register_count);
        write_uint8(meta.is_variadic);
        write_uint8(meta.reserved);
        bytecode_offset += meta.bytecode_length;
    }

    // Write function bytecode
    for (const auto& bytecode : function_bytecodes) {
        output_.insert(output_.end(), bytecode.begin(), bytecode.end());
    }

    // Write globals table
    compile_globals();

    return output_;
}

void BytecodeGenerator::generate_to_file(const std::string& filename) {
    std::vector<uint8_t> bytecode = generate();
    std::ofstream file(filename, std::ios::binary);
    if(!file) throw std::runtime_error("Failed to open file");
    file.write((char*)bytecode.data(), bytecode.size());
    file.close();
}

ModuleHeader BytecodeGenerator::compile_module_header() {
    ModuleHeader header;
    header.magic = 0x564F4C54;  // "VOLT"
    header.version_major = 0;
    header.version_minor = 1;
    header.constant_count = constants_.size();
    header.function_count = functions_.size();
    header.global_count = module_.globals().size();

    // Find entry point: look for __init__ or main function by iterating through IR module
    header.entry_point = 0;
    size_t func_idx = 0;
    for (const auto& func : module_.functions()) {
        if (func->isForeign()) continue;  // Skip foreign functions

        const std::string& name = func->name();
        if (name == "__init__" || name == "main") {
            header.entry_point = func_idx;
            break;
        }
        func_idx++;
    }

    return header;
}

void BytecodeGenerator::compile_globals() {
    // Write global names to constant pool and output
    for (const auto& global : module_.globals()) {
        uint32_t name_offset = constants_.add_string(global->name());
        write_uint32(name_offset);
    }
}

void BytecodeGenerator::write_header(const ModuleHeader& header) {
    write_uint32(header.magic);
    write_uint16(header.version_major);
    write_uint16(header.version_minor);
    write_uint32(header.constant_count);
    write_uint32(header.function_count);
    write_uint32(header.global_count);
    write_uint32(header.entry_point);
}

void BytecodeGenerator::write_uint32(uint32_t value) {
    output_.push_back((value >> 24) & 0xFF);
    output_.push_back((value >> 16) & 0xFF);
    output_.push_back((value >> 8) & 0xFF);
    output_.push_back(value & 0xFF);
}

void BytecodeGenerator::write_uint16(uint16_t value) {
    output_.push_back((value >> 8) & 0xFF);
    output_.push_back(value & 0xFF);
}

void BytecodeGenerator::write_uint8(uint8_t value) {
    output_.push_back(value);
}

// ============================================================================
// Disassembler
// ============================================================================

std::string BytecodeDisassembler::disassemble(const uint8_t* bytecode, size_t length) {
    std::string result;
    size_t offset = 0;
    while (offset < length)  {
        auto [instruction, bytes] = disassemble_instruction(bytecode + offset, offset);
        result += instruction + "\n";
        offset += bytes;
    }
    return result;
}

std::pair<std::string, size_t> BytecodeDisassembler::disassemble_instruction(
    const uint8_t* bytecode,
    size_t offset
) {
    using namespace volta::vm;

    // Read opcode
    Opcode op = static_cast<Opcode>(bytecode[0]);
    std::string result = std::to_string(offset) + ": " + opcode_name(op);

    // Get operand count
    int operands = opcode_operand_count(op);
    size_t bytes = 1 + operands;

    // Decode operands based on format
    if (operands == 1) {
        // Format A: single byte operand
        uint8_t a = bytecode[1];
        result += " R" + std::to_string(a);
    } else if (operands == 2) {
        // Format AB: two byte operands
        uint8_t a = bytecode[1];
        uint8_t b = bytecode[2];
        result += " R" + std::to_string(a) + ", ";

        // Special cases for different opcodes
        if (op == Opcode::LOAD_INT) {
            // B is signed immediate
            int8_t imm = static_cast<int8_t>(b);
            result += std::to_string(imm);
        } else {
            result += "R" + std::to_string(b);
        }
    } else if (operands == 3) {
        // Could be ABC or ABx format
        uint8_t a = bytecode[1];

        // Check if this is an ABx format opcode
        if (op == Opcode::LOAD_CONST || op == Opcode::GET_GLOBAL ||
            op == Opcode::SET_GLOBAL || op == Opcode::NEW_ARRAY ||
            op == Opcode::NEW_STRUCT || op == Opcode::JMP_IF_FALSE ||
            op == Opcode::JMP_IF_TRUE || op == Opcode::CALL_FFI) {
            // Format ABx: 16-bit immediate
            uint16_t bx = (static_cast<uint16_t>(bytecode[2]) << 8) | bytecode[3];
            result += " R" + std::to_string(a) + ", " + std::to_string(bx);
        } else if (op == Opcode::JMP) {
            // Format Ax: 24-bit immediate (signed offset)
            int32_t ax = (static_cast<int32_t>(bytecode[1]) << 16) |
                        (static_cast<int32_t>(bytecode[2]) << 8) |
                        bytecode[3];
            // Sign extend from 24-bit
            if (ax & 0x800000) {
                ax |= 0xFF000000;
            }
            result += " " + std::to_string(ax);
        } else {
            // Format ABC: three register operands
            uint8_t b = bytecode[2];
            uint8_t c = bytecode[3];
            result += " R" + std::to_string(a) + ", R" + std::to_string(b) + ", R" + std::to_string(c);
        }
    }

    return {result, bytes};
}

} // namespace volta::bytecode
