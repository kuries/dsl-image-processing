#pragma once

#include <fstream>
#include <iostream>
#include <chrono>

#include "Mem2RegPass.h"
#include "CSEPass.h"
#include "ConstantFoldingPass.h"
#include "CopyPropagationPass.h"
#include "antlr4-runtime.h"
#include "ImageLangLexer.h"
#include "ImageLangParser.h"
#include "ImageLangVisitorImpl.h"   
#include "ImageRuntime.h"
#include "AST.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include "llvm/IR/Dominators.h"



struct CompilerOptions {
    bool enableMem2RegOpt = true;
    bool enableCSE = true;
    bool enableConstantFolding = true;
    bool enableCopyPropagation = true;
    bool printLogs = false;
};

class ImageDSLExecutor {
public:
    std::unique_ptr<llvm::orc::LLJIT> JIT;
    std::unique_ptr<llvm::LLVMContext> Context;

    ImageDSLExecutor() {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();

        auto JITOrErr = llvm::orc::LLJITBuilder().create();
        if (!JITOrErr) {
            llvm::errs() << "Failed to create JIT\n";
            exit(1);
        }
        JIT = std::move(*JITOrErr);

        // Allow JIT to see runtime symbols like load_image/save_image
        if (llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr)) 
        {
            std::cerr << "Failed to load current process symbols!\n";
        }

        auto generator = cantFail(
            llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
                JIT->getDataLayout().getGlobalPrefix()
            )
        );

        JIT->getMainJITDylib().addGenerator(std::move(generator));
    }

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

        std::cout<<"Running "<<PassName<<" pass\n";
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

    int compileAndRun(std::string &dslCode, const CompilerOptions &opts, int &execTime, bool printResults = true) {
        using namespace llvm;
        using namespace std::chrono;

        auto ContextPtr = std::make_unique<LLVMContext>();
        TheModule = std::make_unique<Module>("ImageDSLModule", *ContextPtr);
        new (&Builder) llvm::IRBuilder<>(*ContextPtr);
        NamedValues.clear();

        std::cout<<"DSL Code \n"<<dslCode<<endl;
        dslCode  =  dslCode +"\n";

        // Parse DSL
        antlr4::ANTLRInputStream inputStream(dslCode);
        ImageLangLexer lexer(&inputStream);
        antlr4::CommonTokenStream tokens(&lexer);
        ImageLangParser parser(&tokens);
        auto tree = parser.program();

        if (opts.printLogs) printParseTree(tree);

        ASTBuilder builder;
        auto programAST = builder.build(tree);
        if (opts.printLogs) programAST->print();

        getImageStructType();
        programAST->codegen();

        if (opts.printLogs)
            printModuleIR(*TheModule, "Before Optimization");

        // Run optimization passes
        if (opts.enableMem2RegOpt) {
            runFunctionPassOnModule(*TheModule, MyMem2RegPass(), "Mem2Reg");
        }
        if (opts.enableConstantFolding) {
            runFunctionPassOnModule(*TheModule, ConstantFoldingPass(), "Constant Folding");
        }
        if (opts.enableCSE) {
            runFunctionPassOnModule(*TheModule, GlobalCSEPass(), "CSE");
        }
        if (opts.enableCopyPropagation) {
            runFunctionPassOnModule(*TheModule, CopyPropagationPass(), "Copy Propagation");
        }

        verifyModuleIR(*TheModule, "After Optimizations");

        // Add module to JIT
        
        auto TSM = orc::ThreadSafeModule(std::move(TheModule), std::move(ContextPtr));

        if (auto err = JIT->addIRModule(std::move(TSM))) {
            llvm::errs() << "Failed to add module to JIT\n";
            return 1;
        }

        // Run JIT-compiled `main`
        auto mainSym = JIT->lookup("main");
        if (!mainSym) {
            llvm::errs() << "JIT lookup failed for main\n";
            return 1;
        }

        using MainFnType = int (*)();
        auto addr = mainSym->toPtr<MainFnType>();
        if (!addr) {
            llvm::errs() << "Failed to convert symbol to function pointer\n";
            return 1;
        }

        if(printResults) std::cout << "[JIT] Executing compiled IR...\n";
        auto start = high_resolution_clock::now();
        int result = addr();
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start).count();
        if(printResults) std::cout << "[JIT] Execution finished with code " << result << "\n";
        if(printResults) std::cout << "[JIT] Execution time: " << duration << " ms\n";

        execTime = duration;

        return result;
    }
};
