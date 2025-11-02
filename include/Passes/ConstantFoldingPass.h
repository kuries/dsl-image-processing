#pragma once

#include "llvm/IR/PassManager.h"

namespace llvm {

struct ConstantFoldingPass : public PassInfoMixin<ConstantFoldingPass> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

}
