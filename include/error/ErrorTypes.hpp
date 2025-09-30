#pragma once 
#include <string>

namespace volta::errors {

struct SourceLocation {
    std::string filename;
    size_t line;
    size_t column;
    size_t length;

    SourceLocation() : filename(""), line(0), column(0), length(0) {}
    SourceLocation(const std::string& filename, size_t line, size_t column, size_t length = 1)
        : filename(filename), line(line), column(column), length(length) {}

    std::string toString() const;
};

class CompilerError {
    public:
        enum class Level {
            Error,
            Warning,
            Note
        };

    protected:
        Level level_;
        std::string message_;
        SourceLocation location_;

    public:
        CompilerError(Level level, std::string message, SourceLocation location);
        virtual ~CompilerError() = default;

        Level getLevel() const { return level_; }
        const std::string& getMessage() const { return message_; }
        const SourceLocation& getLocation() const { return location_; }
        virtual std::string formatError() const;
};

class SyntaxError : public CompilerError {
    SyntaxError(std::string message, SourceLocation location);
    std::string formatError() const override;
};

}