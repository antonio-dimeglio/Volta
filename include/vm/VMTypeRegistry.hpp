#pragma once

#include "gc/TypeRegistry.hpp"
#include "vm/BytecodeModule.hpp"

namespace volta::vm {

/**
 * @brief Type registry implementation that delegates to BytecodeModule
 * 
 * Provides type information to the GC by querying the current module's
 * type metadata table.
 */
class VMTypeRegistry : public Volta::GC::TypeRegistry {
public:
    /**
     * @brief Construct a new VMTypeRegistry
     * @param module Pointer to the bytecode module (can be null initially)
     */
    explicit VMTypeRegistry(BytecodeModule* module = nullptr);
    
    /**
     * @brief Set the current module
     * @param module Pointer to the bytecode module
     */
    void setModule(BytecodeModule* module);
    
    /**
     * @brief Get type information for a type ID (from GC)
     * @param typeId The type ID to query
     * @return Type information, or nullptr if not found
     */
    const Volta::GC::TypeInfo* getTypeInfo(uint32_t typeId) const override;

private:
    BytecodeModule* module_;  ///< Current bytecode module
};

} // namespace volta::vm