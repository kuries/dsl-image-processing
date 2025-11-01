#include "antlr4-runtime.h"
#include "ImageLangLexer.h"
#include "ImageLangParser.h"
#include "ImageLangVisitorImpl.h"   
#include "ImageRuntime.h"
#include "Mem2RegPass.h"
#include "AST.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include "llvm/IR/Dominators.h"

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

    bool enableOpt = false;

    if (argc >= 3) { // we expect: ./image-dsl <file> -opt=true/false
        std::string optArg = argv[2];
        if (optArg == "-opt=true")
            enableOpt = true;
        else if (optArg == "-opt=false")
            enableOpt = false;
    }

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

    if (!stream.is_open()) 
    {
        std::cerr << "Failed to open file: " << argv[1] << "\n";
        return 1;
    }

    antlr4::ANTLRInputStream inputStream(stream);
    ImageLangLexer lexer(&inputStream);
    antlr4::CommonTokenStream tokens(&lexer);

    for (auto token : tokens.getTokens())
        std::cout << token->toString() << "\n";

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
    llvm::Module* m = TheModule.get();
    if (llvm::verifyModule(*m, &llvm::errs())) {
        llvm::errs() << "IR verification failed!\n";
    }

    std::cout<<"\n--------------------------------------------------------------------\n";

    if(enableOpt)
    {
    // ======== Apply Mem2RegPass via PassManager ========
        {
            llvm::PassBuilder PB;
            llvm::LoopAnalysisManager LAM;
            llvm::FunctionAnalysisManager FAM;
            llvm::CGSCCAnalysisManager CGAM;
            llvm::ModuleAnalysisManager MAM;

            // Register built-in analyses
            PB.registerModuleAnalyses(MAM);
            PB.registerCGSCCAnalyses(CGAM);
            PB.registerFunctionAnalyses(FAM);
            PB.registerLoopAnalyses(LAM);
            PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

            // Create a FunctionPassManager and add your plugin pass
            llvm::FunctionPassManager FPM;
            FPM.addPass(MyMem2RegPass()); // You defined this in your plugin .so

            // Run it on all functions in your module
            for (llvm::Function &F : *TheModule) {
                if (!F.isDeclaration())
                    FPM.run(F, FAM);
            }
        }

        std::cout<<"Printing the LLVM IR after Mem2Reg Transformation pass: \n";

        TheModule->print(llvm::errs(), nullptr);

        llvm::Module* m = TheModule.get();

        std::cout<<"\n\n\n";
        if (llvm::verifyModule(*m, &llvm::errs())) {
            llvm::errs() << "IR verification failed!\n";
        }

    std::cout<<"\n--------------------------------------------------------------------\n";

        // =================================================
    }
     




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
    auto start = chrono::high_resolution_clock::now();
    int result = addr();
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    std::cout << "[JIT] Execution finished with code " << result << "\n";
    std::cout << "[JIT] Execution time: " << duration << " ms\n";
}