#pragma once

#include "semantic/Type.hpp"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

namespace volta::vir {

/**
 * Strategy for mapping Volta function calls to C runtime functions
 */
class RuntimeDispatchStrategy {
public:
    virtual ~RuntimeDispatchStrategy() = default;

    /**
     * Get the C function name for this call
     * @param argTypes The types of the arguments
     * @return The C function name (e.g., "volta_print_int")
     */
    virtual std::string getCFunctionName(
        const std::vector<std::shared_ptr<semantic::Type>>& argTypes
    ) const = 0;
};

/**
 * Direct mapping: Volta function name → C function name
 * Example: panic() → volta_panic
 */
class DirectDispatch : public RuntimeDispatchStrategy {
public:
    explicit DirectDispatch(std::string cFunctionName)
        : cFunctionName_(std::move(cFunctionName)) {}

    std::string getCFunctionName(
        const std::vector<std::shared_ptr<semantic::Type>>& argTypes
    ) const override {
        return cFunctionName_;
    }

private:
    std::string cFunctionName_;
};

/**
 * Type-based dispatch: Choose C function based on first argument type
 * Example: print(42) → volta_print_int, print(3.14) → volta_print_float
 */
class TypeDispatch : public RuntimeDispatchStrategy {
public:
    explicit TypeDispatch(std::string baseName)
        : baseName_(std::move(baseName)) {}

    std::string getCFunctionName(
        const std::vector<std::shared_ptr<semantic::Type>>& argTypes
    ) const override;

private:
    std::string baseName_;  // e.g., "volta_print"
    std::string getTypeSuffix(std::shared_ptr<semantic::Type> type) const;
};

/**
 * Registry of all runtime functions
 * Makes it trivial to add new runtime functions
 */
class RuntimeFunctionRegistry {
public:
    RuntimeFunctionRegistry();

    /**
     * Check if a function name is a registered runtime function
     */
    bool isRuntimeFunction(const std::string& name) const;

    /**
     * Get the C function name for a runtime function call
     * @param voltaName The Volta function name (e.g., "print")
     * @param argTypes The argument types
     * @return The C function name (e.g., "volta_print_int"), or empty if not found
     */
    std::string getCFunctionName(
        const std::string& voltaName,
        const std::vector<std::shared_ptr<semantic::Type>>& argTypes
    ) const;

private:
    void registerFunction(std::string voltaName, std::unique_ptr<RuntimeDispatchStrategy> strategy);

    std::unordered_map<std::string, std::unique_ptr<RuntimeDispatchStrategy>> functions_;
};

} // namespace volta::vir