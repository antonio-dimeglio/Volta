#include "vm/BytecodeDecompiler.hpp"
#include <iomanip> 
#include <format>

namespace volta::vm {

void BytecodeDecompiler::disassemble() {
    for (const auto& fn : module_.getFunctions()) {  
        disassembleFunction(fn); 
    }
}

void BytecodeDecompiler::disassembleFunction(const FunctionInfo& fn) {
    out_ << "\n=== Function: " << fn.getName() 
         << " (params: " << (int)fn.getParamCount() << ") ===\n";
    
    if (fn.isNative()) {
        out_ << "  [Native function - no bytecode]\n";
        return;
    }

    out_ << "  Offset | Opcode          | Operands\n";
    out_ << "  -------|-----------------|---------------------------\n";
    

    uint32_t offset = fn.getCodeOffset();
    uint32_t end = offset + fn.getCodeLength();
    
    while (offset < end) {
        size_t consumed = disassembleInstruction(offset);
        offset += consumed;
    }
}

size_t BytecodeDecompiler::disassembleInstruction(uint32_t offset) {
    Opcode op = static_cast<Opcode>(module_.peekByte(offset));
    size_t length = getInstructionLength(op);

    out_ << "  " << std::format("{:06}", offset) << " | ";
    out_ << std::setfill(' ') << std::setw(15) << std::left << getOpcodeName(op) << " | ";

    switch (op) {
        case Opcode::ADD:
        case Opcode::SUB:
        case Opcode::MUL:
        case Opcode::DIV:
        case Opcode::REM:
        case Opcode::POW: {
            byte rD = module_.peekByte(offset + 1);
            byte rA = module_.peekByte(offset + 2);
            byte rB = module_.peekByte(offset + 3);
            out_ << "r" << (int)rD << ", r" << (int)rA << ", r" << (int)rB;
            break;
        }

        // Unary arithmetic
        case Opcode::NEG: {
            byte rD = module_.peekByte(offset + 1);
            byte rA = module_.peekByte(offset + 2);
            out_ << "r" << (int)rD << ", r" << (int)rA;
            break;
        }

        // Comparisons (3-register form)
        case Opcode::CMP_EQ:
        case Opcode::CMP_NE:
        case Opcode::CMP_LT:
        case Opcode::CMP_LE:
        case Opcode::CMP_GT:
        case Opcode::CMP_GE:
        case Opcode::AND:
        case Opcode::OR: {
            byte rD = module_.peekByte(offset + 1);
            byte rA = module_.peekByte(offset + 2);
            byte rB = module_.peekByte(offset + 3);
            out_ << "r" << (int)rD << ", r" << (int)rA << ", r" << (int)rB;
            break;
        }

        // Unary logical
        case Opcode::NOT: {
            byte rD = module_.peekByte(offset + 1);
            byte rA = module_.peekByte(offset + 2);
            out_ << "r" << (int)rD << ", r" << (int)rA;
            break;
        }

        // Constants
        case Opcode::LOAD_CONST_INT: {
            byte rD = module_.peekByte(offset + 1);
            uint16_t poolIdx = module_.peekU16(offset + 2);
            int64_t value = module_.getIntConstant(poolIdx);
            out_ << "r" << (int)rD << ", #" << poolIdx << " (" << value << ")";
            break;
        }
        case Opcode::LOAD_CONST_FLOAT:
        case Opcode::LOAD_CONST_STRING: {
            byte rD = module_.peekByte(offset + 1);
            uint16_t poolIdx = module_.peekU16(offset + 2);
            out_ << "r" << (int)rD << ", #" << poolIdx;
            break;
        }

        case Opcode::LOAD_TRUE:
        case Opcode::LOAD_FALSE:
        case Opcode::LOAD_NONE: {
            byte rD = module_.peekByte(offset + 1);
            out_ << "r" << (int)rD;
            break;
        }

        // Control flow
        case Opcode::BR: {
            int16_t off = module_.peekI16(offset + 1);
            out_ << off;
            break;
        }
        case Opcode::BR_IF_TRUE:
        case Opcode::BR_IF_FALSE: {
            byte rCond = module_.peekByte(offset + 1);
            int16_t off = module_.peekI16(offset + 2);
            out_ << "r" << (int)rCond << ", " << off;
            break;
        }

        case Opcode::CALL: {
            uint16_t funcIdx = module_.peekU16(offset + 1);
            byte rResult = module_.peekByte(offset + 3);
            out_ << "#" << funcIdx << ", r" << (int)rResult;
            break;
        }

        case Opcode::RET: {
            byte rVal = module_.peekByte(offset + 1);
            out_ << "r" << (int)rVal;
            break;
        }

        case Opcode::RET_VOID:
        case Opcode::HALT:
            break;

        // Move
        case Opcode::MOVE: {
            byte rD = module_.peekByte(offset + 1);
            byte rS = module_.peekByte(offset + 2);
            out_ << "r" << (int)rD << ", r" << (int)rS;
            break;
        }

        // Array ops
        case Opcode::ARRAY_NEW: {
            byte rD = module_.peekByte(offset + 1);
            byte rSize = module_.peekByte(offset + 2);
            out_ << "r" << (int)rD << ", r" << (int)rSize;
            break;
        }
        case Opcode::ARRAY_GET: {
            byte rD = module_.peekByte(offset + 1);
            byte rArray = module_.peekByte(offset + 2);
            byte rIndex = module_.peekByte(offset + 3);
            out_ << "r" << (int)rD << ", r" << (int)rArray << ", r" << (int)rIndex;
            break;
        }
        case Opcode::ARRAY_SET: {
            byte rArray = module_.peekByte(offset + 1);
            byte rIndex = module_.peekByte(offset + 2);
            byte rVal = module_.peekByte(offset + 3);
            out_ << "r" << (int)rArray << ", r" << (int)rIndex << ", r" << (int)rVal;
            break;
        }
        case Opcode::ARRAY_LEN: {
            byte rD = module_.peekByte(offset + 1);
            byte rArray = module_.peekByte(offset + 2);
            out_ << "r" << (int)rD << ", r" << (int)rArray;
            break;
        }
        case Opcode::ARRAY_SLICE: {
            byte rD = module_.peekByte(offset + 1);
            byte rArray = module_.peekByte(offset + 2);
            byte rStart = module_.peekByte(offset + 3);
            byte rEnd = module_.peekByte(offset + 4);
            out_ << "r" << (int)rD << ", r" << (int)rArray << ", r" << (int)rStart << ", r" << (int)rEnd;
            break;
        }

        // Type ops
        case Opcode::CAST_INT_FLOAT:
        case Opcode::CAST_FLOAT_INT:
        case Opcode::IS_SOME:
        case Opcode::OPTION_WRAP:
        case Opcode::OPTION_UNWRAP: {
            byte rD = module_.peekByte(offset + 1);
            byte rSrc = module_.peekByte(offset + 2);
            out_ << "r" << (int)rD << ", r" << (int)rSrc;
            break;
        }

        default:
            out_ << "<unknown operands>";
            break;
        }

        out_ << "\n";
        return length; 
}

const char* BytecodeDecompiler::getOpcodeName(Opcode op) {
    switch (op) {
        // Arithmetic
        case Opcode::ADD: return "ADD";
        case Opcode::SUB: return "SUB";
        case Opcode::MUL: return "MUL";
        case Opcode::DIV: return "DIV";
        case Opcode::REM: return "REM";
        case Opcode::POW: return "POW";
        case Opcode::NEG: return "NEG";

        // Comparison
        case Opcode::CMP_EQ: return "CMP_EQ";
        case Opcode::CMP_NE: return "CMP_NE";
        case Opcode::CMP_LT: return "CMP_LT";
        case Opcode::CMP_LE: return "CMP_LE";
        case Opcode::CMP_GT: return "CMP_GT";
        case Opcode::CMP_GE: return "CMP_GE";

        // Logical
        case Opcode::AND: return "AND";
        case Opcode::OR:  return "OR";
        case Opcode::NOT: return "NOT";

        // Constants
        case Opcode::LOAD_CONST_INT:    return "LOAD_CONST_INT";
        case Opcode::LOAD_CONST_FLOAT:  return "LOAD_CONST_FLOAT";
        case Opcode::LOAD_CONST_STRING: return "LOAD_CONST_STRING";
        case Opcode::LOAD_TRUE:         return "LOAD_TRUE";
        case Opcode::LOAD_FALSE:        return "LOAD_FALSE";
        case Opcode::LOAD_NONE:         return "LOAD_NONE";

        // Control Flow
        case Opcode::BR:          return "BR";
        case Opcode::BR_IF_TRUE:  return "BR_IF_TRUE";
        case Opcode::BR_IF_FALSE: return "BR_IF_FALSE";
        case Opcode::CALL:        return "CALL";
        case Opcode::RET:         return "RET";
        case Opcode::RET_VOID:    return "RET_VOID";
        case Opcode::HALT:        return "HALT";

        // Array Operations
        case Opcode::ARRAY_NEW:   return "ARRAY_NEW";
        case Opcode::ARRAY_GET:   return "ARRAY_GET";
        case Opcode::ARRAY_SET:   return "ARRAY_SET";
        case Opcode::ARRAY_LEN:   return "ARRAY_LEN";
        case Opcode::ARRAY_SLICE: return "ARRAY_SLICE";

        // String Operations
        case Opcode::STRING_LEN:  return "STRING_LEN";

        // Type Operations
        case Opcode::CAST_INT_FLOAT: return "CAST_INT_FLOAT";
        case Opcode::CAST_FLOAT_INT: return "CAST_FLOAT_INT";
        case Opcode::IS_SOME:        return "IS_SOME";
        case Opcode::OPTION_WRAP:    return "OPTION_WRAP";
        case Opcode::OPTION_UNWRAP:  return "OPTION_UNWRAP";

        // Register Operations
        case Opcode::MOVE: return "MOVE";

        default: return "<UNKNOWN_OP>";
    }
}

}