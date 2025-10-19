#pragma once
#include <string>
#include <vector>
#include <iostream>

enum class DiagnosticLevel {
    Error,
    Warning,
    Info
};

struct SourceLocation {
    size_t line;
    size_t column;

    SourceLocation(size_t line, size_t column)
        : line(line), column(column) {}
};

struct Diagnostic {
    DiagnosticLevel level;
    std::string message;
    SourceLocation location;

    Diagnostic(DiagnosticLevel level, std::string message, SourceLocation location)
        : level(level), message(std::move(message)), location(location) {}
};

class DiagnosticManager {
private:
    std::vector<Diagnostic> diagnostics;
    size_t errorCount = 0;
    size_t warningCount = 0;
    bool useColor = true;

public:
    DiagnosticManager(bool useColor = true) : useColor(useColor) {}

    void report(DiagnosticLevel level, const std::string& message,
                size_t line, size_t column);

    void error(const std::string& message, size_t line, size_t column);
    void warning(const std::string& message, size_t line, size_t column);
    void info(const std::string& message, size_t line, size_t column);
    void note(const std::string& message);

    bool hasErrors() const { return errorCount > 0; }
    size_t getErrorCount() const { return errorCount; }
    size_t getWarningCount() const { return warningCount; }

    void printAll(std::ostream& os = std::cerr, const std::string& filename = "") const;
    void clear();
};