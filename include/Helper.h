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

        static llvm::Value* GetGEP(llvm::Value *array, int index){
            llvm::Value *idx = llvm::ConstantInt::get(llvm::IntegerType::getInt64Ty(TheContext), index);
            llvm::Value *gep = Builder.CreateInBoundsGEP(llvm::Type::getInt8Ty(TheContext), array, idx, "img_ptr");
            return gep;
        }

        static llvm::Value* GEPLoad(llvm::Value *array, int index){
            llvm::Value *gep = GetGEP(array, index);
	        llvm::Value *loaded_byte = Builder.CreateLoad(Builder.getInt8Ty(), gep, "loaded_byte");
            return loaded_byte;
        }

        static llvm::Value* GEPStore(llvm::Value *array, int index, llvm::Value *val){
            llvm::Value *gep = GetGEP(array, index);
	        llvm::Value *store_inst = Builder.CreateStore(val, gep, "loaded_byte");
            return store_inst;
        }

        static void printInt(llvm::Value *val){
            llvm::Function *printInt = getRuntimeFunction("printInt", llvm::Type::getVoidTy(TheContext),
						{ llvm::Type::getDoubleTy(TheContext) });
	        Builder.CreateCall(printInt, { val }, "print");
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