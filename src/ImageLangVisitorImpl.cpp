#include "ImageLangVisitorImpl.h"
#include "ImageLangParser.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include "AST.h"

using namespace std;


llvm::Value *LoadExprAST::codegen() {
    auto *ImagePtrTy = llvm::PointerType::getUnqual(getImageStructType());

    // Declare or retrieve extern "C" function: Image* load_image(const char*)
    llvm::Function *loadFunc =
        getRuntimeFunction("load_image", ImagePtrTy,
                           { llvm::PointerType::get(TheContext, 0) });

    llvm::Value *pathValue = Builder.CreateGlobalStringPtr(Path, "path");
    llvm::Value *imgHandle = Builder.CreateCall(loadFunc, { pathValue }, "img");

    return imgHandle;  // Image*
}



llvm::Value *ImageDeclExprAST::codegen() {
    llvm::Value *initVal = InitExpr->codegen();
    if (!initVal) return nullptr;

    NamedValues[Name] = initVal;
    return initVal;
}


llvm::Value *StoreExprAST::codegen() {
    auto *ImagePtrTy = llvm::PointerType::getUnqual(getImageStructType());

    llvm::Function *saveFunc =
        getRuntimeFunction("save_image", llvm::Type::getVoidTy(TheContext),
                           { ImagePtrTy, llvm::PointerType::get(TheContext, 0) });

    auto it = NamedValues.find(ImageName);
    if (it == NamedValues.end()) {
        std::cerr << "Unknown image variable: " << ImageName << "\n";
        return nullptr;
    }

    llvm::Value *imgHandle = it->second;
    llvm::Value *pathValue = Builder.CreateGlobalStringPtr(Path, "save_path");

    Builder.CreateCall(saveFunc, { imgHandle, pathValue });
    return nullptr;
}



llvm::Value *ProgramAST::codegen() {
    llvm::FunctionType *FT = llvm::FunctionType::get(llvm::Type::getInt32Ty(TheContext), false);
    llvm::Function *MainFunc = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "main", TheModule.get());
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(TheContext, "entry", MainFunc);
    Builder.SetInsertPoint(BB);

    for (auto &stmt : Statements)
        stmt->codegen();

    Builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(TheContext), 0));

    return MainFunc;
}

