#include "ConstantFoldingPass.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

PreservedAnalyses ConstantFoldingPass::run(Function &F, FunctionAnalysisManager &AM) {
    bool Changed = false;

    for (auto &BB : F) {
        for (auto InstIt = BB.begin(), E = BB.end(); InstIt != E; ) {
            Instruction *I = &*InstIt++;

            // Handle floating-point binary operations
            if (auto *BO = dyn_cast<BinaryOperator>(I)) {
                Value *Op0 = BO->getOperand(0);
                Value *Op1 = BO->getOperand(1);

                // --- FLOAT FOLDING ---
                if (auto *C1 = dyn_cast<ConstantFP>(Op0)) {
                    if (auto *C2 = dyn_cast<ConstantFP>(Op1)) {
                        Constant *Folded = nullptr;

                        switch (BO->getOpcode()) {
                            case Instruction::FAdd:
                                Folded = ConstantFP::get(C1->getType(),
                                    C1->getValueAPF() + C2->getValueAPF());
                                break;
                            case Instruction::FSub:
                                Folded = ConstantFP::get(C1->getType(),
                                    C1->getValueAPF() - C2->getValueAPF());
                                break;
                            case Instruction::FMul:
                                Folded = ConstantFP::get(C1->getType(),
                                    C1->getValueAPF() * C2->getValueAPF());
                                break;
                            case Instruction::FDiv:
                                if (!C2->isZero())
                                    Folded = ConstantFP::get(C1->getType(),
                                        C1->getValueAPF() / C2->getValueAPF());
                                break;
                            default:
                                break;
                        }

                        if (Folded) {
                            BO->replaceAllUsesWith(Folded);
                            BO->eraseFromParent();
                            Changed = true;
                            continue;
                        }
                    }
                }

                // --- INTEGER FOLDING ---
                if (auto *C1 = dyn_cast<ConstantInt>(Op0)) {
                    if (auto *C2 = dyn_cast<ConstantInt>(Op1)) {
                        Constant *Folded = nullptr;
                        switch (BO->getOpcode()) {
                            case Instruction::Add:
                                Folded = ConstantInt::get(C1->getType(),
                                    C1->getValue() + C2->getValue());
                                break;
                            case Instruction::Sub:
                                Folded = ConstantInt::get(C1->getType(),
                                    C1->getValue() - C2->getValue());
                                break;
                            case Instruction::Mul:
                                Folded = ConstantInt::get(C1->getType(),
                                    C1->getValue() * C2->getValue());
                                break;
                            case Instruction::SDiv:
                            case Instruction::UDiv:
                                if (!C2->isZero())
                                    Folded = ConstantInt::get(C1->getType(),
                                        C1->getValue().sdiv(C2->getValue()));
                                break;
                            default:
                                break;
                        }

                        if (Folded) {
                            BO->replaceAllUsesWith(Folded);
                            BO->eraseFromParent();
                            Changed = true;
                            continue;
                        }
                    }
                }
            }
        }
    }

    return Changed ? PreservedAnalyses::none()
                   : PreservedAnalyses::all();
}


// ===== Pass Registration =====
llvm::PassPluginLibraryInfo getConstantFoldingPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "ConstantFoldingPass", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "const-fold") {
                        FPM.addPass(ConstantFoldingPass());
                        return true;
                    }
                    return false;
                });
        }
    };
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return getConstantFoldingPluginInfo();
}
