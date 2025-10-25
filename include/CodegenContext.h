#ifndef CODEGEN_CONTEXT_H
#define CODEGEN_CONTEXT_H

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DerivedTypes.h"
#include <map>
#include <memory>
#include <string>

// Global LLVM objects (defined in one .cpp file)
extern llvm::LLVMContext TheContext;
extern std::unique_ptr<llvm::Module> TheModule;
extern llvm::IRBuilder<> Builder;

// Symbol table for variable → Value*
extern std::map<std::string, llvm::Value*> NamedValues;

llvm::Function* getRuntimeFunction(const std::string &name,
                                   llvm::Type *retType,
                                   const std::vector<llvm::Type*> &args);

llvm::StructType *getImageStructType();

#endif // CODEGEN_CONTEXT_H
