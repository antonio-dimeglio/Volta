#pragma once
#include <stdint.h>

namespace volta::vm {

enum class Opcode : uint8_t {
    // Arithmetic (0x01-0x0F)
    ADD = 0x01,
    SUB = 0x02,
    MUL = 0x03,
    DIV = 0x04,
    REM = 0x05,
    POW = 0x06,
    NEG = 0x07,

    // Comparison (0x10-0x1F)
    CMP_EQ = 0x10,
    CMP_NE = 0x11,
    CMP_LT = 0x12,
    CMP_LE = 0x13,
    CMP_GT = 0x14,
    CMP_GE = 0x15,

    // Logical (0x20-0x2F)
    AND = 0x20,
    OR  = 0x21,
    NOT = 0x22,

    // Constants (0x30-0x3F)
    LOAD_CONST_INT    = 0x30,
    LOAD_CONST_FLOAT  = 0x31,
    LOAD_CONST_STRING = 0x32,
    LOAD_TRUE         = 0x33,
    LOAD_FALSE        = 0x34,
    LOAD_NONE         = 0x35,

    // Control Flow (0x40-0x4F)
    BR            = 0x40,
    BR_IF_TRUE    = 0x41,
    BR_IF_FALSE   = 0x42,
    CALL          = 0x43,
    RET           = 0x44,
    RET_VOID      = 0x45,
    HALT          = 0x46,

    // Array Operations (0x50-0x5F)
    ARRAY_NEW = 0x50,
    ARRAY_GET = 0x51,
    ARRAY_SET = 0x52,
    ARRAY_LEN = 0x53,
    ARRAY_SLICE = 0x54,

    // Struct Operations (0x55-0x59)
    STRUCT_GET = 0x55,     // Extract field from struct
    STRUCT_SET = 0x56,     // Insert field into struct (creates new struct)
    GCALLOC    = 0x57,     // Allocate GC object (struct, etc.)

    // String Operations (0x60-0x6F)
    STRING_LEN = 0x60,

    // Type Operations (0x70-0x7F)
    CAST_INT_FLOAT = 0x70,
    CAST_FLOAT_INT = 0x71,
    IS_SOME        = 0x72,
    OPTION_WRAP    = 0x73,
    OPTION_UNWRAP  = 0x74,

    // Register Operations (0x80-0x8F)
    MOVE = 0x80,

    // Reserved ranges
    // 0x90-0x9F: Struct operations (Phase 3)
    // 0xA0-0xAF: Tuple operations (Phase 3)
    // 0xB0-0xBF: Matrix operations
    // 0xC0-0xCF: Advanced control flow
    // 0xD0-0xEF: Reserved
    // 0xF0-0xFF: Debug/profiling
};

// Helper to get instruction length (in bytes)
inline uint8_t getInstructionLength(Opcode op) {
    switch (op) {
        // 1 byte
        case Opcode::RET_VOID:
        case Opcode::HALT:
            return 1;

        // 2 bytes
        case Opcode::LOAD_TRUE:
        case Opcode::LOAD_FALSE:
        case Opcode::LOAD_NONE:
        case Opcode::RET:
            return 2;

        // 3 bytes
        case Opcode::BR:
        case Opcode::NEG:
        case Opcode::NOT:
        case Opcode::ARRAY_NEW:
        case Opcode::ARRAY_LEN:
        case Opcode::STRING_LEN:
        case Opcode::CAST_INT_FLOAT:
        case Opcode::CAST_FLOAT_INT:
        case Opcode::IS_SOME:
        case Opcode::OPTION_WRAP:
        case Opcode::OPTION_UNWRAP:
        case Opcode::MOVE:
            return 3;

        // 4 bytes
        case Opcode::ADD:
        case Opcode::SUB:
        case Opcode::MUL:
        case Opcode::DIV:
        case Opcode::REM:
        case Opcode::POW:
        case Opcode::CMP_EQ:
        case Opcode::CMP_NE:
        case Opcode::CMP_LT:
        case Opcode::CMP_LE:
        case Opcode::CMP_GT:
        case Opcode::CMP_GE:
        case Opcode::AND:
        case Opcode::OR:
        case Opcode::LOAD_CONST_INT:
        case Opcode::LOAD_CONST_FLOAT:
        case Opcode::LOAD_CONST_STRING:
        case Opcode::BR_IF_TRUE:
        case Opcode::BR_IF_FALSE:
        case Opcode::CALL:
        case Opcode::ARRAY_GET:
        case Opcode::ARRAY_SET:
        case Opcode::STRUCT_GET:
        case Opcode::STRUCT_SET:
        case Opcode::GCALLOC:
            return 4;

        case Opcode::ARRAY_SLICE:
            return 5;
        default:
            return 0; // Unknown/unimplemented
    }
}

} // namespace volta::vm