#include "antlr4-runtime.h"
#include "ImageLangLexer.h"
#include "ImageLangParser.h"
#include "ImageLangVisitorImpl.h"   
#include "ImageRuntime.h"
#include "AST.h"
#include <fstream>
#include <iostream>

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

int main(int argc, const char* argv[])
{
    // Initialize LLVM target for JIT
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    
    llvm::sys::DynamicLibrary::LoadLibraryPermanently("libruntime.so");
    auto JITOrErr = llvm::orc::LLJITBuilder().create();
    if (!JITOrErr) {
        llvm::errs() << "Failed to create JIT\n";
        return 1;
    }

    // This defines the JIT variable
    auto JIT = std::move(*JITOrErr);

    // Allow JIT to see runtime symbols like load_image/save_image
    if (llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr)) 
    {
        std::cerr << "Failed to load current process symbols!\n";
    }
   

    //Load DSL from stream
    std::ifstream stream(argv[1]);
    antlr4::ANTLRInputStream inputStream(stream);
    ImageLangLexer lexer(&inputStream);
    antlr4::CommonTokenStream tokens(&lexer);
    ImageLangParser parser(&tokens);

    //Parse Tree
    auto tree = parser.program();

    //printParseTree(tree);

    //AST
    ASTBuilder builder;
    auto programAST = builder.build(tree);

    std::cout<<"Printing the Parse Tree : \n";

    programAST->print();

    
    std::cout<<"Get Struct Type : \n";

    getImageStructType();

    std::cout<<"Codegen : \n";

    programAST->codegen();
    std::cout<<"\n--------------------------------------------------------------------\n";
    std::cout<<"Printing the LLVM IR : \n";

    TheModule->print(llvm::errs(), nullptr);

    std::cout<<"\n--------------------------------------------------------------------\n";

    auto TheContextPtr = std::make_unique<llvm::LLVMContext>();
    auto TSM = llvm::orc::ThreadSafeModule(std::move(TheModule), std::move(TheContextPtr));

    auto generator = cantFail(
        llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
            JIT->getDataLayout().getGlobalPrefix()
        )
    );

    JIT->getMainJITDylib().addGenerator(std::move(generator));

    if (auto err = JIT->addIRModule(std::move(TSM))) {
        llvm::errs() << "Failed to add module to JIT\n";
        return 1;
    }

    auto sym = JIT->lookup("load_image");
    if (!sym) {
        llvm::errs() << "Symbol 'load_image' not found in JIT\n";
    }

    auto mainSym = JIT->lookup("main");
    if (!mainSym) {
        llvm::errs() << "JIT lookup failed for main\n";
        return 1;
    }

    using MainFnType = int (*)();

    // Convert ExecutorAddr to function pointer
    auto addr = mainSym->toPtr<MainFnType>();
    if (!addr) {
        llvm::errs() << "Failed to convert symbol to function pointer\n";
        return 1;
    }

    std::cout << "[JIT] Executing compiled IR...\n";
    int result = addr();
    std::cout << "[JIT] Execution finished with code " << result << "\n";
    
}