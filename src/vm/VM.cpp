#include "vm/VM.hpp"
#include "vm/OPCode.hpp"
#include <stdexcept>
#include <cmath>
#include <iostream>

namespace volta::vm {

VM::~VM() {
    // Nothing to clean up (RAII handles everything)
}

Value VM::execute(BytecodeModule& module, const std::string& functionName) {
    return execute(module, functionName, std::vector<Value>{});
}

Value VM::execute(
    BytecodeModule& mod, 
    const std::string& functionName,
    const std::vector<Value>& args) {
    
    module_ = &mod;
    
    if (!module_->hasFunction(functionName)) {
        throw std::runtime_error("Function not found: " + functionName);
    }

    index funcIdx = module_->getFunctionIndex(functionName);
    FunctionInfo& func = module_->getFunction(funcIdx);


    if (args.size() != func.getParamCount()) {
        throw std::runtime_error("Argument count mismatch");
    }

    if (func.isNative()) {
        Value result = callNativeFunction(func.getNativePtr(), args);
        module_ = nullptr;  // Clean up
        return result;
    }

    Frame initialFrame(&func, func.getRegisterCount(), 0);

    // Copy arguments into the first N registers
    for (size_t i = 0; i < args.size(); ++i) {
        initialFrame.registers[i] = args[i];
    }

    callStack.push_back(initialFrame);

    Value result = run();
    
    callStack.clear();
    module_ = nullptr;
    
    return result;
}

Value VM::run() {
    while (true) {
        if (callStack.empty()) {
            throw std::runtime_error("Call stack is empty at start of loop iteration");
        }
        Frame& frame  = callStack.back();
        byte opcode = readByte(frame);
        // Debug: print opcode
        // printf("Executing opcode %02X at PC=%zu\n", opcode, frame.pc - 1);

        switch(static_cast<Opcode>(opcode)) {
            case Opcode::ADD: {
                uint8_t dest = readByte(frame);
                uint8_t r1 = readByte(frame);
                uint8_t r2 = readByte(frame);
                frame.registers[dest] = add(frame.registers[r1], frame.registers[r2]);
                break;
            }
            case Opcode::SUB: {
                uint8_t dest = readByte(frame);
                uint8_t r1 = readByte(frame);
                uint8_t r2 = readByte(frame);
                frame.registers[dest] = subtract(frame.registers[r1], frame.registers[r2]);
                break;
            }
            case Opcode::MUL: {
                uint8_t dest = readByte(frame);
                uint8_t r1 = readByte(frame);
                uint8_t r2 = readByte(frame);
                frame.registers[dest] = multiply(frame.registers[r1], frame.registers[r2]);
                break;
            }
            case Opcode::DIV: {
                uint8_t dest = readByte(frame);
                uint8_t r1 = readByte(frame);
                uint8_t r2 = readByte(frame);
                frame.registers[dest] = divide(frame.registers[r1], frame.registers[r2]);
                break;
            }
            case Opcode::REM: {
                uint8_t dest = readByte(frame);
                uint8_t r1 = readByte(frame);
                uint8_t r2 = readByte(frame);
                frame.registers[dest] = modulo(frame.registers[r1], frame.registers[r2]);
                break;
            }
            case Opcode::POW: {
                uint8_t dest = readByte(frame);
                uint8_t r1 = readByte(frame);
                uint8_t r2 = readByte(frame);
                frame.registers[dest] = power(frame.registers[r1], frame.registers[r2]);
                break;
            }
            case Opcode::NEG: {
                uint8_t dest = readByte(frame);
                uint8_t r1 = readByte(frame);
                frame.registers[dest] = negate(frame.registers[r1]);
                break;
            }
            case Opcode::CMP_EQ: {
                uint8_t dest = readByte(frame);
                uint8_t r1 = readByte(frame);
                uint8_t r2 = readByte(frame);
                frame.registers[dest] = equal(frame.registers[r1], frame.registers[r2]);
                break;
            }
            case Opcode::CMP_NE: {
                uint8_t dest = readByte(frame);
                uint8_t r1 = readByte(frame);
                uint8_t r2 = readByte(frame);
                frame.registers[dest] = notEqual(frame.registers[r1], frame.registers[r2]);
                break;
            }
            case Opcode::CMP_LT: {
                uint8_t dest = readByte(frame);
                uint8_t r1 = readByte(frame);
                uint8_t r2 = readByte(frame);
                frame.registers[dest] = lessThan(frame.registers[r1], frame.registers[r2]);
                break;
            }
            case Opcode::CMP_LE: {
                uint8_t dest = readByte(frame);
                uint8_t r1 = readByte(frame);
                uint8_t r2 = readByte(frame);
                frame.registers[dest] = lessEqual(frame.registers[r1], frame.registers[r2]);
                break;
            }
            case Opcode::CMP_GT: {
                uint8_t dest = readByte(frame);
                uint8_t r1 = readByte(frame);
                uint8_t r2 = readByte(frame);
                frame.registers[dest] = greaterThan(frame.registers[r1], frame.registers[r2]);
                break;
            }            
            case Opcode::CMP_GE: {
                uint8_t dest = readByte(frame);
                uint8_t r1 = readByte(frame);
                uint8_t r2 = readByte(frame);
                frame.registers[dest] = greaterEqual(frame.registers[r1], frame.registers[r2]);
                break;
            }
            case Opcode::AND: {
                uint8_t dest = readByte(frame);
                uint8_t r1 = readByte(frame);
                uint8_t r2 = readByte(frame);
                frame.registers[dest] = logicalAnd(frame.registers[r1], frame.registers[r2]);
                break;
            }
            case Opcode::OR: {
                uint8_t dest = readByte(frame);
                uint8_t r1 = readByte(frame);
                uint8_t r2 = readByte(frame);
                frame.registers[dest] = logicalOr(frame.registers[r1], frame.registers[r2]);
                break;
            }
            case Opcode::NOT: {
                uint8_t dest = readByte(frame);
                uint8_t r1 = readByte(frame);
                frame.registers[dest] = logicalNot(frame.registers[r1]);
                break;
            }
            case Opcode::CAST_INT_FLOAT: {
                uint8_t dest = readByte(frame);
                uint8_t r1 = readByte(frame);
                frame.registers[dest] = castIntToFloat(frame.registers[r1]);
                break;
            }
            case Opcode::CAST_FLOAT_INT: {
                uint8_t dest = readByte(frame);
                uint8_t r1 = readByte(frame);
                frame.registers[dest] = castFloatToInt(frame.registers[r1]);
                break;
            }
            case Opcode::LOAD_CONST_INT: {
                uint8_t dest = readByte(frame);
                uint16_t constIdx = readShort(frame);
                int64_t value = module_->getIntConstant(constIdx);
                frame.registers[dest] = Value::makeInt(value);
                break;
            }

            case Opcode::LOAD_CONST_FLOAT: {
                uint8_t dest = readByte(frame);
                uint16_t constIdx = readShort(frame);
                double value = module_->getFloatConstant(constIdx);
                frame.registers[dest] = Value::makeFloat(value);
                break;
            }

            case Opcode::LOAD_CONST_STRING: {
                uint8_t dest = readByte(frame);
                uint16_t constIdx = readShort(frame);
                (void)dest;  // TODO: Need to create a String object from the string constant
                (void)constIdx;  // For now, stub it out or skip
                break;
            }

            case Opcode::LOAD_TRUE: {
                uint8_t dest = readByte(frame);
                frame.registers[dest] = Value::makeBool(true);
                break;
            }

            case Opcode::LOAD_FALSE: {
                uint8_t dest = readByte(frame);
                frame.registers[dest] = Value::makeBool(false);
                break;
            }

            case Opcode::LOAD_NONE: {
                uint8_t dest = readByte(frame);
                frame.registers[dest] = Value::makeNone();
                break;
            }

            case Opcode::MOVE: {
                uint8_t dest = readByte(frame);
                uint8_t src = readByte(frame);
                frame.registers[dest] = frame.registers[src];
                break;
            }
            case Opcode::RET: {
                uint8_t reg = readByte(frame);
                Value result = frame.registers[reg];
                uint8_t retReg = frame.returnRegister; 
                callStack.pop_back();
                
                if (callStack.empty()) {
                    return result; 
                }
                
                Frame& caller = callStack.back();
                caller.registers[retReg] = result; 
                break;
            }

            case Opcode::RET_VOID: {
                callStack.pop_back();
                
                if (callStack.empty()) {
                    return Value::makeNone();
                }
                break;
            }

            case Opcode::HALT: {
                return Value::makeNone();
            }

            case Opcode::BR: {
                int16_t offset = readShort(frame);
                frame.pc += offset;
                break;
            }

            case Opcode::BR_IF_TRUE: {
                uint8_t cond = readByte(frame);
                int16_t offset = readShort(frame);
                bool condVal = (frame.registers[cond].type == ValueType::BOOL && frame.registers[cond].as.as_b);
                if (condVal) {
                    frame.pc += offset;
                }
                break;
            }

            case Opcode::BR_IF_FALSE: {
                uint8_t cond = readByte(frame);
                int16_t offset = readShort(frame);
                bool condVal = (frame.registers[cond].type == ValueType::BOOL && frame.registers[cond].as.as_b);
                if (frame.registers[cond].type == ValueType::BOOL && !frame.registers[cond].as.as_b) {
                    frame.pc += offset;
                }
                break;
            }

            case Opcode::CALL: {
                uint16_t funcIdx = readShort(frame);
                uint8_t dest = readByte(frame);
            
                auto& func = module_->getFunction(funcIdx);
                
                std::vector<Value> args;
                for (uint8_t i = 0; i < func.getParamCount(); i++) {
                    args.push_back(frame.registers[i]);  
                }
                
                if (func.isNative()) {
                    frame.registers[dest] = callNativeFunction(func.getNativePtr(), args);
                } else {
                    callFunction(&func, args, dest);
                }
                break;
            }
            case Opcode::ARRAY_GET:
            case Opcode::ARRAY_SET:
            case Opcode::ARRAY_LEN:
            case Opcode::ARRAY_NEW:
            case Opcode::ARRAY_SLICE:
            case Opcode::STRING_LEN:
            case Opcode::IS_SOME:
            case Opcode::OPTION_WRAP:
            case Opcode::OPTION_UNWRAP:
                throw std::runtime_error("Array/string/option operations not yet implemented");
        }
    }
}

byte VM::readByte(Frame& frame) {
    if (frame.pc >= frame.function->getCodeLength()) {
        throw std::runtime_error("PC out of bounds: pc=" + std::to_string(frame.pc) +
                                 " codeLength=" + std::to_string(frame.function->getCodeLength()) +
                                 " function=" + frame.function->getName());
    }

    auto bc = module_->bytecode;
    size_t absPos = frame.function->getCodeOffset() + frame.pc;
    byte bt = bc[absPos];
    frame.pc++;
    return bt;
}

uint16_t VM::readShort(Frame& frame) {
    byte lower = readByte(frame);
    byte higher = readByte(frame);


    return static_cast<uint16_t>(lower | (higher << 8));
}

uint32_t VM::readInt(Frame& frame) {
    byte b1 = readByte(frame);
    byte b2 = readByte(frame);
    byte b3 = readByte(frame);
    byte b4 = readByte(frame);

    return static_cast<uint32_t>(b1 | (b2 << 8) | (b3 << 16) | (b4 << 24));
}

Value VM::add(const Value& a, const Value& b) {
    if (a.type == ValueType::INT64 && b.type == ValueType::INT64) {
        return Value::makeInt(a.as.as_i64 + b.as.as_i64);
    }
    if (a.type == ValueType::FLOAT64 && b.type == ValueType::FLOAT64) {
        return Value::makeFloat(a.as.as_f64 + b.as.as_f64);
    }
    if (a.type == ValueType::INT64 && b.type == ValueType::FLOAT64) {
        return Value::makeFloat(a.as.as_i64 + b.as.as_f64);
    }
    if (a.type == ValueType::FLOAT64 && b.type == ValueType::INT64) {
        return Value::makeFloat(a.as.as_f64 + b.as.as_i64);
    }

    throw std::runtime_error("Type mismatch in addition");
}

Value VM::subtract(const Value& a, const Value& b) {
    if (a.type == ValueType::INT64 && b.type == ValueType::INT64) {
        return Value::makeInt(a.as.as_i64 - b.as.as_i64 );
    }
    if (a.type == ValueType::FLOAT64 && b.type == ValueType::FLOAT64) {
        return Value::makeFloat(a.as.as_f64 - b.as.as_f64);
    }
    if (a.type == ValueType::INT64 && b.type == ValueType::FLOAT64) {
        return Value::makeFloat(a.as.as_i64 - b.as.as_f64);
    }
    if (a.type == ValueType::FLOAT64 && b.type == ValueType::INT64) {
        return Value::makeFloat(a.as.as_f64 - b.as.as_i64);
    }


    throw std::runtime_error("Type mismatch in subtraction");
}

Value VM::multiply(const Value& a, const Value& b) {
    if (a.type == ValueType::INT64 && b.type == ValueType::INT64) {
        return Value::makeInt(a.as.as_i64 * b.as.as_i64 );
    }
    if (a.type == ValueType::FLOAT64 && b.type == ValueType::FLOAT64) {
        return Value::makeFloat(a.as.as_f64 * b.as.as_f64);
    }
    if (a.type == ValueType::INT64 && b.type == ValueType::FLOAT64) {
        return Value::makeFloat(a.as.as_i64 * b.as.as_f64);
    }
    if (a.type == ValueType::FLOAT64 && b.type == ValueType::INT64) {
        return Value::makeFloat(a.as.as_f64 * b.as.as_i64);
    }

    throw std::runtime_error("Type mismatch in multiplication");
}

Value VM::divide(const Value& a, const Value& b) {
    if (a.type == ValueType::INT64 && b.type == ValueType::INT64) {
        if (b.as.as_i64 == 0) throw std::runtime_error("Division by zero error.");
        return Value::makeInt(a.as.as_i64 / b.as.as_i64 );
    }
    if (a.type == ValueType::FLOAT64 && b.type == ValueType::FLOAT64) {
        return Value::makeFloat(a.as.as_f64 / b.as.as_f64);
    }
    if (a.type == ValueType::INT64 && b.type == ValueType::FLOAT64) {
        return Value::makeFloat(a.as.as_i64 / b.as.as_f64);
    }
    if (a.type == ValueType::FLOAT64 && b.type == ValueType::INT64) {
        if (b.as.as_i64 == 0) throw std::runtime_error("Division by zero error.");
        return Value::makeFloat(a.as.as_f64 / b.as.as_i64);
    }

    throw std::runtime_error("Type mismatch in multiplication");
}

Value VM::modulo(const Value& a, const Value& b) {
    if (a.type == ValueType::INT64 && b.type == ValueType::INT64) {
        if (b.as.as_i64 == 0) throw std::runtime_error("Division by zero error.");
        return Value::makeInt(a.as.as_i64 % b.as.as_i64 );
    }

    throw std::runtime_error("Type mismatch in multiplication");
}

Value VM::power(const Value& a, const Value& b) {
    double base, exponent;
    
    // Convert 'a' to double
    if (a.type == ValueType::INT64) {
        base = static_cast<double>(a.as.as_i64);
    } else if (a.type == ValueType::FLOAT64) {
        base = a.as.as_f64;
    } else {
        throw std::runtime_error("Power requires numeric types");
    }
    
    // Convert 'b' to double
    if (b.type == ValueType::INT64) {
        exponent = static_cast<double>(b.as.as_i64);
    } else if (b.type == ValueType::FLOAT64) {
        exponent = b.as.as_f64;
    } else {
        throw std::runtime_error("Power requires numeric types");
    }
    
    // Always return float
    return Value::makeFloat(std::pow(base, exponent));
}

Value VM::negate(const Value& a) {
    if (a.type == ValueType::INT64) {
        return Value::makeInt(-a.as.as_i64);
    }
    if (a.type == ValueType::FLOAT64) {
        return Value::makeFloat(-a.as.as_f64);
    }

    throw std::runtime_error("Tried to negate non numeric type");
}

Value VM::equal(const Value& a, const Value& b) {
    // Int == Int
    if (a.type == ValueType::INT64 && b.type == ValueType::INT64) {
        return Value::makeBool(a.as.as_i64 == b.as.as_i64);
    }
    
    // Float == Float
    if (a.type == ValueType::FLOAT64 && b.type == ValueType::FLOAT64) {
        return Value::makeBool(a.as.as_f64 == b.as.as_f64);
    }
    
    // Int == Float (with promotion)
    if (a.type == ValueType::INT64 && b.type == ValueType::FLOAT64) {
        return Value::makeBool(a.as.as_i64 == b.as.as_f64);
    }
    
    // Float == Int (with promotion)
    
    if (a.type == ValueType::FLOAT64 && b.type == ValueType::INT64) {
        return Value::makeBool(a.as.as_f64 == b.as.as_i64);
    }
    
    // Bool == Bool
    if (a.type == ValueType::BOOL && b.type == ValueType::BOOL) {
        return Value::makeBool(a.as.as_b == b.as.as_b);
    }
    
    // Object == Object (pointer equality)
    if (a.type == ValueType::OBJECT && b.type == ValueType::OBJECT) {
        return Value::makeBool(a.as.as_obj == b.as.as_obj);
    }
    
    // Different types are never equal
    return Value::makeBool(false );
}

Value VM::notEqual(const Value& a, const Value& b) {
    Value eq = equal(a, b);
    return Value::makeBool(!eq.as.as_b);
}

Value VM::lessThan(const Value& a, const Value& b) {
    if (a.type == ValueType::INT64 && b.type == ValueType::INT64) {
        return Value::makeBool(a.as.as_i64 < b.as.as_i64);
    }
    
    // Float == Float
    if (a.type == ValueType::FLOAT64 && b.type == ValueType::FLOAT64) {
        return Value::makeBool(a.as.as_f64 < b.as.as_f64);
    }
    
    // Int == Float (with promotion)
    if (a.type == ValueType::INT64 && b.type == ValueType::FLOAT64) {
        return Value::makeBool(a.as.as_i64 < b.as.as_f64);
    }
    
    // Float == Int (with promotion)
    
    if (a.type == ValueType::FLOAT64 && b.type == ValueType::INT64) {
        return Value::makeBool(a.as.as_f64 < b.as.as_i64);
    }
    
    
    throw std::runtime_error("Cannot compare non-numeric types");
}

Value VM::lessEqual(const Value& a, const Value& b) {
    if (a.type == ValueType::INT64 && b.type == ValueType::INT64) {
        return Value::makeBool(a.as.as_i64 <= b.as.as_i64);
    }
    
    // Float == Float
    if (a.type == ValueType::FLOAT64 && b.type == ValueType::FLOAT64) {
        return Value::makeBool(a.as.as_f64 <= b.as.as_f64);
    }
    
    // Int == Float (with promotion)
    if (a.type == ValueType::INT64 && b.type == ValueType::FLOAT64) {
        return Value::makeBool(a.as.as_i64 <= b.as.as_f64);
    }
    
    // Float == Int (with promotion)
    
    if (a.type == ValueType::FLOAT64 && b.type == ValueType::INT64) {
        return Value::makeBool(a.as.as_f64 <= b.as.as_i64);
    }
    
    
    throw std::runtime_error("Cannot compare non-numeric types");
}

Value VM::greaterThan(const Value& a, const Value& b) {
    if (a.type == ValueType::INT64 && b.type == ValueType::INT64) {
        return Value::makeBool(a.as.as_i64 > b.as.as_i64);
    }
    
    // Float == Float
    if (a.type == ValueType::FLOAT64 && b.type == ValueType::FLOAT64) {
        return Value::makeBool(a.as.as_f64 > b.as.as_f64);
    }
    
    // Int == Float (with promotion)
    if (a.type == ValueType::INT64 && b.type == ValueType::FLOAT64) {
        return Value::makeBool(a.as.as_i64 > b.as.as_f64);
    }
    
    // Float == Int (with promotion)
    
    if (a.type == ValueType::FLOAT64 && b.type == ValueType::INT64) {
        return Value::makeBool(a.as.as_f64 > b.as.as_i64);
    }
    
    
    throw std::runtime_error("Cannot compare non-numeric types");
}


Value VM::greaterEqual(const Value& a, const Value& b) {
    if (a.type == ValueType::INT64 && b.type == ValueType::INT64) {
        return Value::makeBool(a.as.as_i64 >= b.as.as_i64);
    }
    
    // Float == Float
    if (a.type == ValueType::FLOAT64 && b.type == ValueType::FLOAT64) {
        return Value::makeBool(a.as.as_f64 >= b.as.as_f64);
    }
    
    // Int == Float (with promotion)
    if (a.type == ValueType::INT64 && b.type == ValueType::FLOAT64) {
        return Value::makeBool(a.as.as_i64 >= b.as.as_f64);
    }
    
    // Float == Int (with promotion)
    
    if (a.type == ValueType::FLOAT64 && b.type == ValueType::INT64) {
        return Value::makeBool(a.as.as_f64 >= b.as.as_i64);
    }
    
    throw std::runtime_error("Cannot compare non-numeric types");
}

Value VM::logicalAnd(const Value& a, const Value& b) {
    if (a.type == ValueType::BOOL && b.type == ValueType::BOOL) {
        return Value::makeBool(a.as.as_b && b.as.as_b);
    }
    
    throw std::runtime_error("Cannot perform logical and for non-boolean types");
}


Value VM::logicalOr(const Value& a, const Value& b) {
    if (a.type == ValueType::BOOL && b.type == ValueType::BOOL) {
        return Value::makeBool(a.as.as_b || b.as.as_b);
    }
    
    throw std::runtime_error("Cannot perform logical or for non-boolean types");
}

Value VM::logicalNot(const Value& a) {
    if (a.type == ValueType::BOOL) {
        return Value::makeBool(!a.as.as_b);
    }
    
    throw std::runtime_error("Cannot perform logical not for non-boolean types");
}


Value VM::castIntToFloat(const Value& a) {
    if (a.type == ValueType::INT64) {
        return Value::makeFloat((double)a.as.as_i64);
    }
    
    throw std::runtime_error("Cannot perform casting on non int type");
}

Value VM::castFloatToInt(const Value& a) {
    if (a.type == ValueType::FLOAT64) {
        return Value::makeInt((int64_t)a.as.as_f64);
    }
    
    throw std::runtime_error("Cannot perform casting on non int type");
}

void VM::callFunction(FunctionInfo* function, const std::vector<Value>& args, uint8_t returnReg) {
    if (args.size() != function->getParamCount()) {
        throw std::runtime_error("Argument count mismatch");
    }

    Frame newFrame(function, function->getRegisterCount(), returnReg);

    for (size_t i = 0; i < args.size(); i++) {
        newFrame.registers[i] = args[i];
    }

    callStack.push_back(newFrame);
}

Value VM::callNativeFunction(RuntimeFunctionPtr function, const std::vector<Value>& args) {
    return function(this, const_cast<Value*>(args.data()), args.size());
}
}
