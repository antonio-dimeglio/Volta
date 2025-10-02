#include "bytecode/RegisterAllocator.hpp"
#include <algorithm>
#include <stdexcept>

namespace volta::bytecode {

RegisterAllocator::RegisterAllocator(const ir::Function& func)
    : func_(func), next_register_(0), max_register_(0) {
    assign_parameter_registers();
    number_instructions();
    analyze_live_ranges();
    allocate_registers();
}

uint8_t RegisterAllocator::get_register(const ir::Value* value) const {
    auto it = allocation_.find(value);
    if (it != allocation_.end()) {
        return it->second;
    }
    throw std::runtime_error("Unallocated value: " + value->name());
}

bool RegisterAllocator::has_register(const ir::Value* value) const {
    return allocation_.find(value) != allocation_.end();
}

void RegisterAllocator::analyze_live_ranges() {
    // Simplified: just assign sequential registers for now
    // TODO: Implement proper live range analysis
}

void RegisterAllocator::allocate_registers() {
    // Sort live ranges by start position
    std::sort(live_ranges_.begin(), live_ranges_.end(),
              [](const LiveRange& a, const LiveRange& b) { return a.start < b.start; });

    // Initialize free registers (skip parameter registers)
    for (int i = next_register_; i < 256; i++) {
        free_registers_.push_back(static_cast<uint8_t>(i));
    }

    // Allocate registers using linear scan
    for (auto& range : live_ranges_) {
        // Expire old intervals
        expire_old_intervals(range.start);

        // Allocate a register
        if (free_registers_.empty()) {
            throw std::runtime_error("Too many live registers (>256)");
        }

        uint8_t reg = free_registers_.front();
        free_registers_.erase(free_registers_.begin());

        range.physical_reg = reg;
        allocation_[range.value] = reg;
        if (static_cast<uint32_t>(reg) > max_register_) max_register_ = reg;

        active_.push_back(&range);
    }
}

void RegisterAllocator::expire_old_intervals(uint32_t position) {
    // Sort active by end position
    std::sort(active_.begin(), active_.end(),
              [](const LiveRange* a, const LiveRange* b) { return a->end < b->end; });

    // Remove ranges that ended before this position
    while (!active_.empty() && active_.front()->end < position) {
        LiveRange* range = active_.front();
        active_.erase(active_.begin());
        free_registers_.push_back(range->physical_reg);
    }
}

void RegisterAllocator::assign_parameter_registers() {
    const auto& params = func_.parameters();
    for (size_t i = 0; i < params.size(); i++) {
        allocation_[params[i].get()] = static_cast<uint8_t>(i);
        if (i > max_register_) max_register_ = i;
    }
    next_register_ = params.size();
}

uint32_t RegisterAllocator::number_instructions() {
    uint32_t position = 0;
    for (const auto& block : func_.basicBlocks()) {
        for (const auto& inst : block->instructions()) {
            instruction_positions_[inst.get()] = position++;
        }
    }
    return position;
}

} // namespace volta::bytecode