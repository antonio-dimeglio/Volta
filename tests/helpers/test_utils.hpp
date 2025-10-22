#pragma once

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <fstream>
#include <cstdio>
#include "../../include/Lexer/Lexer.hpp"
#include "../../include/Lexer/Token.hpp"
#include "../../include/Parser/Parser.hpp"
#include "../../include/Parser/AST.hpp"
#include "../../include/Type/Type.hpp"
#include "../../include/Type/TypeRegistry.hpp"
#include "../../include/Semantic/SemanticAnalyzer.hpp"
#include "../../include/HIR/Lowering.hpp"
#include "../../include/Error/Error.hpp"

namespace VoltaTest {

// Helper to create a lexer from source code
inline Lexer createLexer(const std::string& source, DiagnosticManager& diag) {
    return Lexer(source, diag);
}

// Helper to tokenize source and return all tokens
inline std::vector<Token> tokenize(const std::string& source, DiagnosticManager& diag) {
    Lexer lexer(source, diag);
    return lexer.tokenize();
}

// Helper overload that creates its own DiagnosticManager
inline std::vector<Token> tokenize(const std::string& source) {
    DiagnosticManager diag(false); // No color for tests
    return tokenize(source, diag);
}

// Helper to check if a token matches expected type and lexeme
inline void expectToken(const Token& token, TokenType expectedType, const std::string& expectedLexeme = "") {
    EXPECT_EQ(token.tt, expectedType)
        << "Expected " << tokenTypeToString(expectedType)
        << " but got " << tokenTypeToString(token.tt);
    if (!expectedLexeme.empty()) {
        EXPECT_EQ(token.lexeme, expectedLexeme);
    }
}

// Helper to parse source code and return AST
inline std::unique_ptr<Program> parse(const std::string& source, Type::TypeRegistry& types, DiagnosticManager& diag) {
    Lexer lexer(source, diag);
    auto tokens = lexer.tokenize();
    Parser parser(tokens, types, diag);
    return parser.parseProgram();
}

// Helper to check if parsing succeeds without errors
inline bool canParse(const std::string& source) {
    try {
        Type::TypeRegistry types;
        DiagnosticManager diag(false);
        auto program = parse(source, types, diag);
        return program != nullptr && !diag.hasErrors();
    } catch (...) {
        return false;
    }
}

// Helper to check if source passes BOTH parsing and semantic analysis without errors
inline bool passesSemanticAnalysis(const std::string& source) {
    try {
        Type::TypeRegistry types;
        DiagnosticManager diag(false);

        // Parse
        auto program = parse(source, types, diag);
        if (!program || diag.hasErrors()) {
            return false;
        }

        // Lower to HIR
        HIRLowering lowering(types);
        auto hirProgram = lowering.lower(*program);
        if (diag.hasErrors()) {
            return false;
        }

        // Run semantic analysis
        Semantic::SemanticAnalyzer analyzer(types, diag);
        analyzer.analyzeProgram(hirProgram);

        return !diag.hasErrors();
    } catch (...) {
        return false;
    }
}

// Helper to get error count from diagnostic manager
inline int getErrorCount(const DiagnosticManager& diag) {
    return diag.getErrorCount();
}

// Helper to check if source contains specific error
inline bool hasError(const std::string& source, const std::string& errorSubstring) {
    try {
        Type::TypeRegistry types;
        DiagnosticManager diag(false);
        auto program = parse(source, types, diag);
        return diag.hasErrors();
    } catch (const std::exception& e) {
        std::string errorMsg(e.what());
        return errorMsg.find(errorSubstring) != std::string::npos;
    }
}

// Helper to compile and execute a .vlt file, returning exit code
inline int compileAndRun(const std::string& vltFilePath, std::string* output = nullptr, std::string* error = nullptr) {
    // This will be implemented when we add integration tests
    // For now, returns -1 to indicate not implemented
    return -1;
}

// Helper to create temporary .vlt file for testing
class TempVoltaFile {
public:
    TempVoltaFile(const std::string& content) {
        static int counter = 0;
        filename = "/tmp/volta_test_" + std::to_string(counter++) + ".vlt";

        std::ofstream file(filename);
        file << content;
        file.close();
    }

    ~TempVoltaFile() {
        std::remove(filename.c_str());
    }

    const std::string& getPath() const { return filename; }

private:
    std::string filename;
};

// Macro for testing expected failures
#define EXPECT_PARSE_FAILURE(source) \
    EXPECT_FALSE(VoltaTest::canParse(source))

#define EXPECT_PARSE_SUCCESS(source) \
    EXPECT_TRUE(VoltaTest::canParse(source))

// Macro for skipping unimplemented feature tests
#define SKIP_UNIMPLEMENTED_FEATURE(feature) \
    GTEST_SKIP() << "Feature not yet implemented: " << feature

} // namespace VoltaTest
