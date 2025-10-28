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

        static llvm::Value* GetGEP(llvm::Value *array, llvm::Value* index){
            //todo
            llvm::Value *idx = ConverDoubleToInt(index);
            llvm::Value *gep = Builder.CreateInBoundsGEP(llvm::Type::getInt8Ty(TheContext), array, idx, "img_ptr");
            return gep;
        }   

        static llvm::Value* GEPLoad(llvm::Value *array, llvm::Value* index){
            llvm::Value *gep = GetGEP(array, index);
	        llvm::Value *loaded_byte = Builder.CreateLoad(Builder.getInt8Ty(), gep, "loaded_byte");
            return ConvertIntToDouble(loaded_byte);
        }

        static llvm::Value* GEPStore(llvm::Value *array, llvm::Value* index, llvm::Value *val){
            llvm::Value *gep = GetGEP(array, index);
            llvm::Value *asInt8 = ConverDoubleToInt8(val);
	        llvm::Value *store_inst = Builder.CreateStore(asInt8, gep, "loaded_byte");
            return store_inst;
        }

        static llvm::Value* TestImageAccess(llvm::Value *img, llvm::Value* idx){
            llvm::Value* val = GEPLoad(img, idx);
            printDouble(val);

            GEPStore(img, idx, idx);

            val = GEPLoad(img, idx);
            printDouble(val);
            return nullptr;
        }
        
        static llvm::Value* ConvertIntToDouble(llvm::Value* val){
            llvm::Value *extended = Builder.CreateZExt(val, Builder.getInt32Ty(), "extended");
            return Builder.CreateUIToFP(extended, llvm::Type::getDoubleTy(TheContext), "asDouble");
        }

        static llvm::Value* ConverDoubleToInt(llvm::Value* val){
            return Builder.CreateFPToSI(val, Builder.getInt32Ty(), "asInt32");
        }

        static llvm::Value* ConverDoubleToInt8(llvm::Value* val){
            llvm::Value *asInt32 = ConverDoubleToInt(val);
            return Builder.CreateTrunc(asInt32, Builder.getInt8Ty(), "asInt8");
        }

        static llvm::Value* ComputeIndex(std::string ArrayName, llvm::Value* i, llvm::Value* j){
            llvm::AllocaInst *heightA = NamedValues[ArrayName+".height"];
	        llvm::AllocaInst *widthA = NamedValues[ArrayName+".width"];

            llvm::Value *h = Builder.CreateLoad(heightA->getAllocatedType(), heightA, ArrayName+".height");
            llvm::Value *w = Builder.CreateLoad(widthA->getAllocatedType(), widthA, ArrayName+".width");

            llvm::Value *rowOffset = Builder.CreateMul(i, w, "rowOffset");
            // Compute i * width + j
            llvm::Value *index1D = Builder.CreateAdd(rowOffset, j, "index1D");
            return index1D;
        }

        static void printDouble(llvm::Value *val){
            llvm::Function *printInt = getRuntimeFunction("printDouble", llvm::Type::getVoidTy(TheContext),
						{ llvm::Type::getDoubleTy(TheContext) });
	        Builder.CreateCall(printInt, { val }, "print");
        }

        static void printInt(llvm::Value *val){
            llvm::Function *printInt = getRuntimeFunction("printInt", llvm::Type::getVoidTy(TheContext),
						{ llvm::Type::getInt8Ty(TheContext) });
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