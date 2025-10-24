#include "CodegenContext.h"


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