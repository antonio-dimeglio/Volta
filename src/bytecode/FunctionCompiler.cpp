#include "bytecode/FunctionCompiler.hpp"
#include "IR/IR.hpp"
#include <stdexcept>

namespace volta::bytecode {

FunctionCompiler::FunctionCompiler(
    const ir::Function& func,
    ConstantPoolBuilder& constants
) : func_(func), constants_(constants), allocator_(func), emitter_(constants) {
}

FunctionMetadata FunctionCompiler::compile() {
    // Compile each basic block
    for (const auto& block : func_.basicBlocks()) {
        compile_basic_block(*block);
    }

    // Add implicit return if the last block doesn't have one
    // This ensures every function has a proper terminator
    if (!func_.basicBlocks().empty()) {
        const auto& lastBlock = func_.basicBlocks().back();
        if (!lastBlock->instructions().empty()) {
            const auto& lastInst = lastBlock->instructions().back();
            if (!dynamic_cast<const ir::ReturnInst*>(lastInst.get())) {
                // No return statement - add implicit "return void"
                emitter_.emit_opcode(vm::Opcode::RETURN_NONE);
            }
        } else {
            // Empty block - add return
            emitter_.emit_opcode(vm::Opcode::RETURN_NONE);
        }
    }

    // Resolve jumps
    emitter_.resolve_jumps();

    // Return metadata
    FunctionMetadata metadata;
    metadata.name_offset = constants_.add_string(func_.name());
    metadata.bytecode_offset = 0;  // Will be set by BytecodeGenerator
    metadata.bytecode_length = bytecode().size();
    metadata.arity = static_cast<uint8_t>(func_.parameters().size());
    metadata.register_count = static_cast<uint8_t>(allocator_.register_count() + next_temp_register_ + 10); // Add margin for temps
    metadata.is_variadic = 0;
    metadata.reserved = 0;

    return metadata;
}

const std::vector<uint8_t>& FunctionCompiler::bytecode() const {
    return emitter_.bytecode();
}

void FunctionCompiler::compile_basic_block(const ir::BasicBlock& bb) {
    // Mark block label
    uint32_t label = get_block_label(&bb);
    emitter_.mark_label(label);

    // Compile each instruction
    for (const auto& inst : bb.instructions()) {
        compile_instruction(*inst);
    }
}

void FunctionCompiler::compile_instruction(const ir::Instruction& inst) {
    // Use dynamic_cast to identify instruction type
    if (auto* bin = dynamic_cast<const ir::BinaryInst*>(&inst)) {
        compile_binary_op(*bin);
    } else if (auto* load = dynamic_cast<const ir::LoadInst*>(&inst)) {
        compile_load(*load);
    } else if (auto* store = dynamic_cast<const ir::StoreInst*>(&inst)) {
        compile_store(*store);
    } else if (auto* call = dynamic_cast<const ir::CallInst*>(&inst)) {
        compile_call(*call);
    } else if (auto* foreign = dynamic_cast<const ir::CallForeignInst*>(&inst)) {
        compile_call_foreign(*foreign);
    } else if (auto* ret = dynamic_cast<const ir::ReturnInst*>(&inst)) {
        compile_return(*ret);
    } else if (auto* br = dynamic_cast<const ir::BranchInst*>(&inst)) {
        compile_branch(*br);
    } else if (auto* get = dynamic_cast<const ir::GetFieldInst*>(&inst)) {
        compile_get_field(*get);
    } else if (auto* set = dynamic_cast<const ir::SetFieldInst*>(&inst)) {
        compile_set_field(*set);
    } else {
        // Silently skip unsupported instructions for now
    }
}

void FunctionCompiler::compile_binary_op(const ir::BinaryInst& inst) {
    uint8_t dest = allocator_.get_register(&inst);
    uint8_t lhs = get_value_register(inst.left());
    uint8_t rhs = get_value_register(inst.right());

    vm::Opcode opcode;
    switch (inst.opcode()) {
        case ir::Instruction::Opcode::Add: opcode = vm::Opcode::ADD; break;
        case ir::Instruction::Opcode::Sub: opcode = vm::Opcode::SUB; break;
        case ir::Instruction::Opcode::Mul: opcode = vm::Opcode::MUL; break;
        case ir::Instruction::Opcode::Div: opcode = vm::Opcode::DIV; break;
        case ir::Instruction::Opcode::Mod: opcode = vm::Opcode::MOD; break;
        case ir::Instruction::Opcode::Eq:  opcode = vm::Opcode::EQ; break;
        case ir::Instruction::Opcode::Ne:  opcode = vm::Opcode::NE; break;
        case ir::Instruction::Opcode::Lt:  opcode = vm::Opcode::LT; break;
        case ir::Instruction::Opcode::Le:  opcode = vm::Opcode::LE; break;
        case ir::Instruction::Opcode::Gt:  opcode = vm::Opcode::GT; break;
        case ir::Instruction::Opcode::Ge:  opcode = vm::Opcode::GE; break;
        case ir::Instruction::Opcode::And: opcode = vm::Opcode::AND; break;
        case ir::Instruction::Opcode::Or:  opcode = vm::Opcode::OR; break;
        default:
            throw std::runtime_error("Unsupported binary operation");
    }

    emitter_.emit_abc(opcode, dest, lhs, rhs);
}

void FunctionCompiler::compile_load(const ir::LoadInst& inst) {
    uint8_t dest = allocator_.get_register(&inst);
    uint8_t addr = get_value_register(inst.address());
    // For now, treat load as a move (SSA form means address is already a value)
    emitter_.emit_ab(vm::Opcode::MOVE, dest, addr);
}

void FunctionCompiler::compile_store(const ir::StoreInst& inst) {
    uint8_t addr = get_value_register(inst.address());
    uint8_t value = get_value_register(inst.value());
    // For now, treat store as a move
    emitter_.emit_ab(vm::Opcode::MOVE, addr, value);
}

void FunctionCompiler::compile_call(const ir::CallInst& inst) {
    // Get destination register for return value
    uint8_t dest = allocator_.has_register(&inst) ? allocator_.get_register(&inst) : 0;

    // Get the function to call
    ir::Function* callee = inst.callee();
    if (!callee) {
        throw std::runtime_error("Call instruction has no callee");
    }

    // Load the function global into a register
    uint32_t func_name_idx = constants_.add_string(callee->name());
    uint8_t func_reg = allocator_.register_count() + next_temp_register_++;
    emitter_.emit_abx(vm::Opcode::GET_GLOBAL, func_reg, func_name_idx);

    // Compile arguments into consecutive registers starting at func_reg + 1
    const auto& args = inst.arguments();
    for (size_t i = 0; i < args.size(); i++) {
        uint8_t arg_reg = func_reg + 1 + i;
        uint8_t value_reg = get_value_register(args[i]);
        if (value_reg != arg_reg) {
            emitter_.emit_ab(vm::Opcode::MOVE, arg_reg, value_reg);
        }
    }

    // Emit CALL instruction: CALL dest, func_reg, argc
    emitter_.emit_abc(vm::Opcode::CALL, dest, func_reg, static_cast<uint8_t>(args.size()));
}

void FunctionCompiler::compile_call_foreign(const ir::CallForeignInst& inst) {
    // Get destination register for return value
    uint8_t dest = allocator_.has_register(&inst) ? allocator_.get_register(&inst) : 0;

    // Load the foreign function global into a register
    uint32_t func_name_idx = constants_.add_string(inst.foreignName());
    uint8_t func_reg = allocator_.register_count() + next_temp_register_++;
    emitter_.emit_abx(vm::Opcode::GET_GLOBAL, func_reg, func_name_idx);

    // Compile arguments into consecutive registers starting at func_reg + 1
    const auto& args = inst.arguments();
    for (size_t i = 0; i < args.size(); i++) {
        uint8_t arg_reg = func_reg + 1 + i;
        uint8_t value_reg = get_value_register(args[i]);
        if (value_reg != arg_reg) {
            emitter_.emit_ab(vm::Opcode::MOVE, arg_reg, value_reg);
        }
    }

    // Emit CALL instruction (VM will handle foreign vs regular transparently)
    emitter_.emit_abc(vm::Opcode::CALL, dest, func_reg, static_cast<uint8_t>(args.size()));
}

void FunctionCompiler::compile_return(const ir::ReturnInst& inst) {
    if (inst.hasReturnValue()) {
        uint8_t value = get_value_register(inst.returnValue());
        emitter_.emit_a(vm::Opcode::RETURN, value);
    } else {
        // Return none/void
        uint32_t none_idx = constants_.add_none();
        uint8_t temp_reg = allocator_.register_count();
        emitter_.emit_abx(vm::Opcode::LOAD_CONST, temp_reg, none_idx);
        emitter_.emit_a(vm::Opcode::RETURN, temp_reg);
    }
}

void FunctionCompiler::compile_branch(const ir::BranchInst& inst) {
    ir::BasicBlock* target = inst.target();
    uint32_t label = get_block_label(target);
    emitter_.emit_jump(label);
}

void FunctionCompiler::compile_get_field(const ir::GetFieldInst& inst) {
    uint8_t dest = allocator_.get_register(&inst);
    uint8_t obj = get_value_register(inst.object());
    uint32_t field_idx = inst.fieldIndex();

    if (field_idx > 65535) {
        throw std::runtime_error("Field index too large");
    }

    emitter_.emit_abx(vm::Opcode::GET_FIELD, dest, static_cast<uint16_t>(field_idx));
}

void FunctionCompiler::compile_set_field(const ir::SetFieldInst& inst) {
    uint8_t obj = get_value_register(inst.object());
    uint8_t value = get_value_register(inst.value());
    uint32_t field_idx = inst.fieldIndex();

    if (field_idx > 65535) {
        throw std::runtime_error("Field index too large");
    }

    emitter_.emit_abx(vm::Opcode::SET_FIELD, obj, static_cast<uint16_t>(field_idx));
}

void FunctionCompiler::compile_load_const(const ir::Constant& constant, uint8_t dest_reg) {
    const auto& val = constant.value();

    uint32_t const_idx;
    if (std::holds_alternative<int64_t>(val)) {
        const_idx = constants_.add_int(std::get<int64_t>(val));
    } else if (std::holds_alternative<double>(val)) {
        const_idx = constants_.add_float(std::get<double>(val));
    } else if (std::holds_alternative<bool>(val)) {
        const_idx = constants_.add_bool(std::get<bool>(val));
    } else if (std::holds_alternative<std::string>(val)) {
        const_idx = constants_.add_string(std::get<std::string>(val));
    } else if (std::holds_alternative<std::monostate>(val)) {
        const_idx = constants_.add_none();
    } else {
        throw std::runtime_error("Unsupported constant type");
    }

    emitter_.emit_abx(vm::Opcode::LOAD_CONST, dest_reg, const_idx);
}

uint32_t FunctionCompiler::get_block_label(const ir::BasicBlock* block) {
    auto it = block_labels_.find(block);
    if (it != block_labels_.end()) {
        return it->second;
    }
    // Create new label for forward references
    uint32_t label = emitter_.create_label();
    block_labels_[block] = label;
    return label;
}

uint8_t FunctionCompiler::get_value_register(const ir::Value* value) {
    // Check if it's a constant
    if (auto* constant = dynamic_cast<const ir::Constant*>(value)) {
        // Allocate a temporary register for the constant
        uint8_t temp_reg = allocator_.register_count();
        compile_load_const(*constant, temp_reg);
        return temp_reg;
    }

    // Check if it's a parameter or instruction result
    if (allocator_.has_register(value)) {
        return allocator_.get_register(value);
    }

    throw std::runtime_error("Value not allocated: " + value->name());
}

vm::Opcode FunctionCompiler::map_opcode(ir::Instruction::Opcode ir_op) {
    switch (ir_op) {
        case ir::Instruction::Opcode::Add: return vm::Opcode::ADD;
        case ir::Instruction::Opcode::Sub: return vm::Opcode::SUB;
        case ir::Instruction::Opcode::Mul: return vm::Opcode::MUL;
        case ir::Instruction::Opcode::Div: return vm::Opcode::DIV;
        case ir::Instruction::Opcode::Mod: return vm::Opcode::MOD;
        case ir::Instruction::Opcode::Neg: return vm::Opcode::NEG;
        case ir::Instruction::Opcode::Eq:  return vm::Opcode::EQ;
        case ir::Instruction::Opcode::Ne:  return vm::Opcode::NE;
        case ir::Instruction::Opcode::Lt:  return vm::Opcode::LT;
        case ir::Instruction::Opcode::Le:  return vm::Opcode::LE;
        case ir::Instruction::Opcode::Gt:  return vm::Opcode::GT;
        case ir::Instruction::Opcode::Ge:  return vm::Opcode::GE;
        case ir::Instruction::Opcode::And: return vm::Opcode::AND;
        case ir::Instruction::Opcode::Or:  return vm::Opcode::OR;
        case ir::Instruction::Opcode::Not: return vm::Opcode::NOT;
        default:
            throw std::runtime_error("Cannot map IR opcode to VM opcode");
    }
}

} // namespace volta::bytecode