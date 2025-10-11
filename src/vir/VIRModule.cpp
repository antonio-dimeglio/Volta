#include "vir/VIRModule.hpp"

namespace volta::vir {

// ============================================================================
// VIRFunction
// ============================================================================

VIRFunction::VIRFunction(std::string name, std::vector<VIRParam> params,
                          std::shared_ptr<volta::semantic::Type> returnType,
                          std::unique_ptr<VIRBlock> body, bool isPublic)
    : name_(std::move(name)), params_(std::move(params)), returnType_(returnType),
      body_(std::move(body)), isPublic_(isPublic) {}

// ============================================================================
// VIRStructDecl
// ============================================================================

VIRStructDecl::VIRStructDecl(std::string name, std::vector<VIRStructField> fields)
    : name_(std::move(name)), fields_(std::move(fields)) {}

// ============================================================================
// VIREnumDecl
// ============================================================================

VIREnumDecl::VIREnumDecl(std::string name, std::vector<VIREnumVariant> variants)
    : name_(std::move(name)), variants_(std::move(variants)) {}

// ============================================================================
// VIRModule
// ============================================================================

VIRModule::VIRModule(std::string name)
    : name_(std::move(name)) {}

void VIRModule::addFunction(std::unique_ptr<VIRFunction> func) {
    functions_.push_back(std::move(func));
}

void VIRModule::addStruct(std::unique_ptr<VIRStructDecl> structDecl) {
    structs_.push_back(std::move(structDecl));
}

void VIRModule::addEnum(std::unique_ptr<VIREnumDecl> enumDecl) {
    enums_.push_back(std::move(enumDecl));
}

} // namespace volta::vir
