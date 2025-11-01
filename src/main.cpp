#include "antlr4-runtime.h"
#include "ImageLangLexer.h"
#include "ImageLangParser.h"
#include "ImageLangVisitorImpl.h"   
#include "ImageRuntime.h"
#include "Mem2RegPass.h"
#include "CSEPass.h"
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

inline void printModuleIR(const llvm::Module &M, const std::string &title = "", std::string filePath="") {
    std::cout << "\n--------------------------------------------------------------------\n";
    if (!title.empty())
        std::cout << "Printing the LLVM IR " << title << ":\n";
    else
        std::cout << "Printing the LLVM IR:\n";
    
    std::error_code EC;
    std::cout << "--------------------------------------------------------------------\n";
    if(filePath.empty())
        M.print(llvm::errs(), nullptr);
    else{
        llvm::raw_fd_ostream OS(filePath, EC, llvm::sys::fs::OF_None);
        M.print(OS, nullptr);
    }
        
    std::cout << "\n--------------------------------------------------------------------\n";
}

inline void verifyModuleIR(const llvm::Module &M, const std::string &context = "") {
    std::cout << "\nVerifying Module";
    if (!context.empty()) std::cout << " (" << context << ")";
    std::cout << "...\n";

    if (llvm::verifyModule(M, &llvm::errs())) {
        llvm::errs() << "IR verification failed";
        if (!context.empty()) llvm::errs() << " after " << context;
        llvm::errs() << "!\n";
    } else {
        std::cout << "Module verification succeeded";
        if (!context.empty()) std::cout << " after " << context;
        std::cout << ".\n";
    }

    std::cout << "--------------------------------------------------------------------\n";
}

template <typename PassType>
void runFunctionPassOnModule(llvm::Module &M, PassType &&Pass, const std::string &PassName) {
    using namespace llvm;

    // Create the analysis managers and register analyses
    PassBuilder PB;
    LoopAnalysisManager LAM;
    FunctionAnalysisManager FAM;
    CGSCCAnalysisManager CGAM;
    ModuleAnalysisManager MAM;

    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    // Create a function pass manager
    FunctionPassManager FPM;
    FPM.addPass(std::forward<PassType>(Pass));

    // Run the pass on each function
    for (Function &F : M) {
        if (!F.isDeclaration())
            FPM.run(F, FAM);
    }
}



int main(int argc, const char* argv[])
{

    bool enableMem2RegOpt = true;
    bool enableCSE = true;

    if (argc >= 3) { // we expect: ./image-dsl <file> -opt=true/false
        for(int i = 2; i < argc; i++)
        {
            std::string optArg = argv[i];
            if (optArg == "-mem2reg=true")
                enableMem2RegOpt = true;
            else if (optArg == "-mem2reg=false")
                enableMem2RegOpt = false;
            else if (optArg == "-cse=false")
                enableCSE = false;
            else if (optArg == "-cse=true")
                enableCSE = true;
        }
        
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


    printModuleIR(*TheModule, "Before Optimization");
    verifyModuleIR(*TheModule, "Before Optimization");

    // After Mem2Reg
    if (enableMem2RegOpt) {
        runFunctionPassOnModule(*TheModule, MyMem2RegPass(), "Mem2Reg");
        // printModuleIR(*TheModule, "After Mem2Reg");
        verifyModuleIR(*TheModule, "After Mem2Reg");
    }

    // After CSE
    if (enableCSE) {
        runFunctionPassOnModule(*TheModule, GlobalCSEPass(), "CSE");
        // printModuleIR(*TheModule, "After GlobalCSE");
        verifyModuleIR(*TheModule, "After GlobalCSE");
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