#include "CLI/cxxopts.hpp"
#include "CLI/CompilerOptions.hpp"
#include "CLI/CompilerDriver.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char** argv) {
    try {
        // Create argument parser
        cxxopts::Options parser("volta", "The Volta programming language compiler");

        // Add option groups
        parser.add_options("Compilation")
            ("o,output", "Output file name", cxxopts::value<std::string>()->default_value("a.out"))
            ("positional", "Source files to compile", cxxopts::value<std::vector<std::string>>())
            ("h,help", "Show this help message")
        ;

        parser.add_options("Optimization")
            ("O", "Optimization level (0-3)", cxxopts::value<int>()->default_value("0"))
        ;

        parser.add_options("Standard Library")
            ("no-auto-std", "Don't automatically include std library")
            ("std-path", "Custom std library path", cxxopts::value<std::string>())
            ("minimal-std", "Only include core.vlt (minimal std)")
        ;

        parser.add_options("Linking")
            ("link", "Link with object files or C sources", cxxopts::value<std::vector<std::string>>())
            ("no-libc", "Don't link with C standard library")
            ("freestanding", "Freestanding mode (bare metal, no hosted environment)")
            ("linker", "Linker to use (gcc, clang, ld)", cxxopts::value<std::string>()->default_value("gcc"))
            ("ldflags", "Additional linker flags", cxxopts::value<std::vector<std::string>>())
        ;

        parser.add_options("Execution")
            ("run", "Compile and run immediately (like 'go run')")
        ;

        parser.add_options("Debug/Dump")
            ("dump-tokens", "Dump tokens after lexing")
            ("dump-ast", "Dump AST after parsing")
            ("dump-hir", "Dump HIR after desugaring")
            ("dump-mir", "Dump MIR after lowering")
            ("dump-llvm", "Dump LLVM IR")
            ("dump-exports", "Dump export table after import resolution")
            ("v,verbose", "Enable verbose output (enables all dump flags)")
        ;

        parser.add_options("Advanced")
            ("emit-llvm", "Output LLVM IR (.ll file)")
            ("emit-obj", "Output object file (.o file)")
            ("emit-asm", "Output assembly (.s file)")
        ;

        parser.parse_positional({"positional"});
        parser.positional_help("<source_files...>");

        // Parse command line
        auto result = parser.parse(argc, argv);

        // Show help if requested
        if (result.count("help")) {
            std::cout << parser.help({"Compilation", "Optimization", "Standard Library",
                                      "Linking", "Execution", "Debug/Dump", "Advanced"}) << "\n";
            return 0;
        }

        // Build CompilerOptions from parsed arguments
        CompilerOptions opts;

        // Source files
        if (result.count("positional")) {
            opts.sourceFiles = result["positional"].as<std::vector<std::string>>();
        }

        if (opts.sourceFiles.empty()) {
            std::cerr << "Error: No source files specified\n\n";
            std::cerr << parser.help({"Compilation"}) << "\n";
            return 1;
        }

        // Output file
        opts.outputFile = result["output"].as<std::string>();

        // Optimization
        opts.optLevel = result["O"].as<int>();
        if (opts.optLevel < 0 || opts.optLevel > 3) {
            std::cerr << "Error: Invalid optimization level: " << opts.optLevel << " (must be 0-3)\n";
            return 1;
        }

        // Standard library
        opts.autoIncludeStd = !result.count("no-auto-std");
        if (result.count("std-path")) {
            opts.stdPath = result["std-path"].as<std::string>();
        }
        opts.useCoreFunctionsOnly = result.count("minimal-std");

        // Linking
        if (result.count("link")) {
            opts.linkObjects = result["link"].as<std::vector<std::string>>();
        }
        opts.useCStdlib = !result.count("no-libc");
        opts.freeStanding = result.count("freestanding");
        opts.linker = result["linker"].as<std::string>();
        if (result.count("ldflags")) {
            opts.linkerFlags = result["ldflags"].as<std::vector<std::string>>();
        }

        // Execution
        opts.run = result.count("run");

        // Debug/Dump flags
        opts.verbose = result.count("verbose");
        if (opts.verbose) {
            // Enable all dump flags
            opts.dumpTokens = true;
            opts.dumpAST = true;
            opts.dumpHIR = true;
            opts.dumpMIR = true;
            opts.dumpLLVM = true;
            opts.dumpExports = true;
        } else {
            // Individual dump flags
            opts.dumpTokens = result.count("dump-tokens");
            opts.dumpAST = result.count("dump-ast");
            opts.dumpHIR = result.count("dump-hir");
            opts.dumpMIR = result.count("dump-mir");
            opts.dumpLLVM = result.count("dump-llvm");
            opts.dumpExports = result.count("dump-exports");
        }

        // Advanced options
        opts.emitLLVMIR = result.count("emit-llvm");
        opts.emitObject = result.count("emit-obj");
        opts.emitAsm = result.count("emit-asm");

        // Validate options
        if (opts.freeStanding && opts.useCStdlib) {
            std::cerr << "Warning: --freestanding implies --no-libc\n";
            opts.useCStdlib = false;
        }

        // Create compiler driver and compile
        CompilerDriver driver(opts);
        return driver.compile();

    } catch (const cxxopts::exceptions::exception& e) {
        std::cerr << "Error parsing arguments: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
}
