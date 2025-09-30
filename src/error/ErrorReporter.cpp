#include "error/ErrorReporter.hpp"
#include <algorithm>

namespace volta::errors {
void ErrorReporter::reportError(std::unique_ptr<CompilerError> error) {
    errors_.push_back(std::move(error));
}

void ErrorReporter::reportSyntaxError(std::string message, SourceLocation location) { 
    auto error = std::make_unique<CompilerError>(CompilerError::Level::Error, message, location);
    reportError(std::move(error));
}

void ErrorReporter::reportWarning(std::string message, SourceLocation location) {
    auto warning = std::make_unique<CompilerError>(CompilerError::Level::Warning, message, location);
    reportError(std::move(warning));
}

bool ErrorReporter::hasErrors() const {
    for (const auto& error : errors_) {
        if (error->getLevel() == CompilerError::Level::Error) {
            return true;
        }
    }
    return false;
}

bool ErrorReporter::hasWarnings() const {
    for (const auto& error : errors_) {
        if (error->getLevel() == CompilerError::Level::Warning) {
            return true;
        }
    }
    return false;
}

size_t ErrorReporter::getErrorCount() const {
    size_t count = 0;
    for (const auto& error : errors_) {
        if (error->getLevel() == CompilerError::Level::Error) {
            count++;
        }
    }
    return count;
}

size_t ErrorReporter::getWarningCount() const {
    size_t count = 0;
    for (const auto& error : errors_) {
        if (error->getLevel() == CompilerError::Level::Warning) {
            count++;
        }
    }
    return count;
}

void ErrorReporter::printErrors(std::ostream& out) const {
    for (const auto& error : errors_) {
        out << error->formatError() << std::endl;
    }
}

void ErrorReporter::printSummary(std::ostream& out) const {
    size_t errorCount = getErrorCount();
    size_t warningCount = getWarningCount();

    out << "Compilation finished with " << errorCount << " error(s) and " 
        << warningCount << " warning(s)." << std::endl;
}

void ErrorReporter::clear() {
    errors_.clear();
}

void ErrorReporter::sortErrorsByLocation() {
    std::sort(errors_.begin(), errors_.end(), [](const std::unique_ptr<CompilerError>& a, const std::unique_ptr<CompilerError>& b) {
        const auto& locA = a->getLocation();
        const auto& locB = b->getLocation();
        if (locA.filename != locB.filename) {
            return locA.filename < locB.filename;
        }
        if (locA.line != locB.line) {
            return locA.line < locB.line;
        }
        return locA.column < locB.column;
    });
}
}