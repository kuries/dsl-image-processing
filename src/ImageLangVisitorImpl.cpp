#include "ImageLangVisitorImpl.h"
#include "ImageLangParser.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include "AST.h"

using namespace std;

llvm::Value *LoadExprAST::codegen() {
    llvm::PointerType  *i8PtrTy = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(TheContext));

    // Create or get the function prototype
    llvm::Function *loadFunc = getRuntimeFunction("load_image", i8PtrTy, { i8PtrTy });

    // Create global string for the path
    llvm::Value *pathValue = Builder.CreateGlobalStringPtr(Path, "path");

    // Call load_image(path)
    llvm::Value *result = Builder.CreateCall(loadFunc, { pathValue }, "img_handle");

    return result; // i8* handle to the loaded image
}


llvm::Value *ImageDeclExprAST::codegen() {
    llvm::Value *initVal = InitExpr->codegen();
    if (!initVal) return nullptr;

    NamedValues[Name] = initVal;
    return initVal;
}


llvm::Value *StoreExprAST::codegen() {
    llvm::PointerType  *i8PtrTy = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(TheContext));
    llvm::Function *saveFunc = getRuntimeFunction("save_image", llvm::Type::getVoidTy(TheContext),
                                                  { i8PtrTy, i8PtrTy });

    auto it = NamedValues.find(ImageName);
    if (it == NamedValues.end()) {
        std::cerr << "Unknown variable: " << ImageName << "\n";
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

