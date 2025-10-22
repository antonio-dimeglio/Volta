#pragma once
#include <vector>
#include <string>
#include <filesystem>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

namespace fs = std::filesystem;

struct CompilerOptions {
    std::vector<std::string> sourceFiles;
    std::string outputFile = "a.out";

    // LLVM Opt level
    int optLevel = 0;
    
    // Automatically includes volta std library
    bool autoIncludeStd = true;

    // Path to std defs, the default is <volta_install>/std
    std::string stdPath = "";

    // Use minimal std
    bool useCoreFunctionsOnly = false;

    // Link with C stdlib
    bool useCStdlib = true;
    // No hosted environment (bare metal)
    bool freeStanding = false;

    // Link file.o or alternatively link file.c
    std::vector<std::string> linkObjects;
    // Default linker is gcc, this should be platform specific.
    std::string linker = "gcc";
    // Linker flags, ie. -lm -lpthread
    std::vector<std::string> linkerFlags;

    // Compile and execute
    bool run = false;

    bool dumpTokens = false;
    bool dumpAST = false;
    bool dumpHIR = false;
    bool dumpMIR = false;
    bool dumpLLVM = false;
    bool dumpExports = false;

    // Turns all other dump flags to true
    bool verbose = false;

    // --emit-llvm (output .ll file)
    bool emitLLVMIR = false;
    // --emit-obj (output .o file)
    bool emitObject = false;
    // --emit-asm (output .s file)
    bool emitAsm = false;

    // Helper method to get the volta installation directory
    std::string getVoltaRoot() const {
        const char* envRoot = std::getenv("VOLTA_ROOT");
        if (envRoot != nullptr) {
            return {envRoot};
        }

        #ifdef __linux__
            fs::path exePath = fs::read_symlink("/proc/self/exe");
            return exePath.parent_path().string();
        #elif __APPLE__
            // macOS implementation
            char path[1024];
            uint32_t size = sizeof(path);
            if (_NSGetExecutablePath(path, &size) == 0) {
                return fs::path(path).parent_path().string();
            }
        #elif _WIN32
            // Windows implementation
            char path[MAX_PATH];
            GetModuleFileNameA(NULL, path, MAX_PATH);
            return fs::path(path).parent_path().string();
        #endif

        // 3. Fallback to compile-time path or current directory
        return ".";
    }   

    // Helper to get default std library path
    std::string getDefaultStdPath() const { return getVoltaRoot() + "/std"; }
};