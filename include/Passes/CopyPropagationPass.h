#pragma once

#include "CopyPropagationPass.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/ValueHandle.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/Debug.h"
#include <unordered_map>

namespace llvm {

class CopyPropagationPass : public PassInfoMixin<CopyPropagationPass> {
public:
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

} 
