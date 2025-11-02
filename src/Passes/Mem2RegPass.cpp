#include "Mem2RegPass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

PreservedAnalyses MyMem2RegPass::run(Function &F, FunctionAnalysisManager &AM) {
    SmallVector<AllocaInst*, 8> Allocas;
    BasicBlock &Entry = F.getEntryBlock();

    for (Instruction &I : Entry)
        if (auto *AI = dyn_cast<AllocaInst>(&I))
            if (AI->getAllocatedType()->isSingleValueType())
                Allocas.push_back(AI);

    if (Allocas.empty())
        return PreservedAnalyses::all();

    auto &DT = AM.getResult<DominatorTreeAnalysis>(F);
    PromoteMemToReg(Allocas, DT, nullptr);

    return PreservedAnalyses::none();
}

// ===== Pass Registration =====
llvm::PassPluginLibraryInfo getMyMem2RegPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "MyMem2RegPass", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "my-mem2reg-custom") {
                        FPM.addPass(MyMem2RegPass());
                        return true;
                    }
                    return false;
                });
        }
    };
}

// Export entry point for plugin
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo_MyMem2Reg() {
    return getMyMem2RegPassPluginInfo();
}
