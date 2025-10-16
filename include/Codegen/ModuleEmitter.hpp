#pragma once
#include <llvm/IR/Module.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/MC/TargetRegistry.h>
#include <string>

class ModuleEmitter {
private:
    llvm::Module* module;  
    llvm::TargetMachine* targetMachine;

public:
    explicit ModuleEmitter(llvm::Module* mod);
    ~ModuleEmitter();

    static void initializeTargets();

    // Optimization
    void optimize(int level); 

    // Output methods
    void emitLLVMIR(const std::string& filename);
    void dumpLLVMIR();  // Print IR to stdout
    void emitBitcode(const std::string& filename);
    void emitObjectFile(const std::string& filename);
    void emitAssembly(const std::string& filename);

    // Compile to executable (generates temp .o, links, cleans up)
    void compileToExecutable(const std::string& outputExe, int optLevel, bool dumpIR = false);
};