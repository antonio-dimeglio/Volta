#include "Lexer/Lexer.hpp"
#include "Parser/Parser.hpp"
#include "Parser/ASTPrinter.hpp"
#include "HIR/Lowering.hpp"
#include "MIR/Elaboration.hpp"
#include "Semantic/TypeRegistry.hpp"
#include "Semantic/SemanticAnalyzer.hpp"
#include "Semantic/ExportTable.hpp"
#include "Semantic/ModuleUtils.hpp"
#include "Semantic/ImportResolver.hpp"
#include "Semantic/FunctionCache.hpp"
#include "Codegen/Codegen.hpp"
#include "Codegen/ModuleEmitter.hpp"
#include "Error/Error.hpp"
#include <llvm/IR/Module.h>
#include <llvm/Linker/Linker.h>
#include <llvm/IR/Verifier.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <vector>
#include <unordered_map>

struct CompilerArgs {
    std::vector<std::string> sourceFiles;
    bool dumpTokens = false;
    bool dumpAST = false;
    bool dumpExport = false;
    bool dumpHIR = false;
    bool dumpLLVM = false;
    int optLevel = 0;
    std::string outputFile = "a.out";
};

struct CompilationUnit {
    std::string sourceFile;
    std::string source;
    std::unique_ptr<Program> ast;
    Program hir;  // Desugared AST
    std::unique_ptr<llvm::Module> module_;
};

void printUsage(const char* progName) {
    std::cerr << "Usage: " << progName << " [options] <source_file> [<source_file> ...]\n";
    std::cerr << "Options:\n";
    std::cerr << "  --dump-tokens    Dump tokens after lexing\n";
    std::cerr << "  --dump-ast       Dump AST after parsing\n";
    std::cerr << "  --dump-exports   Dump export table after import collection";
    std::cerr << "  --dump-hir       Dump HIR after desugaring\n";
    std::cerr << "  --dump-llvm      Dump LLVM IR to stdout\n";
    
    std::cerr << "  -O<level>        Optimization level (0-3, default: 0)\n";
    std::cerr << "  -o <output>      Output executable name (default: a.out)\n";
    std::cerr << "  --help           Show this help message\n";
}

CompilerArgs parseArgs(int argc, char** argv) {
    CompilerArgs args;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            std::exit(0);
        } else if (std::strcmp(argv[i], "--dump-tokens") == 0) {
            args.dumpTokens = true;
        } else if (std::strcmp(argv[i], "--dump-ast") == 0) {
            args.dumpAST = true;
        } else if (std::strcmp(argv[i], "--dump-exports") == 0) {
            args.dumpExport = true;
        } else if (std::strcmp(argv[i], "--dump-hir") == 0) {
            args.dumpHIR = true;
        } else if (std::strcmp(argv[i], "--dump-llvm") == 0) {
            args.dumpLLVM = true;
        } else if (std::strncmp(argv[i], "-O", 2) == 0) {
            args.optLevel = argv[i][2] - '0';
            if (args.optLevel < 0 || args.optLevel > 3) {
                std::cerr << "Error: Invalid optimization level: " << argv[i] << "\n";
                std::exit(1);
            }
        } else if (std::strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                args.outputFile = argv[++i];
            } else {
                std::cerr << "Error: -o requires an argument\n";
                std::exit(1);
            }
        } else if (argv[i][0] != '-') {
            args.sourceFiles.push_back(argv[i]);
        } else {
            std::cerr << "Error: Unknown option: " << argv[i] << "\n";
            printUsage(argv[0]);
            std::exit(1);
        }
    }

    return args;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    // Parse command line arguments
    CompilerArgs args = parseArgs(argc, argv);

    if (args.sourceFiles.empty()) {
        std::cerr << "Error: No source files specified\n";
        printUsage(argv[0]);
        return 1;
    }

    DiagnosticManager diag;
    std::vector<CompilationUnit> units;
    Semantic::ExportTable exportTable;

    // Parse all source files
    for (const auto& sourceFile : args.sourceFiles) {
        std::ifstream file(sourceFile);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file '" << sourceFile << "'\n";
            return 1;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string source = buffer.str();
        file.close();

        // Lex
        Lexer lexer(source, diag);
        auto tokens = lexer.tokenize();

        if (diag.hasErrors()) {
            diag.printAll(std::cerr, sourceFile.c_str());
            continue; // Continue to collect errors from other files
        }

        if (args.dumpTokens) {
            std::cout << "=== Token Dump [" << sourceFile << "] ===\n";
            for (const auto& token : tokens) {
                std::cout << token.toString() << "\n";
            }
            std::cout << "\n";
        }

        // Parse
        Parser parser(tokens, diag);
        auto astProgram = parser.parseProgram();

        if (diag.hasErrors()) {
            diag.printAll(std::cerr, sourceFile.c_str());
            continue;
        }

        if (args.dumpAST) {
            std::cout << "=== AST Dump [" << sourceFile << "] ===\n";
            ASTPrinter printer;
            printer.print(*astProgram, std::cout);
            std::cout << "\n";
        }

        // Store compilation unit
        CompilationUnit unit;
        unit.sourceFile = sourceFile;
        unit.source = source;
        unit.ast = std::move(astProgram);
        units.push_back(std::move(unit));
    }

    // If any file had errors, stop here
    if (diag.hasErrors()) {
        return 1;
    }

    // Build export table from all parsed files
    for (const auto& unit : units) {
        std::string moduleName = Semantic::fileToModule(unit.sourceFile);
        exportTable.collectExportsFromAST(*unit.ast, moduleName);
    }

    if (args.dumpExport) {
        std::cout << "=== Export Table ===\n";
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
        return 1;
    }

    // Build import map - tells us which symbols each module imports from where
    Semantic::ImportMap importMap = Semantic::buildImportMap(unitPairs);

    // Detect circular dependencies
    if (Semantic::detectCircularDependencies(importMap, diag)) {
        diag.printAll(std::cerr);
        return 1;
    }

    // Build function signature cache with interned types (survives AST destruction)
    Semantic::TypeRegistry globalTypeRegistry;  // Shared type registry for caching
    Semantic::FunctionCache functionCache;
    for (const auto& unit : units) {
        std::string moduleName = Semantic::fileToModule(unit.sourceFile);
        for (const auto& stmt : unit.ast->statements) {
            if (auto* fnDecl = dynamic_cast<const FnDecl*>(stmt.get())) {
                functionCache[moduleName].emplace(fnDecl->name, Semantic::FunctionSignatureCache(fnDecl, globalTypeRegistry));
            }
        }
    }

    // ===== PHASE 1: HIR Lowering for all files =====
    std::cout << "Phase 1: HIR Lowering\n";
    for (auto& unit : units) {
        std::string moduleName = Semantic::fileToModule(unit.sourceFile);
        std::cout << "  Lowering: " << moduleName << "\n";

        HIRLowering lowering;
        // Move the AST into HIR lowering (function cache still has valid pointers)
        unit.hir = lowering.lower(std::move(*unit.ast));

        if (args.dumpHIR) {
            std::cout << "=== HIR Dump [" << moduleName << "] ===\n";
            ASTPrinter printer;
            printer.print(unit.hir, std::cout);
            std::cout << "\n";
        }
    }

    // ===== PHASE 2: Semantic Analysis for all files =====
    std::cout << "Phase 2: Semantic Analysis\n";
    std::vector<std::unique_ptr<Semantic::SemanticAnalyzer>> analyzers;

    for (auto& unit : units) {
        std::string moduleName = Semantic::fileToModule(unit.sourceFile);
        std::cout << "  Analyzing: " << moduleName << "\n";

        // Use the global type registry (shared across all modules)
        auto analyzer = std::make_unique<Semantic::SemanticAnalyzer>(globalTypeRegistry, diag);

        // Add imported functions to symbol table
        Semantic::addImportedFunctionsToSymbolTable(analyzer->getSymbolTable(), globalTypeRegistry, moduleName, importMap, functionCache, diag);

        analyzer->analyzeProgram(unit.hir);

        if (diag.hasErrors()) {
            diag.printAll(std::cerr, unit.sourceFile.c_str());
            return 1;
        }

        analyzers.push_back(std::move(analyzer));
    }

    // ===== PHASE 3: Code Generation for all files =====
    std::cout << "Phase 3: Code Generation\n";
    ModuleEmitter::initializeTargets();

    // Create a shared LLVM context for all modules
    auto sharedContext = std::make_unique<llvm::LLVMContext>();

    for (size_t i = 0; i < units.size(); ++i) {
        auto& unit = units[i];
        std::string moduleName = Semantic::fileToModule(unit.sourceFile);
        std::cout << "  Codegen: " << moduleName << "\n";

        // MIR Elaboration
        MIRElaboration elaboration(&analyzers[i]->getSymbolTable());
        auto mirProgram = elaboration.elaborate(std::move(unit.hir));

        // Code Generation (use global type registry and shared context)
        Codegen codegen(mirProgram, globalTypeRegistry, diag, *sharedContext);

        // Tell codegen which symbols this module imports
        if (importMap.count(moduleName) > 0) {
            codegen.setImportedSymbols(importMap[moduleName]);

            // Add imported function signatures to codegen for declaration generation
            for (const auto& [symbolName, sourceModule] : importMap[moduleName]) {
                if (functionCache.count(sourceModule) > 0 && functionCache.at(sourceModule).count(symbolName) > 0) {
                    const auto& cached = functionCache.at(sourceModule).at(symbolName);
                    codegen.addImportedFunction(cached.name, cached.params, cached.returnType);
                }
            }
        }

        unit.module_ = codegen.generate();

        if (diag.hasErrors()) {
            diag.printAll(std::cerr, unit.sourceFile.c_str());
            return 1;
        }

        if (args.dumpLLVM) {
            std::cout << "=== LLVM IR [" << moduleName << "] ===\n";
            unit.module_->print(llvm::outs(), nullptr);
            std::cout << "\n";
        }
    }

    std::cout << "All modules compiled successfully!\n";

    // ===== PHASE 4: Validate main function exists =====
    bool hasMain = false;
    std::string mainModule;
    int mainCount = 0;

    for (size_t i = 0; i < units.size(); ++i) {
        const auto& unit = units[i];
        const auto& analyzer = analyzers[i];

        if (analyzer->getSymbolTable().lookupFunction("main") != nullptr) {
            hasMain = true;
            mainModule = Semantic::fileToModule(unit.sourceFile);
            mainCount++;
        }
    }

    if (!hasMain) {
        diag.error("No 'main' function found in any module", 0, 0);
        diag.printAll(std::cerr);
        return 1;
    }

    if (mainCount > 1) {
        diag.error("Multiple 'main' functions found across modules", 0, 0);
        diag.printAll(std::cerr);
        return 1;
    }

    std::cout << "Found main function in module: " << mainModule << "\n";

    // ===== PHASE 5: Merge all LLVM modules into one =====
    std::cout << "Phase 4: Module Merging\n";

    // Create merged module in the same shared context
    auto mergedModule = std::make_unique<llvm::Module>("volta_merged", *sharedContext);

    // Use LLVM Linker to merge all modules into the merged module
    llvm::Linker linker(*mergedModule);

    for (auto& unit : units) {
        std::string moduleName = Semantic::fileToModule(unit.sourceFile);
        std::cout << "  Linking: " << moduleName << "\n";

        // Link this module into the merged module
        // Note: linkInModule moves the module, so we clone it if we need to keep the original
        if (linker.linkInModule(std::move(unit.module_))) {
            diag.error("Failed to link module: " + moduleName, 0, 0);
            diag.printAll(std::cerr);
            return 1;
        }
    }

    std::cout << "All modules linked successfully!\n";

    // ===== PHASE 6: Verify the merged module =====
    std::cout << "Phase 5: Module Verification\n";

    std::string verifyErrors;
    llvm::raw_string_ostream errorStream(verifyErrors);

    if (llvm::verifyModule(*mergedModule, &errorStream)) {
        diag.error("Module verification failed: " + verifyErrors, 0, 0);
        diag.printAll(std::cerr);
        return 1;
    }

    std::cout << "Module verification passed!\n";

    if (args.dumpLLVM) {
        std::cout << "=== Merged LLVM IR ===\n";
        mergedModule->print(llvm::outs(), nullptr);
        std::cout << "\n";
    }

    // ===== PHASE 7: Compile merged module to executable =====
    std::cout << "Phase 6: Compilation to Executable\n";

    ModuleEmitter emitter(mergedModule.get());

    // Apply optimizations if specified
    if (args.optLevel > 0) {
        std::cout << "  Optimizing with level -O" << args.optLevel << "\n";
        emitter.optimize(args.optLevel);
    }

    // Use the convenient compileToExecutable method
    emitter.compileToExecutable(args.outputFile, args.optLevel, args.dumpLLVM);

    std::cout << "Executable generated: " << args.outputFile << "\n";
    std::cout << "\n✓ Compilation successful!\n";

    return 0;
}
