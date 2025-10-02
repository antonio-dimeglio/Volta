#include "vm/Opcode.hpp"

namespace volta::vm {

std::string opcode_name(Opcode op) {
    #define CASE(op) case Opcode::op: return #op
    switch (op) {
        CASE(HALT);
        CASE(RETURN_NONE);
        CASE(LOAD_TRUE);
        CASE(LOAD_FALSE);
        CASE(LOAD_NONE);
        CASE(RETURN);
        CASE(PRINT);
        CASE(MOVE);
        CASE(LOAD_INT);
        CASE(NEG);
        CASE(NOT);
        CASE(GET_LEN);
        CASE(ADD);
        CASE(SUB);
        CASE(MUL);
        CASE(DIV);
        CASE(MOD);
        CASE(EQ);
        CASE(NE);
        CASE(LT);
        CASE(LE);
        CASE(GT);
        CASE(GE);
        CASE(AND);
        CASE(OR);
        CASE(CALL);
        CASE(GET_INDEX);
        CASE(SET_INDEX);
        CASE(GET_FIELD);
        CASE(SET_FIELD);
        CASE(CALL_FFI);
        CASE(LOAD_CONST);
        CASE(GET_GLOBAL);
        CASE(SET_GLOBAL);
        CASE(NEW_ARRAY);
        CASE(NEW_STRUCT);
        CASE(JMP);
        CASE(JMP_IF_FALSE);
        CASE(JMP_IF_TRUE);
        default: return "UNKNOWN";
    }
    #undef CASE
}

int opcode_operand_count(Opcode op) {
    // - A format (1 operand): 1 byte (e.g., LOAD_TRUE RA)
    // - AB format (2 operands): 2 bytes (e.g., MOVE RA, RB)
    // - ABC format (3 operands): 3 bytes (e.g., ADD RA, RB, RC)
    // - ABx format (1 reg + 16-bit): 3 bytes (e.g., LOAD_CONST RA, Kx)
    // - Ax format (24-bit immediate): 3 bytes (e.g., JMP offset)
    // - No operands: 0 bytes (e.g., HALT, RETURN_NONE)
    
    switch (op) {
        // No operands (0 bytes)
        case Opcode::HALT:
        case Opcode::RETURN_NONE:
            return 0;
    
        // A format (1 byte)
        case Opcode::LOAD_TRUE:
        case Opcode::LOAD_FALSE:
        case Opcode::LOAD_NONE:
        case Opcode::RETURN:
        case Opcode::PRINT:
            return 1;
    
        // AB format (2 bytes)
        case Opcode::MOVE:
        case Opcode::LOAD_INT:
        case Opcode::NEG:
        case Opcode::NOT:
        case Opcode::GET_LEN:
            return 2;
    
        // ABC format (3 bytes)
        case Opcode::ADD:
        case Opcode::SUB:
        case Opcode::MUL:
        case Opcode::DIV:
        case Opcode::MOD:
        case Opcode::EQ:
        case Opcode::NE:
        case Opcode::LT:
        case Opcode::LE:
        case Opcode::GT:
        case Opcode::GE:
        case Opcode::AND:
        case Opcode::OR:
        case Opcode::CALL:
        case Opcode::GET_INDEX:
        case Opcode::SET_INDEX:
        case Opcode::GET_FIELD:
        case Opcode::SET_FIELD:
        case Opcode::CALL_FFI:
            return 3;
    
        // ABx/Ax format (3 bytes)
        case Opcode::LOAD_CONST:
        case Opcode::GET_GLOBAL:
        case Opcode::SET_GLOBAL:
        case Opcode::NEW_ARRAY:
        case Opcode::NEW_STRUCT:
        case Opcode::JMP:
        case Opcode::JMP_IF_FALSE:
        case Opcode::JMP_IF_TRUE:
            return 3;
    
        default:
            return 0;
    }

    // YOUR CODE HERE
    return 0;
}

} // namespace volta::vm
