#include "vir/RuntimeFunctionRegistry.hpp"

namespace volta::vir {

// TypeDispatch implementation
std::string TypeDispatch::getCFunctionName(
    const std::vector<std::shared_ptr<semantic::Type>>& argTypes
) const {
    if (argTypes.empty()) {
        return "";  // Error: need at least one argument for type dispatch
    }

    std::string suffix = getTypeSuffix(argTypes[0]);
    if (suffix.empty()) {
        return "";  // Unsupported type
    }

    return baseName_ + "_" + suffix;
}

std::string TypeDispatch::getTypeSuffix(std::shared_ptr<semantic::Type> type) const {
    using Kind = semantic::Type::Kind;

    switch (type->kind()) {
        // Signed integers
        case Kind::I8:
        case Kind::I16:
        case Kind::I32:
        case Kind::I64:
            return "int";

        // Unsigned integers
        case Kind::U8:
        case Kind::U16:
        case Kind::U32:
        case Kind::U64:
            return "uint";

        // Floats
        case Kind::F8:
        case Kind::F16:
        case Kind::F32:
        case Kind::F64:
            return "float";

        case Kind::Bool:
            return "bool";

        case Kind::String:
            return "string";

        default:
            return "";  // Unsupported
    }
}

// RuntimeFunctionRegistry implementation
RuntimeFunctionRegistry::RuntimeFunctionRegistry() {
    // Register all runtime functions here
    // Adding a new runtime function is just one line!

    // === Printing ===
    registerFunction("print", std::make_unique<TypeDispatch>("volta_print"));

    // === Future: Panic ===
    // registerFunction("panic", std::make_unique<DirectDispatch>("volta_panic"));

    // === Future: Arrays (Phase 7) ===
    // registerFunction("Array.new", std::make_unique<DirectDispatch>("volta_array_new"));
    // registerFunction("Array.push", std::make_unique<DirectDispatch>("volta_array_push"));
    // registerFunction("Array.pop", std::make_unique<DirectDispatch>("volta_array_pop"));
    // registerFunction("Array.get", std::make_unique<DirectDispatch>("volta_array_get"));
    // registerFunction("Array.set", std::make_unique<DirectDispatch>("volta_array_set"));
    // registerFunction("Array.length", std::make_unique<DirectDispatch>("volta_array_length"));

    // === Future: Strings (Phase 6) ===
    // registerFunction("String.concat", std::make_unique<DirectDispatch>("volta_string_concat"));
    // registerFunction("String.length", std::make_unique<DirectDispatch>("volta_string_length"));
}

bool RuntimeFunctionRegistry::isRuntimeFunction(const std::string& name) const {
    return functions_.find(name) != functions_.end();
}

std::string RuntimeFunctionRegistry::getCFunctionName(
    const std::string& voltaName,
    const std::vector<std::shared_ptr<semantic::Type>>& argTypes
) const {
    auto it = functions_.find(voltaName);
    if (it == functions_.end()) {
        return "";  // Not a runtime function
    }

    return it->second->getCFunctionName(argTypes);
}

void RuntimeFunctionRegistry::registerFunction(
    std::string voltaName,
    std::unique_ptr<RuntimeDispatchStrategy> strategy
) {
    functions_[std::move(voltaName)] = std::move(strategy);
}

} // namespace volta::vir