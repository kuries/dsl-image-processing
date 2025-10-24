#include "antlr4-runtime.h"
#include "ImageLangLexer.h"
#include "ImageLangParser.h"
#include "ImageLangVisitorImpl.h"   // ✅ include the visitor header
#include "ImageRuntime.h"
#include "AST.h"
#include <fstream>
#include <iostream>


#include "llvm/Support/TargetSelect.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"


// Recursive pretty printer
void printParseTree(antlr4::tree::ParseTree *tree, const std::string &indent = "", bool last = true) {
    std::cout << indent;
    if (last)
        std::cout << "└─";
    else
        std::cout << "├─";

    std::string nodeText = tree->toString();
    std::cout << nodeText << std::endl;

    auto children = tree->children;
    for (size_t i = 0; i < children.size(); ++i) {
        printParseTree(children[i], indent + (last ? "  " : "│ "), i == children.size() - 1);
    }


}


// int main(int argc, const char* argv[]) {
//     if (argc < 2) {
//         std::cerr << "Usage: image-dsl <file.imgdsl>\n";
//         return 1;
//     }

//     std::ifstream stream(argv[1]);
//     antlr4::ANTLRInputStream input(stream);
//     ImageLangLexer lexer(&input);
//     antlr4::CommonTokenStream tokens(&lexer);
//     ImageLangParser parser(&tokens);

//     antlr4::tree::ParseTree *tree = parser.program();
//     printParseTree(tree);
    
//     // ImageLangVisitorImpl visitor;
//     // visitor.visit(tree);

//     // ImageLangParser::ProgramContext* programCtx = parser.program();

//     // ASTBuilderVisitor builder;
//     // auto ASTProgram = builder.build(programCtx);

//     // std::cout << "IR generated with " << ASTProgram->size() << " statements.\n";

//     // IRPrinter::print(*ASTProgram);

//     return 0;
// }


int main(int argc, const char* argv[])
{
    // Initialize LLVM target for JIT
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    // // Allow JIT to see runtime symbols like load_image/save_image
    llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);

    llvm::sys::DynamicLibrary::LoadLibraryPermanently("libruntime.so");
    auto JITOrErr = llvm::orc::LLJITBuilder().create();
    if (!JITOrErr) {
        llvm::errs() << "Failed to create JIT\n";
        return 1;
    }

    // This defines the JIT variable
    auto JIT = std::move(*JITOrErr);

//    auto &JD = JIT->getMainJITDylib();
//     llvm::orc::MangleAndInterner Mangle(JIT->getExecutionSession(), JIT->getDataLayout());

//     // Register runtime functions using absoluteSymbols
// llvm::orc::SymbolMap symbols;

// symbols[Mangle("load_image")] = llvm::JITEvaluatedSymbol(
//     llvm::pointerToJITTargetAddress(&load_image),
//     llvm::JITSymbolFlags::Exported
// );

// symbols[Mangle("save_image")] = llvm::JITEvaluatedSymbol(
//     llvm::pointerToJITTargetAddress(&save_image),
//     llvm::JITSymbolFlags::Exported
// );

// JD.define(llvm::orc::absoluteSymbols(std::move(symbols)));


    //Load DSL from stream
    std::ifstream stream(argv[1]);
    antlr4::ANTLRInputStream inputStream(stream);
    ImageLangLexer lexer(&inputStream);
    antlr4::CommonTokenStream tokens(&lexer);
    ImageLangParser parser(&tokens);

    //Parse Tree
    auto tree = parser.program();

    //AST
    ASTBuilder builder;
    auto programAST = builder.build(tree);

    std::cout<<"Printing the Parse Tree : \n";

    programAST->print();

    programAST->codegen();
    std::cout<<"\n--------------------------------------------------------------------\n";
    std::cout<<"Printing the LLVM IR : \n";

    TheModule->print(llvm::errs(), nullptr);

    std::cout<<"\n--------------------------------------------------------------------\n";

    auto TheContextPtr = std::make_unique<llvm::LLVMContext>();
    auto TSM = llvm::orc::ThreadSafeModule(std::move(TheModule), std::move(TheContextPtr));

    if (auto err = JIT->addIRModule(std::move(TSM))) {
        llvm::errs() << "Failed to add module to JIT\n";
        return 1;
    }

}