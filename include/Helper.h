#pragma once

#include "AST.h"
#include "ImageRuntime.h"

#include <unordered_map>
#include <string>

using namespace std;

class Helper{

   public: 
        /// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
        /// the function.  This is used for mutable variables etc.
        static llvm::AllocaInst *CreateEntryBlockAlloca(
            llvm::Function *TheFunction, 
            llvm::StringRef VarName, 
            llvm:: Type *type = llvm::Type::getDoubleTy(TheContext)
        ){
            llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
            return TmpB.CreateAlloca(type, nullptr, VarName);
        }

        /// LogError* - These are little helper functions for error handling.
        static std::unique_ptr<ExprAST> LogError(const char *Str) {
            fprintf(stderr, "Error: %s\n", Str);
            return nullptr;
        }

        static llvm::Value *LogErrorV(const char *Str) {
            LogError(Str);
            return nullptr;
        }
};