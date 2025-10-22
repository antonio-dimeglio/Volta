#include "CLI/CompilerDriver.hpp"
#include "Lexer/Lexer.hpp"
#include "Parser/Parser.hpp"
#include "Parser/ASTPrinter.hpp"
#include "HIR/Lowering.hpp"
#include "Semantic/SemanticAnalyzer.hpp"
#include "Semantic/ExportTable.hpp"
#include "Semantic/ModuleUtils.hpp"
#include "Semantic/ImportResolver.hpp"
#include "Semantic/FunctionRegistry.hpp"
#include "MIR/HIRToMIR.hpp"
#include "MIR/MIRPrinter.hpp"
#include "MIR/MIRVerifier.hpp"
#include "Codegen/MIRToLLVM.hpp"
#include "Codegen/ModuleEmitter.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace fs = std::filesystem;

// Helper functions
static std::string readFileToString(const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file)
        throw std::runtime_error("Failed to open file: " + path);
    return std::string((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
}

static bool fileExists(const std::string& filepath) {
    return fs::exists(filepath);
}


CompilerDriver::CompilerDriver(const CompilerOptions& opts)
    : opts(opts), diag(), typeRegistry() {}


int CompilerDriver::compile() {
    std::cout << "Volta Compiler\n";
    std::cout << "==============\n\n";

    // Phase 0: Collect all source files
    std::cout << "Phase 0: Collecting source files\n";
    auto files = collectSourceFiles();
    if (files.empty()) {
        std::cerr << "Error: No source files to compile\n";
        return 1;
    }
    std::cout << "  Found " << files.size() << " file(s)\n";

    // Phase 1: Parse all files
    std::cout << "\nPhase 1: Parsing\n";
    if (!parseAllFiles(files)) {
        return 1;
    }

    // Phase 2: Resolve imports
    std::cout << "\nPhase 2: Resolving imports\n";
    Semantic::ExportTable exportTable;
    if (!resolveImports(exportTable)) {
        return 1;
    }

    // Phase 2.5: Pre-register struct names (for HIR desugaring of Point.new() → StaticMethodCall)
    std::cout << "\nPhase 2.5: Pre-registering struct types\n";
    registerStructNames();

    // Phase 3: HIR lowering
    std::cout << "\nPhase 3: HIR Lowering\n";
    if (!lowerToHIR()) {
        return 1;
    }

    // Phase 4: Semantic analysis
    std::cout << "\nPhase 4: Semantic Analysis\n";
    Semantic::FunctionRegistry functionRegistry;
    auto analyzers = performSemanticAnalysis(functionRegistry);
    if (analyzers.empty()) {
        return 1;  // Error already printed
    }

    // Phase 5: MIR lowering (reuse analyzers from Phase 4)
    std::cout << "\nPhase 5: MIR Lowering\n";
    if (!lowerToMIR(analyzers)) {
        return 1;
    }

    // Phase 6: MIR verification
    std::cout << "\nPhase 6: MIR Verification\n";
    if (!verifyMIR()) {
        return 1;
    }

    // Phase 7: Merge MIR modules
    std::cout << "\nPhase 7: Merging modules\n";
    MIR::Program mergedMIR = mergeMIRModules();

    // Phase 8: LLVM code generation
    std::cout << "\nPhase 8: LLVM Code Generation\n";
    if (!generateLLVM(mergedMIR)) {
        return 1;
    }

    // Phase 9: Link and emit
    std::cout << "\nPhase 9: Linking and emitting\n";
    if (!linkAndEmit()) {
        return 1;
    }

    std::cout << "\n✓ Compilation successful!\n";
    std::cout << "  Output: " << opts.outputFile << "\n";

    // Optional: Run the executable
    if (opts.run) {
        return compileAndRun();
    }

    return 0;
}

std::vector<std::string> CompilerDriver::collectSourceFiles() {
    std::vector<std::string> files;

    // Validate source file extensions
    for (const auto& file : opts.sourceFiles) {
        fs::path filePath(file);
        std::string ext = filePath.extension().string();

        // Check if it's a Volta source file
        if (ext != ".vlt" && ext != ".volta") {
            std::cerr << "Warning: Skipping file with unrecognized extension: " << file << "\n";
            std::cerr << "         Expected .vlt or .volta extension\n";
            std::cerr << "         (Did you mean to use --link for object files or libraries?)\n";
            continue;
        }

        // Check if file exists
        if (!fs::exists(filePath)) {
            std::cerr << "Error: Source file not found: " << file << "\n";
            continue;
        }

        files.push_back(file);
    }

    if (opts.autoIncludeStd) {
        std::string stdPath = opts.stdPath.empty() ? opts.getDefaultStdPath() : opts.stdPath;
        fs::path stdDir(stdPath);

        if (!fs::exists(stdDir) || !fs::is_directory(stdDir)) {
            if (opts.verbose) {
                std::cerr << "Warning: std library path not found: " << stdPath << "\n";
                std::cerr << "         Continuing without std library\n";
            }
            return files;
        }

        fs::path coreFile = stdDir / "core.vlt";
        if (fs::exists(coreFile)) {
            files.insert(files.begin(), coreFile.string());  // Add at beginning
        } else if (opts.verbose) {
            std::cerr << "Warning: std/core.vlt not found\n";
        }

        if (!opts.useCoreFunctionsOnly) {
            std::vector<std::string> additionalStdFiles;

            for (const auto& entry : fs::directory_iterator(stdDir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".vlt") {
                    std::string filename = entry.path().filename().string();

                    if (filename != "core.vlt") {
                        additionalStdFiles.push_back(entry.path().string());
                    }
                }
            }

            std::sort(additionalStdFiles.begin(), additionalStdFiles.end());
            files.insert(files.begin() + 1, additionalStdFiles.begin(), additionalStdFiles.end());
        }
    }

    return files;
}

bool CompilerDriver::parseAllFiles(const std::vector<std::string>& files) {
    for (const auto& file : files) {
        std::cout << "  Parsing: " << file << "\n";

        std::string source = readFileToString(file);
        if (source.empty() && !fileExists(file)) {
            diag.error("Could not open file: " + file, 0, 0);
            return false;
        }

        // Check for binary data (null bytes or excessive non-printable characters)
        size_t nullBytes = 0;
        size_t nonPrintable = 0;
        size_t sampleSize = std::min(source.size(), size_t(1024)); // Check first 1KB

        for (size_t i = 0; i < sampleSize; i++) {
            unsigned char c = source[i];
            if (c == 0) nullBytes++;
            if (c < 32 && c != '\n' && c != '\r' && c != '\t') nonPrintable++;
        }

        // If we find null bytes or >30% non-printable chars, it's likely binary
        if (nullBytes > 0 || (nonPrintable * 100 / sampleSize) > 30) {
            std::cerr << "Error: File appears to be binary data, not Volta source: " << file << "\n";
            std::cerr << "       (Did you mean to use --link for this file?)\n";
            return false;
        }

        Lexer lexer(source, diag);
        auto tokens = lexer.tokenize();

        if (diag.hasErrors()) {
            diag.printAll(std::cerr, file.c_str());
            return false;
        }

        if (opts.dumpTokens) {
            std::cout << "\n=== Tokens [" << file << "] ===\n";
            for (const auto& token : tokens) {
                std::cout << token.toString() << "\n";
            }
            std::cout << "\n";
        }

        Parser parser(tokens, typeRegistry, diag);
        auto ast = parser.parseProgram();

        if (diag.hasErrors()) {
            diag.printAll(std::cerr, file.c_str());
            return false;
        }

        if (opts.dumpAST) {
            std::cout << "\n=== AST [" << file << "] ===\n";
            ASTPrinter printer;
            printer.print(*ast, std::cout);
            std::cout << "\n";
        }

        CompilationUnit unit;
        unit.sourceFile = file;
        unit.source = source;
        unit.ast = std::move(ast);
        units.push_back(std::move(unit));
    }

    return true;
}

bool CompilerDriver::resolveImports(Semantic::ExportTable& exportTable) {
    // Build export table from all parsed files
    for (const auto& unit : units) {
        std::string moduleName = Semantic::fileToModule(unit.sourceFile);
        exportTable.collectExportsFromAST(*unit.ast, moduleName);
    }

    if (opts.dumpExports) {
        std::cout << "\n=== Export Table ===\n";
        std::cout << exportTable.dump() << "\n\n";
    }

    // Validate all imports against the export table
    std::vector<std::pair<std::string, const Program*>> unitPairs;
    for (const auto& unit : units) {
        std::string moduleName = Semantic::fileToModule(unit.sourceFile);
        unitPairs.push_back({moduleName, unit.ast.get()});
    }

    if (!Semantic::validateImports(unitPairs, exportTable, diag)) {
        diag.printAll(std::cerr);
        return false;
    }

    // Build import map
    Semantic::ImportMap importMap = Semantic::buildImportMap(unitPairs);

    // Detect circular dependencies
    if (Semantic::detectCircularDependencies(importMap, diag)) {
        diag.printAll(std::cerr);
        return false;
    }

    return true;
}

void CompilerDriver::registerStructNames() {
    // Pre-register struct names from all parsed ASTs (including imported files).
    // This allows HIR lowering to check if "Point" is a type name for Point.new() desugaring.
    // The full struct definitions (fields, methods) are registered later in semantic analysis.

    int structCount = 0;
    for (const auto& unit : units) {
        for (const auto& stmt : unit.ast->statements) {
            if (auto* structDecl = dynamic_cast<StructDecl*>(stmt.get())) {
                // Register just the name as a stub (no fields yet)
                typeRegistry.registerStructStub(structDecl->name.lexeme);
                structCount++;
            }
        }
    }

    std::cout << "  Registered " << structCount << " struct type name(s)\n";
}

bool CompilerDriver::lowerToHIR() {
    for (auto& unit : units) {
        std::string moduleName = Semantic::fileToModule(unit.sourceFile);
        std::cout << "  Lowering: " << moduleName << "\n";

        HIRLowering lowering(typeRegistry);
        unit.hir = lowering.lower(*unit.ast);

        if (opts.dumpHIR) {
            std::cout << "\n=== HIR [" << moduleName << "] ===\n";
            ASTPrinter printer;
            printer.print(unit.hir, std::cout);
            std::cout << "\n";
        }
    }

    return true;
}

std::vector<std::unique_ptr<Semantic::SemanticAnalyzer>> CompilerDriver::performSemanticAnalysis(Semantic::FunctionRegistry& functionRegistry) {
    // Phase 1: Register all struct types from all modules
    std::cout << "  Phase 1: Registering struct types across all modules\n";
    for (const auto& unit : units) {
        Semantic::SemanticAnalyzer tempAnalyzer(typeRegistry, diag);
        tempAnalyzer.registerStructTypes(unit.hir);
    }

    // Phase 2: Resolve all types in all modules (this updates method signatures)
    std::cout << "  Phase 2: Resolving types across all modules\n";
    for (auto& unit : units) {
        Semantic::SemanticAnalyzer tempAnalyzer(typeRegistry, diag);
        tempAnalyzer.resolveUnresolvedTypes(unit.hir);
    }

    // Phase 3: Build function registry from all HIR modules
    std::cout << "  Phase 3: Building function registry\n";
    for (const auto& unit : units) {
        std::string moduleName = Semantic::fileToModule(unit.sourceFile);
        functionRegistry.collectFromHIR(unit.hir, moduleName);
    }

    // Phase 4: Perform full semantic analysis on each module
    std::vector<std::unique_ptr<Semantic::SemanticAnalyzer>> analyzers;
    for (auto& unit : units) {
        std::string moduleName = Semantic::fileToModule(unit.sourceFile);
        std::cout << "  Analyzing: " << moduleName << "\n";

        auto analyzer = std::make_unique<Semantic::SemanticAnalyzer>(typeRegistry, diag);
        analyzer->setFunctionRegistry(&functionRegistry);
        analyzer->analyzeProgram(unit.hir);

        if (diag.hasErrors()) {
            diag.printAll(std::cerr, unit.sourceFile.c_str());
            return {};  // Return empty vector on error
        }

        analyzers.push_back(std::move(analyzer));
    }

    return analyzers;
}

bool CompilerDriver::lowerToMIR(const std::vector<std::unique_ptr<Semantic::SemanticAnalyzer>>& analyzers) {
    for (size_t i = 0; i < units.size(); ++i) {
        auto& unit = units[i];
        auto& analyzer = analyzers[i];
        std::string moduleName = Semantic::fileToModule(unit.sourceFile);
        std::cout << "  Lowering: " << moduleName << "\n";

        MIR::HIRToMIR mirLowering(typeRegistry, diag, analyzer->getExprTypes());
        unit.mir = mirLowering.lower(unit.hir);

        if (diag.hasErrors()) {
            diag.printAll(std::cerr, unit.sourceFile.c_str());
            return false;
        }

        if (opts.dumpMIR) {
            std::cout << "\n=== MIR [" << moduleName << "] ===\n";
            MIR::MIRPrinter printer(std::cout);
            printer.print(unit.mir);
            std::cout << "\n";
        }
    }

    return true;
}

bool CompilerDriver::verifyMIR() {
    for (const auto& unit : units) {
        std::string moduleName = Semantic::fileToModule(unit.sourceFile);
        std::cout << "  Verifying: " << moduleName << "\n";

        MIR::MIRVerifier verifier(diag);
        if (!verifier.verify(unit.mir)) {
            std::cerr << "MIR verification failed for module: " << moduleName << "\n";
            diag.printAll(std::cerr);
            return false;
        }
    }

    return true;
}

MIR::Program CompilerDriver::mergeMIRModules() {
    MIR::Program merged;

    for (auto& unit : units) {
        std::string moduleName = Semantic::fileToModule(unit.sourceFile);
        std::cout << "  Merging: " << moduleName << "\n";

        for (auto& func : unit.mir.functions) {
            merged.addFunction(std::move(func));
        }
    }

    if (opts.dumpMIR) {
        std::cout << "\n=== Merged MIR ===\n";
        MIR::MIRPrinter printer(std::cout);
        printer.print(merged);
        std::cout << "\n";
    }

    return merged;
}

bool CompilerDriver::generateLLVM(const MIR::Program& mergedMIR) {
    // Initialize LLVM targets
    ModuleEmitter::initializeTargets();

    // Create LLVM context and module
    llvmContext = std::make_unique<llvm::LLVMContext>();
    llvmModule = std::make_unique<llvm::Module>("volta_program", *llvmContext);

    // Lower MIR to LLVM IR
    Codegen::MIRToLLVM mirToLLVM(*llvmContext, *llvmModule, typeRegistry);
    mirToLLVM.lower(mergedMIR);

    if (opts.dumpLLVM) {
        std::cout << "\n=== LLVM IR ===\n";
        llvmModule->print(llvm::outs(), nullptr);
        std::cout << "\n";
    }

    return true;
}

bool CompilerDriver::linkAndEmit() {
    // Auto-link Volta runtime library if not in freestanding mode
    if (!opts.freeStanding) {
        // Check if build/libvolta.a exists
        std::string runtimePath = "build/libvolta.a";
        if (fs::exists(runtimePath)) {
            // Add to link objects if not already there
            bool alreadyLinked = false;
            for (const auto& obj : opts.linkObjects) {
                if (obj == runtimePath || obj.find("libvolta") != std::string::npos) {
                    alreadyLinked = true;
                    break;
                }
            }
            if (!alreadyLinked) {
                opts.linkObjects.push_back(runtimePath);
            }
        }
    }

    // Compile external C files if any
    compileExternalCFiles();

    // Emit object file or executable
    ModuleEmitter emitter(llvmModule.get());

    if (opts.emitLLVMIR) {
        std::string llFile = opts.outputFile + ".ll";
        std::cout << "  Emitting LLVM IR: " << llFile << "\n";
        // TODO: Implement emit to .ll file
    }

    if (opts.emitAsm) {
        std::string asmFile = opts.outputFile + ".s";
        std::cout << "  Emitting assembly: " << asmFile << "\n";
        // TODO: Implement emit to .s file
    }

    if (opts.emitObject) {
        std::string objFile = opts.outputFile + ".o";
        std::cout << "  Emitting object file: " << objFile << "\n";
        // TODO: Implement emit to .o file
    }

    // Default: compile to executable, pass link objects
    emitter.compileToExecutable(opts.outputFile, opts.optLevel, false, opts.linkObjects);

    return true;
}

void CompilerDriver::addStdLibraryFiles(std::vector<std::string>& files) {
    // This is already handled in collectSourceFiles()
    // Kept for API compatibility
}

void CompilerDriver::compileExternalCFiles() {
    if (opts.linkObjects.empty()) {
        return;
    }

    std::cout << "  Compiling external C files\n";

    for (auto& obj : opts.linkObjects) {
        // Check if it's a C source file
        if (obj.size() > 2 && obj.substr(obj.size() - 2) == ".c") {
            std::string objFile = obj.substr(0, obj.size() - 2) + ".o";
            std::string compileCmd = "gcc -c " + obj + " -o " + objFile;

            std::cout << "    Compiling: " << obj << "\n";
            int result = std::system(compileCmd.c_str());

            if (result != 0) {
                std::cerr << "Error: Failed to compile " << obj << "\n";
            } else {
                // Replace .c with .o in the list
                obj = objFile;
            }
        }
    }
}

std::string CompilerDriver::buildLinkerCommand(const std::string& objectFile) {
    std::string cmd;

    if (opts.freeStanding) {
        // Freestanding: use ld directly, no stdlib
        cmd = "ld " + objectFile;
    } else if (opts.useCStdlib) {
        // Hosted: use gcc/clang (includes libc)
        cmd = opts.linker + " " + objectFile;
    } else {
        // No C stdlib, but not freestanding - use nostdlib
        cmd = opts.linker + " -nostdlib " + objectFile;
    }

    // Add link objects
    for (const auto& obj : opts.linkObjects) {
        cmd += " " + obj;
    }

    // Add linker flags
    for (const auto& flag : opts.linkerFlags) {
        cmd += " " + flag;
    }

    // Add output file
    cmd += " -o " + opts.outputFile;

    return cmd;
}

int CompilerDriver::compileAndRun() {
    std::cout << "\n--- Running executable ---\n";

    std::string command;

    #ifdef _WIN32
        command = opts.outputFile;
    #else
        command = "./" + opts.outputFile;
    #endif

    int exitCode = std::system(command.c_str());

    // Extract actual exit code
    #ifdef _WIN32
        int actualExitCode = exitCode;
    #else
        int actualExitCode = WEXITSTATUS(exitCode);
    #endif

    std::cout << "\n--- Program exited with code " << actualExitCode << " ---\n";
    return actualExitCode;
}
