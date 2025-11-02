#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include "ImageDSLExecutor.h"

namespace fs = std::filesystem;

static bool endsWith(const std::string& str, const std::string& suffix) {
    if (suffix.size() > str.size()) return false;
    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

class DSLBuilder {
    std::ostringstream code;
public:
    DSLBuilder& loadImage(const std::string& path) {
        code << "image img = load(\"" << path << "\");\n";
        return *this;
    }

    DSLBuilder& saveImage(const std::string& path) {
        code << "img.save (\"" << path << "\");";
        return *this;
    }

    DSLBuilder& applyThreshold(double t1, double t2) {
        code << "apply_threshold( img " << ", " << t1 << ", " << t2 << ");\n";
        return *this;
    }

    DSLBuilder& adjustBrightness(double value) {
        code << "adjust_brightness( img " << ", " << value << ");\n";
        return *this;
    }

    DSLBuilder& adjustContrast(double value) {
        code << "adjust_contrast( img " << ", " << value << ");\n";
        return *this;
    }

    DSLBuilder& convertToGreyscale() {
        code << "convertToGreyscale( img );\n";
        return *this;
    }

    std::string str() const { return code.str(); }
};

class DSLBenchmarkRunner {

public:

    void runBenchmark(const std::string& inputFolder, const std::string& outputCSV, CompilerOptions& opts) {
        std::ofstream csv(outputCSV);
        if (!csv.is_open()) {
            std::cerr << "❌ Failed to open CSV file for writing: " << outputCSV << "\n";
            return;
        }

        // Define the operations to test (column order)
        std::vector<std::string> operations = {
            "threshold", "brightness", "contrast", "greyscale"
        };

        // Write CSV header
        csv << "Image";
        for (const auto& op : operations)
            csv << "," << capitalize(op) << "(ms)";
        csv << "\n";

        // Gather image files
        std::vector<std::string> images;
        for (const auto& entry : fs::directory_iterator(inputFolder)) {
            if (entry.is_regular_file()) {
                auto path = entry.path().string();
                if (endsWith(path, ".jpg") || endsWith(path, ".png") || endsWith(path, ".jpeg"))
                    images.push_back(path);
            }
        }

        if (images.empty()) {
            std::cerr << "⚠️  No images found in " << inputFolder << "\n";
            return;
        }

        // Benchmark each image for all operations
        for (const auto& imgPath : images) {
            std::string filename = fs::path(imgPath).filename().string();
            csv << filename;

            std::unordered_map<std::string, long long> timings;

            for (const auto& op : operations) {
                DSLBuilder builder;
                builder.loadImage(imgPath);

                if (op == "threshold")
                    builder.applyThreshold(100.3, 50.999);
                else if (op == "brightness")
                    builder.adjustBrightness(100);
                else if (op == "contrast")
                    builder.adjustContrast(2);
                else if (op == "greyscale")
                    builder.convertToGreyscale();

                std::string outPath = inputFolder + "/output_" + op + "_" + filename;
                builder.saveImage(outPath);

                std::string dslCode = builder.str();

                std::cout<<dslCode<<endl;

                int duration;
                ImageDSLExecutor executor;
                executor.compileAndRun(dslCode, opts, duration, false);

                timings[op] = duration;
                std::cout << "[BENCH] " << filename << " - " << op << " = " << duration << " ms\n";
            }

            // Write times in the same order as header
            for (const auto& op : operations)
                csv << "," << timings[op];
            csv << "\n";
        }

        csv.close();
        std::cout << "\n✅ Benchmark results written to " << outputCSV << "\n";
    }

private:
    static std::string capitalize(const std::string& s) {
        if (s.empty()) return s;
        std::string result = s;
        result[0] = std::toupper(result[0]);
        return result;
    }
};