#include "Codegen/ModuleEmitter.hpp"
#include <llvm/IR/Verifier.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/TargetParser/Host.h>
#include <iostream>

ModuleEmitter::ModuleEmitter(llvm::Module* mod) : module(mod), targetMachine(nullptr) {
    // Get native target triple
    auto targetTriple = llvm::sys::getProcessTriple();
    module->setTargetTriple(targetTriple);

    // Lookup target
    std::string error;
    const auto *target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
    if (target == nullptr) {
        llvm::errs() << "Failed to lookup target: " << error << "\n";
        return;
    }

    // Create target machine
    llvm::TargetOptions opt;
    targetMachine = target->createTargetMachine(
        targetTriple, "generic", "", opt, llvm::Reloc::PIC_
    );
    module->setDataLayout(targetMachine->createDataLayout());
}

ModuleEmitter::~ModuleEmitter() {
    delete targetMachine;
}

void ModuleEmitter::initializeTargets() {
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
}

void ModuleEmitter::optimize(int level) {
    // Verify module before optimization
    if (llvm::verifyModule(*module, &llvm::errs())) {
        llvm::errs() << "Module verification failed!\n";
        return;
    }

    // Create analysis managers
    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;

    // Create pass builder
    llvm::PassBuilder PB;

    // Register analyses
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    // Build optimization pipeline based on level
    llvm::ModulePassManager MPM;
    switch (level) {
        case 0:
            // O0 - no optimization
            break;
        case 1:
            MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O1);
            break;
        case 2:
            MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2);
            break;
        case 3:
            MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);
            break;
        default:
            llvm::errs() << "Invalid optimization level: " << level << "\n";
            return;
    }

    // Run optimization passes
    if (level > 0) {
        MPM.run(*module, MAM);
    }
}

void ModuleEmitter::emitLLVMIR(const std::string& filename) {
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);

    if (EC) {
        llvm::errs() << "Could not open file: " << EC.message() << "\n";
        return;
    }

    module->print(dest, nullptr);
}

void ModuleEmitter::emitBitcode(const std::string& filename) {
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);

    if (EC) {
        llvm::errs() << "Could not open file: " << EC.message() << "\n";
        return;
    }

    llvm::WriteBitcodeToFile(*module, dest);
}

void ModuleEmitter::emitObjectFile(const std::string& filename) {
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);

    if (EC) {
        llvm::errs() << "Could not open file: " << EC.message() << "\n";
        return;
    }

    llvm::legacy::PassManager pass;
    auto fileType = llvm::CodeGenFileType::ObjectFile;

    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
        llvm::errs() << "TargetMachine can't emit object file\n";
        return;
    }

    pass.run(*module);
    dest.flush();
}

void ModuleEmitter::emitAssembly(const std::string& filename) {
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);

    if (EC) {
        llvm::errs() << "Could not open file: " << EC.message() << "\n";
        return;
    }

    llvm::legacy::PassManager pass;
    auto fileType = llvm::CodeGenFileType::AssemblyFile;

    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
        llvm::errs() << "TargetMachine can't emit assembly file\n";
        return;
    }

    pass.run(*module);
    dest.flush();
}

void ModuleEmitter::dumpLLVMIR() {
    module->print(llvm::outs(), nullptr);
}

void ModuleEmitter::compileToExecutable(const std::string& outputExe, int optLevel, bool dumpIR,
                                         const std::vector<std::string>& linkObjects) {
    // Optimize
    optimize(optLevel);

    // Optionally dump IR to stdout
    if (dumpIR) {
        llvm::outs() << "\n=== LLVM IR ===\n";
        dumpLLVMIR();
        llvm::outs() << "\n";
    }

    // Generate temporary object file
    std::string objFile = outputExe + ".tmp.o";
    emitObjectFile(objFile);

    // Build link command with all object files
    std::string linkCmd = "gcc " + objFile;
    for (const auto& obj : linkObjects) {
        linkCmd += " " + obj;
    }
    // Always link with Boehm GC for heap allocation
    linkCmd += " -lgc";
    linkCmd += " -o " + outputExe;

    int result = std::system(linkCmd.c_str());

    if (result != 0) {
        llvm::errs() << "Linking failed\n";
        std::remove(objFile.c_str());
        return;
    }

    // Clean up temporary object file
    std::remove(objFile.c_str());
}