#include "error/ErrorTypes.hpp"

namespace volta::errors {
std::string SourceLocation::toString() const {
    return filename + ":" + std::to_string(line) + ":" + std::to_string(column) + "-" + std::to_string(column + length);
}


CompilerError::CompilerError(Level level, std::string message, SourceLocation location) {
    level_ = level;
    message_ = std::move(message);
    location_ = std::move(location);
}

std::string CompilerError::formatError() const {
    std::string levelStr;
    switch (level_) {
        case Level::Error: levelStr = "error"; break;
        case Level::Warning: levelStr = "warning"; break;
        case Level::Note: levelStr = "note"; break;
    }
    return location_.toString() + ": " + levelStr + ": " + message_;
}

SyntaxError::SyntaxError(std::string message, SourceLocation location) 
    : CompilerError(Level::Error, std::move(message), std::move(location)) {}

std::string SyntaxError::formatError() const {
    return CompilerError::formatError();
}
}