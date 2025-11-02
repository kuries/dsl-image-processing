#include "ImageDSLExecutor.h"
#include "DSLBenchmarking.h"

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./image-dsl <dsl-file> [options]\n";
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Failed to open DSL file: " << argv[1] << "\n";
        return 1;
    }

    std::string dslCode((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

    bool benchmarkFunctions = false;
    CompilerOptions opts;
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-mem2reg=false") opts.enableMem2RegOpt = false;
        else if (arg == "-cse=false") opts.enableCSE = false;
        else if (arg == "-cf=false") opts.enableConstantFolding = false;
        else if (arg == "-copyprop=false") opts.enableCopyPropagation = false;
        else if (arg == "-logs=true") opts.printLogs = true;
        else if (arg == "-benchmark=true") benchmarkFunctions = true;
    }

    ImageDSLExecutor executor;
    int execTime_ms;
    executor.compileAndRun(dslCode, opts, execTime_ms);

    ImageDSLExecutor executor2;
    executor2.compileAndRun(dslCode, opts, execTime_ms);

    if(benchmarkFunctions)
    {
        DSLBenchmarkRunner benchmarkRunner;

        std::string imageFolder = "/workspace/dsl-image-processing/build/testing_images";
        std::string outputCsv = "/workspace/dsl-image-processing/build/testing_images/results.csv";

        benchmarkRunner.runBenchmark(imageFolder, outputCsv, opts);
    }

    return 0;
}