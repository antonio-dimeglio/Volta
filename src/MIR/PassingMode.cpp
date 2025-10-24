#include "MIR/PassingMode.hpp"

namespace MIR {

const char* passingModeToString(PassingMode mode) {
    switch (mode) {
        case PassingMode::ByValue:
            return "ByValue";
        case PassingMode::ByImmutableRef:
            return "ByImmutableRef";
        case PassingMode::ByMutableRef:
            return "ByMutableRef";
        default:
            return "Unknown";
    }
}

} // namespace MIR
