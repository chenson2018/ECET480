#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Passes/PassBuilder.h"
#include <memory>
#include <string>
#include <cstdlib>

using namespace llvm;

/* Unrolling Section - Adjust Loop Control */
void adjustLoopControl(Loop *L, size_t unroll_factor, LLVMContext &Context) {
}

void cloneLoopBody(Loop *L, size_t unroll_factor, LLVMContext &Context) {
}

void unrollLoop(Loop *L, size_t unroll_factor, LLVMContext &Context) 
{
    if (unroll_factor == 0)
        return;

    adjustLoopControl(L, unroll_factor, Context);
    cloneLoopBody(L, unroll_factor, Context);
}

void opt(Module &M, size_t unroll_factor) 
{
    // Create the analysis managers
    FunctionAnalysisManager FAM;
    LoopAnalysisManager LAM;
    CGSCCAnalysisManager CGAM;
    ModuleAnalysisManager MAM;

    // Create the pass builder and register analysis managers
    PassBuilder PB;
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);


    for (Function &F : M) 
    {
        if (!F.isDeclaration()) 
        {
            // Run loop analysis
            LoopInfo &LI = FAM.getResult<LoopAnalysis>(F);

            for (auto *L : LI) 
            {
                unrollLoop(L, unroll_factor, F.getContext());
            }
        }
    }

}


int main(int argc, char **argv) {
    if (argc < 4) {
        errs() << "Usage: " << argv[0] << " <input.bc> <output.bc> <unroll_factor>\n";
        return 1;
    }

    LLVMContext Context;
    SMDiagnostic Err;

    // Read the input bitcode
    std::unique_ptr<Module> M = parseIRFile(argv[1], Err, Context);
    if (!M) {
        Err.print(argv[0], errs());
        return 1;
    }
    errs() << "\n******************* Original IR ******************* \n";
    M->print(errs(), nullptr);
    errs() << "*************************************************** \n";

    // Identify loops
    opt(*M, std::stoi(argv[3]));

    errs() << "\n********************* New IR ********************** \n";
    M->print(errs(), nullptr);
    errs() << "*************************************************** \n";
    opt(*M, 0);

    // Write the (unmodified) module to the output bitcode file
    std::error_code EC;
    raw_fd_ostream OS(argv[2], EC);
    if (EC) {
        errs() << "Error opening file " << argv[2] << ": " << EC.message() << "\n";
        return 1;
    }
    WriteBitcodeToFile(*M, OS);

    return 0;
}

