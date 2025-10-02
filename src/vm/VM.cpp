#include "vm/VM.hpp"
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <cstring>
namespace volta::vm {

// ============================================================================
// Constructor & Destructor
// ============================================================================

VM::VM()
    : ip_(nullptr),
      bytecode_start_(nullptr),
      bytecode_end_(nullptr),
      current_frame_(nullptr),
      halted_(false),
      objects_(nullptr),
      bytes_allocated_(0),
      next_gc_threshold_(GarbageCollector::INITIAL_GC_THRESHOLD),
      gc_(this),
      ffi_(this),
      debug_mode_(false) {
    register_builtins();
}

VM::~VM() {
    // Free all allocated objects
    // Need to call destructors for non-POD members before freeing
    Object* obj = objects_;
    while (obj != nullptr) {
        Object* next = obj->next;

        // Call destructors for objects with non-POD members
        if (obj->type == ObjectType::NATIVE_FUNCTION) {
            NativeFunctionObject* native = (NativeFunctionObject*)obj;
            native->name.~basic_string();  // Destruct std::string
        }
        // Note: ClosureObject::Function is a separate allocation with its own lifecycle
        // Don't destruct it here - it should be deleted elsewhere

        std::free(obj);
        obj = next;
    }
}

// ============================================================================
// Execution
// ============================================================================

Value VM::execute(uint8_t* bytecode, size_t length) {
    // Parse module header
    uint8_t* ptr = bytecode;

    uint32_t magic = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
    if (magic != 0x564F4C54) {  // "VOLT"
        throw std::runtime_error("Invalid bytecode magic number");
    }
    ptr += 4;

    uint16_t version_major = (ptr[0] << 8) | ptr[1];
    uint16_t version_minor = (ptr[2] << 8) | ptr[3];
    ptr += 4;

    uint32_t constant_count = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
    ptr += 4;

    uint32_t function_count = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
    ptr += 4;

    uint32_t global_count = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
    ptr += 4;

    uint32_t entry_point = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
    ptr += 4;

    // Load constant pool
    // Skip the duplicate count in the serialized constant pool (already have it from header)
    uint32_t pool_count = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
    ptr += 4;

    if (pool_count != constant_count) {
        throw std::runtime_error("Constant pool count mismatch");
    }

    constants_.resize(constant_count);
    for (uint32_t i = 0; i < constant_count; i++) {
        uint8_t type = *ptr++;
        if (type == 0) {  // NONE (ConstantType::NONE == 0)
            constants_[i] = Value::none();
        } else if (type == 1) {  // BOOL (ConstantType::BOOL == 1)
            constants_[i] = Value::bool_value(*ptr++);
        } else if (type == 2) {  // INT (ConstantType::INT == 2)
            int64_t val = 0;
            for (int j = 0; j < 8; j++) {
                val = (val << 8) | *ptr++;
            }
            constants_[i] = Value::int_value(val);
        } else if (type == 3) {  // FLOAT (ConstantType::FLOAT == 3)
            uint64_t bits = 0;
            for (int j = 0; j < 8; j++) {
                bits = (bits << 8) | *ptr++;
            }
            double fval;
            std::memcpy(&fval, &bits, sizeof(double));
            constants_[i] = Value::float_value(fval);
        } else if (type == 4) {  // STRING (ConstantType::STRING == 4)
            uint32_t len = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
            ptr += 4;
            StringObject* str = StringObject::create(this, (const char*)ptr, len);
            constants_[i] = Value::object_value(str);
            ptr += len;
        }
    }

    // Skip function metadata (we'll need this later for CALL)
    uint8_t* functions_start = ptr;
    uint8_t* bytecode_section_start = ptr + (function_count * 16);  // Each function metadata is 16 bytes

    // Parse function metadata to find entry point bytecode
    uint8_t* entry_ptr = functions_start + (entry_point * 16);
    uint32_t name_offset = (entry_ptr[0] << 24) | (entry_ptr[1] << 16) | (entry_ptr[2] << 8) | entry_ptr[3];
    uint32_t bytecode_offset = (entry_ptr[4] << 24) | (entry_ptr[5] << 16) | (entry_ptr[6] << 8) | entry_ptr[7];
    uint32_t bytecode_length = (entry_ptr[8] << 24) | (entry_ptr[9] << 16) | (entry_ptr[10] << 8) | entry_ptr[11];

    // Point to entry function bytecode (offset is relative to bytecode section start)
    ip_ = bytecode_section_start + bytecode_offset;
    bytecode_end_ = ip_ + bytecode_length;

    // Create initial call frame (use dummy closure for top-level)
    CallFrame frame;
    frame.function_name = "__init__";
    frame.line_number = 1;
    frame.return_address = nullptr;
    frame.return_register = 0;

    call_stack_.push_back(frame);
    current_frame_ = &call_stack_.back();

    // Reset halted flag
    halted_ = false;

    // Run execution loop
    run();

    // Return first register or none
    if (current_frame_ && current_frame_->registers[0].type != ValueType::NONE) {
        return current_frame_->registers[0];
    }
    return Value::none();
}

void VM::run() {
    while (!halted_) {
        if (ip_ >= bytecode_end_) {
            runtime_error("Instruction pointer out of bounds");
        }
    
        if (debug_mode_) {
            std::cout << disassemble_instruction() << std::endl;
        }
    
        uint8_t opcode_byte = fetch_byte();
        Opcode op = (Opcode)opcode_byte;
    

        switch (op) {
            case Opcode::LOAD_CONST:   op_load_const(); break;
            case Opcode::LOAD_INT:     op_load_int(); break;
            case Opcode::LOAD_TRUE:    op_load_true(); break;
            case Opcode::LOAD_FALSE:   op_load_false(); break;
            case Opcode::LOAD_NONE:    op_load_none(); break;
            case Opcode::MOVE:         op_move(); break;
            case Opcode::GET_GLOBAL:   op_get_global(); break;
            case Opcode::SET_GLOBAL:   op_set_global(); break;
    
            case Opcode::ADD:          op_add(); break;
            case Opcode::SUB:          op_sub(); break;
            case Opcode::MUL:          op_mul(); break;
            case Opcode::DIV:          op_div(); break;
            case Opcode::MOD:          op_mod(); break;
            case Opcode::NEG:          op_neg(); break;
    
            case Opcode::EQ:           op_eq(); break;
            case Opcode::NE:           op_ne(); break;
            case Opcode::LT:           op_lt(); break;
            case Opcode::LE:           op_le(); break;
            case Opcode::GT:           op_gt(); break;
            case Opcode::GE:           op_ge(); break;
    
            case Opcode::AND:          op_and(); break;
            case Opcode::OR:           op_or(); break;
            case Opcode::NOT:          op_not(); break;
    
            case Opcode::JMP:          op_jmp(); break;
            case Opcode::JMP_IF_FALSE: op_jmp_if_false(); break;
            case Opcode::JMP_IF_TRUE:  op_jmp_if_true(); break;
            case Opcode::CALL:         op_call(); break;
            case Opcode::RETURN:       op_return(); break;
            case Opcode::RETURN_NONE:  op_return_none(); break;
    
            case Opcode::NEW_ARRAY:    op_new_array(); break;
            case Opcode::NEW_STRUCT:   op_new_struct(); break;
            case Opcode::GET_INDEX:    op_get_index(); break;
            case Opcode::SET_INDEX:    op_set_index(); break;
            case Opcode::GET_FIELD:    op_get_field(); break;
            case Opcode::SET_FIELD:    op_set_field(); break;
            case Opcode::GET_LEN:      op_get_len(); break;
    
            case Opcode::CALL_FFI:     op_call_ffi(); break;
    
            case Opcode::PRINT:        op_print(); break;
            case Opcode::HALT:         op_halt(); break;
    
            default:
                runtime_error("Unknown opcode: " + std::to_string(opcode_byte));
        }
    }
}

bool VM::step() {
    if (halted_) return false ;
    run();
    return !halted_;
}

// ============================================================================
// Bytecode Fetching
// ============================================================================

uint8_t VM::fetch_byte() {
    return *ip_++;
}

uint16_t VM::fetch_short() {
    uint16_t value = (uint16_t(ip_[0]) << 8) | uint16_t(ip_[1]);
    ip_ += 2;
    return value;
}

uint32_t VM::fetch_int() {
    uint32_t value = (uint32_t(ip_[0]) << 24) |
                    (uint32_t(ip_[1]) << 16) |
                    (uint32_t(ip_[2]) << 8)  |
                    uint32_t(ip_[3]);
    ip_ += 4;
    return value;
}

uint8_t VM::peek_byte() const {
    return *ip_;
}

// ============================================================================
// Register Access
// ============================================================================

Value* VM::registers() {
    if (!current_frame_) runtime_error("No active frame");
    return current_frame_->registers;

}

Value VM::get_register(uint8_t index)  {
    if (!current_frame_) runtime_error("No active frame");
    return current_frame_->registers[index];
}

void VM::set_register(uint8_t index, const Value& value) {
    if (!current_frame_) runtime_error("No active frame");
    current_frame_->registers[index] = value;

}

// ============================================================================
// Call Stack Management
// ============================================================================

void VM::call_function(
    ClosureObject* closure,
    const std::vector<Value>& args,
    uint8_t return_register
) {
    if (args.size() != closure->function->arity) {
        runtime_error("Wrong number of arguments");
    }
    

    if (call_stack_.size() >= MAX_CALL_FRAMES) {
        runtime_error("Stack overflow");
    }
    
    CallFrame frame(closure, ip_, return_register);
    
    for (size_t i = 0; i < args.size(); i++) {
        frame.registers[i] = args[i];
    }

    call_stack_.push_back(frame);
    current_frame_ = &call_stack_.back();
    

    ip_ = closure->function->bytecode;
}

void VM::return_from_function(const Value& return_value) {
    uint8_t* return_addr = current_frame_->return_address;
    uint8_t return_reg = current_frame_->return_register;

    call_stack_.pop_back();

    // If stack empty (returned from top-level), halt
    if (call_stack_.empty()) {
        halted_ = true;
        return;
    }

    // Restore previous frame
    current_frame_ = &call_stack_.back();

    // Restore IP
    ip_ = return_addr;

    // Store return value in caller's return register
    current_frame_->registers[return_reg] = return_value;
}

size_t VM::call_depth() const {
    return call_stack_.size();
}

std::vector<std::pair<std::string, int>> VM::get_stack_trace() const {
    std::vector<std::pair<std::string, int>> trace;
    for (const auto& frame : call_stack_) {
        trace.push_back({frame.function_name, frame.line_number});
    }
    return trace;
}

// ============================================================================
// Global Variables
// ============================================================================

Value VM::get_global(const std::string& name) const {
    auto it = globals_.find(name);
    if (it == globals_.end()) {
        throw std::runtime_error("Undefined variable: " + name);
    }
    return it->second;
}

void VM::set_global(const std::string& name, const Value& value) {
    globals_[name] = value;
}

bool VM::has_global(const std::string& name) const {
    return globals_.find(name) != globals_.end();
}

// ============================================================================
// Constant Pool
// ============================================================================

size_t VM::add_constant(const Value& value) {
    constants_.push_back(value);
    return constants_.size() - 1;
}

Value VM::get_constant(size_t index) const {
    if (index >= constants_.size()) {
        throw std::runtime_error("Constant index out of bounds");
    }
    return constants_[index];

}

size_t VM::constant_count() const {
    return constants_.size();
}

// ============================================================================
// Object Allocation
// ============================================================================

Object* VM::allocate_object(size_t size, ObjectType type) {
    if (bytes_allocated_ >= next_gc_threshold_) {
        gc_.collect();
    }

    // Allocate raw memory of the requested size
    void* memory = std::malloc(size);
    if (!memory) {
        gc_.collect();
        memory = std::malloc(size);
        if (!memory) {
            runtime_error("Out of memory");
        }
    }

    // Initialize the Object header (but don't construct the whole derived object)
    Object* obj = static_cast<Object*>(memory);
    obj->type = type;
    obj->marked = false;
    obj->next = objects_;

    objects_ = obj;
    bytes_allocated_ += size;
    
    return obj;
}

size_t VM::bytes_allocated() const {
    return bytes_allocated_;
}

size_t VM::object_count() const {
    size_t count = 0;
    Object* obj = objects_;
    while (obj) { count++; obj = obj->next; }
    return count;
}

// ============================================================================
// Built-in Functions (see Builtins.cpp for implementations)
// ============================================================================

void VM::register_builtins() {
    register_native_function("print", builtin_print, 1);
    register_native_function("println", builtin_println, 1);
    register_native_function("len", builtin_len, 1);
    register_native_function("type", builtin_type, 1);
    register_native_function("str", builtin_str, 1);
    register_native_function("int", builtin_int, 1);
    register_native_function("float", builtin_float, 1);
    register_native_function("assert", builtin_assert, 255);  // Variadic (1 or 2 args)
}

void VM::register_native_function(
    const std::string& name,
    NativeFunctionObject::NativeFn function,
    uint8_t arity
) {
    NativeFunctionObject* native = NativeFunctionObject::create(this, name, function, arity);
    builtins_[name] = native;
    globals_[name] = Value::object_value(native);
}

// ============================================================================
// Error Handling
// ============================================================================

[[noreturn]] void VM::runtime_error(const std::string& message) {
    std::cerr << "Runtime Error: " << message << std::endl;
    
    std::cerr << "\nStack trace:\n";
    auto trace = get_stack_trace();
    for (auto& [func, line] : trace) {
        std::cerr << "  at " << func << ":" << line << std::endl;
    }
    
    halted_ = true;
    last_error_ = message;
    
    
    throw std::runtime_error(message);
}

bool VM::has_error() const {
    return !last_error_.empty();
}

// ============================================================================
// Debugging
// ============================================================================

void VM::set_debug_mode(bool enabled) {
    debug_mode_ = enabled;
}

void VM::print_state() const {
    std::cout << "=== VM State ===\n";
    std::cout << "IP: " << (void*)ip_ << "\n";
    std::cout << "Halted: " << (halted_ ? "yes" : "no") << "\n";
    std::cout << "Call stack depth: " << call_stack_.size() << "\n";
    std::cout << "Bytes allocated: " << bytes_allocated_ << "\n";
    std::cout << "Object count: " << object_count() << "\n";
    
    if (current_frame_) {
        std::cout << "\nCurrent frame: " << current_frame_->function_name << "\n";
        std::cout << "Registers:\n";
        for (int i = 0; i < 16; i++) {  // Show first 16 registers
            Value v = current_frame_->registers[i];
            if (!v.is_none()) {
                std::cout << "  R" << i << " = " << v.to_string() << "\n";
            }
        }
    }

}

std::string VM::disassemble_instruction() const {
    uint8_t opcode_byte = *ip_;
    Opcode op = (Opcode)opcode_byte;
    std::ostringstream oss;
    oss << std::hex << (void*)ip_ << ": " << opcode_name(op);
    
    int operand_count = opcode_operand_count(op);
    for (int i = 0; i < operand_count && i < 3; i++) {
        oss << " " << (int)ip_[i+1];
    }
    
    return oss.str();


}

// ============================================================================
// Instruction Implementations
// ============================================================================
void VM::op_load_const() {
    uint8_t ra = fetch_byte();
    uint16_t kx = fetch_short();
    Value constant = get_constant(kx);
    registers()[ra] = constant;
}

void VM::op_add() {
    uint8_t ra = fetch_byte(), rb = fetch_byte(), rc = fetch_byte();
    Value b = registers()[rb], c = registers()[rc];
    if (b.is_int() && c.is_int()) {
        registers()[ra] = Value::int_value(b.as_int() + c.as_int());
    } else if (b.is_number() && c.is_number()) {
        registers()[ra] = Value::float_value(b.to_number() + c.to_number());
    } else {
        runtime_error("Cannot add non-numeric values");
    }
}

void VM::op_jmp() {
    int32_t offset = (int32_t)fetch_int();
    if (offset & 0x800000) offset |= 0xFF000000;
    ip_ += offset;
}

void VM::op_call() {
    // TODO: CALL RA, RB, nargs - Call function in RB, store result in RA
    //
    // Format: ABC (RA = result register, RB = closure register, RC = arg count)
    //
    // Implementation:
    uint8_t ra = fetch_byte(), rb = fetch_byte(), nargs = fetch_byte();
    Value closure_val = registers()[rb];
    if (!closure_val.is_object()) runtime_error("Not callable");
    Object* obj = closure_val.as_object();
    
    if (obj->is_closure()) {
        ClosureObject* closure = (ClosureObject*)obj;
        // Collect args from registers[rb+1..rb+nargs]
        std::vector<Value> args;
        for (int i = 0; i < nargs; i++) {
            args.push_back(registers()[rb + 1 + i]);
        }
        call_function(closure, args, ra);
    } else if (obj->is_native_function()) {
        NativeFunctionObject* native = (NativeFunctionObject*)obj;
        std::vector<Value> args;
        for (int i = 0; i < nargs; i++) {
            args.push_back(registers()[rb + 1 + i]);
        }
        registers()[ra] = native->call(this, args);
    } else {
        runtime_error("Object is not callable");
    }
}

void VM::op_print() {
    uint8_t ra = fetch_byte();
    Value v = registers()[ra];
    std::cout << v.to_string() << std::flush;
}

void VM::op_halt() {
    halted_ = true;
}

// TODO: Implement remaining opcodes following the same pattern:
// - op_load_int, op_load_true, op_load_false, op_load_none, op_move
// - op_get_global, op_set_global
// - op_sub, op_mul, op_div, op_mod, op_neg
// - op_eq, op_ne, op_lt, op_le, op_gt, op_ge
// - op_and, op_or, op_not
// - op_jmp_if_false, op_jmp_if_true
// - op_return, op_return_none
// - op_new_array, op_new_struct
// - op_get_index, op_set_index, op_get_field, op_set_field, op_get_len
// - op_call_ffi
//
// Each follows the same pattern:
// 1. Fetch operands
// 2. Perform operation
// 3. Store result or update state

void VM::op_load_int() {
    uint8_t ra = fetch_byte();
    int32_t value = (int32_t)fetch_int();
    registers()[ra] = Value::int_value(value);
}

void VM::op_load_true() {
    uint8_t ra = fetch_byte();
    registers()[ra] = Value::bool_value(true);
}

void VM::op_load_false() {
    uint8_t ra = fetch_byte();
    registers()[ra] = Value::bool_value(false);
}

void VM::op_load_none() {
    uint8_t ra = fetch_byte();
    registers()[ra] = Value::none();
}

void VM::op_move() {
    uint8_t ra = fetch_byte(), rb = fetch_byte();
    registers()[ra] = registers()[rb];
}

void VM::op_get_global() {
    uint8_t ra = fetch_byte();
    uint16_t kx = fetch_short();
    Value name_val = get_constant(kx);
    if (!name_val.is_object() || !name_val.as_object()->is_string()) {
        runtime_error("Global name must be a string");
    }
    StringObject* name_str = (StringObject*)name_val.as_object();
    std::string name(name_str->data, name_str->length);  // Use length, not null-terminated
    registers()[ra] = get_global(name);
}

void VM::op_set_global() {
    uint8_t ra = fetch_byte();
    uint16_t kx = fetch_short();
    Value name_val = get_constant(kx);
    if (!name_val.is_object() || !name_val.as_object()->is_string()) {
        runtime_error("Global name must be a string");
    }
    StringObject* name_str = (StringObject*)name_val.as_object();
    std::string name(name_str->data, name_str->length);  // Use length, not null-terminated
    set_global(name, registers()[ra]);
}

void VM::op_sub() {
    uint8_t ra = fetch_byte(), rb = fetch_byte(), rc = fetch_byte();
    Value b = registers()[rb], c = registers()[rc];
    if (b.is_int() && c.is_int()) {
        registers()[ra] = Value::int_value(b.as_int() - c.as_int());
    } else if (b.is_number() && c.is_number()) {
        registers()[ra] = Value::float_value(b.to_number() - c.to_number());
    } else {
        runtime_error("Cannot subtract non-numeric values");
    }
}

void VM::op_mul() {
    uint8_t ra = fetch_byte(), rb = fetch_byte(), rc = fetch_byte();
    Value b = registers()[rb], c = registers()[rc];
    if (b.is_int() && c.is_int()) {
        registers()[ra] = Value::int_value(b.as_int() * c.as_int());
    } else if (b.is_number() && c.is_number()) {
        registers()[ra] = Value::float_value(b.to_number() * c.to_number());
    } else {
        runtime_error("Cannot multiply non-numeric values");
    }
}

void VM::op_div() {
    uint8_t ra = fetch_byte(), rb = fetch_byte(), rc = fetch_byte();
    Value b = registers()[rb], c = registers()[rc];
    if (c.is_number() && c.to_number() == 0) runtime_error("Division by zero");
    if (b.is_number() && c.is_number()) {
        registers()[ra] = Value::float_value(b.to_number() / c.to_number());
    } else {
        runtime_error("Cannot divide non-numeric values");
    }
}

void VM::op_mod() {
    uint8_t ra = fetch_byte(), rb = fetch_byte(), rc = fetch_byte();
    Value b = registers()[rb], c = registers()[rc];
    if (!b.is_int() || !c.is_int()) runtime_error("Modulo only supports integers");
    if (c.as_int() == 0) runtime_error("Modulo by zero");
    registers()[ra] = Value::int_value(b.as_int() % c.as_int());
}

void VM::op_neg() {
    uint8_t ra = fetch_byte(), rb = fetch_byte();
    Value v = registers()[rb];
    if (v.is_int()) {
        registers()[ra] = Value::int_value(-v.as_int());
    } else if (v.is_number()) {
        registers()[ra] = Value::float_value(-v.to_number());
    } else {
        runtime_error("Cannot negate non-numeric value");
    }
}

void VM::op_eq() {
    uint8_t ra = fetch_byte(), rb = fetch_byte(), rc = fetch_byte();
    registers()[ra] = Value::bool_value(registers()[rb] == registers()[rc]);
}

void VM::op_ne() {
    uint8_t ra = fetch_byte(), rb = fetch_byte(), rc = fetch_byte();
    registers()[ra] = Value::bool_value(registers()[rb] != registers()[rc]);
}

void VM::op_lt() {
    uint8_t ra = fetch_byte(), rb = fetch_byte(), rc = fetch_byte();
    registers()[ra] = Value::bool_value(registers()[rb] < registers()[rc]);
}

void VM::op_le() {
    uint8_t ra = fetch_byte(), rb = fetch_byte(), rc = fetch_byte();
    registers()[ra] = Value::bool_value(registers()[rb] <= registers()[rc]);
}

void VM::op_gt() {
    uint8_t ra = fetch_byte(), rb = fetch_byte(), rc = fetch_byte();
    registers()[ra] = Value::bool_value(registers()[rb] > registers()[rc]);
}

void VM::op_ge() {
    uint8_t ra = fetch_byte(), rb = fetch_byte(), rc = fetch_byte();
    registers()[ra] = Value::bool_value(registers()[rb] >= registers()[rc]);
}

void VM::op_and() {
    uint8_t ra = fetch_byte(), rb = fetch_byte(), rc = fetch_byte();
    registers()[ra] = Value::bool_value(registers()[rb].to_bool() && registers()[rc].to_bool());
}

void VM::op_or() {
    uint8_t ra = fetch_byte(), rb = fetch_byte(), rc = fetch_byte();
    registers()[ra] = Value::bool_value(registers()[rb].to_bool() || registers()[rc].to_bool());
}

void VM::op_not() {
    uint8_t ra = fetch_byte(), rb = fetch_byte();
    registers()[ra] = Value::bool_value(!registers()[rb].to_bool());
}

void VM::op_jmp_if_false() {
    uint8_t rb = fetch_byte();
    int32_t offset = (int32_t)fetch_int();
    if (!registers()[rb].to_bool()) ip_ += offset;
}

void VM::op_jmp_if_true() {
    uint8_t rb = fetch_byte();
    int32_t offset = (int32_t)fetch_int();
    if (registers()[rb].to_bool()) ip_ += offset;
}

void VM::op_return() {
    uint8_t ra = fetch_byte();
    return_from_function(registers()[ra]);
}

void VM::op_return_none() {
    return_from_function(Value::none());
}

void VM::op_new_array() {
    uint8_t ra = fetch_byte();
    uint16_t length = fetch_short();
    ArrayObject* arr = ArrayObject::create(this, length);
    registers()[ra] = Value::object_value(arr);
}

void VM::op_new_struct() {
    uint8_t ra = fetch_byte();
    uint16_t field_count = fetch_short();
    StructObject* s = StructObject::create(this, field_count);
    registers()[ra] = Value::object_value(s);
}

void VM::op_get_index() {
    //TODO IMPLEMENT
}

void VM::op_set_index() {
    //TODO IMPLEMENT
}

void VM::op_get_field() {
//TODO IMPLEMENT
}

void VM::op_set_field() {
//TODO IMPLEMENT
}

void VM::op_get_len() {
//TODO IMPLEMENT
}

void VM::op_call_ffi() {
//TODO IMPLEMENT
}

} // namespace volta::vm
