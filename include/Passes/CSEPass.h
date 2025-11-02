#pragma once
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

struct GlobalCSEPass : public PassInfoMixin<GlobalCSEPass> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

struct ExprKey {
    unsigned Opcode;
    SmallVector<Value*, 2> Operands;
    Type* Ty;

    bool operator==(const ExprKey &Other) const {
        return Opcode == Other.Opcode && Ty == Other.Ty && Operands == Other.Operands;
    }
};

struct ExprKeyInfo {
    static inline ExprKey getEmptyKey() {
        return ExprKey{~0U, {}};
    }

    static inline ExprKey getTombstoneKey() {
        return ExprKey{~1U, {}};
    }

    static unsigned getHashValue(const ExprKey &K) {
        unsigned Hash = hash_value(K.Opcode);
        for (auto *Op : K.Operands)
            Hash = hash_combine(Hash, Op);
        return Hash;
    }

    static bool isEqual(const ExprKey &A, const ExprKey &B) {
        return A.Opcode == B.Opcode && A.Operands == B.Operands;
    }
};