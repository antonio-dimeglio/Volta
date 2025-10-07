#include "vm/VMTypeRegistry.hpp"

namespace volta::vm {

VMTypeRegistry::VMTypeRegistry(BytecodeModule* module)
    : module_(module)
{}

void VMTypeRegistry::setModule(BytecodeModule* module) {
    module_ = module;
}

const Volta::GC::TypeInfo* VMTypeRegistry::getTypeInfo(uint32_t typeId) const {
    if (!module_) {
        return nullptr;
    }
    return module_->getTypeInfo(typeId);
}

} // namespace volta::vm