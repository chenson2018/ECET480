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
#include<tuple> 

using namespace llvm;

void removeRedundantLoads(Function &F) {
  std::map<Value*, LoadInst*> lastLoad;
  
  for (auto &B : F) {
    lastLoad.clear();
    for (auto Inst = B.begin(), E = B.end(); Inst != E;) {
      Instruction *I = &*Inst++;
      if (auto *LI = dyn_cast<LoadInst>(I)) {
        Value *ptr = LI->getPointerOperand();
	if (lastLoad.find(ptr) != lastLoad.end()) {
	  LI->replaceAllUsesWith(lastLoad[ptr]);
	  LI->eraseFromParent();
	  continue;
	} else {
	  lastLoad[ptr] = LI;
	}
      } else if (I->mayWriteToMemory()) {
	 if (auto *SI = dyn_cast<StoreInst>(I)) {
	   Value *ptr = SI->getPointerOperand();
	   lastLoad.erase(ptr);
	 }
      } 
    }
  }    
}

void removeRedundantBinaryOps(Function &F) {
  std::map<Value*, BinaryOperator*> lastAdd;
  
  for (auto &B : F) {
    lastAdd.clear();
    for (auto Inst = B.begin(), E = B.end(); Inst != E;) {
      Instruction *I = &*Inst++;
      if (auto *AI = dyn_cast<BinaryOperator>(I)) {
        Value *ptr_lhs = AI->getOperand(0);
        Value *ptr_rhs = AI->getOperand(1);
        if (lastAdd.find(ptr_lhs) != lastAdd.end() && lastAdd.find(ptr_rhs) != lastAdd.end()) {
          AI->replaceAllUsesWith(lastAdd[ptr_lhs]);
          AI->eraseFromParent();
          continue;
        } else {
          lastAdd[ptr_lhs] = AI;
          lastAdd[ptr_rhs] = AI;
        }
      } else if (I->mayWriteToMemory()) {
         if (auto *SI = dyn_cast<StoreInst>(I)) {
           Value *ptr = SI->getPointerOperand();
           lastAdd.erase(ptr);
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

