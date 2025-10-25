#include "CodegenContext.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"

llvm::LLVMContext TheContext;
std::unique_ptr<llvm::Module> TheModule = std::make_unique<llvm::Module>("main", TheContext);
llvm::IRBuilder<> Builder(TheContext);
std::map<std::string, llvm::Value*> NamedValues;

llvm::Function* getRuntimeFunction(const std::string &name, llvm::Type *retType,
                                   const std::vector<llvm::Type*> &args) {
    llvm::Function *func = TheModule->getFunction(name);
    if (!func) {
        llvm::FunctionType *FT =
            llvm::FunctionType::get(retType, args, false);
        func = llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
                                      name, TheModule.get());
    }
    return func;
}


llvm::StructType *getImageStructType() {
    static llvm::StructType *ImageStruct = nullptr;
    if (ImageStruct)
        return ImageStruct;

    // Create new named struct in this context
    ImageStruct = llvm::StructType::create(TheContext, "struct.Image");

    // Must explicitly create a vector for setBody
    std::vector<llvm::Type *> members = {
        llvm::PointerType::get(TheContext, 0),  // data
        llvm::Type::getInt32Ty(TheContext),    // width
        llvm::Type::getInt32Ty(TheContext),    // height
        llvm::Type::getInt32Ty(TheContext)     // channels
    };

    ImageStruct->setBody(members, /*isPacked*/ false);
    return ImageStruct;
}