#include "CSEPass.h"


using namespace llvm;

PreservedAnalyses GlobalCSEPass::run(Function &F, FunctionAnalysisManager &AM) {
    DenseMap<ExprKey, Instruction *, ExprKeyInfo> ExprTable;
    bool Changed = false;

    for (auto &BB : F) {
        for (auto &I : BB) {
            if (I.isTerminator() || I.mayHaveSideEffects())
                continue;

            ExprKey Key{I.getOpcode(), {}};
            for (auto &Op : I.operands())
                Key.Operands.push_back(Op.get());

            auto It = ExprTable.find(Key);
            if (It != ExprTable.end()) {
                Instruction *Existing = It->second;
                I.replaceAllUsesWith(Existing);
                I.eraseFromParent();
                Changed = true;
                break; // important: restart BB iteration
            } else {
                ExprTable[Key] = &I;
            }
        }
    }

    return Changed ? PreservedAnalyses::none()
                    : PreservedAnalyses::all();
}


llvm::PassPluginLibraryInfo getGlobalCSEPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "GlobalCSEPass", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "global-cse") {
                        FPM.addPass(GlobalCSEPass());
                        return true;
                    }
                    return false;
                });
        }
    };
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return getGlobalCSEPluginInfo();
}
