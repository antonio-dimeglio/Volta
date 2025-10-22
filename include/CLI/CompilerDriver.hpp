#pragma once
#include "CompilerOptions.hpp"
#include "../Error/Error.hpp"
#include "../Type/TypeRegistry.hpp"
#include "../Lexer/Lexer.hpp"
#include "../Parser/Parser.hpp"
#include "../Parser/ASTPrinter.hpp"
#include "../MIR/MIR.hpp"
#include "../HIR/HIR.hpp"
#include "../Semantic/SemanticAnalyzer.hpp"
#include "../Semantic/ExportTable.hpp"
#include "../Semantic/FunctionRegistry.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <vector>
#include <string>
#include <memory>

// Forward declarations
class Program;

struct CompilationUnit {
    std::string sourceFile;
    std::string source;
    std::unique_ptr<Program> ast;
    HIR::HIRProgram hir;
    MIR::Program mir;
};

class CompilerDriver {
public:
    explicit CompilerDriver(const CompilerOptions& opts);
    int compile();

private:
    CompilerOptions opts;
    DiagnosticManager diag;
    Type::TypeRegistry typeRegistry;

    std::vector<CompilationUnit> units;
    std::unique_ptr<llvm::LLVMContext> llvmContext;
    std::unique_ptr<llvm::Module> llvmModule;

    std::vector<std::string> collectSourceFiles();
    bool parseAllFiles(const std::vector<std::string>& files);
    bool resolveImports(Semantic::ExportTable& exportTable);
    void registerStructNames();  // Pre-register struct names for HIR desugaring
    bool lowerToHIR();
    std::vector<std::unique_ptr<Semantic::SemanticAnalyzer>> performSemanticAnalysis(Semantic::FunctionRegistry& functionRegistry);
    bool lowerToMIR(const std::vector<std::unique_ptr<Semantic::SemanticAnalyzer>>& analyzers);
    bool verifyMIR();
    MIR::Program mergeMIRModules();
    bool generateLLVM(const MIR::Program& mergedMIR);
    bool linkAndEmit();

    // Auto-discover and add std library files
    void addStdLibraryFiles(std::vector<std::string>& files);

    // Compile C files to .o if needed
    void compileExternalCFiles();

    // Determine which linker to use and build linker command
    std::string buildLinkerCommand(const std::string& objectFile);

    // Handle --run flag (compile then execute)
    int compileAndRun();
};