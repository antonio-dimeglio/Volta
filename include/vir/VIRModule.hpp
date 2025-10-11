#pragma once

#include "vir/VIRStatement.hpp"
#include <vector>
#include <string>
#include <memory>

namespace volta::vir {

// ============================================================================
// Function Definition
// ============================================================================

struct VIRParam {
    std::string name;
    std::shared_ptr<volta::semantic::Type> type;
    bool isMutable;
    VIRParam(std::string name,
            std::shared_ptr<volta::semantic::Type> type,
            bool isMutable)
    : name(std::move(name)), type(std::move(type)), isMutable(isMutable) {}
};

class VIRFunction {
public:
    VIRFunction(std::string name, std::vector<VIRParam> params,
                std::shared_ptr<volta::semantic::Type> returnType,
                std::unique_ptr<VIRBlock> body, bool isPublic);

    const std::string& getName() const { return name_; }
    const std::vector<VIRParam>& getParams() const { return params_; }
    std::shared_ptr<volta::semantic::Type> getReturnType() const { return returnType_; }
    const VIRBlock* getBody() const { return body_.get(); }
    bool isPublic() const { return isPublic_; }

private:
    std::string name_;
    std::vector<VIRParam> params_;
    std::shared_ptr<volta::semantic::Type> returnType_;
    std::unique_ptr<VIRBlock> body_;
    bool isPublic_;
};

// ============================================================================
// Struct Declaration (for later phases)
// ============================================================================

struct VIRStructField {
    std::string name;
    std::shared_ptr<volta::semantic::Type> type;
};

class VIRStructDecl {
public:
    VIRStructDecl(std::string name, std::vector<VIRStructField> fields);

    const std::string& getName() const { return name_; }
    const std::vector<VIRStructField>& getFields() const { return fields_; }

private:
    std::string name_;
    std::vector<VIRStructField> fields_;
};

// ============================================================================
// Enum Declaration (for later phases)
// ============================================================================

struct VIREnumVariant {
    std::string name;
    size_t tag;
    std::vector<std::shared_ptr<volta::semantic::Type>> dataTypes;
};

class VIREnumDecl {
public:
    VIREnumDecl(std::string name, std::vector<VIREnumVariant> variants);

    const std::string& getName() const { return name_; }
    const std::vector<VIREnumVariant>& getVariants() const { return variants_; }

private:
    std::string name_;
    std::vector<VIREnumVariant> variants_;
};

// ============================================================================
// Module (Top-Level Compilation Unit)
// ============================================================================

class VIRModule {
public:
    VIRModule(std::string name);

    const std::string& getName() const { return name_; }

    // Add declarations
    void addFunction(std::unique_ptr<VIRFunction> func);
    void addStruct(std::unique_ptr<VIRStructDecl> structDecl);
    void addEnum(std::unique_ptr<VIREnumDecl> enumDecl);

    // Get declarations
    const std::vector<std::unique_ptr<VIRFunction>>& getFunctions() const { return functions_; }
    const std::vector<std::unique_ptr<VIRStructDecl>>& getStructs() const { return structs_; }
    const std::vector<std::unique_ptr<VIREnumDecl>>& getEnums() const { return enums_; }

private:
    std::string name_;
    std::vector<std::unique_ptr<VIRFunction>> functions_;
    std::vector<std::unique_ptr<VIRStructDecl>> structs_;
    std::vector<std::unique_ptr<VIREnumDecl>> enums_;
    std::vector<std::string> imports_;  // For future module system
};

} // namespace volta::vir
