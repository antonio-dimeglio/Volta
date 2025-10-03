#pragma once

#include "Value.hpp"

namespace volta::ir {

/**
 * Instruction - Forward declaration and minimal stub
 *
 * This is a placeholder. You'll implement the full Instruction hierarchy later.
 * For now, this allows Value.cpp and tests to compile.
 */
class Instruction : public Value {
public:
    // Placeholder - you'll expand this later
    virtual ~Instruction() = default;

protected:
    Instruction(ValueKind kind, std::shared_ptr<semantic::Type> type, const std::string& name = "")
        : Value(kind, type, name) {}
};

} // namespace volta::ir
