#pragma once

#include "IR/IR.hpp"
#include "bytecode/BytecodeEmitter.hpp"
#include "bytecode/RegisterAllocator.hpp"
#include "bytecode/ConstantPool.hpp"
#include <unordered_map>

namespace volta::bytecode {

/**
 * @brief Function metadata in bytecode
 */
struct FunctionMetadata {
    uint32_t name_offset;      // Offset into string table
    uint32_t bytecode_offset;  // Offset into bytecode section
    uint32_t bytecode_length;  // Length of bytecode
    uint8_t arity;            // Number of parameters
    uint8_t register_count;   // Total registers needed
    uint8_t is_variadic;      // 0 or 1
    uint8_t reserved;         // Padding
};

/**
 * @brief Compiles a single IR function to bytecode
 *
 * Handles:
 * - Register allocation
 * - Instruction compilation
 * - Control flow (jumps, branches)
 * - Function calls
 */
class FunctionCompiler {
public:
    /**
     * @brief Construct function compiler
     * @param func IR function to compile
     * @param constants Constant pool builder
     */
    FunctionCompiler(
        const volta::ir::Function& func,
        ConstantPoolBuilder& constants
    );

    /**
     * @brief Compile function to bytecode
     * @return Function metadata
     *
     * Implementation:
     * 1. Create RegisterAllocator for func_
     * 2. Create BytecodeEmitter
     * 3. Compile function prologue (if needed)
     * 4. For each basic block:
     *    - Mark block label
     *    - Compile instructions
     * 5. Resolve jumps
     * 6. Return FunctionMetadata:
     *    - name_offset (from string table)
     *    - bytecode_offset (0 for now, set by generator)
     *    - bytecode_length
     *    - arity
     *    - register_count
     */
    FunctionMetadata compile();

    /**
     * @brief Get compiled bytecode
     */
    const std::vector<uint8_t>& bytecode() const;

private:
    // ========================================================================
    // Basic block compilation
    // ========================================================================

    /**
     * @brief Compile a basic block
     * @param bb Basic block to compile
     *
     * Implementation:
     * 1. Mark label for block
     * 2. For each instruction in bb:
     *    - Call compile_instruction()
     */
    void compile_basic_block(const ir::BasicBlock& bb);

    /**
     * @brief Compile a single instruction
     * @param inst Instruction to compile
     *
     * Implementation:
     * Switch on instruction type:
     * - BinaryInst: compile_binary_op()
     * - CallInst: compile_call()
     * - ReturnInst: compile_return()
     * - AllocaInst: compile_alloca()
     * - LoadInst: compile_load()
     * - StoreInst: compile_store()
     * - GetFieldInst: compile_get_field()
     * - SetFieldInst: compile_set_field()
     * - GetIndexInst: compile_get_index()
     * - SetIndexInst: compile_set_index()
     * - BranchInst: compile_branch()
     * - CondBranchInst: compile_cond_branch()
     * - etc.
     */
    void compile_instruction(const ir::Instruction& inst);

    // ========================================================================
    // Instruction type compilation
    // ========================================================================

    /**
     * @brief Compile binary operation
     * @param inst Binary instruction (add, sub, mul, etc.)
     *
     * Implementation:
     * 1. Get destination register: rd = allocator_.get_register(&inst)
     * 2. Get operand registers: r1, r2
     * 3. Map opcode: Add->ADD, Sub->SUB, etc.
     * 4. Emit: emitter_.emit_abc(opcode, rd, r1, r2)
     */
    void compile_binary_op(const ir::BinaryInst& inst);

    /**
     * @brief Compile constant load
     * @param constant Constant to load
     * @param dest_reg Destination register
     *
     * Implementation:
     * 1. Check constant type
     * 2. For special cases (true, false, none):
     *    - Use LOAD_TRUE, LOAD_FALSE, LOAD_NONE
     * 3. For other constants:
     *    - Add to constant pool: kx = constants_.add_xxx(value)
     *    - Emit: emitter_.emit_abx(LOAD_CONST, dest_reg, kx)
     */
    void compile_load_const(const ir::Constant& constant, uint8_t dest_reg);

    /**
     * @brief Compile function call
     * @param inst Call instruction
     *
     * Implementation:
     * 1. Get function value (could be constant or register)
     * 2. If constant function name:
     *    - Add name to constant pool
     *    - Load to temporary register
     * 3. Compile arguments to registers
     * 4. Emit CALL: emitter_.emit_abc(CALL, dest, func_reg, argc)
     */
    void compile_call(const ir::CallInst& inst);

    /**
     * @brief Compile foreign function call
     * @param inst CallForeign instruction
     *
     * Implementation:
     * 1. Load foreign function global into a register
     * 2. Compile arguments to consecutive registers
     * 3. Emit CALL instruction (VM handles foreign/native transparently)
     */
    void compile_call_foreign(const ir::CallForeignInst& inst);

    /**
     * @brief Compile return statement
     * @param inst Return instruction
     *
     * Implementation:
     * 1. If has return value:
     *    - Get return register
     *    - Emit: emitter_.emit_a(RETURN, reg)
     * 2. Else:
     *    - Emit: emitter_.emit_opcode(RETURN_NONE)
     */
    void compile_return(const ir::ReturnInst& inst);

    /**
     * @brief Compile alloca (struct/array allocation)
     * @param inst Alloca instruction
     *
     * Implementation:
     * 1. Get destination register
     * 2. Determine type (struct or array)
     * 3. If struct:
     *    - Get field count
     *    - Emit: NEW_STRUCT dest, field_count
     * 4. If array:
     *    - Get size (from operand or constant)
     *    - Emit: NEW_ARRAY dest, size_reg
     */
    // void compile_alloca(const ir::AllocaInst& inst);  // TODO: Define AllocaInst in IR

    /**
     * @brief Compile field get
     * @param inst GetField instruction
     *
     * Implementation:
     * 1. Get registers: dest, obj, field_index
     * 2. Emit: GET_FIELD dest, obj, field_index
     */
    void compile_get_field(const ir::GetFieldInst& inst);

    /**
     * @brief Compile field set
     * @param inst SetField instruction
     *
     * Implementation:
     * 1. Get registers: obj, field_index, value
     * 2. Emit: SET_FIELD obj, field_index, value
     */
    void compile_set_field(const ir::SetFieldInst& inst);

    /**
     * @brief Compile array index get
     * @param inst GetIndex instruction
     *
     * Implementation:
     * 1. Get registers: dest, array, index
     * 2. Emit: GET_INDEX dest, array, index
     */
    // void compile_get_index(const ir::GetIndexInst& inst);  // TODO: Define in IR

    /**
     * @brief Compile array index set
     * @param inst SetIndex instruction
     *
     * Implementation:
     * 1. Get registers: array, index, value
     * 2. Emit: SET_INDEX array, index, value
     */
    // void compile_set_index(const ir::SetIndexInst& inst);  // TODO: Define in IR

    /**
     * @brief Compile unconditional branch
     * @param inst Branch instruction
     *
     * Implementation:
     * 1. Get target basic block
     * 2. Get label for block: label = block_labels_[target]
     * 3. Emit: emitter_.emit_jump(label)
     */
    void compile_branch(const ir::BranchInst& inst);

    /**
     * @brief Compile conditional branch
     * @param inst Conditional branch instruction
     *
     * Implementation:
     * 1. Get condition register
     * 2. Get true/false target blocks
     * 3. Get labels for blocks
     * 4. Emit: emitter_.emit_jump_if_false(cond_reg, false_label)
     * 5. Emit: emitter_.emit_jump(true_label)
     */
    // void compile_cond_branch(const ir::CondBranchInst& inst);  // TODO: Define in IR

    /**
     * @brief Compile load instruction
     * @param inst Load instruction
     *
     * Implementation:
     * 1. Get source register (pointer)
     * 2. Get destination register
     * 3. Emit: MOVE dest, src
     *    (Load is typically optimized away or becomes move)
     */
    void compile_load(const ir::LoadInst& inst);

    /**
     * @brief Compile store instruction
     * @param inst Store instruction
     *
     * Implementation:
     * 1. Get destination register (pointer)
     * 2. Get value register
     * 3. Emit: MOVE dest, value
     */
    void compile_store(const ir::StoreInst& inst);

    // ========================================================================
    // Helpers
    // ========================================================================

    /**
     * @brief Get or create label for basic block
     * @param bb Basic block
     * @return Label ID
     */
    uint32_t get_block_label(const ir::BasicBlock* bb);

    /**
     * @brief Get register for IR value
     * @param value IR value
     * @return Physical register
     *
     * Implementation:
     * 1. If value is Constant:
     *    - Allocate temporary register
     *    - Compile load constant
     *    - Return temporary
     * 2. Else:
     *    - Return allocator_.get_register(value)
     */
    uint8_t get_value_register(const ir::Value* value);

    /**
     * @brief Map IR opcode to VM opcode
     * @param ir_op IR instruction opcode
     * @return VM opcode
     */
    vm::Opcode map_opcode(ir::Instruction::Opcode ir_op);

    const ir::Function& func_;
    ConstantPoolBuilder& constants_;
    RegisterAllocator allocator_;
    BytecodeEmitter emitter_;

    // Basic block labels
    std::unordered_map<const ir::BasicBlock*, uint32_t> block_labels_;

    // Temporary registers for constants
    uint8_t next_temp_register_ = 0;
};

} // namespace volta::bytecode
