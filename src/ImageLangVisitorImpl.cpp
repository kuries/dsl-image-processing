#include "ImageLangVisitorImpl.h"
#include "ImageLangParser.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include "AST.h"
#include "Helper.h"

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
  	llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();

    llvm::Value *initVal = InitExpr->codegen();
    if (!initVal) 
		return nullptr;
	
	llvm::AllocaInst *Alloca = Helper::CreateEntryBlockAlloca(TheFunction, Name.c_str(), getImageStructType());
	Builder.CreateStore(initVal, Alloca);
    NamedValues[Name] = Alloca;
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
	llvm::Value *imgHandle = Builder.CreateLoad(it->second->getAllocatedType(), it->second, ImageName.c_str());
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

// llvm::Value *IfExprAST::codegen() {
//   llvm::Value *CondV = Cond->codegen();
//   if (!CondV)
//     return nullptr;

//   // Convert condition to a bool by comparing non-equal to 0.0.
//   CondV = Builder.CreateFCmpONE(
//       CondV, llvm::ConstantFP::get(TheContext, llvm::APFloat(0.0)), "ifcond");

//   llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();

//   // Create blocks for the then and else cases.  Insert the 'then' block at the
//   // end of the function.
//   llvm::BasicBlock *ThenBB = llvm::BasicBlock::Create(TheContext, "then", TheFunction);
//   llvm::BasicBlock *ElseBB = llvm::BasicBlock::Create(TheContext, "else");
//   llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(TheContext, "ifcont");

//   Builder.CreateCondBr(CondV, ThenBB, ElseBB);

//   // Emit then value.
//   Builder.SetInsertPoint(ThenBB);

//   llvm::Value *ThenV = Then->codegen();
//   if (!ThenV)
//     return nullptr;

//   Builder.CreateBr(MergeBB);
//   // Codegen of 'Then' can change the current block, update ThenBB for the PHI.
//   ThenBB = Builder.GetInsertBlock();

//   // Emit else block.
//   TheFunction->insert(TheFunction->end(), ElseBB);
//   Builder.SetInsertPoint(ElseBB);

//   llvm::Value *ElseV = Else->codegen();
//   if (!ElseV)
//     return nullptr;

//   Builder.CreateBr(MergeBB);
//   // Codegen of 'Else' can change the current block, update ElseBB for the PHI.
//   ElseBB = Builder.GetInsertBlock();

//   // Emit merge block.
//   TheFunction->insert(TheFunction->end(), MergeBB);
//   Builder.SetInsertPoint(MergeBB);
//   llvm::PHINode *PN = Builder.CreatePHI(llvm::Type::getDoubleTy(TheContext), 2, "iftmp");

//   PN->addIncoming(ThenV, ThenBB);
//   PN->addIncoming(ElseV, ElseBB);
//   return PN;
// }