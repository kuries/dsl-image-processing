#include "ImageLangVisitorImpl.h"
#include "ImageLangParser.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include "AST.h"
#include "Helper.h"

using namespace std;


llvm::Value *LoadExprAST::codegen() {
	cout<<"entered load";
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
	auto *ImagePtrTy = llvm::PointerType::getUnqual(getImageStructType());
  	llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();

    llvm::Value *initVal = InitExpr->codegen();
    if (!initVal) 
		return nullptr;
	
	//Store image
	llvm::AllocaInst *Alloca = Helper::CreateEntryBlockAlloca(TheFunction, Name.c_str(), ImagePtrTy);
	Builder.CreateStore(initVal, Alloca);
    NamedValues[Name] = Alloca;
	//Store image properties
	llvm::Function *loadImageHeight =
        getRuntimeFunction("get_image_height", llvm::Type::getDoubleTy(TheContext),
                           { llvm::PointerType::get(TheContext, 0) });
	llvm::Function *loadImageWidth =
        getRuntimeFunction("get_image_width", llvm::Type::getDoubleTy(TheContext),
                           { llvm::PointerType::get(TheContext, 0) });
	llvm::Function *loadImageData =
        getRuntimeFunction("get_image_data", llvm::PointerType::get(TheContext, 0),
                           { llvm::PointerType::get(TheContext, 0) });
	
						   
	llvm::AllocaInst *AllocaH = Helper::CreateEntryBlockAlloca(TheFunction, Name.c_str(), llvm::Type::getDoubleTy(TheContext));
	llvm::AllocaInst *AllocaW = Helper::CreateEntryBlockAlloca(TheFunction, Name.c_str(), llvm::Type::getDoubleTy(TheContext));
	llvm::AllocaInst *AllocaD = Helper::CreateEntryBlockAlloca(TheFunction, Name.c_str(), llvm::PointerType::get(TheContext, 0));

	llvm::Value *height = Builder.CreateCall(loadImageHeight, { initVal }, "h");
	llvm::Value *width = Builder.CreateCall(loadImageWidth, { initVal }, "w");
	llvm::Value *data = Builder.CreateCall(loadImageData, { initVal }, "imgdata");

	Builder.CreateStore(data, AllocaD);
	NamedValues[Name+".data"] = AllocaD;

	Builder.CreateStore(height, AllocaH);
	NamedValues[Name+".height"] = AllocaH;

	Builder.CreateStore(width, AllocaW);
	NamedValues[Name+".width"] = AllocaW;

    return initVal;
}


llvm::Value *StoreExprAST::codegen() {
    auto *ImagePtrTy = llvm::PointerType::getUnqual(getImageStructType());

    llvm::Function *saveFunc =
        getRuntimeFunction("save_image", llvm::Type::getVoidTy(TheContext),
                           { ImagePtrTy, llvm::PointerType::get(TheContext, 0), llvm::PointerType::get(TheContext, 0) });
    
	auto it = NamedValues.find(ImageName);
    if (it == NamedValues.end()) {
        std::cerr << "Unknown image variable: " << ImageName << "\n";
        return nullptr;
    }
	
	auto imgDataA = NamedValues.find(ImageName+".data");
	llvm::Value *imgHandle = Builder.CreateLoad(it->second->getAllocatedType(), it->second, ImageName.c_str());
	llvm::Value *imgData = Builder.CreateLoad(imgDataA->second->getAllocatedType(), imgDataA->second, ImageName.c_str());
    llvm::Value *pathValue = Builder.CreateGlobalStringPtr(Path, "save_path");

    Builder.CreateCall(saveFunc, { imgHandle, imgData, pathValue });
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


llvm::Value* AssignExprAST::codegen() {
	std::string lhsName;

	if (VariableExprAST *LHSEVar = dynamic_cast<VariableExprAST *>(LHS.get()))
	{
		cout<<"Variable\n";
		lhsName = LHSEVar->getName();
		llvm::Value* lhsAlloc = NamedValues[lhsName];
		llvm::Value *rhs = RHS->codegen();

		Helper::printDouble(rhs);

		return Builder.CreateStore(rhs, lhsAlloc);
	}
	else if (ArrayAccessExprAST *LHSEArr = dynamic_cast<ArrayAccessExprAST *>(LHS.get())) 
	{
		cout<<"Array\n";
		lhsName = LHSEArr->getName();
		llvm::AllocaInst *imgDataA = NamedValues[lhsName+".data"];
		llvm::Value *imageData = Builder.CreateLoad(imgDataA->getAllocatedType(), imgDataA, lhsName+".data");

		llvm::Value *i = LHSEArr->Index1->codegen();
		llvm::Value *j = LHSEArr->Index2->codegen();
		llvm::Value *k = LHSEArr->Index3->codegen();
		llvm::Value *rhs = RHS->codegen();
		cout<<"Good so far?\n";
		llvm::Value* index1D = Helper::ComputeIndex(lhsName, i, j, k);
		
		Helper::printDouble(rhs);
		return Helper::GEPStore(imageData, index1D, rhs);
	}
	else
		return Helper::LogErrorV("destination of '=' must be a Variable or Array");
	
	return nullptr;
}

llvm::Value* ArrayAccessExprAST::codegen() {
    // TODO: implement codegen
	llvm::AllocaInst *imgDataA = NamedValues[ArrayName+".data"];
	llvm::Value *imageData = Builder.CreateLoad(imgDataA->getAllocatedType(), imgDataA, ArrayName+".data");

	llvm::Value *i = Index1->codegen();
	llvm::Value *j = Index2->codegen();
	llvm::Value *k = Index3->codegen();

	llvm::Value* index1D = Helper::ComputeIndex(ArrayName, i, j, k);

	return Helper::GEPLoad(imageData, index1D);
}


//Primary Expressions

llvm::Value *NumberExprAST::codegen() {
  return llvm::ConstantFP::get(TheContext, llvm::APFloat(Val));
}

llvm::Value *VariableExprAST::codegen() {
  // Look this variable up in the function.
  llvm::AllocaInst *A = NamedValues[Name];
  if (!A)
  {
	std::string errorStr = "Unknown variable name " + Name;
	return Helper::LogErrorV(errorStr.c_str());

  }
    

  // Load the value.
  return Builder.CreateLoad(A->getAllocatedType(), A, Name.c_str());
}

llvm::Value *VarDeclExprAST::codegen() {
	llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();
	
	llvm::AllocaInst *Alloca = Helper::CreateEntryBlockAlloca(TheFunction, Name.c_str(), llvm::Type::getDoubleTy(TheContext));
	NamedValues[Name] = Alloca;

	// Store the value.
	if(InitExpr != nullptr){
		llvm::Value *val = InitExpr->codegen();
		Builder.CreateStore(val, Alloca);
	}
	return nullptr;
}

llvm::Value *BinaryExprAST::codegen() {
	// Special case '=' because we don't want to emit the LHS as an expression.
		if (Op == "=") {
		// Assignment requires the LHS to be an identifier.
		VariableExprAST *LHSE = static_cast<VariableExprAST *>(LHS.get());
		if (!LHSE)
			return Helper::LogErrorV("destination of '=' must be a variable");

				// Codegen the RHS.
		llvm::Value *Val = RHS->codegen();
		if (!Val)
			return nullptr;
		
		// Look up the name.
		llvm::Value *Variable = NamedValues[LHSE->getName()];
		if (!Variable)
			return Helper::LogErrorV("Unknown variable name");
		
		Builder.CreateStore(Val, Variable);
		return Val;
	}

	llvm::Value *L = LHS->codegen();
	llvm::Value *R = RHS->codegen();
	if (!L || !R)
		return nullptr;

	if (Op == "+")
        return Builder.CreateFAdd(L, R, "addtmp");
    else if (Op == "-")
        return Builder.CreateFSub(L, R, "subtmp");
    else if (Op == "*")
        return Builder.CreateFMul(L, R, "multmp");
    else if (Op == "/")
        return Builder.CreateFDiv(L, R, "divtmp");
    else if (Op == "%")
        return Builder.CreateFRem(L, R, "modtmp");

    // Comparisons (return double 0.0 or 1.0)
    else if (Op == "<") {
        llvm::Value *Cmp = Builder.CreateFCmpULT(L, R, "cmptmp");
        return Builder.CreateUIToFP(Cmp, llvm::Type::getDoubleTy(TheContext), "booltmp");
    } 
    else if (Op == "<=") {
        llvm::Value *Cmp = Builder.CreateFCmpULE(L, R, "cmptmp");
        return Builder.CreateUIToFP(Cmp, llvm::Type::getDoubleTy(TheContext), "booltmp");
    }
    else if (Op == ">") {
        llvm::Value *Cmp = Builder.CreateFCmpUGT(L, R, "cmptmp");
        return Builder.CreateUIToFP(Cmp, llvm::Type::getDoubleTy(TheContext), "booltmp");
    }
    else if (Op == ">=") {
        llvm::Value *Cmp = Builder.CreateFCmpUGE(L, R, "cmptmp");
        return Builder.CreateUIToFP(Cmp, llvm::Type::getDoubleTy(TheContext), "booltmp");
    }
    else if (Op == "==") {
        llvm::Value *Cmp = Builder.CreateFCmpUEQ(L, R, "cmptmp");
        return Builder.CreateUIToFP(Cmp, llvm::Type::getDoubleTy(TheContext), "booltmp");
    }
    else if (Op == "!=") {
        llvm::Value *Cmp = Builder.CreateFCmpUNE(L, R, "cmptmp");
        return Builder.CreateUIToFP(Cmp, llvm::Type::getDoubleTy(TheContext), "booltmp");
    }

    // Logical operators (assuming nonzero means true)
    else if (Op == "&&") {
        llvm::Value *LCond = Builder.CreateFCmpONE(
            L, llvm::ConstantFP::get(TheContext, llvm::APFloat(0.0)), "lcond");
        llvm::Value *RCond = Builder.CreateFCmpONE(
            R, llvm::ConstantFP::get(TheContext, llvm::APFloat(0.0)), "rcond");
        llvm::Value *AndVal = Builder.CreateAnd(LCond, RCond, "andtmp");
        return Builder.CreateUIToFP(AndVal, llvm::Type::getDoubleTy(TheContext), "booltmp");
    } 
    else if (Op == "||") {
        llvm::Value *LCond = Builder.CreateFCmpONE(
            L, llvm::ConstantFP::get(TheContext, llvm::APFloat(0.0)), "lcond");
        llvm::Value *RCond = Builder.CreateFCmpONE(
            R, llvm::ConstantFP::get(TheContext, llvm::APFloat(0.0)), "rcond");
        llvm::Value *OrVal = Builder.CreateOr(LCond, RCond, "ortmp");
        return Builder.CreateUIToFP(OrVal, llvm::Type::getDoubleTy(TheContext), "booltmp");
    }

    // Bitwise (if your language allows it, usually for ints)
    else if (Op == "&")
        return Builder.CreateAnd(L, R, "andtmp");
    else if (Op == "|")
        return Builder.CreateOr(L, R, "ortmp");
    else if (Op == "^")
        return Builder.CreateXor(L, R, "xortmp");

    return Helper::LogErrorV("Invalid binary operator");
}

llvm::Value *IfExprAST::codegen() {
	llvm::Value *CondV = Cond->codegen();
	if (!CondV)
		return nullptr;

	// Convert condition to a bool by comparing non-equal to 0.0.
	CondV = Builder.CreateFCmpONE(
		CondV, llvm::ConstantFP::get(TheContext, llvm::APFloat(0.0)), "ifcond");

	llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();

	// Create blocks for the then and else cases.  Insert the 'then' block at the
	// end of the function.
	llvm::BasicBlock *ThenBB = llvm::BasicBlock::Create(TheContext, "then", TheFunction);
	llvm::BasicBlock *ElseBB = llvm::BasicBlock::Create(TheContext, "else");
	llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(TheContext, "ifcont");

	Builder.CreateCondBr(CondV, ThenBB, ElseBB);
	// Emit then value.
	Builder.SetInsertPoint(ThenBB);
	if(Then.empty())
		return nullptr;
	for(int i=0; i<Then.size()-1; i++)
		Then[i]->codegen();
	llvm::Value *ThenV = Then.back()->codegen();
	if(!ThenV)
		return nullptr;

	Builder.CreateBr(MergeBB);
	// Codegen of 'Then' can change the current block, update ThenBB for the PHI.
	ThenBB = Builder.GetInsertBlock();
	// Emit else block.
	TheFunction->insert(TheFunction->end(), ElseBB);
	Builder.SetInsertPoint(ElseBB);
	llvm::Value *ElseV;
	if(!Else.empty())
	{
		for(int i=0; i<Else.size()-1; i++)
		Else[i]->codegen();
		ElseV = Else.back()->codegen();
		if (!ElseV)
			return nullptr;
	}
	else{
		ElseV = llvm::ConstantFP::get(TheContext, llvm::APFloat(0.0));
	}

	Builder.CreateBr(MergeBB);
	// Codegen of 'Else' can change the current block, update ElseBB for the PHI.
	ElseBB = Builder.GetInsertBlock();

	// Emit merge block.
	TheFunction->insert(TheFunction->end(), MergeBB);
	Builder.SetInsertPoint(MergeBB);
	llvm::PHINode *PN = Builder.CreatePHI(llvm::Type::getDoubleTy(TheContext), 2, "iftmp");

	PN->addIncoming(ThenV, ThenBB);
	PN->addIncoming(ElseV, ElseBB);
	return PN;
}


llvm::Value *ForExprAST::codegen() {
	llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();

	// Create an alloca for the variable in the entry block.
	llvm::AllocaInst *Alloca = Helper::CreateEntryBlockAlloca(TheFunction, VarName);

	// Emit the start code first, without 'variable' in scope.
	llvm::Value *StartVal = Start->codegen();
	if (!StartVal)
	return nullptr;

	// Store the value into the alloca.
	Builder.CreateStore(StartVal, Alloca);

	// Make the new basic block for the loop header, inserting after current
	// block.
	llvm::BasicBlock *LoopBB = llvm::BasicBlock::Create(TheContext, "loop", TheFunction);

	// Insert an explicit fall through from the current block to the LoopBB.
	Builder.CreateBr(LoopBB);

	// Start insertion in LoopBB.
	Builder.SetInsertPoint(LoopBB);

	// Within the loop, the variable is defined equal to the PHI node.  If it
	// shadows an existing variable, we have to restore it, so save it now.
	llvm::AllocaInst *OldVal = NamedValues[VarName];
	NamedValues[VarName] = Alloca;

	// Emit the body of the loop.  This, like any other expr, can change the
	// current BB.  Note that we ignore the value computed by the body, but don't
	// allow an error.
	if(Body.empty())
		return nullptr;
	for(int i=0; i<Body.size(); i++){
		llvm::Value* ret = Body[i]->codegen();
		if(!ret)
			return nullptr;
	}
	

	// Emit the step value.
	llvm::Value *StepVal = nullptr;
	if (Step) {
		StepVal = Step->codegen();
	if (!StepVal)
		return nullptr;
	} else {
	// If not specified, use 1.0.
		StepVal = llvm::ConstantFP::get(TheContext, llvm::APFloat(1.0));
	}

	// Compute the end condition.
	llvm::Value *EndCond = End->codegen();
	if (!EndCond)
		return nullptr;

	// Reload, increment, and restore the alloca.  This handles the case where
	// the body of the loop mutates the variable.
	llvm::Value *CurVar =
		Builder.CreateLoad(Alloca->getAllocatedType(), Alloca, VarName.c_str());
	llvm::Value *NextVar = Builder.CreateFAdd(CurVar, StepVal, "nextvar");
	Builder.CreateStore(NextVar, Alloca);

	// i<End
	EndCond = Builder.CreateFCmpULT(NextVar, EndCond, "cond");

	// Create the "after loop" block and insert it.
	llvm::BasicBlock *AfterBB =
		llvm::BasicBlock::Create(TheContext, "afterloop", TheFunction);

	// Insert the conditional branch into the end of LoopEndBB.
	Builder.CreateCondBr(EndCond, LoopBB, AfterBB);

	// Any new code will be inserted in AfterBB.
	Builder.SetInsertPoint(AfterBB);

	// Restore the unshadowed variable.
	if (OldVal)
		NamedValues[VarName] = OldVal;
	else
		NamedValues.erase(VarName);

	// for expr always returns 0.0.
	return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(TheContext));
}