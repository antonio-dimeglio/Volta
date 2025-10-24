#pragma once

namespace MIR {

// Describes how a parameter is passed to a function
enum class PassingMode {
    ByValue,           // x: Type - copy the value
    ByImmutableRef,    // x: ref Type - pass pointer, read-only
    ByMutableRef       // x: mut ref Type - pass pointer, writable
};

// Helper: Convert PassingMode to string for debugging
const char* passingModeToString(PassingMode mode);

} // namespace MIR
