#include "CopyPropagationPass.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/ValueHandle.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Support/Debug.h"
#include <unordered_map>

using namespace llvm;

PreservedAnalyses CopyPropagationPass::run(Function &F, FunctionAnalysisManager &) {
    bool Changed = false;
    std::unordered_map<Value*, Value*> CopyMap;

    for (auto &BB : F) {
        for (auto &I : BB) {
            // Handle trivial bitcasts (same type)
            if (auto *BI = dyn_cast<BitCastInst>(&I)) {
                if (BI->getSrcTy() == BI->getDestTy()) {
                    CopyMap[&I] = BI->getOperand(0);
                    continue;
                }
            }

            // Replace operands if known copies exist
            for (unsigned op = 0; op < I.getNumOperands(); ++op) {
                Value *Op = I.getOperand(op);
                if (CopyMap.count(Op)) {
                    I.setOperand(op, CopyMap[Op]);
                    Changed = true;
                }
            }

            // Record new trivial copies from stores
            if (auto *SI = dyn_cast<StoreInst>(&I)) {
                Value *Src = SI->getValueOperand();
                Value *Dest = SI->getPointerOperand();
                if (Src != Dest)
                    CopyMap[Dest] = Src;
            }
        }
    }

    if (Changed)
        errs() << "[CopyPropagationPass] Replaced copies in " << F.getName() << "\n";

    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}


// ===== Pass Registration =====
llvm::PassPluginLibraryInfo getCopyPropagationPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "CopyPropagationPass", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](llvm::StringRef Name, llvm::FunctionPassManager &FPM,
                   llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                    if (Name == "copy-prop") {
                        FPM.addPass(CopyPropagationPass());
                        return true;
                    }
                    return false;
                });
        }
    };
}

// Export entry point for plugin
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return getCopyPropagationPluginInfo();
}