#include "Error/Error.hpp"
#include <sstream>

// ANSI color codes
namespace Color {
    constexpr const char* Reset = "\033[0m";
    constexpr const char* Red = "\033[1;31m";
    constexpr const char* Yellow = "\033[1;33m";
    constexpr const char* Cyan = "\033[1;36m";
    constexpr const char* Bold = "\033[1m";
}

void DiagnosticManager::report(DiagnosticLevel level, const std::string& message,
                                size_t line, size_t column) {
    // Don't record errors during speculative parsing
    if (suppressErrors && level == DiagnosticLevel::Error) {
        return;
    }

    diagnostics.emplace_back(level, message, SourceLocation(line, column));

    if (level == DiagnosticLevel::Error) {
        errorCount++;
    } else if (level == DiagnosticLevel::Warning) {
        warningCount++;
    }
}

void DiagnosticManager::error(const std::string& message, size_t line, size_t column) {
    report(DiagnosticLevel::Error, message, line, column);
}

void DiagnosticManager::warning(const std::string& message, size_t line, size_t column) {
    report(DiagnosticLevel::Warning, message, line, column);
}

void DiagnosticManager::info(const std::string& message, size_t line, size_t column) {
    report(DiagnosticLevel::Info, message, line, column);
}

void DiagnosticManager::note(const std::string& message) const {
    std::ostream& os = std::cerr;

    if (useColor) {
        os << Color::Cyan << Color::Bold << "note" << Color::Reset;
    } else {
        os << "note";
    }

    os << ": " << message << "\n";
}

void DiagnosticManager::printAll(std::ostream& os, const std::string& filename) const {
    for (const auto& diag : diagnostics) {
        // Print diagnostic level
        if (useColor) {
            switch (diag.level) {
                case DiagnosticLevel::Error:
                    os << Color::Red << Color::Bold << "error" << Color::Reset;
                    break;
                case DiagnosticLevel::Warning:
                    os << Color::Yellow << Color::Bold << "warning" << Color::Reset;
                    break;
                case DiagnosticLevel::Info:
                    os << Color::Cyan << Color::Bold << "info" << Color::Reset;
                    break;
            }
        } else {
            switch (diag.level) {
                case DiagnosticLevel::Error:
                    os << "error";
                    break;
                case DiagnosticLevel::Warning:
                    os << "warning";
                    break;
                case DiagnosticLevel::Info:
                    os << "info";
                    break;
            }
        }

        // Print message
        os << ": " << diag.message << "\n";

        // Print location
        os << "  --> ";
        if (!filename.empty()) {
            os << filename << ":";
        }
        os << diag.location.line << ":" << diag.location.column << "\n\n";
    }

    // Print summary
    if (errorCount > 0 || warningCount > 0) {
        if (errorCount > 0) {
            os << errorCount << " error" << (errorCount > 1 ? "s" : "");
        }
        if (errorCount > 0 && warningCount > 0) {
            os << ", ";
        }
        if (warningCount > 0) {
            os << warningCount << " warning" << (warningCount > 1 ? "s" : "");
        }
        os << " generated.\n";
    }
}

void DiagnosticManager::clear() {
    diagnostics.clear();
    errorCount = 0;
    warningCount = 0;
}
