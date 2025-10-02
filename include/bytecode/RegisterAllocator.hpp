#pragma once

#include "IR/IR.hpp"
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace volta::bytecode {

/**
 * @brief Live range for a virtual register
 */
struct LiveRange {
    const ir::Value* value;  // IR value
    uint32_t start;          // First use position
    uint32_t end;            // Last use position
    uint8_t physical_reg;    // Assigned physical register (0-255)
};

/**
 * @brief Register allocator using linear scan
 *
 * Maps unlimited IR virtual registers to 256 physical VM registers.
 * Uses simple linear scan algorithm:
 * 1. Compute live ranges for all values
 * 2. Sort by start position
 * 3. Assign registers greedily, reusing freed registers
 */
class RegisterAllocator {
public:
    /**
     * @brief Construct allocator for a function
     * @param func IR function to allocate
     *
     * Implementation:
     * 1. Reserve r0-r{arity-1} for parameters
     * 2. Analyze live ranges
     * 3. Allocate registers
     */
    explicit RegisterAllocator(const ir::Function& func);

    /**
     * @brief Get physical register for IR value
     * @param value IR value (instruction, parameter, constant)
     * @return Physical register number (0-255)
     *
     * Implementation:
     * 1. Look up value in allocation_ map
     * 2. If found, return physical register
     * 3. Else error: unallocated value
     */
    uint8_t get_register(const ir::Value* value) const;

    /**
     * @brief Check if value has allocated register
     */
    bool has_register(const ir::Value* value) const;

    /**
     * @brief Get total register count needed
     * @return Highest register number + 1
     */
    uint8_t register_count() const { return max_register_ + 1; }

    /**
     * @brief Get live ranges (for debugging)
     */
    const std::vector<LiveRange>& live_ranges() const { return live_ranges_; }

private:
    /**
     * @brief Analyze live ranges for all values
     *
     * Implementation:
     * 1. Number all instructions sequentially
     * 2. For each value (instruction result, parameter):
     *    - Find first use (definition point)
     *    - Find last use (scan all instructions)
     *    - Create LiveRange(value, start, end)
     * 3. Store in live_ranges_
     */
    void analyze_live_ranges();

    /**
     * @brief Allocate physical registers
     *
     * Linear scan algorithm:
     * 1. Sort live_ranges_ by start position
     * 2. Maintain active list (currently live ranges)
     * 3. Maintain free list (available physical registers)
     * 4. For each live range:
     *    a. Expire old ranges that ended before current start
     *    b. Free their physical registers
     *    c. If free register available:
     *       - Assign it
     *    d. Else:
     *       - Error: too many live registers (>256)
     *       - Future: spill to memory
     */
    void allocate_registers();

    /**
     * @brief Expire live ranges that ended
     * @param position Current position
     *
     * Implementation:
     * For each range in active_:
     * - If range.end < position:
     *   - Add range.physical_reg to free_registers_
     *   - Remove from active_
     */
    void expire_old_intervals(uint32_t position);

    /**
     * @brief Assign parameter registers
     *
     * Implementation:
     * For each parameter (up to arity):
     * - Assign parameter i to register i
     * - Add to allocation_ map
     * - Mark register as used
     */
    void assign_parameter_registers();

    /**
     * @brief Number all instructions
     * @return Total instruction count
     *
     * Implementation:
     * Visit all basic blocks in order:
     * - Assign sequential numbers to each instruction
     * - Store in instruction_positions_ map
     */
    uint32_t number_instructions();

    const ir::Function& func_;
    std::vector<LiveRange> live_ranges_;
    std::unordered_map<const ir::Value*, uint8_t> allocation_;
    std::unordered_map<const ir::Instruction*, uint32_t> instruction_positions_;

    std::vector<LiveRange*> active_;       // Currently live ranges
    std::vector<uint8_t> free_registers_;  // Available physical registers

    uint8_t next_register_ = 0;
    uint8_t max_register_ = 0;
};

} // namespace volta::bytecode
