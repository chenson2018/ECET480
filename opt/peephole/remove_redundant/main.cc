#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include <memory>
#include <map>

using namespace llvm;

void removeRedundantLoads(Function &F) {
  for (BasicBlock &BB : F) {
    auto it = BB.begin();
    bool past_add = false;
    while(it != BB.end()) {
      Instruction &Inst = *it;
      ++it;

      if (isa<BinaryOperator>(Inst)) {
        past_add = true;
      }

      if (past_add && isa<LoadInst>(Inst)) {
        Inst.replaceAllUsesWith(Inst.getOperand(0)); 
        Inst.eraseFromParent();
      }
    }
  }
}

void removeRedundantBinaryOps(Function &F) {
  for (BasicBlock &BB : F) {
    auto it = BB.begin();
    Instruction *first_add = nullptr;

    while(it != BB.end()) {
      Instruction &Inst = *it;
      ++it;
      if (isa<BinaryOperator>(Inst)) {
	if (first_add == nullptr) {
	  first_add = &Inst;
	} else {
          Inst.replaceAllUsesWith(first_add); 
          Inst.eraseFromParent();
	}
      }
    }
  }
}


int main(int argc, char** argv) {

    if (argc < 3) {
        llvm::errs() << "Usage: " << argv[0] << " <input IR file> <output IR file>\n";
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

    // Iterate through all functions in the module and apply the optimizations
    for (llvm::Function &F : *M) {
        if (!F.isDeclaration()) {
            removeRedundantLoads(F);
            removeRedundantBinaryOps(F);
        }
    }

    errs() << "\n********************* New IR ********************** \n";
    M->print(errs(), nullptr);
    errs() << "*************************************************** \n";

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

